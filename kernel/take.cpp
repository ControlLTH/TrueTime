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

#ifndef TAKE
#define TAKE

#include "getnode.cpp"

void ttTake(const char *nameOfSemaphore) {
  
  Semaphore* sem;
  UserTask* task;
  
  DataNode* dn = getNode(nameOfSemaphore, rtsys->semaphoreList);
  if (dn == NULL) {
    // Semaphore does not exist 
    char buf[200];
    sprintf(buf, "ttTake: Non-existent semaphore '%s'!", nameOfSemaphore);
    TT_MEX_ERROR(buf);
    return;
  }

  task = (UserTask*) rtsys->running;
  sem = (Semaphore*) dn->data;

  sem->value--;

  if (sem->value < 0) { // Not free 
    task->moveToList(sem->waitingQ);
    task->state = WAITING;
    // Execute suspend hook
    task->suspend_hook(task);

  }

}

#endif
