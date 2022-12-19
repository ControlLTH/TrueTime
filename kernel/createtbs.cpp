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

#ifndef CREATE_TBS
#define CREATE_TBS

#include "getnode.cpp"
#define max(A,B) ((A) > (B) ? (A) : (B))

// TBS budget handler -- called every time there is a new server task arriving (see tbshooks.cpp)

double tbsCodeFcn(int segment, void *data) {

  //char buf[200];
  TBS *tbs = (TBS*) data;
  Task *t, *n;

  // Find the tasks which are arrived
    t = (Task*) rtsys->readyQs[tbs->affinity]->getFirst();
    while (t != NULL) {
      n = (Task*) t->getNext();
      if (t->isUserTask()) {
	UserTask* task = (UserTask*) t;
	if (task->tbs == tbs && task->release - rtsys->time <= 0.00000000001f) {
	  // Change the deadline
	  task->absDeadline = max(tbs->lastDeadline, rtsys->time) + task->wcExecTime / tbs->Us;
	  tbs->lastDeadline = task->absDeadline;
	  //sprintf(buf, "disp(\'TBS_arrival %s @%.3f: new_deadline = %.3f\')", task->name, rtsys->time, task->absDeadline);
	  //mexEvalString(buf);
	}
      }
      t = n;
    }


  // Changing the server deadline changes the prio of all tasks associated with the TBS -- must resort the readyQ! Move all tasks associated with CBS to tmpQ
  //sprintf(buf, "disp(\'readyQ[%d].length = %d.\')", tbs->affinity, rtsys->readyQs[tbs->affinity]->length());
  //mexEvalString(buf);
    t = (Task*) rtsys->readyQs[tbs->affinity]->getFirst();
    while (t != NULL) {
      n = (Task*) t->getNext();
      if (t->isUserTask()) {
	if (((UserTask*)t)->tbs == tbs) {
	  t->moveToList(rtsys->tmpQ);
	  //sprintf(buf, "disp(\'Task %s(dl = %f) removed from readyQ.\')", t->name, ((UserTask*)t)->absDeadline);
	  //mexEvalString(buf);
	}
      }
      t = n;
    }
    // Move all tasks back to readyQ again (insertion sort)
    t = (Task*) rtsys->tmpQ->getFirst();
    while (t != NULL) {
      n = (Task*) t->getNext();
      t->moveToList(rtsys->readyQs[tbs->affinity]);
      //sprintf(buf, "disp(\'Task %s(dl = %f) moved to readyQ.\')", t->name, ((UserTask*)t)->absDeadline);
      //mexEvalString(buf);
      t = n;
    }
  return FINISHED;
}

void ttCreateTBS(const char *name, double Us) {

  DataNode *dn;
  TBS* tbs;

  debugPrintf("ttCreateTBS('%s')\n", name);

  if (strcmp(name,"") == 0) {
    TT_MEX_ERROR("ttCreateTBS: Name should be a non-empty string!"); 
    return;
  }
  if (rtsys->prioFcn == NULL) {
    TT_MEX_ERROR("ttCreateTBS: Kernel must be initialized before creation of TBSs!");
    return;
  }

  dn = getNode(name, rtsys->tbsList);
  if (dn!=NULL) {
    TT_MEX_ERROR("ttCreateTBS: Name of TBS not unique!");
    return;
  }

  tbs = new TBS(name,Us); 
  rtsys->tbsList->appendNode(new DataNode(tbs, tbs->name));

  InterruptHandler *hdl = new InterruptHandler("tbsoverrunhandler");
  hdl->codeFcn = tbsCodeFcn;
  hdl->priority = -1000.0;
  hdl->display = false;
  hdl->nonpreemptible = true;
  hdl->data = (void *)tbs;

  Timer *timer = new Timer("tbsoverruntimer");
  timer->period = -1.0;
  timer->isPeriodic = false;
  timer->isOverrunTimer = true;   // = do not delete after expiry
  timer->task = hdl;
  tbs->tbsTimer = timer;

}

#endif
