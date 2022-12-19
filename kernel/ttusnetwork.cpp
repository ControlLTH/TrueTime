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

#include "ttnetwork.h"

/**
 * Returns the time of the next invocation (nextHit)
*/

static double runNetwork(RTnetwork *nwsys) {

  int i, j;
  NWmsg *m, *m2;  
  NWmsg *next;
  double timeElapsed;
  double nextHit = nwsys->time + TT_MAX_TIMESTEP;

  timeElapsed = nwsys->time - nwsys->prevHit; // time since last invocation
  nwsys->prevHit = nwsys->time;

  //mexPrintf("Running network at %f\n", nwsys->time);

  // Check if messages have finished waiting in the preprocQ's

  for (i=0; i<nwsys->nbrOfNodes; i++) {
    m = (NWmsg *)nwsys->nwnodes[i]->preprocQ->getFirst();
    while (m != NULL) {
      if (m->waituntil - nwsys->time < TT_TIME_RESOLUTION) {
	//mexPrintf("moving message from preprocQ to inputQ at %f\n", nwsys->time);
	m->remaining = nwsys->pinglength;
	nwsys->nwnodes[i]->preprocQ->removeNode(m);
	nwsys->nwnodes[i]->inputQ->appendNode(m);
      } else {
	// update nextHit?
	if (m->waituntil < nextHit) {
	  nextHit = m->waituntil;
	}
      }
      m = (NWmsg *)m->getNext();
    }
  }
  
  // go through all nodes and count down transmission times
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    if (nwsys->nwnodes[i]->state == 1) { // node has been sending
      m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst();
      // decrease remaining number of bits in current frame
      m->remaining -= timeElapsed;
      // frame finished?
      if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {
	// transmission is finished, move to outputQ
	//mexPrintf("transmission finished\n");
	nwsys->nwnodes[i]->inputQ->removeNode(m);
	
	// broadcast the message to all other nodes
	for (j=0; j<nwsys->nbrOfNodes; j++) {
	  if (j != m->sender) {

	    // compute the distance from sending to receiving node
	    double dx = nwsys->nwnodes[i]->xCoordinate - nwsys->nwnodes[j]->xCoordinate;
	    double dy = nwsys->nwnodes[i]->yCoordinate - nwsys->nwnodes[j]->yCoordinate;
	    double distance = sqrt(dx*dx + dy*dy);
	    
	    if (distance <= nwsys->reach) {
	      // Duplicate message
	      m2 = new NWmsg();
	      *m2 = *m;
	      nwsys->nwnodes[j]->outputQ->appendNode(m2);

	      double propdelay = distance / nwsys->speedofsound;
	      //mexPrintf("prop.delay = %f\n", propdelay);
      
	      m2->waituntil = nwsys->time + propdelay + nwsys->nwnodes[j]->postdelay;

	      // update nextHit?
	      if (m2->waituntil < nextHit) {
		nextHit = m2->waituntil;
	      }
	    } else {

	      //mexPrintf("node %d out of reach to receive ping\n", j);

	    }

	  }
	}
	// delete the original message
	delete m;
	m = NULL;
	
	nwsys->nwnodes[i]->state = 0;
	
      } else { // frame not finished
	// update nextHit?
	if (nwsys->time + m->remaining < nextHit) {
	  nextHit = nwsys->time + m->remaining;
	} 
      }
    }
  }
    
  // check if any new transmissions should be started
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    if (nwsys->nwnodes[i]->state == 0) { // idle?
      // check if we should start a new transmission
      if ((m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) {
	//mexPrintf("Node %d starting transmission of frame at %f\n", i, nwsys->time);
	nwsys->nwnodes[i]->state = 1; // we're sending
	// update nextHit?
	if (nwsys->time + m->remaining < nextHit) {
	  nextHit = nwsys->time + m->remaining;
	} 
      }
    }
  }

  // Check if messages have finished waiting in the outputQ's

  for (i=0; i<nwsys->nbrOfNodes; i++) {
    m = (NWmsg *)nwsys->nwnodes[i]->outputQ->getFirst();
    while (m != NULL) {
      next = (NWmsg *)m->getNext();
      if (m->waituntil - nwsys->time < TT_TIME_RESOLUTION) {
	// finished waiting, move to postprocQ
	//mexPrintf("moving message from outputQ %d to postprocQ at %f\n", i+1, nwsys->time);
	nwsys->nwnodes[i]->outputQ->removeNode(m);

	double rNbr = ((double) rand() / (double) RAND_MAX);
	if (rNbr < nwsys->lossprob) {
	  // packet lost, do not forward to post-proc
	  //mexPrintf("Network: A packet headed for node # %d, was lost at time %f\n", i + 1 , nwsys->time);
	  delete m;
	  m = NULL;
	} else {
	  delete m; // the message should never be read by the receiver
	  m = NULL;
	  if (nwsys->outputs[i] == 0.0) {
	    nwsys->outputs[i] = 1.0; // trigger rcv output
	  } else {
	    nwsys->outputs[i] = 0.0; // trigger rcv output
	  }
	}
      } else {
	// update nextHit?
	if (m->waituntil < nextHit) {
	  nextHit = m->waituntil;
	}
      }
      m = next; // get next
    }
  }

  // done

  //mexPrintf("Next hit scheduled for %f\n", nextHit);

  // produce output graph

  for (i=0; i<nwsys->nbrOfNodes; i++) {
    if (nwsys->nwnodes[i]->state == 1) {
      nwsys->sendschedule[i] = i+1.5;  // sending
    } else if ((m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) {
      nwsys->sendschedule[i] = i+1.25; // waiting
    } else {
      nwsys->sendschedule[i] = i+1.0;     // idle
    }
  }

  return nextHit;
}

