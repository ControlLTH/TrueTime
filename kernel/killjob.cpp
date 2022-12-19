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

#ifndef KILL_JOB
#define KILL_JOB

#include "getnode.cpp"

void ttKillJob(const char *taskName) {

  DataNode* dn = getNode(taskName, rtsys->taskList);
  UserTask* task = (UserTask*) dn->data; 

  if (task == NULL) {
    char buf[200];
    sprintf(buf, "ttKillJob: Non-existent task '%s'!", taskName);
    TT_MEX_ERROR(buf);
    return;
  }
  if (task->nbrInvocations == 0) {
    char buf[200];
    sprintf(buf, "ttKillJob: No job to kill for task '%s'!", taskName);
    TT_MEX_ERROR(buf);
    return;
  }
  if (task == rtsys->running) {
    char buf[200];
    sprintf(buf,"ttKillJob: Can not kill myself while running!");
    TT_MEX_ERROR(buf);
    return;
  }

  task->execTime = 0.0;
  task->segment = 0;
  // Remove task from current Q
  task->remove();

  // Execute finished-hook
  task->finish_hook(task);

  task->nbrInvocations--;
  // Immediately release next job of the task, if any
  if (task->nbrInvocations > 0) {
    dn = (DataNode*) task->pending->getFirst();
    TaskInvocation *ti = (TaskInvocation *)dn->data;
    task->moveToList(rtsys->readyQs[task->affinity]);
    task->release = ti->timestamp;
    task->release_hook(task);
    task->pending->deleteNode(dn);
    delete ti;
  }
}

#endif
