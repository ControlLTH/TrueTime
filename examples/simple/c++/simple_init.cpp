#define S_FUNCTION_NAME simple_init
#include "ttkernel.cpp"

// Data structure used for the task data
struct TaskData {
  double u;
  double K;
  double exectime;
};

// Code function for the P-controller
double ctrl_code(int segment, void* data) {

  TaskData *d = (TaskData*)data;
  double y;
  
  switch (segment) {
  case 1:
    y = ttAnalogIn(1);
    d->u = - d->K * y;
    return d->exectime;
  default:
    ttAnalogOut(1, d->u);
    return FINISHED;
  }
}

// Kernel init function    
void init() {

  // Allocate memory for the task and store pointer in UserData
  TaskData *data = new TaskData;
  ttSetUserData(data);

  data->K = 2.0;
  data->exectime = 0.1;

  ttInitKernel(prioFP);
  ttCreatePeriodicTask("ctrl_task", 0.0, 0.5, ctrl_code, data);

}

// Kernel cleanup function
void cleanup() {

  // Free the allocated memory
  TaskData *d = (TaskData *)ttGetUserData();
  delete d;
}
