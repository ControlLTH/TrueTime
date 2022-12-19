#define S_FUNCTION_NAME mega128_init

#include "math.h"
#include "ttkernel.cpp"
#include "defines.h"

// Defines used by kalman update and predict
#define HEIGHT 0.0	// Kalman node Z value
#define SQR(a) ((a)*(a))
#define ABS(x)   ((x) > 0 ? (x) : (-(x)))
#define RK 10.0
#define PI 3.14159265

#define QK11 0.5f
#define QK12 0.0f
#define QK13 0.0f
#define QK21 0.0f
#define QK22 0.5f
#define QK23 0.0f
#define QK31 0.0f
#define QK32 0.0f
#define QK33 0.4f

#define H_KALMAN (0.4f)

// LENGTH OF BOT
//#define L_BOT    0.95f //Teknologerna
//#define L_BOT    0.23f  //Roboten
#define L_BOT    30.0f  //Verkligheten
//#define L_BOT 0.08f
class data
{
public:
  int bot_nbr;
 	
  double u[2];
  double uo[2];
  double un[2];
	
  //---KALMAN-----
  double node_x;
  double node_y;
  double node_dist;
  unsigned int node_nodeID;
	
  double X[3];
  double Xold[2];
  double P[3][3];
  //--------------
	
  //---WAYPOINTLIST---
  double WP[10][2];
	
  unsigned int nbr_of_wp;
  unsigned int cur_wp;
  //------------------
	
  //-Obstacle avoidance
  int obs_avoid;
  //-------------------
};

data* da[3];

/**	Periodic task for correction of heading angle.
 * Executes once per second caculating angle from
 * movement and correcting kalman with 50 % of the
 * error.
 */
double mega128_code(int segment, void* t_d)
{
  data* d = (data *) t_d;
    
  switch (segment){
  case 1: {
    double x = d->X[0];
    double y = d->X[1];
    double xo = d->Xold[0];
    double yo = d->Xold[1];
    double old_th = d->X[2];
    double new_th = atan2(y - yo, x - xo) * 180 / PI;
    double diff = old_th - new_th;
	    	
    // Double modulo
    while (diff > 180.0)
      diff -= 360.0;
    while (diff < -180.0)
      diff += 360.0;
	  			
    if (diff < 50.0)
      d->X[2] -= diff / 2;
	  		
    d->Xold[0] = x;
    d->Xold[1] = y;
	    	
    return 0.0001;
  }
  default:
    return FINISHED;
  }
}

/** Task executed when Mega_128 requests wheel velocities. */                  
double send_wheel_vel_code(int seg, void* t_d)
{
  data *d = (data *)t_d;
  
  switch (seg){
  case 1:	// Send vel for right wheel
    {
      generic *m; 
      m = new generic;
      m->t = SEND_WHEEL_VEL;
      SEND_WHEEL_VEL_MSG* msg = new SEND_WHEEL_VEL_MSG;
      m->msg = msg;
    	
      msg->vel = d->un[0];
      msg->hdr.receiver = LEFT_WHEEL;
      msg->hdr.sender = MEGA_128;
      ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, TMOTE, m, 8+32);
  
      return 0.0001;
    }  
    
  case 2: // Send vel for left wheel
    {
      generic *m; 
      m = new generic;
      m->t = SEND_WHEEL_VEL;
      SEND_WHEEL_VEL_MSG* msg = new SEND_WHEEL_VEL_MSG;
      m->msg = msg;
		  
      msg->vel = d->un[1];
      msg->hdr.receiver = RIGHT_WHEEL;
      msg->hdr.sender = MEGA_128;
      ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, TMOTE, m, 8+32);
  
      return 0.0001;
    }
      
  default:
    return FINISHED;
  }
}

