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

#ifndef GIVE
#define GIVE

#include "getnode.cpp"

void ttGive(const char *nameOfSemaphore) {

  Semaphore* sem;
  UserTask* task;

  DataNode* dn = getNode(nameOfSemaphore, rtsys->semaphoreList);
  if (dn == NULL) {
    // Semaphore does not exist 
    char buf[200];
    sprintf(buf, "ttGive: Non-existent semaphore '%s'!", nameOfSemaphore);
    TT_MEX_ERROR(buf);
    return;
  }

  task = (UserTask*) rtsys->running;
  sem = (Semaphore*) dn->data;
  
  if (sem->value >= sem->maxval) {  // Check if maximum already reached
    return;                         // In that case, do nothing
  }

  sem->value++;

  if (sem->value <= 0) {  // This test is probably unnecessary

    // Move first waiting task to readyQ
    task = (UserTask*) sem->waitingQ->getFirst();
    if (task != NULL) {
      task->moveToList(rtsys->readyQs[task->affinity]);
      task->state = READY;
    }

  }

}

#endif
