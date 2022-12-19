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

#ifndef ATTACH_CBS
#define ATTACH_CBS

#include "getnode.cpp"
#include "cbshooks.cpp"

void ttAttachCBS(const char* nameOfTask, const char* nameOfCBS) {

  DataNode *dn1, *dn2;

  dn1 = (DataNode*) getNode(nameOfTask, rtsys->taskList);
  if (dn1 == NULL) {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachCBS: Non-existent task '%s'", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }

  UserTask* task = (UserTask*) dn1->data;

  dn2 = (DataNode*) getNode(nameOfCBS, rtsys->cbsList);
  if (dn2 == NULL) {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachCBS: Non-existent CBS '%s'", nameOfCBS);
    TT_MEX_ERROR(buf);
    return;
  }

  CBS* cbs = (CBS*) dn2->data;

  task->cbs = cbs;

  if (cbs->affinity == -1 || cbs->affinity == task->affinity) {
    cbs->affinity = task->affinity;
    cbs->cbsTimer->task->affinity = task->affinity;
    if (cbs->type == 1) {
      cbs->overloadTimer->task->affinity = task->affinity;
    }
  } else {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachCBS: CBS '%s' already has affinity to core %d\n", nameOfCBS, cbs->affinity);
    TT_MEX_ERROR(buf);
    return;
  }

  task->arrival_hook = CBS_arrival;
  task->start_hook = CBS_start;
  task->suspend_hook = CBS_suspend;
  task->resume_hook = CBS_resume;
  task->finish_hook = CBS_finish;
  task->runkernel_hook = CBS_runkernel;

}

#endif
