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

#ifndef ATTACH_NETWORK_HANDLER
#define ATTACH_NETWORK_HANDLER

#include "getnode.cpp"
#include "getnetwork.cpp"

void ttAttachNetworkHandler(int nwnbr, const char *nwhandler) {
 
  DataNode *dn1 = getNode(nwhandler, rtsys->taskList);
  DataNode *dn2 = getNode(nwhandler, rtsys->handlerList);
  Task* hdl;

  if (dn1 == NULL && dn2 == NULL) {
    char buf[200];
    sprintf(buf, "ttAttachNetworkHandler: Non-existent task or handler '%s'\n", nwhandler);
    TT_MEX_ERROR(buf);
    return;
  }
  if (nwnbr < 1) {
    TT_MEX_ERROR("ttAttachNetworkHandler: Network number must be positive!");
    return;
  }

  if (dn1 != NULL) {
    hdl = (Task*) dn1->data;
  } else {
    hdl = (Task*) dn2->data;
  }
  NetworkInterface *nwi = getNetwork(nwnbr);

  if (nwi != NULL) {
    if (nwi->handler == NULL) {
      nwi->handler = hdl;
    } else {
      TT_MEX_ERROR("ttAttachNetworkHandler: Network handler already assigned!");
      return;
    }
  } else {
    char buf[200];
    sprintf(buf, "ttAttachNetworkHandler: The node is not connected to network %d!",nwnbr);
    TT_MEX_ERROR(buf);
  }
}

// If there is only one network
void ttAttachNetworkHandler(const char *nwhandler)
{
  if (rtsys->nbrOfNetworks == 0) {
    char buf[200];
    sprintf(buf, "ttAttachNetworkHandler: The node is not connected to to any network!");
    TT_MEX_ERROR(buf);
    return;
  }
  if (rtsys->nbrOfNetworks > 1) {
    char buf[200];
    sprintf(buf, "ttAttachNetworkHandler: The node is connected to more than one network - specify which!\n");
    TT_MEX_ERROR(buf);
    return;
  }
  
  int networkNbr = rtsys->networkInterfaces[0].networkNbr;  // find first (and only) network number
  ttAttachNetworkHandler(networkNbr, nwhandler);
}

#endif
