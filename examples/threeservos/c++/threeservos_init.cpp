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

// Task scheduling and control.
//
// This example extends the simple PID control example (located in
// $DIR/examples/simple_pid) to the case of three PID-tasks running
// concurrently on the same CPU controlling three different servo
// systems. The effect of the scheduling policy on the global control
// performance is demonstrated.

#define S_FUNCTION_NAME threeservos_init

#include "ttkernel.cpp"

// PID task data structure
struct TaskData {
  double u, Iold, Dold, yold;
  double K, Ti, Td, beta, N, h;
  int rChan, yChan, uChan; 
  bool late;
};

// Kernel data structure, for proper memory allocation and deallocation
struct KernelData {
  TaskData *taskData[3];
};

// calculate PID control signal and update states
void pidcalc(TaskData* d, double r, double y) {

  double P = d->K*(d->beta*r-y);
  double I = d->Iold;
  double D = d->Td/(d->N*d->h+d->Td)*d->Dold+d->N*d->K*d->Td/(d->N*d->h+d->Td)*(d->yold-y); 

  d->u = P + I + D;
  d->Iold = d->Iold + d->K*d->h/d->Ti*(r-y);
  d->Dold = D;
  d->yold = y;
}

// ---- PID code function ----
double pid_code(int seg, void* data) {

  double r, y;
  TaskData* d = (TaskData*) data;

  switch (seg) {
  case 1:  
    r = ttAnalogIn(d->rChan);
    y = ttAnalogIn(d->yChan);
    pidcalc(d, r, y); 
    return 0.002;
  default:
    ttAnalogOut(d->uChan, d->u);
    return FINISHED;
  }
}

// ---- PID code function with skips ----
double pid_code_skips(int seg, void* data) {

  double r, y;
  TaskData* d = (TaskData*) data;

  switch (seg) {
  case 1:
    if (d->late) {
      d->late = false;
      return FINISHED;
    }
    r = ttAnalogIn(d->rChan);
    y = ttAnalogIn(d->yChan);
    pidcalc(d, r, y); 
    return 0.002;
  default:
    ttAnalogOut(d->uChan, d->u);
    if (ttCurrentTime() > ttGetAbsDeadline()) {
      d->late = true;
    }
    return FINISHED;
  }
}

double dl_miss_code(int seg, void* data) {

  char task[MAXCHARS];

  if (sscanf(ttGetInvoker(), "DLtimer:%s", task)) {
    ttKillJob(task);
  }
  return FINISHED;
}

// Task parameters 
double starttimes[] = {0.0, 0.0, 0.0};
double periods[] = {0.006, 0.005, 0.004};
const char* tasknames[] = {"pid_task1", "pid_task2", "pid_task3"};
const char* lognames[] = {"response1", "response2", "response3"};

void init() {

  // Read the input argument from the block dialogue
  mxArray *initarg = ttGetInitArg();
  if (!mxIsDoubleScalar(initarg)) {
    TT_MEX_ERROR("The init argument must be a number!\n");
    return;
  }
  int arg = (int)mxGetPr(initarg)[0];

  double (*codeFcn)(int, void*);

  switch (arg) {
    case 1: // DM scheduling
      ttInitKernel(prioDM);
      codeFcn = pid_code;
      break;
    case 2: // plain EDF scheduling
    case 3: // EDF scheduling, kill jobs that overrun
    case 5: // EDF scheduling with pid_task3 inside CBS
    case 6: // EDF scheduling with pid_task3 inside TBS
      ttInitKernel(prioEDF);
      codeFcn = pid_code;
      break;
    case 4: // EDF scheduling, skip next job if current one late
      ttInitKernel(prioEDF);
      codeFcn = pid_code_skips;
      break;
    default:
      TT_MEX_ERROR("Illegal init argument!");
      return;
  }

  // Allocate kernel data memory and store pointer
  KernelData *kernelData = new KernelData;
  ttSetUserData(kernelData);

  // Create the three tasks
  for (int i=0; i<3; i++) {
    TaskData *data = new TaskData;
    kernelData->taskData[i] = data;  // Store pointer in kernel data
    data->K = 0.96;
    data->Ti = 0.12;
    data->Td = 0.049;
    data->beta = 0.5;
    data->N = 10;
    data->h = periods[i];
    data->u = 0.0;
    data->Iold = 0.0;
    data->Dold = 0.0;
    data->yold = 0.0;
    data->rChan = 1;
    data->yChan = i+2;
    data->uChan = i+1;
    data->late = false;

    ttCreatePeriodicTask(tasknames[i], starttimes[i], periods[i], codeFcn, data);
    ttCreateLog(tasknames[i], 1, lognames[i], 1000);  // Create response time log
  }

  switch (arg) {
    case 3: // EDF scheduling, kill jobs that overrun
      ttCreateHandler("dl_miss_handler", 1, dl_miss_code);
      for (int i=0; i<3; i++) {
        ttAttachDLHandler(tasknames[i], "dl_miss_handler");
      }
      break;
    case 5: // EDF scheduling with pid_task3 inside CBS
      ttCreateCBS("cbs", 0.001, 0.004);  // Allow max 25% utilization for pid_task3
      ttAttachCBS(tasknames[2], "cbs"); 
      break;
    case 6: // EDF scheduling with pid_task3 inside TBS
      ttCreateTBS("tbs", 0.25);          // Allow max 25% utilization for pid_task3
      ttSetWCET(0.002, tasknames[2]);    // Set WCET of the task
      ttAttachTBS(tasknames[2], "tbs"); 
      break;
  }
}

void cleanup() {

  KernelData *kernelData = (KernelData*)ttGetUserData();

  if (kernelData) {
    for (int i=0; i<3; i++) {
      delete kernelData->taskData[i];
    }
    delete kernelData;
  }
}
