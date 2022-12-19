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

#ifndef ATTACH_WCET_HANDLER
#define ATTACH_WCET_HANDLER

#include "getnode.cpp"

void ttAttachWCETHandler(const char* nameOfTask, const char* nameOfHandler) {

  DataNode *dn1, *dn2;
  
  dn1 = (DataNode*) getNode(nameOfTask, rtsys->taskList);
  if (dn1 == NULL) {
    char buf[200];
    sprintf(buf, "ttAttachWCETHandler: Non-existent task '%s'", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }

  UserTask* task = (UserTask*) dn1->data;

  if (task->WCETtimer != NULL) {
    char buf[200];
    sprintf(buf, "ttAttachWCETHandler: A handler is already attached to task '%s'", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }

  dn2 = (DataNode*) getNode(nameOfHandler, rtsys->handlerList);
  if (dn2 == NULL) {
    char buf[200];
    sprintf(buf, "ttAttachWCETHandler: Non-existent handler '%s'", nameOfHandler);
    TT_MEX_ERROR(buf);
    return;
  }

  InterruptHandler* hdl = (InterruptHandler*) dn2->data;

  // Create an execution-time overrun timer
  char timername[MAXCHARS];
  snprintf(timername, MAXCHARS, "WCETtimer:%s", task->name);
  Timer *timer = new Timer(timername);
  timer->period = -1.0;
  timer->isPeriodic = false;
  timer->isOverrunTimer = true;
  timer->task = hdl;
  task->WCETtimer = timer;
  // Insert timer in timerList so that it can be later retrieved by name
  rtsys->timerList->appendNode(new DataNode(timer, timer->name));
}

#endif
