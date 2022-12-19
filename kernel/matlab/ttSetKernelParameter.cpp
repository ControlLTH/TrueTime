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



#include "../setkernelparameter.cpp"
#include "getrtsys.cpp"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  double value;
  char parametername[MAXCHARS];

  rtsys = getrtsys(); // Get pointer to rtsys 

  if (rtsys==NULL) {
    return;
  }
  // Check number and type of arguments. 
  if (nrhs < 1 || nrhs > 2) {
    TT_MEX_ERROR("ttSetKernelParameter: Wrong number of input arguments!\nUsage: ttSetKernelParameter(parameter, value) or ttSetKernelParameter(switch)");
    return;
  }
  if (mxIsChar(prhs[0]) != 1) {
    TT_MEX_ERROR("ttSetKernelParameter: parameter name must be a string");
    return;
  }
  mxGetString(prhs[0], parametername, MAXCHARS);

  if (nrhs == 2) {
    if (!mxIsDouble(prhs[1])){
      TT_MEX_ERROR("ttSetKernelParameter: value must be a double");
      return;
    }
    value = (double)*mxGetPr(prhs[1]);
  }  else {
    value = 0.0;
  }

  //mexPrintf("%s, %f\n", parametername, value);
  ttSetKernelParameter(parametername, value);
}
