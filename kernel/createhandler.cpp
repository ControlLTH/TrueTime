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

#ifndef CREATE_HANDLER
#define CREATE_HANDLER

#include "getnode.cpp"

bool ttCreateHandler(const char *name, double priority, double (*codeFcn)(int, void*)) {
  
  InterruptHandler* hdl;
  
  if (strcmp(name,"") == 0) {
    TT_MEX_ERROR("ttCreateInterruptHandler: Name should be a non-empty string!");
    return false;
  }
  if (rtsys->prioFcn == NULL) {
    TT_MEX_ERROR("ttCreateInterruptHandler: Kernel must be initialized before creation of handlers!");
    return false;
  }
  DataNode* dn =getNode(name, rtsys->handlerList);
  if (dn != NULL) {
    TT_MEX_ERROR("ttCreateInterruptHandler: Name of handler not unique!");
    return false;
  }
  
  hdl = new InterruptHandler(name);
  hdl->codeFcn = codeFcn;

  hdl->priority = priority;

  hdl->display = true;
  rtsys->nbrOfSchedTasks++;

  rtsys->handlerList->appendNode(new DataNode(hdl, hdl->name));


  return true;
}

void ttCreateHandler(const char *name, double priority, double (*codeFcn)(int, void*), void* data) {

  DataNode* n;

  if (ttCreateHandler(name, priority, codeFcn)) {
    n = (DataNode*) rtsys->handlerList->getLast();
    ((Task*) n->data)->data = data;
  } 
}


void ttCreateInterruptHandler(const char *name, double priority, double (*codeFcn)(int, void*), void* data) {

  mexPrintf("Warning: Use of deprecated function ttCreateInterruptHandler. Use ttCreateHandler instead.\n");
  ttCreateHandler(name, priority, codeFcn, data);

}



void ttCreateInterruptHandler(const char *name, double priority, double (*codeFcn)(int, void*)) { 

  mexPrintf("Warning: Use of deprecated function ttCreateInterruptHandler. Use ttCreateHandler instead.\n");
  ttCreateHandler(name, priority, codeFcn);

}




#endif
