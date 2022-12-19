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

#ifndef INIT_KERNEL
#define INIT_KERNEL

void ttInitKernel(double (*prioFcn)(UserTask*)) {

  char qname[MAXCHARS];

  rtsys->prioFcn = prioFcn;

  //rtsys->readyQ = new List("ReadyQ", rtsys->prioCmp);
  rtsys->timeQ = new List("TimeQ", rtsys->timeCmp);
  rtsys->tmpQ = new List("TmpQ", rtsys->prioCmp); // for some Q sorting operations

  rtsys->readyQs = new List *[rtsys->nbrOfCPUs];
  rtsys->runnings = new Task *[rtsys->nbrOfCPUs];
  rtsys->runningUserTasks = new Task *[rtsys->nbrOfCPUs];

  for (int i=0; i<rtsys->nbrOfCPUs; i++) {
    snprintf(qname, MAXCHARS, "ReadyQ%d", i);
    rtsys->readyQs[i] = new List(qname, rtsys->prioCmp);
    rtsys->runnings[i] = NULL;
    rtsys->runningUserTasks[i] = NULL;
  }

  rtsys->initialized = true;
}


void ttInitKernelMATLAB(int dispatch) {

  switch (dispatch) {
  case EDF:
    ttInitKernel(rtsys->prioEDF);
    break;
  case DM:
    ttInitKernel(rtsys->prioDM);
    break;
  case FP:
    ttInitKernel(rtsys->prioFP);
    break;
  default:
    TT_MEX_ERROR("ttInitKernel: Invalid priority function!");
    return;
  }

}

// Creates an interrupt handler to simulate context switches
InterruptHandler *createCShandler(void) {

  InterruptHandler* hdl = new InterruptHandler("CShandler");
  
  hdl->codeFcn = rtsys->contextSwitchCode;
  hdl->priority = -2000.0;
  hdl->nonpreemptible = true;
  hdl->display = true;

  rtsys->handlerList->appendNode(new DataNode(hdl, hdl->name));
  
  rtsys->nbrOfSchedTasks++;
  
  return hdl;
}

void ttInitKernel(double (*prioFcn)(UserTask*), double contextSwitchOH) {  

  ttInitKernel(prioFcn);

  if (contextSwitchOH < 0.0) {
    TT_MEX_ERROR("ttInitKernel: Invalid context switch overhead!");
    return;
  }

  if (contextSwitchOH > TT_TIME_RESOLUTION) {
    rtsys->contextSwitchTime = contextSwitchOH;
    rtsys->kernelHandler = createCShandler();
  }
}

void ttInitKernelMATLAB(int dispatch, double contextSwitchOH) {  

  ttInitKernelMATLAB(dispatch);

  if (contextSwitchOH < 0.0) {
    TT_MEX_ERROR("ttInitKernel: Invalid context switch overhead!");
    return;
  }

  if (contextSwitchOH > TT_TIME_RESOLUTION) {
    rtsys->contextSwitchTime = contextSwitchOH;
    rtsys->kernelHandler = createCShandler();
  }

}

#endif
