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

#ifndef ATTACH_TBS
#define ATTACH_TBS

#include "getnode.cpp"
#include "tbshooks.cpp"

void ttAttachTBS(const char* nameOfTask, const char* nameOfTBS) {

  DataNode *dn1, *dn2;

  dn1 = (DataNode*) getNode(nameOfTask, rtsys->taskList);
  if (dn1 == NULL) {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachTBS: Non-existent task '%s'", nameOfTask);
    TT_MEX_ERROR(buf);
    return;
  }

  UserTask* task = (UserTask*) dn1->data;

  dn2 = (DataNode*) getNode(nameOfTBS, rtsys->tbsList);
  if (dn2 == NULL) {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachTBS: Non-existent TBS '%s'", nameOfTBS);
    TT_MEX_ERROR(buf);
    return;
  }

  if (task->wcExecTime == task->deadline)   // WCET not defined, WCET set to deadline in createtask.cpp
  {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachTBS: Worst-case execution time for task %s not specified", task->name);
    TT_MEX_ERROR(buf);
    return;
  }

  TBS* tbs = (TBS*) dn2->data;

  task->tbs = tbs;

  if (tbs->affinity == -1 || tbs->affinity == task->affinity) {
    tbs->affinity = task->affinity;
  } else {
    char buf[MAXERRBUF];
    sprintf(buf, "ttAttachTBS: TBS '%s' already has affinity to core %d\n", tbs->name, tbs->affinity);
    TT_MEX_ERROR(buf);
    return;
  }

  task->arrival_hook = TBS_arrival;

}

#endif
