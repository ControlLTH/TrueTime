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

#ifndef DEFAULT_HOOKS
#define DEFAULT_HOOKS

/**
 * Default hooks used to implement overrun handling, 
 * context switch simulation and logging.
 */

#include "logstart.cpp"
#include "logstop.cpp"
#include "lognow.cpp"

/* forward declaration */



void default_runkernel(UserTask *task, double duration) {

  task->budget -= duration;

}

void default_arrival(UserTask *task) {

  // Store arrival time in relevant logs
  logstart(task->logs[RESPONSETIMELOG]);
  logstart(task->logs[RELEASELATENCYLOG]);
  logstart(task->logs[STARTLATENCYLOG]);

}


void default_release(UserTask *task) {

  debugPrintf("Release-hook for task '%s' at time %5.8f\n",task->name,rtsys->time);

  // Calculate absolute deadline and assign execution budget
  task->absDeadline = task->arrival + task->deadline;
  task->budget = task->wcExecTime;

  // Simulate context switch  
  if (rtsys->contextSwitchTime > TT_TIME_RESOLUTION) {
    rtsys->contextSimTime = rtsys->contextSwitchTime;
    invoke_task(rtsys->kernelHandler, "");
  }

  // Set the deadline overrun timer (if there is one)
  if (task->DLtimer != NULL) {
    task->DLtimer->time = task->absDeadline;
    task->DLtimer->moveToList(rtsys->timeQ);
  }

  // Store release latency in log
  logstop(task->logs[RELEASELATENCYLOG]);

}

void default_start(UserTask *task) {

  debugPrintf("Start-hook for task '%s' at time %5.8f\n",task->name,rtsys->time);

  // Set the execution-time overrun timer (if there is one)
  if (task->WCETtimer != NULL && task->budget > TT_TIME_RESOLUTION) {
    task->WCETtimer->time = rtsys->time + task->budget;
    task->WCETtimer->moveToList(rtsys->timeQ);
  }

  // store start latency in log
  logstop(task->logs[STARTLATENCYLOG]);  

  // execution time log entry
  Log* log = task->logs[EXECTIMELOG-1];
  if (log) {
    if (log->entries < log->size) {
      log->vals[log->entries] = 0.0;
      log->temp = simtime2time(rtsys->time);
    }
  }

}


void default_suspend(UserTask* task) {

  debugPrintf("Suspend-hook for task '%s' at time %5.8f\n",task->name,rtsys->time);

  // Remove exectime-time overrun timer from timeQ (if it exists)
  if (task->WCETtimer) {
    task->WCETtimer->remove();
  }

  if (rtsys->contextSwitchTime > TT_TIME_RESOLUTION) {
    rtsys->contextSimTime = rtsys->contextSwitchTime;
    invoke_task(rtsys->kernelHandler, "");
  } 

  // update execution time log entry
  Log *log = task->logs[EXECTIMELOG-1];
  if (log) {
    if (log->entries < log->size) {
      log->vals[log->entries] += (simtime2time(rtsys->time) - log->temp);
      log->temp = simtime2time(rtsys->time);
    }
  }
}


void default_resume(UserTask* task) {

  debugPrintf("Resume-hook for task '%s' at time %5.8f\n",task->name,rtsys->time);

  // Set the execution-time overrun timer (if there is one)
  if (task->WCETtimer != NULL && task->budget > TT_TIME_RESOLUTION) {
    task->WCETtimer->time = rtsys->time + task->budget;
    task->WCETtimer->moveToList(rtsys->timeQ);
  }
}


void default_finish(UserTask* task) {

  debugPrintf("Finish-hook for task '%s' at time %5.8f\n",task->name,rtsys->time);
  
  // Remove deadline and exectime overrun timers from timeQ (if they exist)
  if (task->DLtimer) {
    task->DLtimer->remove();
  }
  if (task->WCETtimer) {
    task->WCETtimer->remove();
  }
  
  // Simulate context switch  
  if (rtsys->contextSwitchTime > TT_TIME_RESOLUTION) {
    rtsys->contextSimTime = rtsys->contextSwitchTime; 
    invoke_task(rtsys->kernelHandler,"");
  } 
  
  // store response time in log
  logstop(task->logs[RESPONSETIMELOG]);  

  // store execution time in log
  Log* log = task->logs[EXECTIMELOG];
  if (log) {
    double exectime = log->vals[log->entries] + (simtime2time(rtsys->time) + 
				rtsys->contextSwitchTime - log->temp);
    logwrite(log, exectime);
  }
}

#endif
