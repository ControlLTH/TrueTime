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

#ifndef SET_ABS_DEADLINE
#define SET_ABS_DEADLINE

#include "getnode.cpp"

// set absolute deadline of specific task
void ttSetAbsDeadline(double time, const char *nameOfTask) {

  DataNode* dn = getNode(nameOfTask, rtsys->taskList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttSetAbsDeadline: Non-existent task '%s'!", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }
  UserTask* task = (UserTask*) dn->data; 
  if (task->nbrInvocations == 0) {
    char buf[200];
    sprintf(buf, "ttSetAbsDeadline: No running job for '%s'!", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }
  
  // set the absolute deadline
  task->absDeadline = time;
  if (task->DLtimer != NULL) {
    
    // update deadline overrun timer (if any)
    if (task->DLtimer->myList == rtsys->timeQ) {
      task->DLtimer->remove();
      task->DLtimer->time = time;
      task->DLtimer->moveToList(rtsys->timeQ);
    }
  }
    
  // Reshuffle readyQ if task there (relevant for EDF)
  if (task->myList == rtsys->readyQs[task->affinity]) {
    task->moveToList(rtsys->readyQs[task->affinity]);
  }
}

// Set absolute deadline of calling task
void ttSetAbsDeadline(double time) {
  
  if (rtsys->running == NULL) {
    TT_MEX_ERROR("ttSetAbsDeadline: No running task!");
    return;
  }

  if (!rtsys->running->isUserTask()) {
    TT_MEX_ERROR("ttSetAbsDeadline: Can not be called by interrupt handler!");
    return;
  }

  ttSetAbsDeadline(time, rtsys->running->name);
}

#endif
