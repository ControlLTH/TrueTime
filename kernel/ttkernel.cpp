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

#ifndef S_FUNCTION_NAME
#error ttkernel.cpp cannot be compiled on its own!
#else

#include "ttkernel.h"

// ------- Internal functions used by kernel -------

#include "compfunctions.cpp"
#include "codefunctions.cpp"
#include "priofunctions.cpp"
#include "invoketask.cpp"
#include "defaulthooks.cpp"

#ifndef KERNEL_MATLAB

#define KERNEL_C

// --- Initialization and creation ----

#include "setnumberofcpus.cpp"
#include "initkernel.cpp"
#include "createtask.cpp"
#include "createhandler.cpp"
#include "createmonitor.cpp"
#include "createevent.cpp"
#include "createmailbox.cpp"
#include "createsemaphore.cpp"
#include "createlog.cpp"
#include "createcbs.cpp"
#include "createtbs.cpp"
#include "attachdlhandler.cpp"
#include "attachwcethandler.cpp"
#include "attachhook.cpp"
#include "attachcbs.cpp"
#include "attachtbs.cpp"
#include "attachtriggerhandler.cpp"
#include "attachnetworkhandler.cpp"
#include "noschedule.cpp"
#include "nonpreemptible.cpp"
#include "createtimetriggereddispatcher.cpp"
#include "enableslotshifting.cpp"

// ------- Real-time primitives -------

#include "createjob.cpp"
#include "killjob.cpp"
#include "createtimer.cpp"
#include "removetimer.cpp"
#include "analogin.cpp"
#include "analogout.cpp"
#include "sleep.cpp"
#include "entermonitor.cpp"
#include "exitmonitor.cpp"
#include "wait.cpp"
#include "notify.cpp"
#include "tryfetch.cpp"
#include "trypost.cpp"
#include "fetch.cpp"
#include "post.cpp"
#include "retrieve.cpp"
#include "take.cpp"
#include "give.cpp"
#include "lognow.cpp"
#include "logstart.cpp"
#include "logstop.cpp"
#include "currenttime.cpp"
#include "currenttask.cpp"
#include "getinvoker.cpp"
#include "setnextsegment.cpp"
#include "callblocksystem.cpp"
#include "sendmsg.cpp"
#include "getmsg.cpp"
#include "setnetworkparameter.cpp"
#include "abortsimulation.cpp"
#include "discardunsent.cpp"

// ---------- Sets and Gets ------------

#include "setdeadline.cpp"
#include "setabsdeadline.cpp"
#include "setpriority.cpp"
#include "setperiod.cpp"
#include "setbudget.cpp"
#include "setwcet.cpp"
#include "setdata.cpp"
#include "getrelease.cpp"
#include "getdeadline.cpp"
#include "getabsdeadline.cpp"
#include "getpriority.cpp"
#include "getperiod.cpp"
#include "getbudget.cpp"
#include "getwcet.cpp"
#include "getdata.cpp"
#include "getcputime.cpp"
#include "getarrival.cpp"
#include "setcbsparameters.cpp"
#include "getcbsbudget.cpp"
#include "getcbsperiod.cpp"
#include "getcbsperiods.cpp"
#include "getcbsoverruns.cpp"
#include "getinitarg.cpp"
#include "setuserdata.cpp"
#include "getuserdata.cpp"
#include "setcpuaffinity.cpp"
#include "getcpuaffinity.cpp"
#include "settaskcriticality.cpp"

#endif

// Static global variables

//static char errbuf[MAXERRBUF]; // for formatted error messages

// -------------------------------------------------------------
// --------------------- Kernel Function -----------------------
// ----  Called from the Simulink callback functions during ----
// -- simulation and returns the time for its next invocation --
// -------------------------------------------------------------

void TT_RUNKERNEL_ERROR(const char *error_msg) {
  
  mxArray *rhs[1];
  rhs[0] = mxCreateString(error_msg);
  mexSetTrapFlag(1);
  mexCallMATLAB(0, NULL, 1, rhs, "error");
}


