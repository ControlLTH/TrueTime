#define S_FUNCTION_NAME megaultra_init

#include <math.h>
#include "ttkernel.cpp"

#include "defines.h"
#include "../network.h"

// Obstacle avoidance constants
#define NBR_OBS 1
#define Y_MAX 100
#define Y_MIN -100

#define PI 3.14159265

// Defines an obstacle
struct vertex {
  double x;
  double y;
  double rad;
};

// Vector of obstacle structs
//vertex obs[NBR_OBS] = {
//	{200, 0, 25},
//	{400, 0, 15},
//	{600, 0, 25}
// };

vertex obs[NBR_OBS] = {
  {200, 0, 25}
};

class data
{
public:
  int bot_nbr;
        
  double x;
  double y;
  double t;
  double svepvinkel;
  double ir_range;
  int ir_result;
};

// Vector of data object pointers, one for each bot
data* da[3];

/**	Periodic task performing ir obstacle detection */
double ir_sweep_code(int segment, void* t_d)
{
  data* d = (data*) t_d;
  bool detected[11] = {false, false, false, false, false, false, 
		       false, false, false, false, false};
	
  switch (segment){
  case 1:{
    // Fetch bots posistion and direction
    d->x = ttAnalogIn(1);
    d->y = ttAnalogIn(2);
    d->t = ttAnalogIn(3);
			
    return 0.0001;
  }
		
  case 2:{
    double gamma;
    bool obst_found = false;
			
    // Fill "detected[]" with true where obstacles occur
    for(int i = 0; i < NBR_OBS; i++) {
      double dist = sqrt(pow((obs[i].x-d->x),2) + pow((obs[i].y-d->y),2));
				
      if (dist - obs[i].rad < d->ir_range) {
	double alfa = atan2((obs[i].y - d->y), (obs[i].x - d->x));
	double beta = asin(obs[i].rad/dist);
					
	double th_diff = d->t - alfa;
					
	// Double modulo
	while (th_diff > PI)
	  th_diff -= 2 * PI;
	while (th_diff < -PI)
	  th_diff += 2 * PI;
		  		
	double dir = 1.0;
	if (th_diff < 0)
	  dir = -1.0;
	th_diff = fabs(th_diff);
					
	for(int j =0 ; j <= 10 ; j++) {
	  gamma = (j - 5) * 10 * PI/180;
	  double th_range = fabs(beta * dir - gamma);
	  if (th_range > th_diff) {
	    detected[j] = true;
	    obst_found = true;
	    //						detected[j] = false;
	    //						obst_found = false;
	  }
	}
      }
    }
    double y_range = 0;
			
    // Obstacle detect the walls
    for(int i =0 ; i <= 10 ; i++) {
      gamma = (i - 5) * 10 * PI/180;
      y_range = d->y + d->ir_range * sin(d->t + gamma);
        
      if(y_range > Y_MAX || y_range < Y_MIN) {
        detected[i] = true;
        obst_found = true;
	//	  detected[i] = false;
	//      obst_found = false;
      }
    }
			
    d->ir_result = -1;
			
    // If obstacle detected in any direction
    // deside wich way to go for optimal avoidance
    if (obst_found) {
      for(int i = 5; i >= 1; i--) {
	if (detected[i+5] != detected[-i+3]) {
	  if (detected[i+5])
	    d->ir_result = 1;
	  else 
	    d->ir_result = -1;
	}
      }
    } else
      d->ir_result = 0;
			
    return 0.0001;
  }
  default:
    return FINISHED;
  }
}

/**	Task sending ultrasound ping when job is created
 * in msg_rcv_handler.
 * #1 wait 20 ms
 * #2 send ultra sound ping
 */
double send_ultra_ping_code(int seg, void* d_t)
{
  switch (seg) {
  case 1:{
    // Wait 20 ms
    return 0.002;
  }
  case 2:{
    ttUltrasoundPing(ULTRANETW);
    return 0.0;
  }
  default:{
    return FINISHED;
  }
  }
}

/**	I2C message receive handler executed upon
 * received message.
 * #1 Request from Tmote to send ultra
 * #2 Request from Tmote to return obstacle avoidance data
 */
double msg_rcv_handler(int seg, void *t_d)
{
  data* d = (data*) t_d;
  switch (seg){
  case 1: {
    generic *m;
    m = (generic *) ttGetMsg(I2C_NETWORK + d->bot_nbr - 1);
    switch (m->t){
    case SEND_ULTRA:{
      ttCreateJob("send_ultra_ping");
      delete m;
      return 0.0;		// IMPORTANT! Sleep 2ms
    }
        
    case REQUEST_IR:{
      delete m;
      generic* m = new generic;
      m->t=SEND_IR;;
      SEND_IR_MSG* msg = new SEND_IR_MSG;
          
      m->msg = msg;
      msg->hdr.sender = MEGA_ULTRA;
      msg->hdr.receiver = MEGA_128;
      msg->ir_result = d->ir_result;
      ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, TMOTE, m, 8+16);
      return FINISHED;
    }
    default:
      return 0.0001;
    }
  }
  default:
    return FINISHED;
  }
}
    
void init()
{
  ttInitKernel(prioFP);
    
  mxArray *initarg = ttGetInitArg();
  int bot_nbr = (int)mxGetPr(initarg)[0];
    
  da[bot_nbr - 1] = new data;
  da[bot_nbr - 1]->x = 0;
  da[bot_nbr - 1]->y = 0;
  da[bot_nbr - 1]->t = 0;
  da[bot_nbr - 1]->svepvinkel = 0;
  da[bot_nbr - 1]->ir_range = 70;
  da[bot_nbr - 1]->bot_nbr = bot_nbr;
    
  double offset = 0.0;
  double prio = 5.0;
    
  // Create periodic task for ir_sweep
  ttCreatePeriodicTask("ir_sweep", offset, 0.4, ir_sweep_code, da[bot_nbr - 1]);
  ttSetPriority(prio, "ir_sweep");
   
  // Create task for sending ultra sound ping
  ttCreateTask("send_ultra_ping", 5, send_ultra_ping_code, da[bot_nbr - 1]);
  ttSetPriority(9, "send_ultra_ping");
    
  // Create I2C network
  ttCreateHandler("msg_rcv_handler", prio, msg_rcv_handler, da[bot_nbr - 1]);
  ttAttachNetworkHandler(I2C_NETWORK + bot_nbr - 1, "msg_rcv_handler");
    
  // Create network message receive tjosan
  ttCreateHandler("receive_ultra_msg", prio, msg_rcv_handler);
  ttAttachNetworkHandler(ULTRANETW, "receive_ultra_msg");
}
    
void cleanup()
{
  mxArray *initarg = ttGetInitArg();
  int bot_nbr = (int)mxGetPr(initarg)[0];
  delete da[bot_nbr - 1];
}
