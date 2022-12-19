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

#ifndef ENTER_MONITOR
#define ENTER_MONITOR

#include "getnode.cpp"

void ttEnterMonitor(const char *nameOfMonitor) {
  
  Monitor* mon;
  UserTask* task;
  
  DataNode* dn = getNode(nameOfMonitor, rtsys->monitorList);
  if (dn == NULL) {
    // Monitor does not exist 
    char buf[200];
    sprintf(buf, "ttEnterMonitor: Non-existent monitor '%s'!", nameOfMonitor);
    TT_MEX_ERROR(buf);
    return;
  }

  task = (UserTask*) rtsys->running;
  mon = (Monitor*) dn->data;

  if (mon->heldBy == NULL) { // Free 
    mon->heldBy = task;
  }
  else { // Not free
    
    task->moveToList(mon->waitingQ);
    task->state = WAITING;
    // Execute suspend hook
    task->suspend_hook(task);

    // Priority Inheritance
    double blockerPrio = (mon->heldBy->prioRaised) ? mon->heldBy->tempPrio : rtsys->prioFcn(mon->heldBy);
    double blockedPrio = rtsys->prioFcn(task);
    if (blockerPrio > blockedPrio) { // blocked by a lower-priority task
      mon->heldBy->tempPrio = blockedPrio;
      mon->heldBy->prioRaised = true;
      // Reshuffle readyQ
      if (mon->heldBy->myList == rtsys->readyQs[rtsys->currentCPU]) {
	mon->heldBy->moveToList(rtsys->readyQs[rtsys->currentCPU]);
 	mon->heldBy->state = READY;
      }
    }

  }

}

#endif