double runKernel(double externalTime) {
  
  Task *task, *temp, *newrunning;
  UserTask *usertask;
  InterruptHandler *handler;
  DataNode* dn;
  
  // If no energy, then we can not run
  if (rtsys->energyLevel <= 0) {
    debugPrintf("'%s': Energy is out at time: %f\n", rtsys->blockName, rtsys->time);
    return TT_MAX_TIMESTEP;
  }
  
  double timeElapsed = externalTime - rtsys->prevHit; // time since last invocation
  rtsys->prevHit = externalTime;  // update previous invocation time
  
  debugPrintf("'%s': runkernel at %.14f\n", rtsys->blockName, rtsys->time);
  
#ifdef KERNEL_MATLAB
  // Write rtsys pointer to global workspace so that MATLAB kernel
  // primitives can access it
  *((void **)mxGetData(rtsys->rtsysptr)) = (void *)rtsys;
#endif
  
  double timestep = 0.0;
  int niter = 0;
  
  while (timestep < TT_TIME_RESOLUTION) {
    
    if (++niter == TT_MAX_ITER) {
      mexPrintf("??? Fatal kernel error: maximum number of iterations reached!\n");
      rtsys->error = 1;
      return 0.0;
    }
    
    // For each CPU, count down execution time for the current task
    
    for (int i=0; i<rtsys->nbrOfCPUs; i++) {
      
      debugPrintf("running core %d\n", i);
      
      rtsys->currentCPU = i;
      rtsys->running = rtsys->runnings[i];
      task = rtsys->running;
      
      if (task != NULL) {
        
        if (task->state == RUNNING) {
          task->state = READY;
        }
        
        double duration = timeElapsed * rtsys->cpuScaling;
        
        // Decrease remaining execution time for current segment and increase total CPU time
        task->execTime -= duration;
        task->CPUTime += duration;
        
        // If user task, call runkernel hook (to e.g. update budgets)
        if (task->isUserTask()) {
          usertask = (UserTask*)task;
          usertask->runkernel_hook(usertask,duration);
        }
        
        // Check if task has finished current segment or not yet started
        if (task->execTime / rtsys->cpuScaling < TT_TIME_RESOLUTION) {
          
          // Execute next segment
          task->segment = task->nextSegment;
          task->nextSegment++; // default, can later be changed by ttSetNextSegment
          
#ifndef KERNEL_MATLAB
          debugPrintf("'%s': executing code segment %d of task '%s'\n", rtsys->blockName, task->segment, task->name);
          task->execTime = task->codeFcn(task->segment, task->data);
          if (rtsys->error) {
            TT_RUNKERNEL_ERROR(errbuf);
            mexPrintf("In task ==> '%s', code segment %d\n", task->name, task->segment);
            return 0.0;
          }
#else
          if (task->codeFcnMATLAB == NULL) {
            task->execTime = task->codeFcn(task->segment, task->data);
          } else {
            
            mxArray *lhs[2];
            mxArray *rhs[2];
            
            debugPrintf("'%s': executing code function '%s'\n", rtsys->blockName, task->codeFcnMATLAB);
            
            *mxGetPr(rtsys->segArray) = (double)task->segment;
            rhs[0] = rtsys->segArray;
            if (task->dataMATLAB) {
              rhs[1] = task->dataMATLAB;
            } else {
              rhs[1] = mxCreateDoubleMatrix(0, 0, mxREAL);
            }
            
            mexSetTrapFlag(1); // return control to the MEX file after an error
            lhs[0] = NULL;     // needed not to crash Matlab after an error
            lhs[1] = NULL;     // needed not to crash Matlab after an error
            if (mexCallMATLAB(2, lhs, 2, rhs, task->codeFcnMATLAB) != 0) {
              rtsys->error = true;
              return 0.0;
            }
            
            if (mxGetClassID(lhs[0]) == mxUNKNOWN_CLASS) {
              snprintf(errbuf, MAXERRBUF, "Execution time not assigned in code function '%s'", task->codeFcnMATLAB);
              TT_RUNKERNEL_ERROR(errbuf);
              rtsys->error = true;
              return 0.0;
            }
            
            if (!mxIsDoubleScalar(lhs[0])) {
              snprintf(errbuf, MAXERRBUF, "Illegal execution time returned by code function '%s'", task->codeFcnMATLAB);
              TT_RUNKERNEL_ERROR(errbuf);
              rtsys->error = true;
              return 0.0;
            }
            
            if (mxGetClassID(lhs[1]) == mxUNKNOWN_CLASS) {
              snprintf(errbuf, MAXERRBUF, "Data not assigned in code function '%s'", task->codeFcnMATLAB);
              TT_RUNKERNEL_ERROR(errbuf);
              rtsys->error = true;
              return 0.0;
            }
            
            //if ( task->dataMATLAB ) {
            if ( task->dataMATLAB != lhs[1] ) {
              mxDestroyArray(task->dataMATLAB);
              //task->dataMATLAB = mxDuplicateArray(lhs[1]);
              task->dataMATLAB = lhs[1];
              mexMakeArrayPersistent(task->dataMATLAB);
            }
            
            task->execTime = *mxGetPr(lhs[0]);
            
            //mxDestroyArray(rhs[1]);
            mxDestroyArray(lhs[0]);
            //mxDestroyArray(lhs[1]);
            
          }
          
#endif
          if (task->execTime < 0.0) {
            
            // Negative execution time = task is finished
            debugPrintf("'%s': task '%s' finished\n", rtsys->blockName, task->name);
            task->execTime = 0.0;
            task->segment = 0;
            task->nextSegment = 1;
            
            // Remove task from readyQ and set running to NULL
            if (task->state == READY) {
              task->remove();
              task->state = SLEEPING;
            } else {
              snprintf(errbuf, MAXERRBUF, "Finished task '%s' not in ReadyQ.", task->name);
              TT_RUNKERNEL_ERROR(errbuf);
              rtsys->error = true;
              return 0.0;
            }
            
            rtsys->runnings[i] = NULL;
            rtsys->running = NULL;
            
            if (task->isUserTask()) {
              // Execute finish-hook
              usertask = (UserTask*)task;
              usertask->finish_hook(usertask);
              rtsys->runningUserTasks[i] = NULL;
            }
            
            task->nbrInvocations--;
            if (task->nbrInvocations > 0) {
              // There are queued invocations, release the next one
              dn = (DataNode*) task->pending->getFirst();
              TaskInvocation *ti = (TaskInvocation *)dn->data;
              if (task->isUserTask()) {
                usertask = (UserTask*)task;
                usertask->arrival = ti->timestamp;
                usertask->release = rtsys->time;
                usertask->release_hook(usertask);  // could affect task prio
              }
              debugPrintf("'%s': releasing task '%s'\n", rtsys->blockName, task->name);
              task->moveToList(rtsys->readyQs[task->affinity]);  // re-insert task into readyQ
              task->state = READY;
              if (task->isHandler()) {
                handler = (InterruptHandler*)task;
                strncpy(handler->invoker, ti->invoker, MAXCHARS);
                handler->timestamp = ti->timestamp;
              }
              task->pending->deleteNode(dn);
              delete ti;
            }
          }
        }
      }
    }
    
    // Check time queue for possible releases and expired timers
    
    task = (Task*) rtsys->timeQ->getFirst();
    while (task != NULL) {
      if ((task->wakeupTime() - rtsys->time) >= TT_TIME_RESOLUTION) {
        break; // timeQ is sorted by time, no use to go further
      }
      
      // Task to be released
      temp = task;
      task = (Task*) task->getNext();
      
      if (temp->isTimer()) {
        Timer *timer = (Timer*)temp;
        debugPrintf("'%s': timer '%s' expired at %f\n", rtsys->blockName, timer->name, rtsys->time);
        invoke_task(timer->task, timer->name);
        if (timer->isPeriodic) {
          // if periodic timer put back in timeQ
          timer->time += timer->period;
          timer->moveToList(rtsys->timeQ);
        } else {
          timer->remove(); // remove timer from timeQ
          if (!timer->isOverrunTimer) {
            // delete the timer
            dn = getNode(timer->name, rtsys->timerList);
            rtsys->timerList->deleteNode(dn);
            delete timer;
          }
        }
      }
      else if (temp->isUserTask()) {
        
        usertask = (UserTask*)temp;
        debugPrintf("'%s': releasing task '%s'\n", rtsys->blockName, usertask->name);
        usertask->moveToList(rtsys->readyQs[usertask->affinity]);
        usertask->state = READY;
        
      }
      else if (temp->isHandler()) {
        mexPrintf("??? Fatal kernel error: interrupt handler in TimeQ!\n");
        rtsys->error = 1;
        return 0.0;
      }
    } // end: checking timeQ for releases
    
    
    // For each core, determine the task with highest priority and make it running task
    
    for (int i=0; i<rtsys->nbrOfCPUs; i++) {
      
      debugPrintf("scheduling core %d\n", i);
      
      rtsys->currentCPU = i;
      
      newrunning = (Task*) rtsys->readyQs[i]->getFirst();
      
      // If old running has been preempted, execute suspend_hook
      if (rtsys->runnings[i] != NULL && rtsys->runnings[i] != newrunning) {
        if (rtsys->runnings[i]->isUserTask()) {
          usertask = (UserTask*)rtsys->runnings[i];
          usertask->suspend_hook(usertask);
        }
      }
      
      // If new running != old running, execute start_hook or resume_hook
      if (newrunning != NULL && newrunning != rtsys->runnings[i]) {
        if (newrunning->isUserTask()) {
          usertask = (UserTask*)newrunning;
          if (usertask->segment == 0) {
            usertask->segment = 1;
            usertask->start_hook(usertask);
          } else {
            usertask->resume_hook(usertask);
          }
          rtsys->runningUserTasks[i] = usertask;
        }
      }
      
      rtsys->runnings[i] = (Task*) rtsys->readyQs[i]->getFirst(); // hooks may have released handlers
      if (rtsys->runnings[i] != NULL) {
        rtsys->runnings[i]->state = RUNNING;
      }
      
    }
    
    // Determine next invocation of kernel
    
    double compTime;
    timestep = TT_MAX_TIMESTEP;
    
    // Next release from timeQ (user task or timer)
    if (rtsys->timeQ->getFirst() != NULL) {
      Task* t = (Task*) rtsys->timeQ->getFirst();
      timestep = t->wakeupTime() - rtsys->time;
    }
    
    // Remaining execution time of running tasks
    
    for (int i=0; i<rtsys->nbrOfCPUs; i++) {
      if (rtsys->runnings[i] != NULL) {
        compTime = rtsys->runnings[i]->execTime / rtsys->cpuScaling;
        timestep = (timestep < compTime) ? timestep : compTime;
      }
    }
    
    timeElapsed = 0.0;
    
  } // end: loop while timestep < TT_TIME_RESOLUTION
  
  return timestep;
}


