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

#ifndef RETRIEVE
#define RETRIEVE

#include "getnode.cpp"

void* ttRetrieve(const char* mailbox) {

  DataNode* dn;
  void* msg;

  dn = getNode(mailbox, rtsys->mailboxList);
  if (dn == NULL) {
    // Mailbox does not exist
    char buf[200];
    sprintf(buf, "ttRetrieve: Non-existent mailbox '%s'\n", mailbox);
    TT_MEX_ERROR(buf);
    return NULL;
  }

  UserTask* task = (UserTask*) rtsys->running;

  msg = task->mb_data;   // NULL is a valid message
  task->mb_data = NULL;  // mark the data as read 

  return msg;
}
#endif

