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

#ifndef NO_SCHEDULE
#define NO_SCHEDULE

#include "getnode.cpp"

void ttNoSchedule(const char* name) {

  DataNode* dn1, *dn2;
  UserTask* task;
  InterruptHandler* handler;

  // can only be called during initialization phase
  if (!rtsys->init_phase) {
    TT_MEX_ERROR("ttNoSchedule: Can only be called from the init-function!");
    return;
  }

  dn1 = getNode(name, rtsys->taskList);
  dn2 = getNode(name, rtsys->handlerList);
  
  if (dn1 == NULL && dn2 == NULL) {
    char buf[200];
    sprintf(buf, "ttNoSchedule: Non-existent task or handler '%s'\n", name);
    TT_MEX_ERROR(buf);
  }
  
  if (dn1 != NULL) {
    task = (UserTask*) dn1->data; 
    if (task->display == true) {
      task->display = false;
      rtsys->nbrOfSchedTasks--;
    }
  }
  
  if (dn2 != NULL) {
    handler = (InterruptHandler*) dn2->data; 
    if (handler->display == true) {
      handler->display = false;
      rtsys->nbrOfSchedTasks--;
    }
  }
}

#endif
