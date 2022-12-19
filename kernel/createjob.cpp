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

#ifndef CREATE_JOB
#define CREATE_JOB

#include "getnode.cpp"
#include "invoketask.cpp"

void ttCreateJob(const char *taskName, double time);

void ttCreateJob(const char *taskName) {

  DataNode* dn = getNode(taskName, rtsys->taskList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttCreateJob: Non-existent task '%s'", taskName);
    TT_MEX_ERROR(buf);
    return;
  }

  if (rtsys->init_phase) {
    debugPrintf("enqueueing a job for future release\n");
    ttCreateJob(taskName, 0.0); // enqueue a job to be released when the simulation starts
  } else {
    UserTask* task = (UserTask*) dn->data; 
    invoke_task(task, "");
  }
}

void ttCreateJob(const char *taskName, double time) {

  DataNode* dn = getNode(taskName, rtsys->taskList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttCreateJob: Non-existent task '%s'", taskName);
    TT_MEX_ERROR(buf);
    return;
  }

  UserTask* task = (UserTask*) dn->data; 

  if (time <= rtsys->time - TT_TIME_RESOLUTION) {
    TT_MEX_ERROR("ttCreateJob: Cannot create job backwards in time!");
    return;
  }

  char timername[MAXCHARS];
  snprintf(timername, MAXCHARS, "tasktimer:%s", task->name);
  Timer *timer = new Timer(timername);
  timer->time = time;
  timer->period = 0.0;
  timer->isPeriodic = false;
  timer->task = task;
    
  // Insert pointer to timer in timerlist
  rtsys->timerList->appendNode(new DataNode(timer, timer->name));
  
  // Enter timer in timeQ
  timer->moveToList(rtsys->timeQ);
}

#endif
