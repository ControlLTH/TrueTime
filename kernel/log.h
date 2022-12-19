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

#ifndef LOG_H
#define LOG_H

class Log {
 public:
  char *name;
  char *variable;
  int size; 
  int entries;
  int tempIndex;
  double *vals;
  double temp;

  Log(const char *name, const char *var, int s);
  ~Log();
};

/**
 * Log Constructor 
 */
Log::Log(const char *logname, const char *var, int s) {
  if (logname==NULL) {
    name = NULL;
  } else {
    name = new char[strlen(logname)+1];
    strcpy(name, logname);
  }
  if (var==NULL) {
    variable = NULL;
  } else {
    variable = new char[strlen(var)+1];
    strcpy(variable, var);
  }
  size = s; 
  entries = 0;
  tempIndex = 0; 
  vals = new double[size];
  for (int i=0; i<size; i++) {
    vals[i] = 0.0;
  }
  temp = 0.0;
}

/**
 * Log Destructor 
 */
Log::~Log() {
  if (name) {
    delete[] name;
    name = NULL;
  }
  if (variable) {
    delete[] variable;
    variable = NULL;
  }
  if (vals) {
    delete[] vals;
    vals = NULL;
  }
}

#endif
