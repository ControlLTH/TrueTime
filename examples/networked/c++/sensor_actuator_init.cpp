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

// Distributed control system: sensor/actuator node
//
// Samples the plant periodically and sends the samples to the 
// controller node. Actuates controls sent from controller.

#define S_FUNCTION_NAME sensor_actuator_init

#include "ttkernel.cpp"

// code functions

double sensor_code(int seg, void *data) {
  static double *y;
  switch (seg) {
  case 1:
    y = new double;
    *y = ttAnalogIn(1);
    return 0.0005;
  case 2:
    ttSendMsg(3, y, 80); // Send measurement (80 bits) to node 3 (controller)
    return 0.0004;
  default:
    return FINISHED;
  }
}

double actuator_code(int seg, void *data) {
  static double *u;
  switch (seg) {
  case 1:
    u = (double *)ttGetMsg(); // Receive message
    return 0.0005;
  case 2:
    if (u != NULL) {
      ttAnalogOut(1, *u);
      delete u; // delete message
    } else {
      mexPrintf("Error: actuator received empty message!\n");
    }
  default:
    return FINISHED;
  }
}


void init()
{
  // Initialize TrueTime kernel
  ttInitKernel(prioDM);   // deadline-monotonic scheduling

  // Periodic sensor task
  double starttime = 0.0;
  double period = 0.010;
  ttCreatePeriodicTask("sensor_task", starttime, period, sensor_code);

  // Sporadic actuator task, activated by arriving network message
  double deadline = 10.0;
  ttCreateTask("actuator_task", deadline, actuator_code);
  ttAttachNetworkHandler("actuator_task");

}

void cleanup() {
}
