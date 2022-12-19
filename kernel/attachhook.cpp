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

#ifndef ATTACH_HOOK
#define ATTACH_HOOK

#include "getnode.cpp"

void ttAttachHook(char* nameOfTask, int ID, void (*hook)(UserTask*)) {

  DataNode* dn = getNode(nameOfTask, rtsys->taskList);
  UserTask* task = (UserTask*) dn->data; 

  if (task == NULL)
    mexPrintf("ttAttachHook: Non-existent task '%s'\n", nameOfTask);
  else {
    switch (ID) {
    case ARRIVAL:
      task->arrival_hook = hook;
      break;
    case RELEASE:
      task->release_hook = hook;
      break;
    case START:
      task->start_hook = hook;
      break;
    case SUSPEND:
      task->suspend_hook = hook;
      break;
    case RESUME:
      task->resume_hook = hook;
      break;
    case FINISH:
      task->finish_hook = hook;
      break;
    default:
      mexPrintf("ttAttachHook: Unknown identifier, no hook attached!\n");
    }
  }
}


#endif
