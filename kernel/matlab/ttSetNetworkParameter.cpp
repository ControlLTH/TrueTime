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



#include "../setnetworkparameter.cpp"
#include "getrtsys.cpp"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  int network;
  double value;
  char parametername[MAXCHARS];

  rtsys = getrtsys(); // Get pointer to rtsys 

  if (rtsys==NULL) {
    return;
  }
  // Check number and type of arguments. 
  if (nrhs < 2 || nrhs > 3) {
    TT_MEX_ERROR("ttSetNetworkParameter: Wrong number of input arguments!\nUsage: ttSetNetworkParameter(parameter, value) or\n       ttSetNetworkParameter(network, parameter, value)");
    return;
  }
  if (mxIsChar(prhs[nrhs-2]) != 1) {
    TT_MEX_ERROR("ttSetNetworkParameter: parameter name must be a string");
    return;
  }

  mxGetString(prhs[nrhs-2], parametername, MAXCHARS);
  if (nrhs == 2){ // no network specified
    if (!mxIsDouble(prhs[1])){
      TT_MEX_ERROR("ttSetNetworkParameter: value must be a double");
      return;
    }
    network = 1;
    value = (double)*mxGetPr(prhs[1]);
  } else { // the network is specified
    if (!mxIsDouble(prhs[0]) || !mxIsDouble(prhs[2])){
      TT_MEX_ERROR("ttSetNetworkParameter: network must be a double\n                       value must be a double");
      return;
    }
    network = (int)*mxGetPr(prhs[0]);
    value = (double)*mxGetPr(prhs[2]);
  }
  //mexPrintf("%d, %s, %f\n", network, parametername, value);
  ttSetNetworkParameter(network, parametername, value);
  //mexPrintf("%s:%d\n", __FILE__,__LINE__);
}
