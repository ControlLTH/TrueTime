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

#ifndef CREATE_MAILBOX
#define CREATE_MAILBOX

#include "getnode.cpp"

void ttCreateMailbox(const char *nameOfMailbox, int maxSize) {

  DataNode* dn;
  Mailbox* m;
  
  if (strcmp(nameOfMailbox,"") == 0) {
    TT_MEX_ERROR("ttCreateMailbox: Name should be a non-empty string!");
    return;
  }
  if (rtsys->prioFcn == NULL) {
    TT_MEX_ERROR("ttCreateMailbox: Kernel must be initialized before creation of mailboxes!");
    return;
  } 
  if (maxSize < 1) {
    TT_MEX_ERROR("ttCreateMailbox: Size of mailbox must be greater than zero!");
    return;
  }
  dn = getNode(nameOfMailbox, rtsys->mailboxList);
  if (dn != NULL) { 
    TT_MEX_ERROR("ttCreateMailbox: Name of mailbox not unique!");
    return;
  }

  m = new Mailbox(nameOfMailbox, maxSize);
  m->readerQ = new List("ReaderQ", NULL); // waiting reader tasks in FIFO order
  m->writerQ = new List("WriterQ", NULL); // waiting writer tasks in FIFO order

  rtsys->mailboxList->appendNode(new DataNode(m, m->name));
}


void ttCreateMailbox(const char *nameOfMailbox) {

  ttCreateMailbox(nameOfMailbox, INT_MAX);

}

#endif