// ------- Simulink callback functions -------

#ifdef __cplusplus
extern "C" { // use the C fcn-call standard for all functions
#endif       // defined within this scope
  
#define S_FUNCTION_LEVEL 2
  
#include "simstruc.h"
  
  char *ssGetBlockName(SimStruct *S) {
    static char buf[MAXCHARS];
    strncpy(buf, ssGetPath(S), MAXCHARS-1);
    char *cut = strstr(buf, "/ttkernel");
    if (cut != NULL) {
      *cut = '\0';
    } else {
      buf[0] = '\0';
    }
    for (unsigned int i=0; i<strlen(buf); i++) if (buf[i]=='\n') buf[i]=' ';
    return buf;
  }
  
  void TT_CALLBACK_ERROR(SimStruct *S, const char *error_msg) {
    
    mxArray *rhs[1];
    rhs[0] = mxCreateString(error_msg);
    mexCallMATLAB(0, NULL, 1, rhs, "error");
    mexPrintf("??? %s\n\nIn block ==> %s\nSimulation aborted!\n", error_msg, ssGetBlockName(S));
    ssSetErrorStatus(S, "");
    
  }
  
  static void mdlInitializeSizes(SimStruct *S)
  {
    
    debugPrintf("'%s': mdlInitializeSizes\n", S->path);
    
    int i;
#ifdef KERNEL_MATLAB
    char initfun[MAXCHARS];
    static mxArray *lhs[1]; // warning: used multiple times
    static mxArray *rhs[3]; // warning: used multiple times
    mxArray *error_msg_array[1];
    char *error_msg;
    int nargin;
#endif
    
    ssSetNumSFcnParams(S, 7);  /* Number of expected parameters */
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
      TT_CALLBACK_ERROR(S, "Wrong number of parameters to S-function!");
      return; /* Parameter mismatch will be reported by Simulink */
    }
    
    rtsys = new RTsys;
    
    strncpy((char*)rtsys->blockName, ssGetBlockName(S), MAXCHARS);
    
#ifdef KERNEL_MATLAB
    rtsys->segArray = mxCreateDoubleScalar(0.0);
    mexMakeArrayPersistent(rtsys->segArray);
#endif
    
    /* Assign various function pointers */
    rtsys->contextSwitchCode = contextSwitchCode;
    rtsys->timeCmp = timeCmp;
    rtsys->prioCmp = prioCmp;
    rtsys->default_arrival = default_arrival;
    rtsys->default_release = default_release;
    rtsys->default_start = default_start;
    rtsys->default_suspend = default_suspend;
    rtsys->default_resume = default_resume;
    rtsys->default_finish = default_finish;
    rtsys->default_runkernel = default_runkernel;
    rtsys->prioFP = prioFP;
    rtsys->prioEDF = prioEDF;
    rtsys->prioDM = prioDM;
    
    /* Create basic kernel data structures */
    rtsys->taskList = new List("TaskList", NULL);
    rtsys->handlerList = new List("HandlerList", NULL);
    rtsys->timerList = new List("TimerList", NULL);
    rtsys->monitorList = new List("MonitorList", NULL);
    rtsys->eventList = new List("EventList", NULL);
    rtsys->mailboxList = new List("MailboxList", NULL);
    rtsys->semaphoreList = new List("SemaphoreList", NULL);
    rtsys->logList = new List("LogList", NULL);
    rtsys->cbsList = new List("CBSList", NULL);
    rtsys->tbsList = new List("TBSList", NULL);
    rtsys->ttdispList = new List("TimeTriggeredDispatcherList", NULL);
    rtsys->slotShiftList = new List("SlotShiftList", NULL);
    
    /* Read number of inputs, outputs, and triggers from block mask */
    const mxArray *arg;
    arg = ssGetSFcnParam(S, 2);
    int m = (int)mxGetM(arg);
    int n = (int)mxGetN(arg);
    if (n != 2 || m != 1) {
      TT_CALLBACK_ERROR(S, "Illegal number of analog inputs/outputs!");
      return;
    }
    int ninputs = (int)mxGetPr(arg)[0];
    int noutputs = (int)mxGetPr(arg)[1];
    if (ninputs < 0 || noutputs < 0) {
      TT_CALLBACK_ERROR(S, "Illegal number of analog inputs/outputs!");
      return;
    }
    rtsys->nbrOfInputs = ninputs;
    rtsys->nbrOfOutputs = noutputs;
    if (ninputs > 0) {
      rtsys->inputs = new double[ninputs];
    }
    if (noutputs > 0) {
      rtsys->outputs = new double[noutputs];
    }
    
    arg = ssGetSFcnParam(S, 3);
    if (!mxIsDoubleScalar(arg)) {
      TT_CALLBACK_ERROR(S, "Illegal number of triggers!");
      return;
    }
    int ntriggers = (int)*mxGetPr(arg);
    if (ntriggers < 0) {
      TT_CALLBACK_ERROR(S, "Illegal number of triggers!");
      return;
    }
    rtsys->nbrOfTriggers = ntriggers;
    rtsys->triggers = new Trigger[ntriggers];
    
    arg = ssGetSFcnParam(S, 4);
    if (!mxIsDoubleScalar(arg)) {
      TT_CALLBACK_ERROR(S, "Illegal trigger type!");
      return;
    }
    int trigType = (int)*mxGetPr(arg);
    rtsys->trigType = trigType;
    
    /* Create network interfaces according to the block mask */
    arg = ssGetSFcnParam(S, 5);
    m = (int)mxGetM(arg);  // number of rows = number of network interfaces
    n = (int)mxGetN(arg);  // number of cols should be 1 or 2
    int networkNbr;
    int nodeNbr;
    char nwname[MAXCHARS];
    if ((n == 1 && m != 1) || n > 2) {
      TT_CALLBACK_ERROR(S, "Illegal network or node numbers!");
      return;
    }
    if (m > 0) {
      rtsys->networkInterfaces = new NetworkInterface[m];
      rtsys->nbrOfNetworks = m;
      for (i=0; i<m; i++) {
        if (n == 1) {
          networkNbr = 1;
          nodeNbr = (int)mxGetPr(arg)[i];
        } else {
          networkNbr = (int)mxGetPr(arg)[i];
          nodeNbr = (int)mxGetPr(arg)[i+m];
        }
        debugPrintf("networkNbr %d : nodeNbr %d\n", networkNbr, nodeNbr);
        NetworkInterface *nwi = &(rtsys->networkInterfaces[i]);
        nwi->networkNbr = networkNbr;
        nwi->nodeNbr = nodeNbr-1;
        nwi->portNbr = i;
        sprintf(nwname, "network_%d", networkNbr);
      }
    }
    
    /* Read clock offset and drift parameters from the block mask */
    arg = ssGetSFcnParam(S, 6);
    
    if (!mxIsEmpty(arg)) {
      if (mxGetM(arg) == 1 && mxGetN(arg) == 2) {
        rtsys->clockOffset = mxGetPr(arg)[0];
        rtsys->clockDrift = mxGetPr(arg)[1] + 1.0;
      } else {
        TT_CALLBACK_ERROR(S, "Illegal offset/drift parameters!");
        return;
      }
    }
    
#ifdef KERNEL_MATLAB
    mexSetTrapFlag(1); // return control to the MEX file after an error
    
    /* Write rtsys pointer to global workspace */
    
    if (mexGetVariablePtr("global", "_rtsys") == 0) {

       //var2 = mxCreateDoubleScalar(7.0);

      // pointer variable does not exist - let's create one
      debugPrintf("Creating global _rtsys variable\n");
      mxArray *var = mxCreateNumericMatrix(2,1,mxUINT64_CLASS,mxREAL);
      mexMakeArrayPersistent(var);
      mexPutVariable("global", "_rtsys", var);
    }
    
    rtsys->rtsysptr = (mxArray*)mexGetVariablePtr("global", "_rtsys");
    debugPrintf("rtsysptr = %p\n", (void *)rtsys->rtsysptr);
    
    if (rtsys->rtsysptr == NULL) {
      mexPrintf("ERROR! No _rtsys found in global workspace!\n");
      return;
    }
    
    debugPrintf("Writing rtsys variable %p\n", (void *)rtsys);
    *((void **)mxGetData(rtsys->rtsysptr)) = (void *)rtsys;
     
    /* Evaluating user-defined init function (MATLAB) */
    mxGetString(ssGetSFcnParam(S, 0), initfun, MAXCHARS);
    rhs[0] = mxCreateString(initfun);
    if (mexCallMATLAB(1, lhs, 1, rhs, "nargin") != 0) {
      goto error;
    }
    nargin = (int)*mxGetPr(lhs[0]);
    if (nargin == 0) {
      if (mexCallMATLAB(0, NULL, 0, NULL, initfun) != 0) {
        goto error;
      } else {
        rtsys->init_phase = false;
      }
    } else if (nargin == 1) {
      rhs[0] = (mxArray *)ssGetSFcnParam(S, 1);
      if (mexCallMATLAB(0, NULL, 1, rhs, initfun) != 0) {
        goto error;
      } else {
        rtsys->init_phase = false;
      }
    } else {
      TT_CALLBACK_ERROR(S, "Init function takes wrong number (> 1) of arguments!");
      return;
    }
    
    if (rtsys->error) {
      error:
        mexCallMATLAB(1 ,error_msg_array, 0, NULL, "lasterr");
        error_msg = mxArrayToString(error_msg_array[0]);
        snprintf(errbuf, MAXERRBUF, "Error in init function '%s'\n%s", initfun, error_msg);
        mxFree(error_msg);
        TT_CALLBACK_ERROR(S, errbuf);
        return;
    }
      
#else
    /* Save pointer to init args */
    mxArray *initArg = (mxArray *)ssGetSFcnParam(S, 1);
    rtsys->initArg = initArg;
    /* Evaluating user-defined init function (C++) */
    init();
    if (rtsys->error) {
      TT_RUNKERNEL_ERROR(errbuf);
      mexPrintf("??? Error in init() function\n%s\n\n", errbuf);
      mexPrintf("In block ==> '%s'\nSimulation aborted!\n", ssGetBlockName(S));
      ssSetErrorStatus(S, "");
      return;
    }
    
    rtsys->init_phase = false;
#endif
    
    debugPrintf("init function successfully executed\n");
    
    if (!rtsys->initialized) {
      TT_CALLBACK_ERROR(S, "ttInitKernel was not called in init function");
      return;
    }
    if (!ssSetNumInputPorts(S, 4)) return;
    ssSetInputPortDirectFeedThrough(S, 0, 0);
    ssSetInputPortDirectFeedThrough(S, 1, 0);
    ssSetInputPortDirectFeedThrough(S, 2, 0);
    ssSetInputPortDirectFeedThrough(S, 3, 0);
    if (!ssSetNumOutputPorts(S, 4)) return;
    
    /* Input Ports */
    
    if (rtsys->nbrOfInputs > 0)
      ssSetInputPortWidth(S, 0, rtsys->nbrOfInputs);
    else
      ssSetInputPortWidth(S, 0, 1);
    
    if (rtsys->nbrOfTriggers > 0)
      ssSetInputPortWidth(S, 1, rtsys->nbrOfTriggers);
    else
      ssSetInputPortWidth(S, 1, 1);
    
    if (rtsys->nbrOfNetworks > 0) {
      ssSetInputPortWidth(S, 2, rtsys->nbrOfNetworks); /* Network receive */
    }
    else
      ssSetInputPortWidth(S, 2, 1);
    
    ssSetInputPortWidth(S, 3, 1); //battery
    
    /* Output Ports */
    
    if (rtsys->nbrOfOutputs > 0)
      ssSetOutputPortWidth(S, 0, rtsys->nbrOfOutputs);
    else
      ssSetOutputPortWidth(S, 0, 1);
    
    if (rtsys->nbrOfNetworks > 0)
      ssSetOutputPortWidth(S, 1, (rtsys->nbrOfNetworks)); /* Network send */
    else
      ssSetOutputPortWidth(S, 1, 1);
    
    if (rtsys->nbrOfSchedTasks > 0)
      ssSetOutputPortWidth(S, 2, rtsys->nbrOfSchedTasks * rtsys->nbrOfCPUs);
    else
      ssSetOutputPortWidth(S, 2, 1);
    
    ssSetOutputPortWidth(S, 3, 1); //Energy consumption
    
    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);
    
    ssSetNumSampleTimes(S, 1);
    
    ssSetNumRWork(S, 0);
    ssSetNumIWork(S, 0);
    ssSetNumPWork(S, 0);
    ssSetNumModes(S, 0);
    ssSetNumNonsampledZCs(S, 1);
    
    ssSetUserData(S, (void *)rtsys);
    
    ssSetOptions(S, SS_OPTION_CALL_TERMINATE_ON_EXIT);
    
  }
  
  
  static void mdlInitializeSampleTimes(SimStruct *S)
  {
    debugPrintf("'%s': mdlInitializeSampleTimes\n", S->path);
    
    ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, FIXED_IN_MINOR_STEP_OFFSET);
  }
  
  