/**	Task executed when receiving a message on the I2C-bus. */
double msg_rcv_handler(int seg, void* t_d)
{
  data* d = (data*) t_d;
  switch (seg){
  case 1:
    generic *m;
    m = (generic *) ttGetMsg(I2C_NETWORK + d->bot_nbr - 1);
      
    switch (m->t){
    case SEND_WHEEL_VELS: {
      SEND_WHEEL_VELS_MSG* msg = (SEND_WHEEL_VELS_MSG*) m->msg;
      d->u[0] = msg->leftVel;
      d->u[1] = msg->rightVel;
          
      delete msg;
      ttCreateJob("kalman_predict");
          
      break;
    }
            
    case REQUEST_WHEEL_VEL:{
      //delete m->msg;
      ttCreateJob("send_wheel_vel");
      break;
    }
    case SEND_ULTRA_POS: {
      SEND_ULTRA_POS_MSG* msg = (SEND_ULTRA_POS_MSG *)m->msg;
        	
      d->node_x = msg->x;
      d->node_y = msg->y;
      d->node_dist = msg->dist;
      d->node_nodeID = msg->nodeID;
        	
      ttCreateJob("kalman_update");
      delete msg;
      break;
    }
        
    case SEND_IR: {
      SEND_IR_MSG* msg = (SEND_IR_MSG *)m->msg;
        	
      d->obs_avoid = msg->ir_result;
      delete msg;
      break;
    }

    default: {}
    }
      
    delete m;
    return 0.0001;
     
  default:
    return FINISHED;
  }
}

/** Task performing kalman update executed each time
 * Tmote sends distance data from node.
 */
double kalman_update(int seg, void* t_d) 
{
  data *d = (data *)t_d;
	
  switch(seg) {
  case 1: {
    double Hk[3];
    double rest;
    double yerr;
    double Kk[3];
    double Sk;
		
    /* Temporary variables used in calculations */
    double pTmp[3][3];
    double pTmp2[3][3];
		
    rest = sqrt(SQR(d->X[0] - d->node_x) + 
		SQR(d->X[1] - d->node_y) + 
		SQR(HEIGHT));
		  						
    Hk[0] = (d->X[0] - d->node_x) / rest;
    Hk[1] = (d->X[1] - d->node_y) / rest;
    Hk[2] = 0.0;
		    
    yerr = d->node_dist - rest;
		  	
    double a = d->P[0][0];
    double b = d->P[1][1];
    //double c = d->P[2][2]; //never used
    double dd = d->P[1][2];
    double e = d->P[0][1];
    double f = d->P[0][2];
		
		
    Sk = (Hk[0]*a + Hk[1]*e)*Hk[0] + (Hk[0]*e + Hk[1]*b)*Hk[1];
		
    Sk += RK;
		
    if (ABS(Sk) < 0.001f) return 0.0001;
		
    Kk[0] = (a*Hk[0] + e*Hk[1])/Sk;
    Kk[1] = (e*Hk[0] + b*Hk[1])/Sk;
    Kk[2] = (f*Hk[0] + dd*Hk[1])/Sk;
    //		  mexPrintf("KK[1] = %f \n",Kk[1]);
    //		  mexPrintf("Yerr = %f \n",yerr);
		
    /* d->X = d->X + Kk*yerr */
    for (int j = 0; j < 3; j++)
      d->X	[j] += Kk[j]*yerr;
		
    /* Init pTmp to identity then subtract Kk*Hk from pTmp */
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 3; i++) {
	/* identity */
	if (i == j) {
	  pTmp[j][i] = 1.0f;
	} else {
	  pTmp[j][i] = 0.0f;
	}
	pTmp[j][i] -= Kk[j]*Hk[i];
      }
    }
		
    /* pTmp2 = pTmp*d->P, pTmp = (I-Kk*Hk), result will be symmetric, don't calc.
       the redundant entries */
    for (int j = 0; j < 3; j++) {
      /* only calc upper right triangle */
      for (int i = j; i < 3; i++) {
	pTmp2[j][i] = 0;
	for (int k = 0; k < 3; k++) {
	  pTmp2[j][i] += pTmp[j][k]*d->P[k][i];
	}
      }
    }
		
    /* Store result (pTmp2) into d->P, utilising symmetry, only upper right triangle
       contains valid data */
    d->P[0][0] = pTmp2[0][0];
    d->P[0][1] = pTmp2[0][1];
    d->P[0][2] = pTmp2[0][2];
		
    d->P[1][0] = pTmp2[0][1]; // symmetry
    d->P[1][1] = pTmp2[1][1];
    d->P[1][2] = pTmp2[1][2];
		
    d->P[2][0] = pTmp2[0][2]; // symmetry
    d->P[2][1] = pTmp2[1][2]; // symmetry
    d->P[2][2] = pTmp2[2][2];
		  
    //		return 0.0001;
    return 0.003;
  }
		
  default: {
    return FINISHED;
  }
  }
	
  return FINISHED;
}

