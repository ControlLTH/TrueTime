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

#ifndef SET_PERIOD
#define SET_PERIOD

#include "getnode.cpp"

// Setting period of a specific task
void ttSetPeriod(double newPeriod, const char *nameOfTask) {
  
  DataNode* dn = getNode(nameOfTask, rtsys->taskList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttSetPeriod: Non-existent task '%s'!", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }
  UserTask* task = (UserTask*) dn->data; 
  if (task->activationTimer == NULL) {
    char buf[200];
    sprintf(buf, "ttSetPeriod: Task '%s' not periodic!", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }

  Timer* timer = task->activationTimer;
  timer->period = newPeriod;
}

// Setting period of calling task
void ttSetPeriod(double newPeriod) {
  
  if(rtsys->running == NULL) {
    TT_MEX_ERROR("ttSetPeriod: No running task!");
    return;
  }

  if (!rtsys->running->isUserTask()) {
    TT_MEX_ERROR("ttSetPeriod: Can not be called by interrupt handler!");
    return;
  }

  ttSetPeriod(newPeriod, rtsys->running->name);
}
#endif
