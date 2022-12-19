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

#ifndef HANDLER_H
#define HANDLER_H

/**
 * InterruptHandler class, inherits from Task
 */
class InterruptHandler : public Task {
 public:
  double priority;
  char invoker[MAXCHARS]; 
  double timestamp; // Time of invocation
  int queueLength;  // Maximum number of queued invocations
  bool display;
  
  // ----------------------

  bool isHandler();
  void print();

  InterruptHandler(const char *n);
  ~InterruptHandler();
};

/**
 * InterruptHandler Constructor
 */
InterruptHandler::InterruptHandler(const char *n) 
  : Task(n)
{

}

/**
 * InterruptHandler Destructor
 */
InterruptHandler::~InterruptHandler() {

}

/**
 * Implementation of virtual methods
 */
bool InterruptHandler::isHandler() {
  return true;
}

void InterruptHandler::print() {
  mexPrintf("HANDLER (name: %s prio: %f time: %f)", name,priority,wakeupTime());
}

#endif
