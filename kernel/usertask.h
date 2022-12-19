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

#ifndef USER_TASK_H
#define USER_TASK_H

class InterruptHandler;
class Timer;
class CBS;
class TBS;
class TimeTriggeredDispatcher;
class SlotShifting;

/**
 * UserTask class, inherits from Task
 */
class UserTask : public Task {
public:
  double priority; 
  double wcExecTime;
  double deadline;

  bool display;       // show schedule graph or not
  
  double absDeadline; // absolute deadline of the curent job
  double arrival;     // arrival time of the current job
  double release;     // release time of the current job
  double budget;      // budget of the current job

  bool prioRaised;    // Implementing priority inheritance
  double tempPrio;

  Timer *activationTimer;  // timer generating periodic jobs
  Timer* DLtimer;    // pointer to deadline overrun timer
  Timer* WCETtimer;  // pointer to execution overrun timer

  CBS* cbs;          // pointer to constant bandwidth server
  TBS* tbs;          // pointer to total bandwidth server
  TimeTriggeredDispatcher* ttdisp;	  // pointer to Time Triggered Dispatcher
  SlotShifting* slotShift;	// pointer to slot shifter
 
  // Time Triggered Dispatcher
  int taskIdentifier;

  // Variables for slot shifting Scheduling
  bool isGuaranteed;				// indicate if task is guaranteed (i.e. associated to any interval) in slot shifting
  bool firmTask;      				// If task is firm or soft

  Log* logs[NBRLOGS];

  void (*arrival_hook)(UserTask*);  
  void (*release_hook)(UserTask*);
  void (*start_hook)(UserTask*);
  void (*suspend_hook)(UserTask*);
  void (*resume_hook)(UserTask*);
  void (*finish_hook)(UserTask*);
  void (*runkernel_hook)(UserTask*, double);
 
  bool isUserTask();
  double wakeupTime();
  void print();

  UserTask(const char *n); 
  ~UserTask();
};

/**
 * UserTask Constructor 
 */
UserTask::UserTask(const char *n) 
  : Task(n)
{
  prioRaised = false;
  tempPrio = 0.0;
  activationTimer = NULL;
  DLtimer = NULL;
  WCETtimer = NULL;
  cbs = NULL;
  tbs = NULL;
  ttdisp = NULL;
  slotShift = NULL;
  isGuaranteed = false;
  firmTask = false;
  for (int i=0; i < NBRLOGS; i++) {
    logs[i] = NULL;
  }
}

/**
 * UserTask Destructor 
 */
UserTask::~UserTask() {
  
  return;

  for (int i=0; i < NBRLOGS; i++) {
    if (logs[i] != NULL) {
      delete logs[i];
    }
  }
}

/**
 * Implementation of virtual methods
 */
bool UserTask::isUserTask() {
  return true;
}

double UserTask::wakeupTime() {
  return release;
}

void UserTask::print() {
  mexPrintf("USERTASK (name: %s prio: %f dl: %f rel.: %f raised prio: %f)", name,priority,deadline,release,tempPrio);
}

#endif
