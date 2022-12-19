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

#ifndef FETCH
#define FETCH

#include "getnode.cpp"

void ttFetch(const char* mailbox) {

  DataNode* dn = getNode(mailbox, rtsys->mailboxList);
  if (dn == NULL) {
    // Mailbox does not exist 
    char buf[200];
    sprintf(buf, "ttFetch: Non-existent mailbox '%s'\n", mailbox);
    TT_MEX_ERROR(buf);
    return;
  }

  UserTask* task = (UserTask*) rtsys->running;

  if (task->mb_data != NULL) {
    // A message was already fetched but not retrieved!
    TT_MEX_ERROR("ttFetch: Previously fetched message was not retrieved!\n");
    return;
  }

  Mailbox* m = (Mailbox*) dn->data;
  if (m->count == 0) {
    //mexPrintf("ttFetch: Mailbox '%s' is empty, blocking\n", mailbox);
    // block the fetching task
    task->moveToList(m->readerQ);
    task->state = WAITING;

    // Execute suspend hook
    task->suspend_hook(task);
  } else {
  
    dn = (DataNode*) m->buffer->getFirst();
    task->mb_data = (void*)dn->data;  // save it for later (ttRetrieve)
    m->buffer->deleteNode(dn);
    m->count--;
  }
  
  // release first waiting writer, if any
  UserTask* task2 = (UserTask*) m->writerQ->getFirst();
  if (task2 != NULL) {
    m->buffer->appendNode(new DataNode(task2->mb_data, NULL));
    task2->mb_data = NULL;
    m->count++;   // number of messages
    task2->moveToList(rtsys->readyQs[task2->affinity]);
    task2->state = READY;
  }
}

#endif
