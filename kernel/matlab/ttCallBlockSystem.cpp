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



#include "../callblocksystem.cpp"
#include "getrtsys.cpp"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  rtsys = getrtsys() ; // Get pointer to rtsys 

  if (rtsys==NULL) {
    return;
  }

  // Check number and type of arguments. 
  if (nrhs != 3) {
    TT_MEX_ERROR("ttCallBlockSystem: Wrong number of input arguments!\nUsage: ttCallBlockSystem(nbrOutp, inpVec, blockname)");
    return;
  }

  if (!mxIsDoubleScalar(prhs[0])) {
    TT_MEX_ERROR("ttCallBlockSystem: nbrOutp must be a number");
    return;
  }
  if (!mxIsDouble(prhs[1])) {
    TT_MEX_ERROR("ttCallBlockSystem: inpVec must contain numbers");
    return;
  }
  if (mxIsChar(prhs[2]) != 1 || mxGetM(prhs[2]) != 1) {
    TT_MEX_ERROR("ttCallBlockSystem: blockname must be a non-empty string");
    return;
  }

  int nbrOutp = (int) *mxGetPr(prhs[0]);
  double* outpVec = new double[nbrOutp];

  int nbrInp = mxGetNumberOfElements(prhs[1]);
  double* inpVec = mxGetPr(prhs[1]);
  
  char blockname[MAXCHARS];
  mxGetString(prhs[2], blockname, MAXCHARS);

  ttCallBlockSystem(nbrOutp, outpVec, nbrInp, inpVec, blockname);

  plhs[0] = mxCreateDoubleMatrix(1, nbrOutp, mxREAL);
  // Copy values (will be zeros if call failed)
  for (int i=0; i<nbrOutp; i++) {
    mxGetPr(plhs[0])[i] = outpVec[i];
  }
  
  delete[] outpVec;

}

