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

#ifndef CREATE_MONITOR
#define CREATE_MONITOR

#include "getnode.cpp"

void ttCreateMonitor(const char *name) {

  DataNode *dn;
  Monitor* mon;

  if (strcmp(name,"") == 0) {
    TT_MEX_ERROR("ttCreateMonitor: Name should be a non-empty string!"); 
    return;
  }
  if (rtsys->prioCmp == NULL) {
    TT_MEX_ERROR("ttCreateMonitor: Kernel must be initialized before creation of monitors!");
    return;
  }

  dn = getNode(name, rtsys->monitorList);
  if (dn!=NULL) {
    TT_MEX_ERROR("ttCreateMonitor: Name of monitor not unique!");
    return;
  }

  mon = new Monitor(name); 
  mon->waitingQ = new List("WaitingQ", rtsys->prioCmp); // Sort waiting tasks by priority function
  
  rtsys->monitorList->appendNode(new DataNode(mon, mon->name));
}

#endif
