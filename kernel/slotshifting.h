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

#ifndef SLOT_SHIFTING_H
#define SLOT_SHIFTING_H

#include "usertask.h"
#include "getnode.cpp"
#include <algorithm>
#include <vector>
#include <cmath>

class SlotShifting {
 public:
  char name[MAXCHARS];
  double slotLength;
  int affinity; 	            			// CPU affinity (-1 if none)
// Addition
  int intervalIndex;					// Currently executing interval
  double hyperPeriod;					// in seconds
  unsigned startIndexForSchedulingCycle;			// Since the 2nd LCM of HP repeats in constraint tasksets, this variable holds the index of the first repeating intervals
  // To remember the offline calculated intervals
  std::vector <double> savedIntervalStart;       	// to hold interval start for slot shifting for the lifetime of simulation
  std::vector <double> savedIntervalEnd;       		// to hold interval start for slot shifting for the lifetime of simulation
  std::vector <double> savedIntervalSpareCapacity;   	// to hold interval start for slot shifting for the lifetime of simulation
  // To hold current state of intervals
  std::vector <double> intervalStart;       		// to hold interval start for slot shifting for the lifetime of simulation
  std::vector <double> intervalEnd;       		// to hold interval start for slot shifting for the lifetime of simulation
  std::vector <double> intervalSpareCapacity;   	// to hold interval start for slot shifting for the lifetime of simulation
  Timer *slotTimer;			    		// timer to invoke dispatcher
  List *tmpQ;     					// temporary Q, used to hold jobs

  SlotShifting(const char *name, double sl);
  ~SlotShifting();
};

/**
 * SlotShifting Constructor 
 */
SlotShifting::SlotShifting(const char *ss_name, double sl) {
  strcpy(name, ss_name);
  slotLength = sl;
  affinity = -1;
  tmpQ = NULL;
  intervalIndex = 0;
  startIndexForSchedulingCycle = 0;
  tmpQ = new List("SlotShiftList", NULL);
}

bool sortAccordingToAbsDeadline (UserTask* c1, UserTask* c2) { return (c1->absDeadline < c2->absDeadline); }

/**
 * SlotShifting Destructor 
 */
SlotShifting::~SlotShifting() {
  // Release memory
  intervalStart.clear();
  intervalEnd.clear();
  intervalSpareCapacity.clear();
  if (tmpQ) delete tmpQ;
}

#endif
