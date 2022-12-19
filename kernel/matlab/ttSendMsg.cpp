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



#include "../sendmsg.cpp"
#include "getrtsys.cpp"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  int network, receiver, length;
  mxArray *dataMATLAB;
  double priority;

  rtsys = getrtsys(); // Get pointer to rtsys 

  if (rtsys==NULL) {
    return;
  }

  // Check number and type of arguments. 
  if (nrhs < 3 || nrhs > 4) {
    TT_MEX_ERROR("ttSendMsg: Wrong number of input arguments!\nUsage: ttSendMsg(receiver, data, length) or\n       ttSendMsg(receiver, data, length, priority) or\n       ttSendMsg([network receiver], data, length) or\n       ttSendMsg([network receiver], data, length, priority)");
    return;
  }

  if (mxIsDoubleScalar(prhs[0])) { // no network specified
    
    if (rtsys->nbrOfNetworks == 0) {
      TT_MEX_ERROR("ttSendMsg: The node is not connected to to any network!");
      return;
    }
    if (rtsys->nbrOfNetworks > 1) {
      TT_MEX_ERROR("ttAttachNetworkHandler: The node is connected to more than one network - specify which!\n");
      return;
    }
    network = rtsys->networkInterfaces[0].networkNbr;  // find first (and only) network number
    receiver = (int)*mxGetPr(prhs[0]);
  } else if (mxIsDouble(prhs[0]) && !mxIsComplex(prhs[0]) && mxGetM(prhs[0])==1 && mxGetN(prhs[0])==2) {
    network = (int)(mxGetPr(prhs[0])[0]);     // first element of vector
    receiver = (int)(mxGetPr(prhs[0])[1]);   // second element of vector
  } else {
    TT_MEX_ERROR("ttSendMsg: receiver must be a number or a vector [network receiver]");
    return;
  }

  dataMATLAB = mxDuplicateArray(prhs[1]);
  mexMakeArrayPersistent(dataMATLAB);

  if (!mxIsDoubleScalar(prhs[2])) {
    TT_MEX_ERROR("ttSendMsg: length must be a number");
    return;
  }
  length = (int)*mxGetPr(prhs[2]);
  if (nrhs == 4) {
    if(!mxIsDoubleScalar(prhs[3])) {
      TT_MEX_ERROR("ttSendMsg: priority must be a number");
      return;
    }
    priority = *mxGetPr(prhs[3]);
    ttSendMsg(network, receiver, length, NULL, dataMATLAB, priority);
  } else {
    ttSendMsg(network, receiver, length, NULL, dataMATLAB);
  }
}
