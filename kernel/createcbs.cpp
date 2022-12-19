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

#ifndef CREATE_CBS
#define CREATE_CBS

#include "getnode.cpp"

// CBS budget handler -- called every time the budget is exhausted

double cbsBudgetCodeFcn(int segment, void *data) {

  Task *task, *next;
  CBS *cbs = (CBS*) data;

  debugPrintf("cbsBudgetCodeFcn on core %d at %f\n", rtsys->currentCPU, rtsys->time);

  switch (cbs->type) {

  case 0:
    // Soft CBS - move the deadline but let the tasks continue

    cbs->ds += cbs->Ts;  // postpone deadlne
    cbs->cs = cbs->Qs;   // recharge budget
    cbs->nbrOverruns++;  // increase total number of overruns
    
    // Changing the server deadline changes the prio of all tasks
    // associated with the CBS -- must resort the readyQ!
    // Move all tasks associated with CBS to tmpQ
    task = (Task*) rtsys->readyQs[cbs->affinity]->getFirst();
    while (task != NULL) {
      next = (Task*) task->getNext();
      if (task->isUserTask()) {
	if (((UserTask*)task)->cbs == cbs) {
	  task->moveToList(rtsys->tmpQ);
	}
      }
      task = next;
    }
    // Move all tasks back to readyQ again (insertion sort)
    task = (Task*) rtsys->tmpQ->getFirst();
    while (task != NULL) {
      next = (Task*) task->getNext();
      task->moveToList(rtsys->readyQs[cbs->affinity]);
      task = next;
    }
    break;

  case 1:
    // Hard CBS

    // First post the overload restore timer
    cbs->state = CBS_OVERLOAD;
    cbs->overloadTimer->time = cbs->ds;
    cbs->overloadEndTime = cbs->ds;
    cbs->overloadTimer->moveToList(rtsys->timeQ);

    // Then put all tasks associated with the CBS to sleep until
    // that same time
    task = (Task*) rtsys->readyQs[cbs->affinity]->getFirst();
    while (task != NULL) {
      next = (Task*) task->getNext();
      if (task->isUserTask()) {
	if (((UserTask*)task)->cbs == cbs) {
	  ((UserTask*)task)->release = cbs->ds; // Set wake-up at CBS deadline
	  task->moveToList(rtsys->timeQ);       // Put it to sleep
	}
      }
      task = next;
    }

    // Finally, update CBS parameters
    cbs->ds += cbs->Ts;  // postpone deadline
    cbs->nbrPeriods++;   // increase total number of CBS periods
    break;


  default:
    TT_MEX_ERROR("CBS type not implemented!");
  }
  return FINISHED;
  
}


// CBS overload reset handler -- for Hard CBS

double cbsOverloadResetCodeFcn(int segment, void *data) {

  // Reset the state of the CBS
  CBS *cbs = (CBS*) data;
  cbs->cs = cbs->Qs;   // recharge budget
  cbs->state = CBS_OK;
  cbs->nbrOverruns++;  // increase total number of overruns
  return FINISHED;

}


void ttCreateCBS(const char *name, double Qs, double Ts, int type) {

  DataNode *dn;
  CBS* cbs;

  debugPrintf("ttCreateCBS('%s')\n", name);

  if (strcmp(name,"") == 0) {
    TT_MEX_ERROR("ttCreateCBS: Name should be a non-empty string!"); 
    return;
  }
  if (rtsys->prioFcn == NULL) {
    TT_MEX_ERROR("ttCreateCBS: Kernel must be initialized before creation of CBSs!");
    return;
  }

  dn = getNode(name, rtsys->cbsList);
  if (dn!=NULL) {
    TT_MEX_ERROR("ttCreateCBS: Name of CBS not unique!");
    return;
  }

  if (type != 0 && type != 1) {
    TT_MEX_ERROR("ttCreateCBS: Unknown CBS type!");
    return;
  }

  cbs = new CBS(name,Qs,Ts,type); 
  rtsys->cbsList->appendNode(new DataNode(cbs, cbs->name));

  // Create budget handler and timer

  InterruptHandler *hdl = new InterruptHandler("cbsoverrunhandler");
  hdl->codeFcn = cbsBudgetCodeFcn;
  hdl->priority = -1000.0;
  hdl->display = false;
  hdl->nonpreemptible = true;
  hdl->data = (void *)cbs;

  Timer *timer = new Timer("cbsoverruntimer");
  timer->period = -1.0;
  timer->isPeriodic = false;
  timer->isOverrunTimer = true;   // = do not delete after expiry
  timer->task = hdl;
  cbs->cbsTimer = timer;

  // Create overload reset handler and timer (Hard CBS only)

  if (type == 1) {

    InterruptHandler *olhdl = new InterruptHandler("cbsreactivationhandler");
    olhdl->codeFcn = cbsOverloadResetCodeFcn;
    olhdl->priority = -1000.0;
    olhdl->display = false;
    olhdl->nonpreemptible = true;
    olhdl->data = (void *)cbs;
    
    Timer *oltimer = new Timer("cbsreactivationtimer");
    oltimer->period = -1.0;
    oltimer->isPeriodic = false;
    oltimer->isOverrunTimer = true;   // = do not delete after expiry
    oltimer->task = olhdl;
    cbs->overloadTimer = oltimer;

  }

}


void ttCreateCBS(const char *name, double Qs, double Ts) {

  ttCreateCBS(name, Qs, Ts, 0);

}

#endif
