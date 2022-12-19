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

#ifndef WAIT
#define WAIT

#include "exitmonitor.cpp"
#include "getnode.cpp"

void ttWait(char *nameOfEvent) {

  DataNode* dn;
  Event* ev;
  UserTask* task;

  dn = getNode(nameOfEvent, rtsys->eventList);
  if (dn == NULL) {
    // Event does not exist 
    char buf[MAXCHARS];
    sprintf(buf, "ttWait: Non-existent event '%s'!", nameOfEvent);
    TT_MEX_ERROR(buf);
    return;
  }

  task = (UserTask*) rtsys->running;
  ev = (Event*) dn->data;
  
  rtsys->running->moveToList(ev->waitingQ);
  task->state = WAITING;

  // Execute suspend hook
  task->suspend_hook(task);

  if (!ev->isFree) {
    ttExitMonitor(ev->mon->name);
  }
}

#endif
