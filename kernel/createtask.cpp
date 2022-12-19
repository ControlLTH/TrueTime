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

#ifndef CREATE_TASK
#define CREATE_TASK

#include "getnode.cpp"

#include "setpriority.cpp"

// Master function used by the other user functions below

bool ttCreateTask(const char *name, double starttime, double period, double deadline, double (*codeFcn)(int, void*), void *data) {
  UserTask* task;
  Timer* timer;

  //mexPrintf("ttCreateTask in kernel %s\n", rtsys->blockName);

  if (strcmp(name,"") == 0) {
    TT_MEX_ERROR("ttCreate(Periodic)Task: Name should be a non-empty string!");
    return false;
  }
  if (rtsys->prioFcn == NULL) {
    TT_MEX_ERROR("ttCreate(Periodic)Task: Kernel must be initialized before creation of tasks!");
    return false;
  }
  DataNode* dn = getNode(name, rtsys->taskList);
  if (dn != NULL) { 
    TT_MEX_ERROR("ttCreate(Periodic)Task: Name of task not unique! Task not created!");
    return false;
  }
  
  task = new UserTask(name);

  task->codeFcn = codeFcn;

  task->priority = 0.0;
  task->wcExecTime = deadline;
  task->deadline = deadline;
  task->data = data;
  
  task->taskIdentifier = rtsys->taskList->length() + 1;

  task->display = true;
  rtsys->nbrOfSchedTasks++;  // One more schedule graph

  task->absDeadline = deadline;
  task->release = 0.0;
  task->budget = deadline;
  
  task->arrival_hook  = rtsys->default_arrival;
  task->release_hook  = rtsys->default_release;
  task->start_hook    = rtsys->default_start;
  task->suspend_hook  = rtsys->default_suspend;
  task->resume_hook   = rtsys->default_resume;
  task->finish_hook   = rtsys->default_finish;
  task->runkernel_hook   = rtsys->default_runkernel;
      
  rtsys->taskList->appendNode(new DataNode(task, task->name));

  // Create (Periodic)Timer if starttime given 
  if (starttime >= 0) {
    char timername[MAXCHARS];
    snprintf(timername, MAXCHARS, "tasktimer:%s", task->name);
    timer = new Timer(timername);
    timer->time = starttime;
    if (period > 0) {
      timer->period = period;      // periodic timer
      timer->isPeriodic = true;
      task->activationTimer = timer;
    } else {
      timer->period = 0.0;         // one-shot timer
      timer->isPeriodic = false;
    }
    timer->task = task;
    
    // Insert pointer to timer in timerlist
    rtsys->timerList->appendNode(new DataNode(timer, timer->name));
  
    // Enter timer in timeQ
    timer->moveToList(rtsys->timeQ);
  }

  return true;
  
}

// ------------------------------ C++ API ------------------------------------


void ttCreateTask(const char *name, double deadline, double (*codeFcn)(int, void*))
{
  ttCreateTask(name, -1.0, -1.0, deadline, codeFcn, NULL);
}

void ttCreateTask(const char *name, double deadline, double (*codeFcn)(int, void*), void *data)
{
  ttCreateTask(name, -1.0, -1.0, deadline, codeFcn, data);
}

void ttCreatePeriodicTask(const char *name, double period, double (*codeFcn)(int, void*))
{
  mexPrintf("Warning: Deprecated use of ttCreatePeriodicTask\n");
  ttCreateTask(name, 0.0, period, period, codeFcn, NULL);
}

void ttCreatePeriodicTask(const char *name, double period, double (*codeFcn)(int, void*), void *data)
{
  mexPrintf("Warning: Deprecated use of ttCreatePeriodicTask\n");
  ttCreateTask(name, 0.0, period, period, codeFcn, data);
}

void ttCreatePeriodicTask(const char *name, double starttime, double period, double (*codeFcn)(int, void*))
{
  ttCreateTask(name, starttime, period, period, codeFcn, NULL);
}

void ttCreatePeriodicTask(const char *name, double starttime, double period, double (*codeFcn)(int, void*), void *data)
{
  ttCreateTask(name, starttime, period, period, codeFcn, data);
}


/* For backward compatibility with TrueTime 1.5 */

void ttCreateTask(const char *name, double deadline, double priority, double (*codeFcn)(int, void*), void *data)
{
  mexPrintf("Warning: deprecated use of ttCreateTask!\n"
	    "Use ttCreateTask and ttSetPriority instead.\n");
  ttCreateTask(name, -1.0, -1.0, deadline, codeFcn, data);
  ttSetPriority(priority, name);
}

void ttCreatePeriodicTask(const char *name, double starttime, double period, double priority, double (*codeFcn)(int, void*), void *data)
{
  mexPrintf("Warning: deprecated use of ttCreatePeriodicTask!\n"
	    "Use ttCreatePeriodicTask and ttSetPriority instead.\n");
  ttCreateTask(name, starttime, period, period, codeFcn, data);
  ttSetPriority(priority, name);
}


// ------------------------- For the MATLAB API -------------------------------

void ttCreateTaskMATLAB(const char *name, double starttime, double period, double deadline, char *codeFcnMATLAB, const mxArray* dataMATLAB)
{

  if (ttCreateTask(name, starttime, period, deadline, NULL, NULL)) {
  
    // Add name of code function (m-file) and data variable
    UserTask *task = (UserTask*)((DataNode*)rtsys->taskList->getLast())->data;
    task->codeFcnMATLAB = new char[strlen(codeFcnMATLAB)+1];
    strcpy(task->codeFcnMATLAB, codeFcnMATLAB);
    mxArray* data;
    if (dataMATLAB == NULL) { // no data specified
      data = mxCreateDoubleMatrix(0, 0, mxREAL);
    } else {
      data = mxDuplicateArray(dataMATLAB);
    }
    mexMakeArrayPersistent(data);
    task->dataMATLAB = data;
  }
}

#endif
