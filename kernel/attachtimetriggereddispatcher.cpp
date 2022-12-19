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

#ifndef ATTACH_TIME_TRIGGERED_DISPATCHER
#define ATTACH_TIME_TRIGGERED_DISPATCHER

#include "getnode.cpp"

void ttAttachTimeTriggeredDispatcher(const char* nameOfTask, const char* nameOfdispatcher) {

  DataNode *dn1, *dn2;

  dn1 = (DataNode*) getNode(nameOfTask, rtsys->taskList);
  if (dn1 == NULL) {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachTimeTriggeredDispatcher: Non-existent task '%s'", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }

  UserTask* task = (UserTask*) dn1->data;
  if (task->arrival_hook != rtsys->default_arrival)		// Check if free
  {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachTimeTriggeredDispatcher: Unable to attach task %s to %s time-triggered dispatcher. It is already attached to some scheduler/dispatcher!", nameOfTask, nameOfdispatcher);
    TT_MEX_ERROR(buf);
    return;
  }

  dn2 = (DataNode*) getNode(nameOfdispatcher, rtsys->ttdispList);
  if (dn2 == NULL) {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachTimeTriggeredDispatcher: Non-existent time-triggered dispatcher '%s'", nameOfdispatcher);
    TT_MEX_ERROR(buf);
    return;
  }

  TimeTriggeredDispatcher* disptacher = (TimeTriggeredDispatcher*) dn2->data;

  if (disptacher->affinity == -1 || disptacher->affinity == task->affinity) { // Assign affinity to core
    disptacher->affinity = task->affinity;
    disptacher->slotTimer->task->affinity = task->affinity;
  } else {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachTimeTriggeredDispatcher: Time-triggered dispatcher class '%s' already has affinity to core %d\n", nameOfdispatcher, disptacher->affinity);
    TT_MEX_ERROR(buf);
    return;
  }
  task->ttdisp = disptacher;

  // No change in task hooks is required
}

#endif
