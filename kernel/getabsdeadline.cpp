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

#ifndef GET_ABS_DEADLINE
#define GET_ABS_DEADLINE

#include "getnode.cpp"

// get absolute deadline of specific task
double ttGetAbsDeadline(const char *nameOfTask) {

  DataNode* dn = getNode(nameOfTask, rtsys->taskList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttGetAbsDeadline: Non-existent task '%s'!", nameOfTask);
    TT_MEX_ERROR(buf);
    return 0.0;
  }

  UserTask* task = (UserTask*) dn->data; 
  if (task->nbrInvocations == 0) {
    char buf[200];
    sprintf(buf, "ttGetAbsDeadline: No running job of task '%s'!", nameOfTask);
    TT_MEX_ERROR(buf);
    return 0.0;
  }

  if (task->cbs) {
    // The task is associated with a CBS: inherit the deadline of the CBS
    return task->cbs->ds;
  } else {
    // Else, return the absolute deadline of the task
    return task->absDeadline;
  }

}  

// get absolute deadline of calling task
double ttGetAbsDeadline() {

  if (!rtsys->running->isUserTask()) {
    TT_MEX_ERROR("ttGetAbsDeadline: Can not be called by interrupt handler!");
    return 0.0;
  }

  UserTask* task = (UserTask*) (rtsys->running); 

  if (task->cbs) {
    // The task is associated with a CBS: inherit the deadline of the CBS
    return task->cbs->ds;
  } else {
    // Else, return the absolute deadline of the task
    return task->absDeadline;
  }
}

#endif
