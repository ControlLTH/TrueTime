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

// ------- Simulink callback functions ------- 

#include "ttnetwork.h"

#ifdef __cplusplus
extern "C" { // use the C fcn-call standard for all functions  
#endif       // defined within this scope   

#define S_FUNCTION_LEVEL 2
#define S_FUNCTION_NAME ttreceive


#include "simstruc.h"
#include "mexhelp.h"

// Local data structure

class RMsys {
 public:
  RTnetwork *nwsys; // pointer to network struct
  int networkNbr;   // number of the network to which the node is attached
  int receiver;     // ID number of the node
  int outdim;       // dimension of the data input port
  double trigger;   // output snd trigger
};


static void mdlInitializeSizes(SimStruct *S)
{
  const mxArray *arg;

  ssSetNumSFcnParams(S, 3);  /* Number of expected parameters */
  if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
    return; /* Parameter mismatch will be reported by Simulink */
  }
  
  if (!ssSetNumInputPorts(S, 0)) return;
  if (!ssSetNumOutputPorts(S, 7)) return;

  RMsys *rmsys = new RMsys;
  rmsys->trigger = 0.0;

  arg = ssGetSFcnParam(S, 0);
  if (mxIsDoubleScalar(arg)) {
    rmsys->networkNbr  = (int)*mxGetPr(arg);
  }
  arg = ssGetSFcnParam(S, 1);
  if (mxIsDoubleScalar(arg)) {
    rmsys->receiver  = (int)*mxGetPr(arg);
  }
  arg = ssGetSFcnParam(S, 2);
  if (mxIsDoubleScalar(arg)) {
    rmsys->outdim  = (int)*mxGetPr(arg);
  }

  /* Output Ports */

  ssSetOutputPortWidth(S, 0, rmsys->outdim);  // data
  ssSetOutputPortWidth(S, 1, 1);              // receiver
  ssSetOutputPortWidth(S, 2, 1);              // length
  ssSetOutputPortWidth(S, 3, 1);              // prio
  ssSetOutputPortWidth(S, 4, 1);              // time stamp
  ssSetOutputPortWidth(S, 5, 1);              // signal power (wireless only)
  ssSetOutputPortWidth(S, 6, 1);              // message id

  ssSetNumContStates(S, 0);
  ssSetNumDiscStates(S, 0);
  
  ssSetNumSampleTimes(S, 1);
    
  ssSetNumRWork(S, 0);
  ssSetNumIWork(S, 0);
  ssSetNumPWork(S, 0); 
  ssSetNumModes(S, 0);
  ssSetNumNonsampledZCs(S, 0);

  ssSetUserData(S, rmsys);
  
  ssSetOptions(S, SS_OPTION_CALL_TERMINATE_ON_EXIT); 
}


static void mdlInitializeSampleTimes(SimStruct *S)
{
  ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
  ssSetOffsetTime(S, 0, FIXED_IN_MINOR_STEP_OFFSET);
}


#define MDL_INITIALIZE_CONDITIONS
static void mdlInitializeConditions(SimStruct *S)
{
  /* Now we can be sure that the network blocks have been initialized 
     and that network pointers have been written to workspace. 
     Do the remaining initialization */
  
  RMsys *rmsys = (RMsys*) ssGetUserData(S);

  // Get the network pointer from the MATLAB workspace
  char nwsysbuf[MAXCHARS];
  sprintf(nwsysbuf, "_nwsys_%d", rmsys->networkNbr);
  mxArray *var = (mxArray*)mexGetVariablePtr("global", nwsysbuf);
  if (var == NULL) {
    mexPrintf("Network %d not found!\n", rmsys->networkNbr);
    ssSetErrorStatus(S, "ttreceive: cannot connect to network block");
    return;
  }
  rmsys->nwsys = (RTnetwork *)(*((long long *)mxGetPr(var)));

  // Check the mask input arguments
  if (rmsys->receiver < 1 || rmsys->receiver > rmsys->nwsys->nbrOfNodes) {
    mexPrintf("Receiver number %d out of bounds\n", rmsys->receiver);
    ssSetErrorStatus(S, "ttreceive: receiver number out of bounds");
    return;
  }

}


static void mdlOutputs(SimStruct *S, int_T tid)
{
  //mexPrintf("mdlOutputs at %g\n", ssGetT(S));
  RMsys *rmsys = (RMsys*) ssGetUserData(S);
  RTnetwork *nwsys = rmsys->nwsys;

  NWmsg *nwmsg;
  if ((nwmsg = (NWmsg *)nwsys->nwnodes[rmsys->receiver-1]->postprocQ->getFirst()) != NULL) {
    nwsys->nwnodes[rmsys->receiver-1]->postprocQ->removeNode(nwmsg);
  }

  if (nwmsg == NULL) {
    mexPrintf("trigged, but no message\n");
    return;
  }

  if ((mxGetM(nwmsg->dataMATLAB) != rmsys->outdim) || (mxGetN(nwmsg->dataMATLAB) != 1)) {
    mexPrintf("Data has the wrong dimensions! It should be %dx1\n", rmsys->outdim);
  } else {
    for (int i=0; i<rmsys->outdim; i++) {
      ssGetOutputPortRealSignal(S,0)[i] = mxGetPr(nwmsg->dataMATLAB)[i];
    }
    ssGetOutputPortRealSignal(S,1)[0] = nwmsg->sender + 1;
    ssGetOutputPortRealSignal(S,2)[0] = nwmsg->length;
    ssGetOutputPortRealSignal(S,3)[0] = nwmsg->prio;
    ssGetOutputPortRealSignal(S,4)[0] = nwmsg->timestamp;
    ssGetOutputPortRealSignal(S,5)[0] = nwmsg->signalPower; // zero for non-wireless
    ssGetOutputPortRealSignal(S,6)[0] = nwmsg->msgID;
  }
  
  // Delete message, including its MATLAB data
  delete nwmsg;
} 


static void mdlTerminate(SimStruct *S)
{   
  //mexPrintf("mdlTerminate\n");

  RMsys *rmsys = (RMsys*) ssGetUserData(S);
  
  if (rmsys == NULL) {
    return;
  }

  delete rmsys;

}

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif

#ifdef __cplusplus
} // end of extern "C" scope
#endif
