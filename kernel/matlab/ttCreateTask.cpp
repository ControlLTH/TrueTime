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



#include "../createtask.cpp"
#include "getrtsys.cpp"
#include "../checkinputargs.cpp"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  rtsys = getrtsys() ; // Get pointer to rtsys 

  if (rtsys==NULL) {
    return;
  }

  /* Check and parse input arguments */

  char name[MAXCHARS];
  double starttime;
  double deadline;
  char codeFcn[MAXCHARS];
  const mxArray *data;

  if (checkinputargs(nrhs,prhs,TT_STRING,TT_SCALAR,TT_STRING)) {

    mxGetString(prhs[0], name, MAXCHARS);
    deadline = *mxGetPr(prhs[1]);
    mxGetString(prhs[2], codeFcn, MAXCHARS);
    starttime = -1.0;
    data = NULL;

  } else if (checkinputargs(nrhs,prhs,TT_STRING,TT_SCALAR,TT_STRING,TT_STRUCT)) {
    
    mxGetString(prhs[0], name, MAXCHARS);
    deadline = *mxGetPr(prhs[1]);
    mxGetString(prhs[2], codeFcn, MAXCHARS);
    data = prhs[3];
    starttime = -1.0;

  } else if (checkinputargs(nrhs,prhs,TT_STRING,TT_SCALAR,TT_SCALAR,TT_STRING)) {

    mxGetString(prhs[0], name, MAXCHARS);
    starttime = *mxGetPr(prhs[1]);
    deadline = *mxGetPr(prhs[2]);
    mxGetString(prhs[3], codeFcn, MAXCHARS);
    data = NULL;

  } else if (checkinputargs(nrhs,prhs,TT_STRING,TT_SCALAR,TT_SCALAR,TT_STRING,TT_STRUCT)) {

    mxGetString(prhs[0], name, MAXCHARS);
    starttime = *mxGetPr(prhs[1]);
    deadline = *mxGetPr(prhs[2]);
    mxGetString(prhs[3], codeFcn, MAXCHARS);
    data = prhs[4];

  } else {
    
    TT_MEX_ERROR("ttCreateTask: Wrong input arguments!\n"
		 "Usage: ttCreateTask(name, deadline, codeFcn)\n"
                 "       ttCreateTask(name, deadline, codeFcn, data)\n"
		 "       ttCreateTask(name, starttime, deadline, codeFcn)\n"
		 "       ttCreateTask(name, starttime, deadline, codeFcn, data)"); 
    return;
  }

  /* Check that the code function exists */

  mxArray *lhs[1], *rhs[1];
  rhs[0] = mxCreateString(codeFcn);
  mexCallMATLAB(1, lhs, 1, rhs, "exist");
  int number = (int) *mxGetPr(lhs[0]);
  if (number != 2) {
    char buf[MAXERRBUF];
    sprintf(buf, "ttCreateTask: codeFcn '%s.m' not in path! Task '%s' not created!", codeFcn, name);
    TT_MEX_ERROR(buf);
    return;
  }

  ttCreateTaskMATLAB(name, starttime, -1.0, deadline, codeFcn, data);
}

