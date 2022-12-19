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

#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

class NetworkInterface {
 public:
  Task *handler;
  int networkNbr; // global network identifier
  int nodeNbr;    // node identifier in the specific network
  int portNbr;    // block port number (used for trigger signals)
  RTnetwork *nwsys;
  double transmitpower; // used to buffer the value in ttSetNetworkParameter(
                        // until the network is set up properly
  double predelay;      // Same as above
  double postdelay;      // Same as above

  NetworkInterface();
};

NetworkInterface::NetworkInterface() {

  handler = NULL;
  nwsys = 0; // So that we later on can be sure if initnetwork2 
  //has been run or not
  transmitpower = -1.0;  // temporary variable, set in mdlInitializeConditions
  predelay = -1.0;       // temporary variable, set in mdlInitializeConditions
  postdelay = -1.0;      // temporary variable, set in mdlInitializeConditions
}

#endif