// ------- Simulink callback functions ------- 

#ifdef __cplusplus
extern "C" { // use the C fcn-call standard for all functions  
#endif       // defined within this scope   

#define S_FUNCTION_NAME ttusnetwork
#define S_FUNCTION_LEVEL 2

#include "simstruc.h"
  static void mdlInitializeSizes(SimStruct *S)
  {
    const mxArray *arg;

    ssSetNumSFcnParams(S, 5);  /* Number of expected parameters */
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
      return; /* Parameter mismatch will be reported by Simulink */
    }
  
    // Parse second argument only, to determine nbrOfNodes

    // 2 - Number of nodes
    int nbrOfNodes = 0;
    arg = ssGetSFcnParam(S, 1);
    if (mxIsDoubleScalar(arg)) {
      nbrOfNodes = (int) *mxGetPr(arg);
    }
    if (nbrOfNodes <= 0) {
      ssSetErrorStatus(S, "TrueTime Ultrasound Network: The number of nodes must be an integer > 0");
      return;
    }

    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);
  
    ssSetNumInputPorts(S, 3);
    ssSetInputPortDirectFeedThrough(S, 0, 0);
    ssSetInputPortWidth(S, 0, nbrOfNodes);
    ssSetInputPortWidth(S, 1, nbrOfNodes);
    ssSetInputPortWidth(S, 2, nbrOfNodes);
  
    ssSetNumOutputPorts(S, 2);
    ssSetOutputPortWidth(S, 0, nbrOfNodes);
    ssSetOutputPortWidth(S, 1, nbrOfNodes);

    ssSetNumSampleTimes(S, 1);
  
    ssSetNumRWork(S, 0);
    ssSetNumIWork(S, 0);
    ssSetNumPWork(S, 0); 
    ssSetNumModes(S, 0);
    ssSetNumNonsampledZCs(S, 1);

    // Make sure cleanup is performed even if errors occur
    ssSetOptions(S, SS_OPTION_CALL_TERMINATE_ON_EXIT);

    int i;

    // Create new network struct

    RTnetwork *nwsys = new RTnetwork;
    ssSetUserData(S, nwsys); // save pointer in UserData

    // Arg 1 - Network Number
    nwsys->networkNbr = 0;
    arg = ssGetSFcnParam(S, 0);
    if (mxIsDoubleScalar(arg)) {
      nwsys->networkNbr = (int) *mxGetPr(arg);
    } 
    if (nwsys->networkNbr <= 0) {
      ssSetErrorStatus(S, "TrueTime Ultrasound Network: The network number must be > 0");
      return;
    }

    // Arg 2 - Number of nodes
    arg = ssGetSFcnParam(S, 1);
    nwsys->nbrOfNodes = (int) *mxGetPr(arg); // we know it's right
    //mexPrintf("nbrOfNodes: %d\n", nwsys->nbrOfNodes);
    
    // Arg 3 - Reach
    nwsys->reach = 0.0;
    arg = ssGetSFcnParam(S, 2);
    if (mxIsDoubleScalar(arg)) {
      nwsys->reach = *mxGetPr(arg);
    }
    if (nwsys->reach < 0.0) {
      ssSetErrorStatus(S, "TrueTime Ultrasound Network: The reach must be >= 0");
      return;
    }
    //mexPrintf("reach: %f\n", nwsys->reach);

    // Arg 4 - Ping length
    nwsys->pinglength = 0.0;
    arg = ssGetSFcnParam(S, 3);
    if (mxIsDoubleScalar(arg)) {
      nwsys->pinglength = *mxGetPr(arg);
    }
    if (nwsys->pinglength < 0.0) {
      ssSetErrorStatus(S, "TrueTime Ultrasound Network: The ping length must be >= 0");
      return;
    }
    //mexPrintf("ping length: %f\n", nwsys->pinglength);

    // Arg 5 - Speed of sound
    nwsys->speedofsound = 0.0;
    arg = ssGetSFcnParam(S, 4);
    if (mxIsDoubleScalar(arg)) {
      nwsys->speedofsound = *mxGetPr(arg);
    }
    if (nwsys->speedofsound <= 0.0) {
      ssSetErrorStatus(S, "TrueTime Ultrasound Network: The speed of sound must be > 0");
      return;
    }
    //mexPrintf("speedofsound: %f\n", nwsys->speedofsound);

    /* Write pointer to Simulink block UserData */
    /*    mexCallMATLAB(1, lhs, 0, NULL, "gcbh");
    sprintf(nwsysp,"%p",nwsys);
    rhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
    *mxGetPr(rhs[0]) = *mxGetPr(lhs[0]);
    rhs[1] = mxCreateString("UserData");
    rhs[2] = mxCreateString(nwsysp);
    mexCallMATLAB(0, NULL, 3, rhs, "set_param"); */

    /* Write pointer to MATLAB global workspace */
    /* FIX: The code above is intended to be removed and replaced by this. */
    /* Write rtsys pointer to global workspace */
    mxArray* var = mxCreateDoubleScalar(0.0);
    mexMakeArrayPersistent(var);
    *((void **)mxGetData(var)) = nwsys;
    char nwsysbuf[MAXCHARS];
    sprintf(nwsysbuf, "_nwsys_%d", nwsys->networkNbr);
    mexPutVariable("global", nwsysbuf, var);
    
    nwsys->inputs = new double[nwsys->nbrOfNodes];
    nwsys->oldinputs = new double[nwsys->nbrOfNodes];
    nwsys->outputs   = new double[nwsys->nbrOfNodes];
    nwsys->sendschedule = new double[nwsys->nbrOfNodes];

    for (i=0; i<nwsys->nbrOfNodes; i++) {
      nwsys->inputs[i] = 0.0;
      nwsys->oldinputs[i] = 0.0;
      nwsys->outputs[i] = 0.0;
      nwsys->sendschedule[i] = i+1;
    }

    nwsys->time = 0.0;
    nwsys->prevHit = 0.0;
  
    nwsys->nwnodes = new NWnode*[nwsys->nbrOfNodes];
    for (i=0; i<nwsys->nbrOfNodes; i++) {
      int j;

      nwsys->nwnodes[i] = new NWnode();
      //nwsys->nwnodes[i]->transmitPower = transmitPowerWatt;

      nwsys->nwnodes[i]->signallevels = new double[nwsys->nbrOfNodes]; //802.11
      for (j=0; j<nwsys->nbrOfNodes; j++) {
	nwsys->nwnodes[i]->signallevels[j] = 0;
      }
      
    }

    nwsys->waituntil = 0.0;
    nwsys->sending = -1;  // Note! -1 means nobody is sending
    nwsys->rrturn = nwsys->nbrOfNodes - 1; // want to start at 0
    nwsys->lasttime = -1.0;

    nwsys->slotcount = nwsys->schedsize - 1; // want to start at 0
    nwsys->currslottime = -nwsys->slottime;  // same here

    // rad, kolum, reella tal
    //nwsys->nbrOfTransmissions = mxCreateDoubleMatrix(nwsys->nbrOfNodes, nwsys->nbrOfNodes, mxREAL);
    //mexMakeArrayPersistent(nwsys->nbrOfTransmissions);
  }


  static void mdlInitializeSampleTimes(SimStruct *S)
  {
    ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, FIXED_IN_MINOR_STEP_OFFSET);
  }


