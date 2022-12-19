/*
 * Copyright (c) 2016 Lund University
 *
 * Written by Anton Cervin, Dan Henriksson and Martin Ohlin,
 * Department of Automatic Control LTH, Lund University, Sweden.
 *   
 * This file is part of TrueTime 2.0.
 *
 * TrueTime 2.0 is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * TrueTime 2.0 is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TrueTime 2.0. If not, see <http://www.gnu.org/licenses/>
 */

// PID-control of a DC servo process.
//
// This example shows four ways to implement a periodic controller
// activity in TrueTime. The task implements a standard
// PID-controller to control a DC-servo process (2nd order system). 

#define S_FUNCTION_NAME servo_init

#include "ttkernel.cpp"

// PID data structure used in Implementations 1a, 2, and 3 below.
struct TaskData {
  double u, Iold, Dold, yold, t; // t only used in Implementation 2
  double K, Ti, Td, beta, N, h;
  int rChan, yChan, uChan; 
};

// Kernel data structure, for proper memory allocation and deallocation
struct KernelData {
  TaskData *d;       // not used in Implementation 2
  double *d2;        // only used in Implementation 2
  int *hdl_data;     // only used in Implementation 4
};

// calculate PID control signal and update states
void pidcalc(TaskData* d, double r, double y) {

  double P = d->K*(d->beta*r-y);
  double I = d->Iold;
  double D = d->Td/(d->N*d->h+d->Td)*d->Dold+d->N*d->K*d->Td/(d->N*d->h+d->Td)*(d->yold-y); 

  d->u = P + I + D;
  d->Iold = d->Iold + d->K*d->h/d->Ti*(r-y);
  d->Dold = D;
  d->yold = y;
}

// Variables used in Implementation 1b below.


// ---- PID code function for Implementation 1 ----
double pid_code1(int seg, void* data) {

  double r, y;
  TaskData* d = (TaskData*) data;

  switch (seg) {
  case 1:  
    r = ttAnalogIn(d->rChan);
    y = ttAnalogIn(d->yChan);
    pidcalc(d, r, y); 
    return 0.002;
  default:
    ttAnalogOut(d->uChan, d->u);
    return FINISHED;
  }
}

// ---- PID code function for Implementation 2 ----
double pid_code2(int seg, void* data) {

  double inp[2];  // controller block diagram inputs 
  double outp[2]; // controller block diagram outputs 
  double *d2 = (double *)data;

  switch (seg) {
  case 1: 
    inp[0] = ttAnalogIn(1);
    inp[1] = ttAnalogIn(2);
    ttCallBlockSystem(2, outp, 2, inp, "controller");
    *d2 = outp[0];      // store control signal
    return outp[1];     // execution time returned from block 
  default:
    ttAnalogOut(1, *d2);
    return FINISHED;
  }
}

// ---- PID code function for Implementation 3 ----
double pid_code3(int seg, void* data) {

  double r, y;
  TaskData* d = (TaskData*) data;

  switch (seg) {
  case 1:
    d->t = ttCurrentTime();
    return 0.0;
  case 2:  
    r = ttAnalogIn(d->rChan);
    y = ttAnalogIn(d->yChan);
    pidcalc(d, r, y); 
    return 0.002;
  case 3:    
    ttAnalogOut(d->uChan, d->u);
    // Sleep
    d->t += d->h;
    ttSleepUntil(d->t);
    return 0.0;
  default:
    ttSetNextSegment(2); // loop
    return 0.0;
  }
}

// ---- PID code function for Implementation 4 ----
double pid_code4(int seg, void* data) {

  double r;
  double *y;
  TaskData* d = (TaskData*) data;

  switch (seg) {
  case 1:  
    r = ttAnalogIn(d->rChan);
    y = (double*) ttTryFetch("Samples");
    pidcalc(d, r, *y); 
    delete y;
    return 0.0018;
  default:
    ttAnalogOut(d->uChan, d->u);
    return FINISHED;
  }
}

// ---- Sampler code function for Implementation 4 ----
double sampler_code(int seg, void* data) {

  double y;
  int* d = (int*) data;

  switch (seg) {
  case 1:  
    y = ttAnalogIn(*d);
    ttTryPost("Samples", new double(y)); // put sample in mailbox
    ttCreateJob("pid_task");  // trigger task job
    return 0.0002;
  default:
    return FINISHED;
  }
}


void init() {

  // Read the input argument from the block dialogue
  mxArray *initarg = ttGetInitArg();
  if (!mxIsDoubleScalar(initarg)) {
    TT_MEX_ERROR("The init argument must be a number!\n");
    return;
  }
  int implementation = (int)mxGetPr(initarg)[0];

  // Allocate KernelData memory and store pointer in kernel
  KernelData *kd = new KernelData;
  ttSetUserData(kd);

  // Allocate memory for implementation != 2
  TaskData *d = new TaskData;
  kd->d = d;  // Store pointer in KernelData

  // Allocate memory for implementation 2
  double *d2 = new double;
  kd->d2 = d2; // Store pointer in KernelData

  // Allocate memory for implementation 4
  int *hdl_data = new int;
  kd->hdl_data = hdl_data;   // Store pointer in KernelData
  
  // Initialize TrueTime kernel
  ttInitKernel(prioFP);

  // Task attributes
  double starttime = 0.0;
  double period = 0.006;
  double deadline = period;

  // Controller parameters and states
  d->K = 0.96;
  d->Ti = 0.12;
  d->Td = 0.049;
  d->beta = 0.5;
  d->N = 10.0;
  d->h = period;
  d->u = 0.0;
  d->t = 0.0;  // only used for implementation 3
  d->Iold = 0.0;
  d->Dold = 0.0;
  d->yold = 0.0;
  d->rChan = 1;
  d->yChan = 2;
  d->uChan = 1;

  switch (implementation) {
    
  case 1:
    // IMPLEMENTATION 1: using the built-in support for periodic tasks
    
    ttCreatePeriodicTask("pid_task", starttime, period, pid_code1, d);
    
    break;

  case 2:
    // IMPLEMENTATION 2: calling Simulink block within code function
    
    ttCreatePeriodicTask("pid_task", starttime, period, pid_code2, d2);
    break;

  case 3:
    // IMPLEMENTATION 3: sleepUntil and loop back
  
    ttCreateTask("pid_task", deadline, pid_code3, d);
    ttCreateJob("pid_task");
    break;
    
  case 4:
    // IMPLEMENTATION 4: sampling in timer handler, triggers task job
  
    *hdl_data = 2; // y_chan for reading samples
    ttCreateHandler("timer_handler", 1, sampler_code, hdl_data);
    ttCreatePeriodicTimer("timer", starttime, period, "timer_handler");
    ttCreateMailbox("Samples", 10);
    ttCreateTask("pid_task", deadline, pid_code4, d);
    break;

  }
}

void cleanup() {
  // This is called also in the case of an error

  KernelData *kd = (KernelData*)ttGetUserData();

  if (kd) {
    delete kd->d;
    delete kd->d2;
    delete kd->hdl_data;
    delete kd;
  }
}
