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

#ifndef TASK_H
#define TASK_H

class UserTask;

/**
 * Data node for storing queued task invocations 
 */
class TaskInvocation : public Node {
 public:
  double timestamp; // Time of invocation
  int type;         // Interrupt type
  char invoker[MAXCHARS];  // What was invoking the task
  void print();
};

void TaskInvocation::print() {
 }

/**
 * General Task class, inherits from Node
 * super-class to UserTask, InterruptHandler and Timer
 */

class Task : public Node {
 public:
  char name[MAXCHARS];
  double execTime;       // remaining execution time of current segment
  double CPUTime;        // total execution time since task creation
  int segment, nextSegment;
  bool nonpreemptible; 
  int affinity;          // CPU affinity (default = 0)
  
  void *data;       // task data in C++ case
  mxArray *dataMATLAB; // variable for task data
  void *mb_data;    // temporary mailbox data (void* (C++) or char* (Matlab))
  double (*codeFcn)(int, void*); // Code function written in C++
  char *codeFcnMATLAB;  // Name of m-file code function
  
  List *blockList; // To store options mexArrays for block system calls 
  int nbrInvocations;
  List *pending;      // list of pending invocations

  int state;         // SLEEPING, READY, RUNNING or WAITING (only user tasks)
 
  virtual bool isUserTask();
  virtual bool isHandler();
  virtual bool isTimer();
  virtual double wakeupTime();
  virtual void print();
  Task(const char *n);
  virtual ~Task();
};

/**
 * Task Constructor 
 */
Task::Task(const char *n) {
  strncpy(name, n, MAXCHARS);
  execTime = 0.0;
  CPUTime = 0.0;
  segment = 0;
  nextSegment = 1;
  nonpreemptible = false;
  affinity = 0;
  data = NULL;
  dataMATLAB = NULL;
  codeFcn = NULL;
  codeFcnMATLAB = NULL;
  mb_data = NULL;
  blockList = new List("BlockList", NULL);
  nbrInvocations = 0;
  pending = new List("Pending", NULL);
  state = SLEEPING;
}

/**
 * Task Destructor 
 */
Task::~Task() {
  DataNode *dn, *tmp;

  if (codeFcnMATLAB) {
    delete codeFcnMATLAB;
    codeFcnMATLAB = NULL;
  }
  if (dataMATLAB) {
    mxDestroyArray(dataMATLAB);
    dataMATLAB = NULL;
  }

#ifdef KERNEL_MATLAB  
  if (mb_data) {
    mxDestroyArray((mxArray *)mb_data);
    mb_data = NULL;
  }
#endif

  if (blockList) {
    dn = (DataNode*) blockList->getFirst();
    while (dn != NULL) {
      Blockdata* bd = (Blockdata*) dn->data;
      delete bd;
      dn = (DataNode*) dn->getNext();
    }
    delete blockList;
    blockList = NULL;
  }
  
  if (pending) {
    dn = (DataNode*) pending->getFirst();
    while (dn != NULL) {
      double* r = (double*) dn->data;
      delete r;
      dn = (DataNode*) dn->getNext();
    }
    delete pending;
    pending = NULL;
  }

}

/** 
 * Default virtual methods
 */
bool Task::isUserTask() {
  return false;
}

bool Task::isHandler() {
  return false;
}

bool Task::isTimer() {
  return false;
}

void Task::print() {
  mexPrintf("TASK (name: %s)", name);

}

double Task::wakeupTime() {
  return 0.0;
}

#endif
