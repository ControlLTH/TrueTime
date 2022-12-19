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



#include "../getmsg.cpp"
#include "getrtsys.cpp"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  mxArray* data;
  int network;

  rtsys = getrtsys(); // Get pointer to rtsys 

  if (rtsys==NULL) {
    return;
  }

  // Check number and type of arguments. 
  if (nrhs > 1) {
    TT_MEX_ERROR("ttGetMsg: Wrong number of input arguments!\nUsage: ttGetMsg or\n       ttGetMsg(network)");
    return;
  }
  
  if (nrhs == 1) {
    if (!mxIsDoubleScalar(prhs[0])) {
      TT_MEX_ERROR("ttGetMsg: network must be an integer scalar");
      return;
    }
    network = (int) *mxGetPr(prhs[0]);
  } else {
    if (rtsys->nbrOfNetworks == 0) {
      TT_MEX_ERROR("ttGetMsg: The node is not connected to to any network!");
      return;
    }
    if (rtsys->nbrOfNetworks > 1) {
      TT_MEX_ERROR("ttGetMsg: The node is connected to more than one network - specify which!\n");
      return;
    }
    network = rtsys->networkInterfaces[0].networkNbr;  // find first (and only) network number
  }

  double signalPower;
  data = ttGetMsgMATLAB(network, &signalPower);

  if (data == NULL) {
    data = mxCreateDoubleMatrix(0,0,mxREAL); // Return empty matrix
  }

  if (nlhs >= 1) {
    plhs[0] = data;
    if (nlhs >= 2){
      plhs[1] = mxCreateDoubleScalar(signalPower);
    }
  }
}