#define MDL_START
  static void mdlStart(SimStruct *S)
  {
 
  }


#define MDL_INITIALIZE_CONDITIONS
  static void mdlInitializeConditions(SimStruct *S)
  {

  }

  static void mdlOutputs(SimStruct *S, int_T tid)
  {
    int i, exttrig = 0;

    RTnetwork *nwsys = (RTnetwork*)ssGetUserData(S);
    nwsys->time = ssGetT(S);
    real_T* output_0 = ssGetOutputPortRealSignal(S,0);
    real_T* output_1 = ssGetOutputPortRealSignal(S,1);

    for (i=0; i < nwsys->nbrOfNodes; i++) {
      // mexPrintf("input %d: %f\n", i+1, input);
      if (fabs(nwsys->inputs[i]-nwsys->oldinputs[i]) > 0.1) {
	// mexPrintf("event at input %d\n", i);
	nwsys->oldinputs[i] = nwsys->inputs[i];
	exttrig = 1;
      }
    }
    
    if (exttrig == 1) {
      // Triggered on external events
      nwsys->nextHit = runNetwork(nwsys);
    } else {
      // Triggered on internal events
      if (nwsys->time >= nwsys->nextHit) {
	nwsys->nextHit = runNetwork(nwsys);
      }
    }

    for (i=0; i<nwsys->nbrOfNodes; i++) {
      output_0[i] = nwsys->outputs[i];
      output_1[i] = nwsys->sendschedule[i];
    }
  } 


