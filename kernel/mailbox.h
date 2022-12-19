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

#ifndef MAILBOX_H
#define MAILBOX_H

class Mailbox {
 public:
  char* name;
  List *buffer;
  int maxSize;
  int count, counter;
  List *readerQ;
  List *writerQ;

  Mailbox(const char *n, int size);
  ~Mailbox();
};

/**
 * Mailbox Constructor 
 */
Mailbox::Mailbox(const char *n, int size) {
  if (n==NULL) {
    name = NULL;
  } else {
    name = new char[strlen(n)+1];
    strcpy(name, n);
  }
  maxSize = size;
  buffer = new List("MailboxBuffer", NULL);
  count = 0;
  counter = 0;
  readerQ = NULL;
  writerQ = NULL;
}

/**
 * Mailbox Destructor 
 */
Mailbox::~Mailbox() {
  DataNode *dn, *tmp;

  if (name != NULL) {
    delete[] name;
  }

  dn = (DataNode*) buffer->getFirst();
  while (dn != NULL) {
    tmp = dn;
    dn = (DataNode*) dn->getNext();

#ifdef KERNEL_MATLAB
    mxDestroyArray((mxArray *)tmp->data);
#endif

    delete tmp;
  }
  delete buffer;

  if (readerQ) delete readerQ;
  if (writerQ) delete writerQ;

}

#endif
