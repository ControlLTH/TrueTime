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

#ifndef COMP_FUNCTIONS
#define COMP_FUNCTIONS

// Sorts tasks according to their wakeup time
// Should return 1 if time(n1) < time(n2), 0 otherwise
int timeCmp(Node* n1, Node* n2) {
  
  double cmp1, cmp2;
  Task* t1 = (Task*) n1;
  Task* t2 = (Task*) n2;

  cmp1 = t1->wakeupTime();
  cmp2 = t2->wakeupTime();

  return (cmp1 < cmp2) ? 1 : 0;
}

// Sorts tasks according to the selected priority function
// see priofunctions.cpp for standard scheduling policies
// Should return 1 if val(n1) < val(n2), 0 otherwise
// n1 is to be inserted, n2 already in the list
int prioCmp(Node* n1, Node* n2) {

  double cmp1, cmp2;
  Task* t1 = (Task*) n1;
  Task* t2 = (Task*) n2;
  UserTask* u1;
  UserTask* u2;

  // Handler always wins over user task
  if (t1->isHandler() && t2->isUserTask()) return 1;
  if (t1->isUserTask() && t2->isHandler()) return 0;

  if (t1->isUserTask()) {

    // Two user tasks
    u1 = (UserTask*) t1;
    u2 = (UserTask*) t2;

    // Nonpreemptible running user task cannot be preempted by other user task
    if (u2 == rtsys->runningUserTasks[rtsys->currentCPU] && u2->nonpreemptible) return 0;
    
    cmp1 = (u1->prioRaised) ? u1->tempPrio : rtsys->prioFcn(u1);
    cmp2 = (u2->prioRaised) ? u2->tempPrio : rtsys->prioFcn(u2);

  } else {
    // Two handlers

    // Nonpreemptible running handler cannot be preempted
    if (t2->state == RUNNING && t2->nonpreemptible) return 0;

    cmp1 = ((InterruptHandler*)n1)->priority;
    cmp2 = ((InterruptHandler*)n2)->priority;

  }
  
  return cmp1 < cmp2;

}

#endif