#define MDL_START
  static void mdlStart(SimStruct *S)
  {
    debugPrintf("'%s': mdlStart\n", S->path);
    
    rtsys = (RTsys*) ssGetUserData(S);
    if (rtsys == NULL) {
      mexPrintf("ERROR: NULL pointer in rtsys\n");
      return;
    }
    
    // Display the TrueTime splash if global variable TTSPLASH not defined
    mxArray* splvar = (mxArray*)mexGetVariablePtr("global", "TTSPLASH");
    if (splvar == NULL) {
      splvar = mxCreateDoubleMatrix(0, 0, mxREAL);
      mexMakeArrayPersistent(splvar);
      mexPutVariable("global", "TTSPLASH", splvar);
      mexPrintf(
              "--------------------------------------------------------------\n"
              "                         TrueTime 2.0                         \n"
              "              Copyright (c) 2016 Lund University              \n"
              "   Written by Anton Cervin, Dan Henriksson and Martin Ohlin,  \n"
              " Department of Automatic Control LTH, Lund University, Sweden \n"
              "--------------------------------------------------------------\n"
              );
    }
    
    if (rtsys->init_phase) {
      /* Failure during initialization */
      return;
    }
    
    /* DATA ALLOCATION */
    
    if (rtsys->nbrOfTriggers > 0) {
      rtsys->oldtriggerinputs = new double[rtsys->nbrOfTriggers];
    }
    
    if (rtsys->nbrOfNetworks > 0) {
      rtsys->nwSnd = new double[rtsys->nbrOfNetworks];
      rtsys->oldnwSnd = new double[rtsys->nbrOfNetworks];
      rtsys->oldnetworkinputs = new double[rtsys->nbrOfNetworks];
    }
  }
  
