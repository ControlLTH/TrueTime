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

// Distributed control system: interference node
//
// Generates disturbing network traffic.

#define S_FUNCTION_NAME interference_init

#include "ttkernel.cpp"

// code function
double interference_code(int seg, void *data)
{
  double BWshare = *((double *)data); // Fraction of the network bandwidth
  double ran = ttAnalogIn(1);
  if (ran < BWshare) {
    ttSendMsg(1, NULL, 80);  // send 80 bits to myself (no data)
  }

  while (ttGetMsg()) {};   // read old received messages (if any)

  return FINISHED;
}


void init()
{
  // Initialize TrueTime kernel
  ttInitKernel(prioFP); // fixed priority

  // Read the input argument from the block dialogue
  mxArray *initarg = ttGetInitArg();
  if (!mxIsDoubleScalar(initarg)) {
    TT_MEX_ERROR("The init argument must be a number!\n");
    return;
  }
  // Allocate memory for the task and store pointer in UserData
  double *data = new double;
  *data = mxGetPr(initarg)[0];
  ttSetUserData(data);

  // Sender task
  double period = 0.001;
  double offset = 0.0005;
  ttCreatePeriodicTask("interference_task", offset, period, interference_code, data);
}

void cleanup() {
  // Free the allocated memory
  double *data = (double *)ttGetUserData();
  delete data;
}