/**	Task executed when receiving wheel velocity data from wheel 
 * avr:s doing kalman predict and robot control calculations.
 */
double kalman_predict(int seg, void* t_d) 
{
  data *d = (data *)t_d;
	
  switch(seg) {
  case 1:	// DO KALMAN PREDICT
    {
      //		mexPrintf("Kalman predict executes at %f.\n",ttCurrentTime());
      /* old theta */
      double tho = d->X[2];
		
      /* Predict new theta using tustin */
      double th =  d->X[2] + H_KALMAN/(2.0f*L_BOT)*((d->u[1]-d->u[0]) + (d->uo[1]-d->uo[0]));
      //		  mexPrintf("Theta = #%f \n",th);
		
      /* Store computed trig funcs, saves approx 70 ms/call */		  
      //		  	  double conv = PI / 180.0;
      double conv = 1.0;
      double cos_th = cos(th * conv);
      double sin_th = sin(th * conv);
      double cos_tho = cos(tho * conv);
      double sin_tho = sin(tho * conv);
		
      /* store computed values to save time (saves approx 7 ms/call) */
      double u_cos_th = (d->u[0]+d->u[1])*cos_th+(d->uo[0]+d->uo[1])*cos_tho;
      double u_sin_th = (d->u[0]+d->u[1])*sin_th+(d->uo[0]+d->uo[1])*sin_tho;

      //		  mexPrintf("new u %f  %f  \n",d->u[0],d->u[1]);
      //		  mexPrintf("old u %f  %f  \n",d->uo[0],d->uo[1]);

      //		   mexPrintf("U_cos_th = %f \n",u_cos_th);
				
      d->X[0] = d->X[0] + H_KALMAN/4.0f * u_cos_th;
      d->X[1] = d->X[1] + H_KALMAN/4.0f * u_sin_th;
      d->X[2] = th;

      //		  mexPrintf("predicted x = %f \n",d->X[0]);
		
      double h1 = -H_KALMAN/4.0f * u_sin_th;
      double h2 = H_KALMAN/4.0f * u_cos_th;
		
      double a = d->P[0][0];
      double b = d->P[1][1];
      double c = d->P[2][2];
      double dd = d->P[1][2];
      double e = d->P[0][1];
      double f = d->P[0][2];
		
      double fh1c = f+h1*c;
      double dh2c = dd+h2*c;
		
      /* pTmp = Fk*P*Fk+Qk' */
      d->P[0][0] = (a+h1*f)+fh1c*h1+QK11;
      d->P[0][1] = (e+h1*dd)+fh1c*h2+QK12;
      d->P[0][2] = fh1c+QK13;
		
      d->P[1][0] = (e+h2*f)+dh2c*h1+QK21;
      d->P[1][1] = (b+h2*dd)+dh2c*h2+QK22;
      d->P[1][2] = dh2c+QK23;
		
      d->P[2][0] = fh1c+QK31;
      d->P[2][1] = dh2c+QK32;
      d->P[2][2] = c+QK33;
		  
      //		return 0.0001;
      return 0.003;
    }
		
  case 2:	// DO ROBOT CTRL
    {
      // VARIABLES
      double dist_to_next;		// Distance to next waypoint in list
      double delta_th;				// Theta to next
      double delta_x;					// X-dist to next
      double delta_y;					// Y-dist to next
      double speed;						// Sum of wheelspeeds
      bool change_wp = true;
      double speed_left;			// Speed of left wheel
      double speed_right;			// Speed of right wheel
  		
      // LIMITS
      double th_big = 45.0*PI/180.0;		// Big angle
      double th_small = 8.0*PI/180.0;	// Small angle
      double wp_rad = 20.0;			// Radius for waypoints
      double close = 30.0;			// Dist considered near to a waypoint
  		
      // Constants
      double stop = 0.0;				// Stand still
      double fast = 3.0;				// High speed
      double medium = 2.2;			// Medium speed
      double slow = 1.3;				// Low speed
  		
      while (change_wp) {
	// Calc delta X, Y
	delta_x =  d->WP[d->cur_wp][0] - d->X[0];
	delta_y =  d->WP[d->cur_wp][1] - d->X[1];
	  		
	// Calc dist to next
	dist_to_next = sqrtf(SQR(delta_x) + SQR(delta_y));
	  		
	if (dist_to_next > wp_rad)// Go on to next stage
	  change_wp = false;	
	else											// Calc new dist (loop again)
	  d->cur_wp += 1;
	  		
	if (d->cur_wp >= d->nbr_of_wp)
	  dist_to_next = -1.0;
      }
	  	
      // Calc speed
      if ((dist_to_next < 0) | (ttAnalogIn(1) == 1.0))
	speed = stop;
      else if (dist_to_next < close)
	speed = slow;
      else if (d->obs_avoid != 0)
	speed = medium;
      else
	speed = fast;
  		
      // Calc delta th
      //		delta_th = d->X[2] - 180 * (atan2(delta_y, delta_x) / PI);
      delta_th = d->X[2] - atan2(delta_y, delta_x);
      // Double modulo
      while (delta_th > PI)
	delta_th -= 2*PI;
      while (delta_th < -PI)
	delta_th += 2*PI;
  			
      // Calc how fast to turn
      if (fabs(delta_th) < th_small && d->obs_avoid == 0) {		// Dont turn
	speed_left = 1.0;
	speed_right = 1.0;
      }
  		
      else if (delta_th > th_big || d->obs_avoid == 1) {			// Turn hard right
	speed_left = 1.0;
	speed_right = -0.4;
      }
  		
      else if (delta_th < -th_big || d->obs_avoid == -1) {		// Turn hard left
	speed_left = -0.4;
	speed_right = 1.0;
      }
  		
      else if (delta_th < 0.0) {				// Turn soft left
	speed_left = 0.4;
	speed_right = 1.00;
      }
  			
      else {														// Turn soft right
	speed_left = 1.0;
	speed_right = 0.4;
      }
  		
      // Set wheel speeds
      speed_left *= speed;
      speed_right *= speed;
  		
      // Save wheel speeds
      d->uo[0] = d->u[0];
      d->uo[1] = d->u[1];
  		
      // Set ctrl signals
      d->un[0] = speed_left;
      d->un[1] = speed_right;
      return 0.003;
    }

		
  case 3: // SEND INFO TO GRAPHICS
    {
      ttAnalogOut(1, d->X[0]);
      ttAnalogOut(2, d->X[1]);
      //		ttAnalogOut(3, PI * (d->X[2] / 180));
      ttAnalogOut(3,d->X[2]);
			
      return 0.0001;
    }
		
  default: {
    return FINISHED;
  }
  }
	
  return FINISHED;
}

