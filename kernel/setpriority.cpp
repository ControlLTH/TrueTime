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

#ifndef SET_PRIORITY
#define SET_PRIORITY

#include "getnode.cpp"

// Setting priority of specific task
void ttSetPriority(double prio, const char *nameOfTask) {
  
  DataNode* dn = getNode(nameOfTask, rtsys->taskList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttSetPriority: Non-existent task '%s'\n",nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }
  if (prio < TT_TIME_RESOLUTION) {
    TT_MEX_ERROR("ttSetPriority: Priorities should be positive numbers!");
    return;
  }

  UserTask* task = (UserTask*) dn->data; 

  task->priority = prio;
  
  // Reshuffle readyQ if task there (relevant for FP)
  if (task->myList == rtsys->readyQs[task->affinity])
    task->moveToList(rtsys->readyQs[task->affinity]);
}

// Setting priority of calling task
void ttSetPriority(double prio) {
  
  if(rtsys->running == NULL) {
    TT_MEX_ERROR("ttSetPriority: No running task");
    return;
  }

  if (!rtsys->running->isUserTask()) {
    TT_MEX_ERROR("ttSetPriority: Can not be called by interrupt handler!");
    return;
  }

  ttSetPriority(prio, rtsys->running->name);
}

#endif