#define MDL_INITIALIZE_CONDITIONS
  static void mdlInitializeConditions(SimStruct *S)
  {
    debugPrintf("'%s': mdlInitializeConditions\n", S->path);
    
    int i;
    
    rtsys = (RTsys*) ssGetUserData(S);
    if (rtsys == NULL) {
      mexPrintf("ERROR: NULL pointer in rtsys\n");
      return;
    }
    
    if (rtsys->init_phase) {
      /* Failure during initialization */
      return;
    }
    
    for (i=0; i<rtsys->nbrOfInputs; i++) {
      rtsys->inputs[i] = *ssGetInputPortRealSignalPtrs(S,0)[i];
    }
    for (i=0; i<rtsys->nbrOfOutputs; i++) {
      rtsys->outputs[i] = 0.0;
    }
    
    for (i=0; i<rtsys->nbrOfTriggers; i++) {
      rtsys->oldtriggerinputs[i] = 0.0;
    }
    
    /* Now we can be sure that the network blocks have been initialized and that network */
    /* pointers have been written to workspace. Do the remaining network interface initialization */
    
    for (i=0; i<rtsys->nbrOfNetworks; i++) {
      
      rtsys->nwSnd[i] = 0.0;
      rtsys->oldnwSnd[i] = 0.0;
      rtsys->oldnetworkinputs[i] = 0.0;
      
      char nwsysvarname[MAXCHARS];
      NetworkInterface* nwi = &(rtsys->networkInterfaces[i]);
      
      // Find global variable containing network pointer
      snprintf(nwsysvarname, MAXCHARS, "_nwsys_%d", nwi->networkNbr);
      mxArray *var = (mxArray*)mexGetVariablePtr("global", nwsysvarname);
      if (var == NULL) {
        mexPrintf("??? Network %d not found!\n", nwi->networkNbr);
        ssSetErrorStatus(S, "");
        return;
      }
      
      RTnetwork *tt_network = (RTnetwork *)*((void **)mxGetData(var));
      
      if (tt_network == NULL) {
        mexPrintf("??? Pointer to network not found in _nwsys_%d!\n", nwi->networkNbr);
        ssSetErrorStatus(S, "");
        return;
      }
      
      if (tt_network->networkNbr == nwi->networkNbr) {
        nwi->nwsys = tt_network;
        // if ttSetNetworkParameter() was called from the init_function
        if (nwi->transmitpower != -1.0) {
          nwi->nwsys->nwnodes[nwi->nodeNbr]->transmitPower =
                  nwi->transmitpower;
          debugPrintf("'%s': transmitpower is set to %.2f mW in node %d from the initfunction\n",
                  rtsys->blockName, nwi->nwsys->nwnodes[nwi->nodeNbr]->transmitPower*1000,
                  nwi->nodeNbr+1);
        }
        if (nwi->predelay != -1.0) {
          nwi->nwsys->nwnodes[nwi->nodeNbr]->predelay =
                  nwi->predelay;
        }
        if (nwi->postdelay != -1.0) {
          nwi->nwsys->nwnodes[nwi->nodeNbr]->postdelay =
                  nwi->postdelay;
        }
      }
    }
    
  }
  
  
  static void mdlOutputs(SimStruct *S, int_T tid)
  {
    debugPrintf("'%s': mdlOutputs at %.14f\n", rtsys->blockName, ssGetT(S));
    
    rtsys = (RTsys*) ssGetUserData(S);
    if (rtsys == NULL) {
      mexPrintf("ERROR: NULL pointer in rtsys\n");
      return;
    }
    
    if (rtsys->init_phase) {
      /* Failure during initialization */
      return;
    }
    
    real_T *y = ssGetOutputPortRealSignal(S,0);
    real_T *n = ssGetOutputPortRealSignal(S,1);
    real_T *s = ssGetOutputPortRealSignal(S,2);
    real_T *e = ssGetOutputPortRealSignal(S,3);
    int i, shouldRunKernel = 0;
    double timestep;
    
    DataNode *dn;
    UserTask* t;
    InterruptHandler* hdl;
    
    if (!rtsys->started && ssGetT(S) == 0.0) {
      rtsys->started = true;
    } else {
      
      /* Storing the time */
      
      rtsys->time = ssGetT(S) * rtsys->clockDrift + rtsys->clockOffset;
      
      shouldRunKernel = 0;
      
      /* Run kernel? */
      
      double externTime =  (rtsys->time- rtsys->clockOffset) / rtsys->clockDrift;
      if ((externTime >= rtsys->nextHit) || (shouldRunKernel > 0)) {
        timestep = runKernel(ssGetT(S));
        if (rtsys->error) {
          mexPrintf("In block ==> '%s'\n", ssGetBlockName(S));
          mexPrintf("Simulation aborted!\n");
          ssSetErrorStatus(S, errbuf);
          return;
        } else {
          rtsys->nextHit = (rtsys->time + timestep - rtsys->clockOffset) / rtsys->clockDrift;
        }
      }
    }
    
    /* Analog outputs */
    
    for (i=0; i<rtsys->nbrOfOutputs; i++) {
      y[i] = rtsys->outputs[i];
    }
    
    /* Network send outputs */
    
    for (i=0; i<rtsys->nbrOfNetworks; i++) {
      n[i] = rtsys->nwSnd[i];
      rtsys->oldnwSnd[i] = rtsys->nwSnd[i];
    }
    
    /* Usertask schedule outputs */
    
    i = 0;
    
    dn = (DataNode*) rtsys->taskList->getFirst();
    while (dn != NULL) {
      t = (UserTask*) dn->data;
      if (t->display) {
        double val = (double) (i+1);
        for (int j = 0; j < rtsys->nbrOfCPUs; j++) {
          s[i + j * rtsys->nbrOfSchedTasks] = val;
        }
        if (t->state == RUNNING) {
          val += 0.5;
        } else if (t->state == READY) {
          val += 0.25;
        } else if (t->state == WAITING) {
          val += 0.125;
        }
        s[i + t->affinity * rtsys->nbrOfSchedTasks] = val;
        i++;
        if (i > rtsys->nbrOfSchedTasks) {
          mexPrintf("FATAL ERROR: schedule output port out of bounds!\n");
          ssSetErrorStatus(S, "error");
          return;
        }
      }
      dn = (DataNode*) dn->getNext();
    }
    
    /* Handler schedule outputs */
    
    dn = (DataNode*) rtsys->handlerList->getFirst();
    while (dn != NULL) {
      hdl = (InterruptHandler*) dn->data;
      if (hdl->display) {
        double val = (double) (i+1);
        if (hdl->state == RUNNING) {
          val += 0.5;
        } else if (hdl->state == READY) {
          val += 0.25;
        }
        s[i + hdl->affinity * rtsys->nbrOfSchedTasks] = val;
        i++;
        if (i > rtsys->nbrOfSchedTasks) {
          mexPrintf("FATAL ERROR: schedule output port out of bounds!\n");
          ssSetErrorStatus(S, "error");
          return;
        }
      }
      dn = (DataNode*) dn->getNext();
    }
    
    /* Energy consumption output */
    e[0] = rtsys->energyConsumption;
    
  }
  
  
