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

#ifndef SET_BUDGET
#define SET_BUDGET

#include "getnode.cpp"

// set budget of specific task
void ttSetBudget(double budget, const char *nameOfTask) {
  
  DataNode* dn = getNode(nameOfTask, rtsys->taskList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttSetBudget: Non-existent task '%s'!", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }
  if (budget < -TT_TIME_RESOLUTION) {
    TT_MEX_ERROR("ttSetBudget: Negative budget!");
    return;
  }

  UserTask* task = (UserTask*) dn->data; 
  if (task->nbrInvocations == 0) {
    char buf[200];
    sprintf(buf, "ttSetBudget: No running job of task '%s'!", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }

  //if (rtsys->running == task->WCEThandler) {
  //  task->budget -= (rtsys->time - task->lastStart); // should be zero
  //  task->lastStart = rtsys->time;
  //}
  
  //  double diff = budget - task->budget; 
  
  task->budget = budget;

  // Update WCET overrun timer
  //if (task->WCEThandler != NULL) {
  //  task->activationtimer->time += diff;
  //  task->activationtimer->moveToList(rtsys->timeQ);
  //}
}

// set budget of calling task
void ttSetBudget(double budget) {

  if (rtsys->running == NULL) {
    TT_MEX_ERROR("ttSetBudget: No running task!");
    return;
  }

  if (!rtsys->running->isUserTask()) {
    TT_MEX_ERROR("ttSetBudget: Can not be called by interrupt handler!");
    return;
  }

  ttSetBudget(budget, rtsys->running->name);
}

#endif
