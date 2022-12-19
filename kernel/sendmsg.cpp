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

#ifndef SEND_MSG
#define SEND_MSG

#include "getnetwork.cpp"

// do the dirty work: poke around inside nwsys of the network block 
void nwSendMsg(NWmsg *nwmsg, RTnetwork *nwsys) {
  // set time when finished preprocessing
  nwmsg->waituntil = (rtsys->time- rtsys->clockOffset) / rtsys->clockDrift
    + nwsys->nwnodes[nwmsg->sender]->predelay;
  nwmsg->collided = 0; // This message has not collided (802.11)

  // enqueue message in preprocQ
  nwsys->nwnodes[nwmsg->sender]->preprocQ->appendNode(nwmsg);
}

// Generic internal functions, used by the various API functions below

void ttSendMsg(int networkNbr, int receiver, int length, void *data, mxArray *dataMATLAB, int priority)
{
  NetworkInterface* nwi = getNetwork(networkNbr);
  if (nwi == NULL) {
    char buf[200];
    sprintf(buf, "ttSendMsg: Network #%d not present!", networkNbr);
    TT_MEX_ERROR(buf);
    return;
  }
  
  if (receiver < 0 || receiver > nwi->nwsys->nbrOfNodes) {
    char buf[200];
    sprintf(buf,"ttSendMsg: receiver number (%d) out of bounds!",receiver);
    TT_MEX_ERROR(buf);
    return;
  }
  
  if (data != NULL && dataMATLAB != NULL) {
    char buf[200];
    sprintf(buf,"ttSendMsg: data and dataMATLAB cannot both be non-NULL!");
    TT_MEX_ERROR(buf);
    return;
  }
  
  if (length < 0) {
    char buf[200];
    sprintf(buf,"ttSendMsg: length cannot be negative!");
    TT_MEX_ERROR(buf);
    return;
  }
 
  NWmsg *nwmsg = new NWmsg();
  nwmsg->sender = nwi->nodeNbr;
  nwmsg->receiver = receiver-1;
  nwmsg->data = data;
  nwmsg->dataMATLAB = dataMATLAB;
  nwmsg->length = length;
  nwmsg->prio = priority;
  nwmsg->timestamp = rtsys->time;

  nwSendMsg(nwmsg, nwi->nwsys);
  if (rtsys->oldnwSnd[nwi->portNbr] == 0.0) {
    rtsys->nwSnd[nwi->portNbr] = 1.0; // trigger output
  } else {
    rtsys->nwSnd[nwi->portNbr] = 0.0; // trigger output
  }

}

void ttSendMsg(int networkNbr, int receiver, int length, void *data, mxArray *dataMATLAB)
{
  NetworkInterface* nwi = getNetwork(networkNbr);
  if (nwi == NULL) {
    char buf[200];
    sprintf(buf, "ttSendMsg: Network #%d not present!", networkNbr);
    TT_MEX_ERROR(buf);
    return;
  }
  int priority = nwi->nodeNbr;
  ttSendMsg(networkNbr, receiver, length, data, dataMATLAB, priority);
}


// C++ API functions

void ttSendMsg(int networkNbr, int receiver, void *data, int length, int priority) 
{
  ttSendMsg(networkNbr, receiver, length, data, NULL, priority);
}

void ttSendMsg(int networkNbr, int receiver, void *data, int length) 
{
  ttSendMsg(networkNbr, receiver, length, data, NULL);
}

void ttSendMsg(int receiver, void *data, int length, int priority)
{
  if (rtsys->nbrOfNetworks == 0) {
    char buf[200];
    sprintf(buf, "ttSendMsg: The node is not connected to to any network!");
    TT_MEX_ERROR(buf);
    return;
  }
  if (rtsys->nbrOfNetworks > 1) {
    char buf[200];
    sprintf(buf, "ttSendMsg: The node is connected to more than one network - specify which!\n");
    TT_MEX_ERROR(buf);
    return;
  }
  int networkNbr = rtsys->networkInterfaces[0].networkNbr;  // find first (and only) network number

  ttSendMsg(networkNbr, receiver, length, data, NULL, priority);
}

void ttSendMsg(int receiver, void *data, int length)
{
  if (rtsys->nbrOfNetworks == 0) {
    char buf[200];
    sprintf(buf, "ttSendMsg: The node is not connected to to any network!");
    TT_MEX_ERROR(buf);
    return;
  }
  if (rtsys->nbrOfNetworks > 1) {
    char buf[200];
    sprintf(buf, "ttSendMsg: The node is connected to more than one network - specify which!\n");
    TT_MEX_ERROR(buf);
    return;
  }
  int networkNbr = rtsys->networkInterfaces[0].networkNbr;  // find first (and only) network number
  
  ttSendMsg(networkNbr, receiver, length, data, NULL);
}



void ttUltrasoundPing(int networkNbr)
{
  NetworkInterface* nwi = getNetwork(networkNbr);
  if (nwi == NULL) {
    char buf[200];
    sprintf(buf, "ttUltrasoundPing: Network #%d not present!", networkNbr);
    TT_MEX_ERROR(buf);
    return;
  }
  
  NWmsg *nwmsg = new NWmsg();
  nwmsg->sender = nwi->nodeNbr;
  nwmsg->receiver = -1; // broadcast
  nwmsg->length = 0;
  nwmsg->prio = 0;
  nwmsg->signalPower = 0.0;

  nwSendMsg(nwmsg, nwi->nwsys);
  if (rtsys->oldnwSnd[nwi->portNbr] == 0.0) {
    rtsys->nwSnd[nwi->portNbr] = 1.0; // trigger output
  } else {
    rtsys->nwSnd[nwi->portNbr] = 0.0; // trigger output
  }

}

void ttUltrasoundPing() 
{
  if (rtsys->nbrOfNetworks == 0) {
    char buf[200];
    sprintf(buf, "ttUltrasoundPing: The node is not connected to to any network!");
    TT_MEX_ERROR(buf);
    return;
  }
  if (rtsys->nbrOfNetworks > 1) {
    char buf[200];
    sprintf(buf, "ttUltrasoundPing: The node is connected to more than one network - specify which!\n");
    TT_MEX_ERROR(buf);
    return;
  }
  int networkNbr = rtsys->networkInterfaces[0].networkNbr;  // find first (and only) network number
  // Default network nbr == 1
  ttUltrasoundPing(networkNbr);
}

#endif
