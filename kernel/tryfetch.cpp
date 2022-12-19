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

#ifndef TRY_FETCH
#define TRY_FETCH

#include "getnode.cpp"

void* ttTryFetch(const char* mailbox) {

  DataNode* dn;
  Mailbox* m;
  void* msg;

  dn = getNode(mailbox, rtsys->mailboxList);
  if (dn == NULL) {
    // Mailbox does not exist
    char buf[200];
    sprintf(buf, "ttTryFetch: Non-existent mailbox '%s'\n", mailbox);
    TT_MEX_ERROR(buf);
    return NULL;
  }

  m = (Mailbox*) dn->data;
  if (m->count == 0) {
    //mexPrintf("ttTryFetch: Mailbox '%s' is empty\n", mailbox);
    return NULL;
  } else {
    dn = (DataNode*) m->buffer->getFirst();
    msg = dn->data;
    m->buffer->deleteNode(dn);
    m->count--;

    // release first waiting writer, if any
    UserTask* task2 = (UserTask*) m->writerQ->getFirst();
    if (task2 != NULL) {
      m->buffer->appendNode(new DataNode(task2->mb_data, NULL));
      task2->mb_data = NULL;
      m->count++;   // number of messages
      task2->moveToList(rtsys->readyQs[task2->affinity]);
    }
  }
  return msg;
}

#endif
