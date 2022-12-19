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

#ifndef CREATE_EVENT
#define CREATE_EVENT

#include "getnode.cpp"

// Create a free event
bool ttCreateEvent(const char *name) {

  DataNode *dn;
  Event* e;

  if (strcmp(name, "") == 0) {
    TT_MEX_ERROR("ttCreateEvent: Name should be a non-empty string!");
    return false;
  }
  if (rtsys->prioCmp == NULL) {
    TT_MEX_ERROR("ttCreateEvent: Kernel must be initialized before creation of events!");
    return false;
  }
  dn = getNode(name, rtsys->eventList);
  if (dn != NULL) {
    TT_MEX_ERROR("ttCreateEvent: Name of event not unique!");
    return false;
  }

  e = new Event(name, true, NULL);
  e->waitingQ = new List("WaitingQ", rtsys->prioCmp); // Sort waiting tasks by priority function
  rtsys->eventList->appendNode(new DataNode(e, e->name));
  return true;
}

// Create an event associated with a monitor
void ttCreateEvent(const char *name, const char *monitor) {

  DataNode *dn;
  Event* e;
  Monitor* mon;

  dn = getNode(monitor, rtsys->monitorList);
  if (dn==NULL) {
    // Monitor does not exist 
    char buf[200];
    sprintf(buf, "ttCreateEvent: Non-existent monitor '%s'!", monitor);
    TT_MEX_ERROR(buf);
    return;
  }
  
  if (ttCreateEvent(name)) {
    mon = (Monitor*) dn->data;
    dn = (DataNode*) rtsys->eventList->getLast();
    e = (Event*) dn->data;
    e->mon = mon;
    e->isFree = false;
  }
}


#endif
