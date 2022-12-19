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

#ifndef GET_INVOKER
#define GET_INVOKER

// To be called from interrupt handler associated
// with task overruns to know the triggering task.

char *ttGetInvoker(double *timestamp) {

  // Check typeid of running task
  if (rtsys->running->isUserTask()) {
    TT_MEX_ERROR("ttInvokingTask: should be called from interrupt handler!");
    return NULL;
  }

  InterruptHandler* hdl = (InterruptHandler*) rtsys->running;
  
  *timestamp = hdl->timestamp;
  return hdl->invoker;

}

char *ttGetInvoker() {

  // Check typeid of running task
  if (rtsys->running->isUserTask()) {
    TT_MEX_ERROR("ttInvokingTask: should be called from interrupt handler!");
    return NULL;
  }

  InterruptHandler* hdl = (InterruptHandler*) rtsys->running;

  return hdl->invoker;

}

#endif