void init()
{
  mxArray *initarg = ttGetInitArg();
    
  ttInitKernel(prioFP);
  int bot_nbr = (int) mxGetPr(initarg)[0];
    
  da[bot_nbr - 1] = new data;
  da[bot_nbr - 1]->bot_nbr = bot_nbr;
  da[bot_nbr - 1]->node_x = 0.0;
  da[bot_nbr - 1]->node_y = 0.0;
  da[bot_nbr - 1]->node_dist = 0.0;
  da[bot_nbr - 1]->Xold[0] = 0.0;
  da[bot_nbr - 1]->Xold[1] = 0.0;
  da[bot_nbr - 1]->X[0] = 0.0;
  da[bot_nbr - 1]->X[1] = 0.0;
  da[bot_nbr - 1]->X[2] = 0.0;
  	
  ttAnalogOut(1, da[bot_nbr - 1]->X[0]);
  ttAnalogOut(2, da[bot_nbr - 1]->X[1]);
  ttAnalogOut(3, da[bot_nbr - 1]->X[2]);
    
  da[bot_nbr - 1]->P[0][0] = 1000.0;
  da[bot_nbr - 1]->P[0][1] = 0.0;
  da[bot_nbr - 1]->P[0][2] = 0.0;
  da[bot_nbr - 1]->P[1][0] = 0.0;
  da[bot_nbr - 1]->P[1][1] = 1000.0;
  da[bot_nbr - 1]->P[1][2] = 0.0;
  da[bot_nbr - 1]->P[2][0] = 0.0;
  da[bot_nbr - 1]->P[2][1] = 0.0;
  da[bot_nbr - 1]->P[2][2] = 100000.0;
  da[bot_nbr - 1]->u[0] = 0.0;
  da[bot_nbr - 1]->u[1] = 0.0;
  da[bot_nbr - 1]->uo[0] = 0.0;
  da[bot_nbr - 1]->uo[1] = 0.0;
  da[bot_nbr - 1]->un[0] = 0.0;
  da[bot_nbr - 1]->un[1] = 0.0;
    
  da[bot_nbr - 1]->cur_wp = 0;
  switch (bot_nbr) {
  case 1: {
    da[bot_nbr - 1]->nbr_of_wp = 1;
    da[bot_nbr - 1]->WP[0][0] = 420.0;
    da[bot_nbr - 1]->WP[0][1] = 0.0;

    break;
  }
		  
  case 2: {
    da[bot_nbr - 1]->nbr_of_wp = 6;
    da[bot_nbr - 1]->WP[0][0] = 50.0;
    da[bot_nbr - 1]->WP[0][1] = 50.0;
    da[bot_nbr - 1]->WP[1][0] = 50.0;
    da[bot_nbr - 1]->WP[1][1] = -50.0;
    da[bot_nbr - 1]->WP[2][0] = 50.0;
    da[bot_nbr - 1]->WP[2][1] = 50.0;
    da[bot_nbr - 1]->WP[3][0] = 350.0;
    da[bot_nbr - 1]->WP[3][1] = 0.0;
    da[bot_nbr - 1]->WP[4][0] = 700.0;
    da[bot_nbr - 1]->WP[4][1] = 50.0;
    da[bot_nbr - 1]->WP[5][0] = 700.0;
    da[bot_nbr - 1]->WP[5][1] = -50.0;
    da[bot_nbr - 1]->WP[6][0] = 350.0;
    da[bot_nbr - 1]->WP[6][1] = 0.0;
    da[bot_nbr - 1]->WP[7][0] = 100.0;
    da[bot_nbr - 1]->WP[7][1] = 0.0;
    break;
  }
		  
  case 3: {
    da[bot_nbr - 1]->nbr_of_wp = 2;
    da[bot_nbr - 1]->WP[1][0] = 750.0;
    da[bot_nbr - 1]->WP[1][1] = -50.0;
    break;
  }
  }
    
  da[bot_nbr - 1]->obs_avoid = 0;
    
  double prio = 5.0;
    
  // Create periodic task performing angle correction
  //    ttCreatePeriodicTask("mega128", 0.5, 1.0, 10, mega128_code, da[bot_nbr - 1]);
    
  // Create task for sending wheel velocities
  ttCreateTask("send_wheel_vel", 1, send_wheel_vel_code, da[bot_nbr - 1]);
  ttSetPriority(1, "send_wheel_vel");
    
  // Create kalman predict task
  ttCreateTask("kalman_predict", 1, kalman_predict, da[bot_nbr - 1]);
  ttSetPriority(1, "kalman_predict");
    
  // Create kalman update task
  ttCreateTask("kalman_update", 1, kalman_update, da[bot_nbr - 1]);
  ttSetPriority(1, "kalman_update");
  ttSetPriority(1, "send_wheel_vel");
    
  // I2C-bus
  ttCreateHandler("msg_rcv_handler", prio, msg_rcv_handler, da[bot_nbr - 1]);
  ttAttachNetworkHandler(I2C_NETWORK + bot_nbr - 1, "msg_rcv_handler");
}

void cleanup()
{
  mxArray *initarg = ttGetInitArg();
  int bot_nbr = (int)mxGetPr(initarg)[0];
  delete da[bot_nbr - 1];
}
