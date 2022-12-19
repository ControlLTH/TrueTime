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

#ifndef SET_CBS_PARAMETERS
#define SET_CBS_PARAMETERS

#include "getnode.cpp"


void ttSetCBSParameters(const char *name, double Qs, double Ts) {

  DataNode *dn;

  dn = (DataNode*) getNode(name, rtsys->cbsList);
  if (dn == NULL) {
    char buf[MAXERRBUF];
    sprintf(buf, "ttSetCBSParameters: Non-existent task '%s'", name);
    TT_MEX_ERROR(buf);
    return;
  }

  CBS* cbs = (CBS*) dn->data;

  cbs->Qs = Qs;
  cbs->Ts = Ts;

}

#endif
