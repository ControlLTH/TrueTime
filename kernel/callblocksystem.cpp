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

#ifndef CALL_BLOCK_SYSTEM
#define CALL_BLOCK_SYSTEM

#include "getnode.cpp"

void ttCallBlockSystem(int nOutp, double *outp, int nInp, double *inp, const char *blockName) {
  
  mxArray *simsetRHS[8];
  mxArray *simRHS[4];
  mxArray *simLHS[3];
  mxArray *options[1];
  mxArray *states;
  double *values;
  double *state_values;
  int i, j, n;

  DataNode* dn;
  Blockdata *bd;

  // Make sure that the Simulink block exists in Matlab path
  mxArray *lhs[1];
  mxArray *rhs[1];
  rhs[0] = mxCreateString(blockName);
  mexCallMATLAB(1, lhs, 1, rhs, "exist");
  int number = (int) *mxGetPr(lhs[0]);
  if (number == 0) {
    TT_MEX_ERROR("ttCallBlockSystem: Simulink block not in path!");
    for (j=0; j < nOutp; j++) {
      outp[j] = 0.0;
    }
    return;
  }

  Task* task = rtsys->running;
  dn = (DataNode*) getNode(blockName, task->blockList);

  if (dn==NULL) {
    // Not found, create options mxArray (Matlab struct) 

    bd = new Blockdata(blockName);
    
    simsetRHS[0] = mxCreateString("Solver");
    simsetRHS[1] = mxCreateString("FixedStepDiscrete");
    simsetRHS[2] = mxCreateString("FixedStep");
    simsetRHS[3] = mxCreateDoubleScalar(1.0);
    simsetRHS[4] = mxCreateString("MaxDataPoints");
    simsetRHS[5] = mxCreateDoubleScalar(2.0);
    simsetRHS[6] = mxCreateString("OutputVariables");
    simsetRHS[7] = mxCreateString("xy");
    
    mexCallMATLAB(1, options, 8, simsetRHS, "simset");
    
    bd->options = options[0];
    mexMakeArrayPersistent(bd->options);
    
    for (i=0; i<8; i++) {
      mxDestroyArray(simsetRHS[i]);
    }
    task->blockList->appendNode(new DataNode(bd, bd->blockName));
  } else {
    bd = (Blockdata*) dn->data;
  }

  // [t,x,outp] = sim(blockName,1,options,[0 inp]) 

  simRHS[0] = mxCreateString(blockName);
  simRHS[1] = mxCreateDoubleScalar(1.0);
  simRHS[2] = bd->options;
  simRHS[3] = mxCreateDoubleMatrix(1, nInp+1, mxREAL);
  values = mxGetPr(simRHS[3]);
  values[0] = 0.0;
  for (j=0; j < nInp; j++) {
    values[j+1] = inp[j];
  }

  i = mexCallMATLAB(3, simLHS, 4, simRHS, "sim");

  mxDestroyArray(simRHS[0]);
  mxDestroyArray(simRHS[1]);
  mxDestroyArray(simRHS[3]);

  if (i==0) { // Successful
    // Update options mxArray with last row of returned state matrix 
    n = (int)mxGetN(simLHS[1]); // Number of cols of state matrix 
    
    states = mxCreateDoubleMatrix(1, n, mxREAL);

    values = mxGetPr(simLHS[1]);
    state_values = mxGetPr(states);
    
    // Transfer values 
    for (j=0; j < n; j++) {
      state_values[j] = values[2*j+1]; // second row elements
    }

    mxArray* oldstates = mxGetField(bd->options, 0, "InitialState");
    mxDestroyArray(oldstates);
    mxSetField(bd->options, 0, "InitialState", states);
    
    // Return the output of the simulation, first row of returned output matrix 
    values = mxGetPr(simLHS[2]);
    for (j=0; j < nOutp; j++) {
      outp[j] = values[2*j];          // first row elements
    }

    mxDestroyArray(simLHS[0]);
    mxDestroyArray(simLHS[1]);
    mxDestroyArray(simLHS[2]);

  } else {

    TT_MEX_ERROR("ttCallBlockSystem: Simulation failed!");
    for (j=0; j < nOutp; j++) {
      outp[j] = 0.0;
    }
  }
}

#endif