#define MDL_ZERO_CROSSINGS
  
  static void mdlZeroCrossings(SimStruct *S)
  {
    rtsys = (RTsys*) ssGetUserData(S);
    if (rtsys == NULL) {
      mexPrintf("ERROR: NULL pointer in rtsys\n");
      return;
    }
    
    debugPrintf("'%s': mdlZeroCrossings at %.14f\n", rtsys->blockName, ssGetT(S));
    
    int i;
    
    if (rtsys->init_phase) {
      /* Failure during initialization */
      return;
    }
    
    /* Copy analog inputs */
    InputRealPtrsType inputs = ssGetInputPortRealSignalPtrs(S,0);
    for (i=0; i<rtsys->nbrOfInputs; i++) {
      rtsys->inputs[i] = *inputs[i];
    }
    
    /* Check trigger input port for events */
    inputs = ssGetInputPortRealSignalPtrs(S,1);
    
    for (i=0; i<rtsys->nbrOfTriggers; i++) {
      
      Trigger* trig = &(rtsys->triggers[i]);
      
      // Only check for events if there is a handler attached
      if (trig->handler != NULL) {
        double input = *inputs[i];
        
        double oldinput = rtsys->oldtriggerinputs[i];
        int event = 0;
        
        switch (trig->state) {
          case ZERO:
            if (input > 0.0) {
              trig->state = POSITIVE;
              event = RISING;
            } else if (input < 0.0) {
              trig->state = NEGATIVE;
              event = FALLING;
            }
            break;
          case NEGATIVE:
            if ((input > 0.0 && oldinput <= 0.0) || (input == 0.0 && oldinput < 0.0)) {
              trig->state = POSITIVE;
              event = RISING;
            }
            break;
          case POSITIVE:
            if ((input < 0.0 && oldinput >= 0.0) || (input == 0.0 && oldinput > 0.0)) {
              trig->state = NEGATIVE;
              event = FALLING;
            }
            break;
        }
        if (event & rtsys->trigType) {
          if (trig->minimumInterval <= 0.0 || rtsys->time - trig->prevHit >= trig->minimumInterval) {
            debugPrintf("'%s': external trigger %d activated at %.14f\n", rtsys->blockName, i+1,  rtsys->time);
            
            // Trigger interrupt handler
            invoke_task(trig->handler, trig->trigName);
            if (trig->handler->nbrInvocations == 1) {
              rtsys->nextHit = ssGetT(S);
            }
            trig->prevHit = rtsys->time;
          }
        }
        rtsys->oldtriggerinputs[i] = *inputs[i];
      }
    }
    
    /* Check network input port for events */
    inputs = ssGetInputPortRealSignalPtrs(S,2);
    
    for (i=0; i<rtsys->nbrOfNetworks; i++) {
      
      NetworkInterface *nwi = &(rtsys->networkInterfaces[i]);
      
      // Only check for events if there is a handler attached
      
      if (nwi->handler != NULL) {
        double input = *inputs[i];
        double oldinput = rtsys->oldnetworkinputs[i];
        if (input != oldinput) {
          debugPrintf("'%s': incoming packet from network %d at %.14f\n", rtsys->blockName, nwi->networkNbr, rtsys->time);
          char trigname[MAXCHARS];
          snprintf(trigname, MAXCHARS, "network:%d", nwi->networkNbr);
          invoke_task(nwi->handler, trigname);
          if (nwi->handler->nbrInvocations == 1) {
            rtsys->nextHit = ssGetT(S);
          }
        }
        rtsys->oldnetworkinputs[i] = input;
      }
    }
    
    /* Read the energy level input */
    rtsys->energyLevel = *ssGetInputPortRealSignalPtrs(S,3)[0];
    
    // Schedule next major time step according to rtsys->nextHit
    ssGetNonsampledZCs(S)[0] = rtsys->nextHit - ssGetT(S);
  }
  
  
  static void mdlTerminate(SimStruct *S)
  {
    debugPrintf("'%s': mdlTerminate\n", S->path);
    
    rtsys = (RTsys*) ssGetUserData(S);
    if (rtsys == NULL) {
      mexPrintf("ERROR: NULL pointer in rtsys\n");
      return;
    }
    
    // Write task execution logs to the MATLAB workspace
    if (rtsys->taskList != NULL) {
      DataNode *dn = (DataNode*) rtsys->taskList->getFirst();
      while (dn != NULL) {
        UserTask *task = (UserTask*) dn->data;
        for (int j=0; j<NBRLOGS; j++) {
          Log *log = task->logs[j];
          if (log) {
            mxArray *ptr = mxCreateDoubleMatrix(log->entries, 1, mxREAL);
            for (int n=0; n<log->entries; n++) {
              mxGetPr(ptr)[n] = log->vals[n];
            }
            mexMakeArrayPersistent(ptr);
            mexPutVariable("base",log->variable,ptr);
          }
        }
        dn = (DataNode*) dn->getNext();
      }
    }
    
    // Write user-defined logs to the MATLAB workspace
    if (rtsys->logList != NULL) {
      DataNode *dn = (DataNode*) rtsys->logList->getFirst();
      while (dn != NULL) {
        Log *log = (Log*) dn->data;
        mxArray *ptr = mxCreateDoubleMatrix(log->entries, 1, mxREAL);
        for (int n=0; n<log->entries; n++) {
          mxGetPr(ptr)[n] = log->vals[n];
        }
        mexMakeArrayPersistent(ptr);
        mexPutVariable("base",log->variable,ptr);
        dn = (DataNode*) dn->getNext();
      }
    }
         
#ifdef KERNEL_MATLAB
    mxDestroyArray(rtsys->segArray);
#else
    cleanup();
#endif
    
    // Delete rtsys and all data structures within
    delete rtsys;
    rtsys = NULL;
    
#ifdef KERNEL_MATLAB
    mxArray* rhs[2];
    rhs[0] = mxCreateString("global");
    rhs[1] = mxCreateString("_rtsys");
    mexCallMATLAB(0, NULL, 2, rhs, "clear");
#endif
  }
  
  
#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
  
#ifdef __cplusplus
} // end of extern "C" scope
#endif

#endif
