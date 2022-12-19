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

#ifndef DISCARD_UNSENT
#define DISCARD_UNSENT

#include "getnetwork.cpp"

// discard messages from queue, beginning from given message
int deleteMessages(NWmsg *msg) {
  NWmsg *next;
  int nbr = 0;

  while(msg){
    next = (NWmsg *)msg->getNext();
    msg->remove();

#ifndef KERNEL_C
    mxDestroyArray(msg->dataMATLAB);
#endif
    delete(msg);

    nbr++;
    msg = next;
  }
  return nbr;
}

// declare two versions
int ttDiscardUnsentMessages();
int ttDiscardUnsentMessages(int networkNbr);

int ttDiscardUnsentMessages()
{
  NetworkInterface* nwi = getNetwork(1);
  if (nwi == NULL) {
    TT_MEX_ERROR("ttDiscardUnsentMessages: Network not present!");
    return -1;
  }
  return ttDiscardUnsentMessages(1);
}

int ttDiscardUnsentMessages(int networkNbr)
{
  NetworkInterface* nwi = getNetwork(networkNbr);
  if (nwi == NULL) {
    char buf[200];
    sprintf(buf, "ttDiscardUnsentMessages: Network #%d not present!", networkNbr);
    TT_MEX_ERROR(buf);
    return -1;
  }

  RTnetwork *nwsys = nwi->nwsys;
  // get own network node handle
  NWnode *sender = nwsys->nwnodes[nwi->nodeNbr];

  int nbr;
  // discard all messages from preprocQ
  nbr = deleteMessages((NWmsg *)sender->preprocQ->getFirst());

  // The wireless network does not use the inputQ in the same way as the wired.
  if ( nwsys->type == CSMACD || nwsys->type == CSMAAMP || nwsys->type == RR ||
       nwsys->type == FDMA || nwsys->type == TDMA || nwsys->type == SFDSE){
    NWmsg *msg = (NWmsg *)sender->inputQ->getFirst();
    // is the first inputQ message just being sent?
    if (nwsys->sending == nwi->nodeNbr) {
      // yes, discard all but the first
      if (msg != NULL)
	nbr += deleteMessages((NWmsg *)msg->getNext());
    }
    else
      // no, discard whole inputQ
      nbr += deleteMessages(msg);
  }
  return nbr;
}

#endif
