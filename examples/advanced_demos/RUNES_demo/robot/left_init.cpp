#define S_FUNCTION_NAME left_init

#include "ttkernel.cpp"
#include "defines.h"

#define WHEEL_RADIUS 3.0

class data
{
public:
  int bot_nbr;
  
  struct {
    double yref;		// Current wheel velocity reference
    double yrefold;	// Old wheel velocity reference
    double yreff;		// Current filtered wheel velocity reference (low pass filtered)
    double e;				// Difference between reference and actual velocity
    double v;				// PI-reg signal before beeing limited 
    double u;				// PI-reg signal to actuator
    double I;				// Integral part of the output signal (with antiwindup)
    double ypos;		// Wheel angle position
    double yposold;	// Old wheel angle position
    double y;				// Wheel velocity
    double yold;		// Old wheel velocity
    
  } s;
  
  struct {
    double K;				// Regulator gain
    double Ti;			// Regulator integral part
    double Tr;			// Regulator antiwindup
    double beta;		// Determining emphasis put on the input signal from the sensors
    double h;				// Period time
    double umax;		// Upper limit for output signal
    double umin;		// Lower limit for output signal
    double a;				// Filter variable
    double b;				// Filter variable
  } p;
};

// Vector of pointers data objects (one for each bot)
data* da[3];

// Limiting the output signal to the actuator
double pi_sat(double v, double umax, double umin)
{
  if(v>umax)
    return umax;
  else if(v<umin)
    return umin;
  else
    return v;
}

//The PI-regulator code
double left_code(int segment, void* t_d)
{
  data* d = (data*) t_d;
  
  switch (segment){
  case 1:
    //Receives angle from the sensor
    d->s.ypos = ttAnalogIn(1);
    
    //Derivates the angle to get the wheel angle velocity
    d->s.y = (d->s.ypos - d->s.yposold)/d->p.h;
    
    
    //Low pass filter for the wheel angle velocity to avoid spikes
    d->s.y = d->s.y*(1-d->p.b*d->p.h) + d->s.yold*d->p.b*d->p.h;
    
    //Low pass filter for the reference signal
    d->s.yreff = d->s.yreff*(1-d->p.a*d->p.h) + d->s.yrefold*d->p.a*d->p.h;
    
    //Sends signals to scopes for monitoring purposes
    ttAnalogOut(2,d->s.yreff);
    ttAnalogOut(3,d->s.y);
    
    //Calculates output to actuator
    d->s.e = d->s.yreff-d->s.y;
    d->s.v = d->p.K*(d->p.beta*d->s.yreff-d->s.y)+d->s.I;
    d->s.u = pi_sat(d->s.v,d->p.umax,d->p.umin);
    
    return 0.0001;
  case 2:
    //Sends output to actuator
    ttAnalogOut(1,d->s.u);
    //Stores old signals for filters
    d->s.yposold = d->s.ypos;
    d->s.yold = d->s.y;
    d->s.yrefold = d->s.yref;
    //Calculates intergral part of the output signal
    d->s.I = d->s.I + (d->p.K*d->p.h/d->p.Ti)*d->s.e + (d->p.h/d->p.Tr)*(d->s.u-d->s.v);
    
    return 0.0001;
  default:
    return FINISHED;
  }
}

// Code to be executed when a message is received on the I2C-bus
double msgRcvhandler(int seg, void* t_d)
{
  data* d = (data*) t_d;
  switch (seg){
  case 1:
    generic *m;
    m = (generic *) ttGetMsg(I2C_NETWORK + d->bot_nbr - 1);
    
    switch (m->t) { 
    case REQUEST_WHEEL_VEL: {
      generic *new_m = new generic;
      new_m->t = SEND_WHEEL_VEL;
      SEND_WHEEL_VEL_MSG* msg = new SEND_WHEEL_VEL_MSG;
      new_m->msg = msg;
      
      msg->hdr.sender = LEFT_WHEEL;
      msg->hdr.receiver = MEGA_128;
      
      msg->vel = d->s.y * WHEEL_RADIUS;
      
      ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, TMOTE, new_m, 10 + 16);
      return 0.0001;
    }
      
    case SEND_WHEEL_VEL: {
      double vel = ((SEND_WHEEL_VEL_MSG *)m->msg)->vel;
      
      // Set vel on v
      d->s.yref = vel;
      
      return 0.0001;  
    }

    default: {}
    
    }
    
    //delete m->msg;
    delete m;
    
    return 0.0001;
  default:
    return FINISHED;
  }
  
}

void init()
{
  mxArray *initarg = ttGetInitArg();
  int bot_nbr = (int)mxGetPr(initarg)[0];
  
  ttInitKernel(prioFP);
  
  da[bot_nbr - 1] = new data;
  da[bot_nbr - 1]->p.K = 2.6;
  da[bot_nbr - 1]->p.beta = 0.9;
  da[bot_nbr - 1]->p.Ti = 1;
  da[bot_nbr - 1]->p.Tr = 5;
  da[bot_nbr - 1]->p.h = 0.1;
  da[bot_nbr - 1]->p.umin = -3;
  da[bot_nbr - 1]->p.umax = 3;
  da[bot_nbr - 1]->p.a = 2;
  da[bot_nbr - 1]->p.b = 2;
  
  da[bot_nbr - 1]->s.yref = 0;
  da[bot_nbr - 1]->s.yrefold = 0;
  da[bot_nbr - 1]->s.yreff = 0;
  da[bot_nbr - 1]->s.e = 0;
  da[bot_nbr - 1]->s.v = 0;
  da[bot_nbr - 1]->s.u = 0;
  da[bot_nbr - 1]->s.I = 0;
  da[bot_nbr - 1]->s.yposold = 0;
  da[bot_nbr - 1]->s.y = 0;
  da[bot_nbr - 1]->s.yold = 0;
  da[bot_nbr - 1]->s.ypos = 0;
  da[bot_nbr - 1]->bot_nbr = bot_nbr;
  
  double prio = 1.0;
  double offset = 0;
  double period = da[bot_nbr - 1]->p.h;
  
  ttCreatePeriodicTask("left", offset, period, left_code, da[bot_nbr -1]);
  ttSetPriority(prio, "left");
  ttCreateHandler("msgRcv", 1, msgRcvhandler, da[bot_nbr -1]);

  ttAttachNetworkHandler(I2C_NETWORK + bot_nbr - 1, "msgRcv");
}

void cleanup()
{
  mxArray *initarg = ttGetInitArg();
  int bot_nbr = (int)mxGetPr(initarg)[0];
    delete da[bot_nbr - 1];
}
