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

#ifndef CREATE_SEMAPHORE
#define CREATE_SEMAPHORE

#include "getnode.cpp"

void ttCreateSemaphore(const char *name, int initval, int maxval) {

  DataNode *dn;
  Semaphore* sem;

  if (strcmp(name,"") == 0) {
    TT_MEX_ERROR("ttCreateSemaphore: Name should be a non-empty string!"); 
    return;
  }
  if (rtsys->prioFcn == NULL) {
    TT_MEX_ERROR("ttCreateSemaphore: Kernel must be initialized before creation of semaphores!");
    return;
  }

  dn = getNode(name, rtsys->semaphoreList);
  if (dn!=NULL) {
    TT_MEX_ERROR("ttCreateSemaphore: Name of semaphore not unique!");
    return;
  }

  sem = new Semaphore(name); 
  sem->value = initval;
  sem->maxval = maxval;
  sem->waitingQ = new List("WaitingQ", NULL); // waiting tasks in FIFO order
  rtsys->semaphoreList->appendNode(new DataNode(sem, sem->name));
}

void ttCreateSemaphore(const char *name, int initval) {

  ttCreateSemaphore(name, initval, INT_MAX);

}


#endif