#define MDL_ZERO_CROSSINGS

  static void mdlZeroCrossings(SimStruct *S)
  {
    int i;
    double now = ssGetT(S);
    RTnetwork *nwsys = (RTnetwork*)ssGetUserData(S);
    InputRealPtrsType input_0 = ssGetInputPortRealSignalPtrs(S,0);
    InputRealPtrsType input_1 = ssGetInputPortRealSignalPtrs(S,1);
    InputRealPtrsType input_2 = ssGetInputPortRealSignalPtrs(S,2);

    /* Check for external events */
    for (i=0; i < nwsys->nbrOfNodes; i++) {
      if (fabs(*input_0[i] - nwsys->inputs[i]) > 0.1) {
	nwsys->nextHit = now;
	break;
      }
    }
    /* Copy inputs */
    for (i=0; i < nwsys->nbrOfNodes; i++) {
      nwsys->inputs[i] = *input_0[i];
      nwsys->nwnodes[i]->xCoordinate = *input_1[i];
      nwsys->nwnodes[i]->yCoordinate = *input_2[i];
    }
    ssGetNonsampledZCs(S)[0] = nwsys->nextHit - now;
  }


  static void mdlTerminate(SimStruct *S)
  {
    RTnetwork *nwsys = (RTnetwork*) ssGetUserData(S);
    if (nwsys == NULL) {
      return;
    }

    char nwsysbuf[MAXCHARS];
    mxArray* rhs[2];
    sprintf(nwsysbuf, "_nwsys_%d", nwsys->networkNbr);
    rhs[0] = mxCreateString("global");
    rhs[1] = mxCreateString(nwsysbuf);
    mexCallMATLAB(0, NULL, 2, rhs, "clear");

    delete nwsys;
  }


#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif

#ifdef __cplusplus
} // end of extern "C" scope
#endif
