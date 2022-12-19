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
#define S_FUNCTION_NAME ttsend


#include "simstruc.h"
#include "mexhelp.h"

// Local data structure

class SMsys {
 public:
  RTnetwork *nwsys; // pointer to network struct
  int networkNbr;   // number of the network to which the node is attached
  int sender;       // ID number of the node
  int indim;        // dimension of the data input port
  int dynamicSegment; // Used in FlexRay
  double trigger;   // output snd trigger
};

static void mdlInitializeSizes(SimStruct *S)
{
  const mxArray *arg;

  ssSetNumSFcnParams(S, 4);  /* Number of expected parameters */
  if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
    return; /* Parameter mismatch will be reported by Simulink */
  }

  if (!ssSetNumInputPorts(S, 5)) return;
  if (!ssSetNumOutputPorts(S, 1)) return;

  SMsys *smsys = new SMsys;
  smsys->trigger = 0.0;

  arg = ssGetSFcnParam(S, 0);
  if (mxIsDoubleScalar(arg)) {
    smsys->networkNbr  = (int)*mxGetPr(arg);
  }
  arg = ssGetSFcnParam(S, 1);
  if (mxIsDoubleScalar(arg)) {
    smsys->sender  = (int)*mxGetPr(arg);
  }
  arg = ssGetSFcnParam(S, 2);
  if (mxIsDoubleScalar(arg)) {
    smsys->indim  = (int)*mxGetPr(arg);
  }
  arg = ssGetSFcnParam(S,3);
  if(mxIsDoubleScalar(arg)) {
    smsys->dynamicSegment = (int)*mxGetPr(arg)-1; // set smsys->dynamicSegment = 0 (static) or 1 (dynamic)
  }
  

  /* Input ports */

  ssSetInputPortDirectFeedThrough(S, 0, 1); 
  ssSetInputPortDirectFeedThrough(S, 1, 1);
  ssSetInputPortDirectFeedThrough(S, 2, 1);
  ssSetInputPortDirectFeedThrough(S, 3, 1);
  ssSetInputPortDirectFeedThrough(S, 4, 1);

  ssSetInputPortWidth(S, 0, 1);             // receiver
  ssSetInputPortWidth(S, 1, smsys->indim);  // data
  ssSetInputPortWidth(S, 2, 1);             // length
  ssSetInputPortWidth(S, 3, 1);             // prio
  ssSetInputPortWidth(S, 4, 1);             // msgID

  /* Output Ports */

  ssSetOutputPortWidth(S, 0, 1);            // snd trigger

  ssSetNumContStates(S, 0);
  ssSetNumDiscStates(S, 0);
  
  ssSetNumSampleTimes(S, 1);
    
  ssSetNumRWork(S, 0);
  ssSetNumIWork(S, 0);
  ssSetNumPWork(S, 0); 
  ssSetNumModes(S, 0);
  ssSetNumNonsampledZCs(S, 0);

  ssSetUserData(S, smsys);
  
  ssSetOptions(S, SS_OPTION_CALL_TERMINATE_ON_EXIT); 
}


static void mdlInitializeSampleTimes(SimStruct *S)
{
  //mexPrintf("mdlInitSample\n");
  ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
  ssSetOffsetTime(S, 0, FIXED_IN_MINOR_STEP_OFFSET);
}


#define MDL_INITIALIZE_CONDITIONS
static void mdlInitializeConditions(SimStruct *S)
{
  /* Now we can be sure that the network blocks have been initialized 
     and that network pointers have been written to workspace. 
     Do the remaining initialization */
  
  SMsys *smsys = (SMsys*) ssGetUserData(S);

  // Get the network pointer from the MATLAB workspace
  char nwsysbuf[MAXCHARS];
  sprintf(nwsysbuf, "_nwsys_%d", smsys->networkNbr);
  mxArray *var = (mxArray*)mexGetVariablePtr("global", nwsysbuf);
  if (var == NULL) {
    mexPrintf("Network %d not found!\n", smsys->networkNbr);
    ssSetErrorStatus(S, "ttsend: cannot connect to network block");
    return;
  }
  smsys->nwsys = (RTnetwork *)(*((long long *)mxGetPr(var)));

  // Check the mask input arguments
  if (smsys->sender < 1 || smsys->sender > smsys->nwsys->nbrOfNodes) {
    mexPrintf("Sender number %d out of bounds\n", smsys->sender);
    ssSetErrorStatus(S, "ttsend: sender number out of bounds");
    return;
  }
}


static void mdlOutputs(SimStruct *S, int_T tid)
{
  //mexPrintf("mdlOutputs at %g\n", ssGetT(S));
  SMsys *smsys = (SMsys*) ssGetUserData(S);
  RTnetwork *nwsys = smsys->nwsys;

  // Read receiver input port
  int receiver = (int)*ssGetInputPortRealSignalPtrs(S,0)[0];

  if (receiver < 0 || receiver > smsys->nwsys->nbrOfNodes) {
    mexPrintf("Receiver number %d out of bounds\n", smsys->sender);
    ssSetErrorStatus(S, "ttsend: receiver number out of bounds");
    return;
  }    

  // Read data input port and create mxArray
  mxArray *data = mxCreateDoubleMatrix(smsys->indim, 1, mxREAL);  
  for (int i=0; i<smsys->indim; i++) {
    mxGetPr(data)[i] = *ssGetInputPortRealSignalPtrs(S,1)[i];
  }
  mexMakeArrayPersistent(data);
  // Read length input port
  int length = (int)*ssGetInputPortRealSignalPtrs(S,2)[0];
  if (length < 0) length = 0;

  // Read prio input port
  double prio = (int)*ssGetInputPortRealSignalPtrs(S,3)[0];

  // Read msgID input port
  int msgID = (int)*ssGetInputPortRealSignalPtrs(S,4)[0];

  // Create a network packet
  NWmsg *nwmsg = new NWmsg();
  nwmsg->sender = smsys->sender-1;
  nwmsg->receiver = receiver-1;
  nwmsg->data = NULL;
  nwmsg->dataMATLAB = data;
  nwmsg->length = length;
  nwmsg->prio = prio;
  nwmsg->timestamp = ssGetT(S);
  nwmsg->signalPower = 0.0;  // default, changed by wireless networks
  nwmsg->msgID = msgID;
  

  // do the dirty work: poke around inside nwsys of the network block 
  // set time when finished preprocessing
  nwmsg->waituntil = ssGetT(S) + nwsys->nwnodes[nwmsg->sender]->predelay;
  nwmsg->collided = 0; // This message has not collided (802.11)
 
  // enqueue message in preprocQ
  if (smsys->dynamicSegment == 0) {//node sending in the static segment
    nwsys->nwnodes[nwmsg->sender]->statpreprocQ->appendNode(nwmsg);
  } else {
    nwsys->nwnodes[nwmsg->sender]->preprocQ->appendNode(nwmsg);
  }

  if (smsys->trigger == 0.0) {
    smsys->trigger = 1.0; // trigger snd output
  } else {
    smsys->trigger = 0.0; // trigger snd output
  }
  ssGetOutputPortRealSignal(S,0)[0] = smsys->trigger;
} 


static void mdlTerminate(SimStruct *S)
{   
  //mexPrintf("mdlTerminate\n");

  SMsys *smsys = (SMsys*) ssGetUserData(S);
  
  if (smsys == NULL) {
    return;
  }

  delete smsys;

}

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif

#ifdef __cplusplus
} // end of extern "C" scope
#endif
