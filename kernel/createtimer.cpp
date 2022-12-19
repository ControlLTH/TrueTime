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

#ifndef CREATE_TIMER
#define CREATE_TIMER

#include "getnode.cpp"

void ttCreatePeriodicTimer(const char *nameOfTimer, double starttime, double period, const char *nameOfHandler) {
  
  DataNode* dn;

  if (rtsys->prioFcn == NULL) {
    TT_MEX_ERROR("ttCreate(Periodic)Timer: Kernel must be initialized before creation of timers!\n");
    return;
  }

  if (starttime - rtsys->time < -TT_TIME_RESOLUTION) {
    TT_MEX_WARNING("ttCreate(Periodic)Timer: Can't create timer backwards in time! Timer not created.");
    return;
  }

  if (strcmp(nameOfTimer,"") == 0) {
    TT_MEX_ERROR("ttCreate(Periodic)Timer: Name empty! Timer not created!");
    return;
  }

  dn = getNode(nameOfTimer, rtsys->timerList);
  if (dn != NULL) { 
    TT_MEX_ERROR("ttCreate(Periodic)Timer: Name of timer not unique! Timer not created!");
    return;
  }

  dn = getNode(nameOfHandler, rtsys->handlerList);
  if (dn == NULL) {
    // Handler does not exist 
    char buf[200];
    sprintf(buf, "ttCreate(Periodic)Timer: Non-existent interrupt handler '%s'!", nameOfHandler);
    TT_MEX_ERROR(buf);
    return;
  }
  
  InterruptHandler* hdl = (InterruptHandler*) dn->data;

  Timer* t = new Timer(nameOfTimer);
  t->time = starttime;
  t->period = period;
  t->isPeriodic = (period > 0.0);
  t->isOverrunTimer = false;
  t->task = hdl;

  // Insert timer in timerList so that it can be later retrieved by name
  rtsys->timerList->appendNode(new DataNode(t, t->name));

  // Enter timer in timeQ
  t->moveToList(rtsys->timeQ);
}  

void ttCreatePeriodicTimer(const char *nameOfTimer, double period, const char *nameOfHandler) {
  ttCreatePeriodicTimer(nameOfTimer, 0.0, period, nameOfHandler);
}

void ttCreateTimer(const char *nameOfTimer, double time, const char *nameOfHandler) {
  
  // Creating timer with negative period will create a one-shot timer
  ttCreatePeriodicTimer(nameOfTimer, time, -1.0, nameOfHandler);
}

#endif
