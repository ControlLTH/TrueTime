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

#ifndef ULTRASOUND_PING
#define ULTRASOUND_PING

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

// Declare two versions
void ttUltrasoundPing();
void ttUltrasoundPing(int network);


void ttUltrasoundPing() 
{
  // Default network nbr == 1
  ttUltrasoundPing(1);
}

void ttUltrasoundPing(int network);
{
  Network* net = getNetwork(networkNbr);
  if (net == NULL) {
    char buf[200];
    sprintf(buf, "ttUltrasoundPing: Network #%d not present!", networkNbr);
    TT_MEX_ERROR(buf);
    return;
  }
  
  NWmsg *nwmsg = new NWmsg();
  nwmsg->sender = net->nodeNbr;
  nwmsg->receiver = receiver-1;
  nwmsg->data = data;
  nwmsg->dataMATLAB = NULL;
  nwmsg->length = length;
  nwmsg->prio = priority;

  nwSendMsg(nwmsg, net->nwsys);
  if (rtsys->nwSnd[net->networkID - 1] == 0.0) {
    rtsys->nwSnd[net->networkID - 1] = 1.0; // trigger output
  } else {
    rtsys->nwSnd[net->networkID - 1] = 0.0; // trigger output
  }

}

// For use from Matlab mex function
void ttUltrasoundPing(int networkNbr)
{
  Network* net = getNetwork(networkNbr);

  if (net == NULL) {
    // is there no network?
    char buf[200];
    sprintf(buf, "ttUltrasoundPing: Network #%d not present!", networkNbr);
    TT_MEX_ERROR(buf);
    return;
  }

  NWmsg *nwmsg = new NWmsg();
  nwmsg->sender = net->nodeNbr;
  nwmsg->receiver = receiver-1;
  nwmsg->data = NULL;
  nwmsg->dataMATLAB = dataMATLAB;
  nwmsg->length = length;
  nwmsg->prio = priority;
  
  nwSendMsg(nwmsg, net->nwsys);
  if (rtsys->nwSnd[net->networkID - 1] == 0.0) {
    rtsys->nwSnd[net->networkID - 1] = 1.0; // trigger output
  } else {
    rtsys->nwSnd[net->networkID - 1] = 0.0; // trigger output
  }
}
 
#endif
