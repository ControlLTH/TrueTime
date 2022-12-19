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

#include "../createhandler.cpp"
//#include "../activatejob.cpp"
#include "getrtsys.cpp"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  rtsys = getrtsys() ; // Get pointer to rtsys 

  if (rtsys==NULL) {
    return;
  }

  // Check number and type of arguments. 
  if (nrhs < 3 || nrhs > 4) {
    TT_MEX_ERROR("ttCreateHandler: Wrong number of input arguments!\nUsage: ttCreateHandler(name, priority, codefcn) \n       ttCreateHandler(name, priority, codefcn, data)");
    return;
  }

  if (mxIsChar(prhs[0]) != 1) {
    TT_MEX_ERROR("ttCreateHandler: name must be a string");
    return;
  }
  if (!mxIsDouble(prhs[1])) {
    TT_MEX_ERROR("ttCreateHandler: priority must be a number");
    return;
  }
  if (mxIsChar(prhs[2]) != 1 || mxGetM(prhs[2]) != 1) {
    TT_MEX_ERROR("ttCreateHandler: codeFcn must be a non-empty string");
    return;
  }
  
  char name[MAXCHARS];
  mxGetString(prhs[0], name, MAXCHARS);
  
  double priority = *mxGetPr(prhs[1]);

  char codeFcn[MAXCHARS];
  mxGetString(prhs[2], codeFcn, MAXCHARS);

  // Make sure that the code function exists in Matlab path
  // and that the code function is syntactically correct.
  mxArray *lhs[1];
  mxArray *rhs[1];
  rhs[0] = mxDuplicateArray(prhs[2]);
  mexCallMATLAB(1, lhs, 1, rhs, "exist");
  int number = (int) *mxGetPr(lhs[0]);
  if (number == 0) {
    char errbuf[MAXERRBUF];
    snprintf(errbuf, MAXERRBUF, "ttCreateHandler: codeFcn '%s' not in path! Interrupt handler '%s' not created!\n", codeFcn, name);
    TT_MEX_ERROR(errbuf);
    return;
  }

  // Create handler
  if (ttCreateHandler(name, priority, NULL)) {

    // Add name of code function (m-file) and data variable
    DataNode *n = (DataNode*) rtsys->handlerList->getLast();
    InterruptHandler* hdl = (InterruptHandler*) n->data;

    hdl->codeFcnMATLAB = new char[strlen(codeFcn)+1];
    strcpy(hdl->codeFcnMATLAB, codeFcn);
     
    if (nrhs == 4) { // data specified
      mxArray* data = mxDuplicateArray(prhs[3]);
      mexMakeArrayPersistent(data);
      hdl->dataMATLAB = data;
    }
  }
}
