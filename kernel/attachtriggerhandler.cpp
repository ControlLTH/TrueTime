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

#ifndef ATTACH_TRIGGER_HANDLER
#define ATTACH_TRIGGER_HANDLER

#include "getnode.cpp"

void ttAttachTriggerHandler(int number, const char *nameOfHandler, double minimumInterval) {

  char errbuf[MAXERRBUF];

  if (number < 1 || number > rtsys->nbrOfTriggers) {
    snprintf(errbuf, MAXERRBUF, "ttAttachTriggerHandler: trigger number out of bounds!");
    TT_MEX_ERROR(errbuf);
    return;
  }

  Trigger *trig = &(rtsys->triggers[number-1]);

  if (trig->handler != NULL) {
    snprintf(errbuf, MAXERRBUF, "ttAttachTriggerHandler: A handler is already attached to trigger %d!", number);
    TT_MEX_ERROR(errbuf);
    return;
  }

  DataNode* dn = getNode(nameOfHandler, rtsys->handlerList);
  if (dn==NULL) {
    snprintf(errbuf, MAXERRBUF, "ttAttachTriggerHandler: Non-existent interrupt handler '%s'!",nameOfHandler);
    TT_MEX_ERROR(errbuf);
    return;
  }

  InterruptHandler* hdl = (InterruptHandler*) dn->data;
  snprintf(trig->trigName, MAXCHARS, "trigger:%d", number);
  trig->minimumInterval = minimumInterval; 
  trig->handler = hdl;

}

void ttAttachTriggerHandler(int number, const char *nameOfHandler) {

  ttAttachTriggerHandler(number, nameOfHandler, 0.0);

}

#endif
