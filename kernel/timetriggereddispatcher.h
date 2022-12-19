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

#ifndef TIME_TRIGGERED_DISPATCHER_H
#define TIME_TRIGGERED_DISPATCHER_H

#include <vector>
#include <list>

class TimeTriggeredDispatcher {
 public:
  char name[MAXCHARS];
  double slotLength;
  int affinity;             			// CPU affinity (-1 if none)
// Addition
  List *tmpQ;     					// temporary Q, used to hold jobs
  std::vector <int> schedulingTable;					// Sequence of tasks
  Timer *slotTimer;			    		// timer to invoke dispatcher

  TimeTriggeredDispatcher(const char *name, double Ts, double *tSeq, int size);
  ~TimeTriggeredDispatcher();
};

/**
 * TimeTriggeredDispatcher Constructor 
 */
TimeTriggeredDispatcher::TimeTriggeredDispatcher(const char *dispatchername, double ts, double *tSeq, int size) {
  strcpy(name, dispatchername);
  slotLength = ts;
  tmpQ = NULL;
  char buf[200];
  sprintf(buf, "TTDispatcherList-%s", dispatchername);
  tmpQ = new List(buf, NULL);
  for(int i = 0;i<size;i++)
  {
	schedulingTable.push_back((int)*(tSeq+i));
  }
  affinity = -1;
}

/**
 * TimeTriggeredDispatcher Destructor 
 */
TimeTriggeredDispatcher::~TimeTriggeredDispatcher() {
  // Release memory
  schedulingTable.clear();
  if (tmpQ) delete tmpQ;
}

#endif
