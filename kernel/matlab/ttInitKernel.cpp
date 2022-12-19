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

#define KERNEL_MATLAB
#include "../ttkernel.h" 

#include "../initkernel.cpp"
#include "getrtsys.cpp"


void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  rtsys = getrtsys(); // Get pointer to rtsys 
  if (rtsys==NULL) {
    return;
  }

  // Check number and type of arguments. 
  if (nrhs < 1 || nrhs > 2) {
    TT_MEX_ERROR("ttInitKernel: Wrong number of input arguments! \nUsage: ttInitKernel(prioFcn) or\n       ttInitKernel(prioFcn, contextSwitchOH)");
    return;
  }
  if (mxIsChar(prhs[0]) != 1 || mxGetM(prhs[0]) != 1) {
    TT_MEX_ERROR("ttInitKernel: prioFcn must be a string");
    return;
  }
  if (nrhs == 2) {
    if (!mxIsDoubleScalar(prhs[1])) {
      TT_MEX_ERROR("ttInitKernel: contextSwitchOH must be a number");
      return;
    }
  }

  char buf[MAXCHARS];
  mxGetString(prhs[0], buf, MAXCHARS);

  int dispatch;
  if (strcmp(buf, "prioFP") == 0) {
    dispatch = FP;
  } else if (strcmp(buf, "prioDM") == 0){
    dispatch = DM;
  } else if (strcmp(buf, "prioEDF") == 0) {
    dispatch = EDF;
  } else {
    char errbuf[MAXERRBUF];
    snprintf(errbuf, MAXERRBUF, "ttInitKernel: Unknown priority function '%s'", buf);
    TT_MEX_ERROR(errbuf);
    return;
  }
  
  if (nrhs == 1) {
    ttInitKernelMATLAB(dispatch);
  } else {
    double contextSwitchOH = *mxGetPr(prhs[1]);
    ttInitKernelMATLAB(dispatch, contextSwitchOH);
  }
}

