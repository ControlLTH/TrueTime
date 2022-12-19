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

#ifndef CBS_H
#define CBS_H

class CBS {
 public:
  char name[MAXCHARS];
  int type;
  double Qs;
  double Ts;
  double ds;
  double cs;
  int nbrJobs;
  int state;
  Timer *cbsTimer;
  Timer *overloadTimer;     // only used for Hard CBS
  double overloadEndTime;   // time that the overload will end
  int nbrPeriods;           // nbr of times a new CBS period has begun
  int nbrOverruns;          // nbr of times the CBS budget has been exhausted
  int affinity;             // CPU affinity (-1 if none)

  CBS(const char *name, double Qs, double Ts, int type);
  ~CBS();
};

/**
 * CBS Constructor 
 */
CBS::CBS(const char *cbsname, double qs, double ts, int tp) {
  strcpy(name, cbsname);
  type = tp;
  Qs = qs;
  Ts = ts;

  ds = 0.0;
  cs = Qs;
  nbrJobs = 0;
  state = CBS_OK;
  nbrPeriods = 0;
  nbrOverruns = 0;
  affinity = -1;
}

/**
 * Cbs Destructor 
 */
CBS::~CBS() {
}

#endif
