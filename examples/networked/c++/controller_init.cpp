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

// Distributed control system: controller node
//
// Receives messages from the sensor node, computes control signal
// and sends it to the actuator node. Also contains a high-priority
// disturbing task.

#define S_FUNCTION_NAME controller_init

#include "ttkernel.cpp"

// PD controller data structure
struct PD_Data {
  // ctrl params
  double K, Td, N, h, ad, bd;
  
  // ctrl states
  double yold, Dold, u;
};

// Kernel data strucutre
struct Kernel_Data {
  struct PD_Data *pddata;
  double *dummydata;
};

// controller code function
double controller_code(int seg, void *data)
{
  double *msg;
  double r, y, P, D;
  PD_Data* d = (PD_Data*) data;

  switch(seg) {
  case 1:
    msg = (double*) ttGetMsg(); // get sensor value
    if (msg != NULL) {
      y = *msg;
      delete msg; // delete message
    } else {
      mexPrintf("Error in controller: no message received!\n");
      y = 0.0;
    }
      
    r = ttAnalogIn(1);
    
    P = d->K*(r-y);
    D = d->ad*d->Dold + d->bd*(d->yold-y);
    d->u = P + D;
    d->Dold = D;
    d->yold = y;
    return 0.0005;

  default:
    msg = new double;
    *msg = d->u;
    ttSendMsg(2, msg, 80); // Send 80 bits to node 2 (actuator)
    return FINISHED; 
  }

}

// interfering task code function
double dummy_code(int seg, void *data)
{
  switch (seg) {
  case 1:
    return *(double *)data;
  default:
    return FINISHED;
  }
}


void init() {

  // Initialize TrueTime kernel  
  ttInitKernel(prioDM); // deadline-monotonic scheduling
  
  // Allocate kernel data and store pointer in UserData
  Kernel_Data *kdata = new Kernel_Data;
  ttSetUserData(kdata);
  
  // Read the input argument from the block dialogue
  mxArray *initarg = ttGetInitArg();
  if (!mxIsDoubleScalar(initarg)) {
    TT_MEX_ERROR("The init argument must be a number!\n");
    return;
  }

  // Create task data (local memory)
  PD_Data *data = new PD_Data;
  kdata->pddata = data;

  data->h = 0.010;
  data->K = 1.0;
  data->Td = 0.04;
  data->N = 100.0;
  data->ad = data->Td/(data->N*data->h+data->Td);
  data->bd = data->N*data->K*data->ad;
  data->yold = 0.0;
  data->Dold = 0.0;
  data->u = 0.0;

  // Sporadic controller task, activated by arriving network message
  double deadline = data->h;
  ttCreateTask("controller_task", deadline, controller_code, data);
  ttAttachNetworkHandler("controller_task");

  // Periodic dummy task with higher priority
  double starttime = 0.0;
  double period = 0.007;
  double *dummydata = new double;
  *dummydata = mxGetPr(initarg)[0] * period;
  kdata->dummydata = dummydata;
  ttCreatePeriodicTask("dummy_task", starttime, period, dummy_code, dummydata);

}

void cleanup() {

  Kernel_Data *kdata = (Kernel_Data *)ttGetUserData();
  delete kdata->pddata;
  delete kdata->dummydata;
  delete kdata;
}
