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

#ifndef NOTIFY_ALL
#define NOTIFY_ALL

#include "getnode.cpp"

void ttNotify(const char *nameOfEvent) {
  
  DataNode* dn;
  Event* ev;
  List* newQ;
  UserTask* task;

  dn = getNode(nameOfEvent, rtsys->eventList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttNotify: Non-existent event '%s'!", nameOfEvent);
    TT_MEX_ERROR(buf);
    return;
  }

  ev = (Event*) dn->data;

  // get first waiting task (if any)
  task = (UserTask*) ev->waitingQ->getFirst();

  if (task != NULL) {
    // move the waiting task to readyQ (if free event)
    // or to waitingQ of associated monitor
    if (ev->isFree) {
      newQ = rtsys->readyQs[task->affinity];
      task->state = READY;
    } else {
      newQ = ev->mon->waitingQ;
      task->state = WAITING;
    }
    task->moveToList(newQ);
  }

}

void ttNotifyAll(const char *nameOfEvent) {
  
  DataNode* dn;
  Event* ev;
  Task *t, *tmp;
  List* newQ;

  dn = getNode(nameOfEvent, rtsys->eventList);
  if (dn == NULL) {
    char buf[200];
    sprintf(buf, "ttNotifyAll: Non-existent event '%s'!", nameOfEvent);
    TT_MEX_ERROR(buf);
    return;
  }

  // Event exists 
  ev = (Event*) dn->data;
  
  // move ALL waiting tasks to readyQ (if free event)
  // or to waitingQ of associated monitor

  t = (Task *) ev->waitingQ->getFirst();
  while (t != NULL) {
    if (ev->isFree) {
      newQ = rtsys->readyQs[t->affinity];
      t->state = READY;
    } else {
      newQ = ev->mon->waitingQ;
      t->state = WAITING;
    }
    tmp = t;
    t = (Task *)t->getNext();
    tmp->moveToList(newQ);
  }
}

#endif
