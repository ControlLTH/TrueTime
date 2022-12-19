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

#ifndef TIMER_H
#define TIMER_H

/**
 * Timer class, inherits from Task
 */
class Timer : public Task {
 public:
  double time;
  Task *task;        // interrupt handler or user task to activate
  bool isPeriodic;
  bool isOverrunTimer;
  double period;
  
  bool isTimer();
  double wakeupTime();
  void print();

  Timer(const char *n);
  ~Timer();
};

/**
 * Timer Constructor 
 */
Timer::Timer(const char *n) 
  : Task(n)
{
}

/**
 * Timer Destructor 
 */
Timer::~Timer() {
}

/**
 * Implementation of virtual methods
 */
bool Timer::isTimer() {
  return true;
}

double Timer::wakeupTime() {
  return time;
}

void Timer::print() {
  mexPrintf("TIMER (name: %s time: %f)", name,wakeupTime());
}

#endif



