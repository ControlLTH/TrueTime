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

#include "truetime.h"
#include "tinymt32.h"

// Note: when adding a new network protocol, also add the correct code
// to discardunsent.cpp
enum { CSMACD, CSMAAMP, RR, FDMA, TDMA, SFDSE, FLEXRAY, PROFINET, NCM, _802_11, _802_15_4, NCM_WIRELESS};

enum { COMMONBUF, OUTPUTBUF };

enum { BUFFULLRETRY, BUFFULLDROP };

enum { FIFO, PRIORITYLOW, PRIORITYHIGH};

enum {SYNC, IRT, NRT};

#define COLLISION_WINDOW 1.0e-6  // 1 us
#define ETHERNET_MINFRAMESIZE 64
#define ADRESSBITS 114
// 802.11
enum { W_IDLE, W_SENDING, W_WAITING };
enum { UNICAST, BROADCAST };
#define SLOTTIME_802_11 0.00002 // 20 us
//#define SIFS_802_11 0.00001 // 10 us
//#define PIFS_802_11 0.00003 // 30 us
#define DIFS_802_11 0.00005 // 50 us
#define CWMIN_802_11 5
#define CWMAX_802_11 1023
#define BACKGROUND_NOISE 8.9e-14


class RTnetwork; // forward declaration


int msgHighPrioComp(Node* n1, Node* n2);
int msgLowPrioComp(Node* n1, Node* n2);

// pathloss function definition
typedef double (*pathlossfun_t)(RTnetwork *, double, int, double, double, int, double, double);

class NWmsg : public Node {
public:
  int sender;
  int receiver;
  void *data;       // message content
  mxArray* dataMATLAB;
  int length;       // in bits - determines the transmission time
  int allocMem;     // (PROFINET), number of bits allocated in switch
  int cutthrough;   // (PROFINET), flag indicating if the message has cut-through
  int msgID;        // Message identification number
  double prio;
  double timestamp;    // time when message was enqueued in network
  double waituntil;    // when wait in pre/postprocQ has finished
  double remaining;    // remaining nbr of bits to be transmitted
  double collided;     // (802.11), 0 if not collided else 1
  double maximum_disturbance; // (802.11) also includes the own sending power
  int type;            // (802.11), set to BROADCAST for broadcast messages
  double signalPower;  // (802.11) what was the received signal power
  int nbrOfBackoffs; // (802.15.4)
  void print();
  NWmsg();  // constructor
  ~NWmsg(); // destructor

};

NWmsg::NWmsg() {
  debugPrintf("NWmsg constructor\n");
  type = UNICAST;
  nbrOfBackoffs = -1; // only used in (802.15.4)
  cutthrough = 0;     // only used in PROFINET
  data = NULL;
  dataMATLAB = NULL;
  debugPrintf("NWmsg constructor done\n");
}

NWmsg::~NWmsg() {
  debugPrintf("NWmsg destructor\n");
  if (dataMATLAB != NULL) {
    mxDestroyArray(dataMATLAB);
    dataMATLAB = NULL;
  }
  debugPrintf("NWmsg destructor done\n"); 
}


void NWmsg::print() {
  mexPrintf("msg: sender=%d, receiver=%d, length=%d ", sender, receiver, length);
}


class NWnode {
 public:
  List *preprocQ;
  List *inputQ;
  List *postprocQ;
  List *outputQ;
  List *switchQ;
  
  List *statpreprocQ; // For the static segment in FlexRay. The dynamic segment uses preprocQ and inputQ.
  List *statinputQ; // For the static segment in FlexRay

  List *port[4]; // PROFINET
  NWmsg *lastMsg;
 
  int state;
  int nbrcollisions;
  double waituntil;
  int swstate;   // state of switch output
  int switchmem; // output buffer memory
  int switchMemLimit; // maximum available memory in switch
  double lastused; // (802.11) time when the signal level was last over the threshold
  double *signallevels; // (802.11) array of nodes signallevels in this node
  double backoff;       // (802.11)
  double lastbackoffcount; // (802.11)
  double xCoordinate;      // (802.11)
  double yCoordinate;      // (802.11)
  double total_received_power;   // (802.11) SNR stuff 

  // To be able to set some parameters individually
  double transmitPower;     //802.11
  double predelay;     /* preprocessing delay (s) */
  double postdelay;    /* postprocessing delay (s) */

  NWnode(); // constructor
  NWnode(int (*compFcn)(Node *, Node *)); // constructor
  ~NWnode(); // destructor

};

NWnode::NWnode() {
  debugPrintf("NWnode constructor\n");

  preprocQ = new List("preprocQ",NULL);
  inputQ = new List("inputQ",NULL);
  outputQ = new List("outputQ",NULL);
  postprocQ = new List("postprocQ",NULL);
  switchQ = new List("switchQ",NULL);

  statpreprocQ = new List("statpreprocQ",NULL); // FlexRay
  statinputQ = new List("statinputQ",NULL); // FlexRay

  for (int i=0; i<4; i++) {  // PROFINET
    port[i] = new List("port", NULL);
  }
  lastMsg = new NWmsg();

  state = 0; // idle
  nbrcollisions = 0;
  waituntil = 0.0;
  swstate = 0; // idle

  lastused = 0;         // (802.11)
  signallevels = NULL;  // (802.11)
  backoff = 0;          // (802.11)
  lastbackoffcount = 0; // (802.11)
  xCoordinate = 0;      // (802.11)
  yCoordinate = 0;      // (802.11)
  total_received_power = BACKGROUND_NOISE; // (802.11)

  predelay = 0;
  postdelay = 0;

  debugPrintf("NWnode constructor done\n");

}

NWnode::NWnode(int (*compFcn)(Node *, Node *)) {
  debugPrintf("NWnode constructor 2\n");

  preprocQ = new List("preprocQ",NULL);
  inputQ = new List("inputQ", compFcn);
  outputQ = new List("outputQ",NULL);
  postprocQ = new List("postprocQ",NULL);
  switchQ = new List("switchQ",compFcn);

  statpreprocQ = new List("statpreprocQ",NULL); // FlexRay
  statinputQ = new List("statinputQ",compFcn); // FlexRay

  for (int i=0; i<4; i++) {  // PROFINET
    port[i] = new List("port", NULL);
  }
  lastMsg = new NWmsg();

  state = 0; // idle
  nbrcollisions = 0;
  waituntil = 0.0;
  swstate = 0; // idle

  lastused = 0;         // (802.11)
  signallevels = NULL;  // (802.11)
  backoff = 0;          // (802.11)
  lastbackoffcount = 0; // (802.11)
  xCoordinate = 0;      // (802.11)
  yCoordinate = 0;      // (802.11)
  total_received_power = BACKGROUND_NOISE; // (802.11)

  predelay = 0;
  postdelay = 0;
}

NWnode::~NWnode() {
  
  debugPrintf("NWnode destructor\n");

  if (preprocQ) {
    delete preprocQ;
    preprocQ = NULL;
  }
  if (inputQ) {
    delete inputQ;
    inputQ = NULL;
  }
  if (outputQ) {
    delete outputQ;
    outputQ = NULL;
  }
  if (postprocQ) {
    delete postprocQ;
    postprocQ = NULL;
  }
  if (switchQ) {
    delete switchQ;
    switchQ = NULL;
  }
  if (statpreprocQ) {
    delete statpreprocQ;
    statpreprocQ = NULL;
  }
  if (statinputQ) {
    delete statinputQ;
    statinputQ = NULL;
  }
  for (int i=0; i<4; i++) {  // PROFINET
    if (port[i]) {
      delete port[i];
      port[i] = NULL;
    }
  }
  if (lastMsg) {
    delete lastMsg;
    lastMsg = NULL;
  }
}


class RTnetwork {
public:
  int type;
  int networkNbr;
  int nbrOfNodes;
  double datarate;     /* bits/s */
  int minsize;         /* minimum frame size (bits) */
  double lossprob;     /* probability of a packet getting lost */
  double *bandwidths;  /* FDMA only: vector of bandwidth factors */
  double slottime;     /* TDMA & FlexRay: size of a slot in seconds */
  int schedsize;       /* TDMA & FlexRay: the length of the cyclic schedule */
  int *schedule;       /* TDMA & FlexRay: vector of senders */
  int memsize;         /* SFDSE only: switch memory size */
  int buftype;         /* SFDSE only: switch buffer type */
  int overflow;        /* SFDSE only: switch overflow behavior */

  int nextNode;

  int dynSchedSize;    /* FlexRay only: length of dynamic schedule */
  int *dynSchedule;    /* FlexRay only: vector of dynamic-segment senders */
  int miniSlotSize;    /* FlexRay only: size of mini slot [bits] */
  int msgMiniSlots;    /* FlexRay only: length of message [mini slots] */
  int miniSlotCount;   /* FlexRay only: count the number of used miniSlots */
  double networkIdleUntil; /* FlexRay only: time when the network idle time is over */
  double NIT;          /* FlexRay only: Network Idle Time  */
  
  int QSortMode;       /* Defines in what way the inputQs should be sorted */

  //PROFINET
  int interval;
  double syncTime;     /* Length of synchronization segment [s] */
  double irtLength;    /* Length of IRT Class 3 segment [s] */
  double nrtLength;    /* Length of RT Class 1 / NRT segment [s] */
  // IRT-segment
  int *irtMsgID;
  int *irtFrom;
  int *irtTo;
  double *irtStartTime;
  double *irtSendTime;
  double *irtTransTime; 
  int irtSchedLength;
  // Routing and port information
  int **portDestination;
  int **nodeRouting;

  NWnode **nwnodes;    /* vector of network nodes */

  double time;      // Current time in simulation
  double prevHit;   // Previous invocation of kernel
  double nextHit;   // Next invocation of kernel

  double *inputs;
  double *oldinputs;
  double *outputs;      // trigger outputs
  double *sendschedule; // schedule outputs
  double *energyconsumption; // energy output to the battery

  int sending;          // nbr of the sending node, -1 if none (not FDMA)
  double pathloss;          //802.11
  double receiverThreshold; //802.11
  double acktimeout;        //802.11
  double retrylimit;        //802.11
  double error_threshold;   //802.11
  pathlossfun_t pathlossfun;  // wireless only
  char codeName[MAXCHARS];

  double waituntil;     // the network is idle until this time (CSMA)
  double lasttime;      // last time a frame started to be sent (CSMA)

  int rrturn;           // Round Robin turn (0 - nbrOfNodes-1) (RR)

  double currslottime;  // time current slot started (TDMA)
  int slotcount;        // current slot in the schedule (TDMA)

  int switchmem;        // global switch memory

  double reach;         // reach radius (Ultrasound network only)
  double pinglength;    // ping duration (Ultrasound network only)
  double speedofsound;  // speed of sound (Ultrasound network only)

  mxArray *nbrOfTransmissions;        // For logging
  mxArray *nbrOfSuccessfulReceptions; // For logging

  tinymt32_t randstate; // random number generator state
   
  RTnetwork(); // constructor
  ~RTnetwork(); // destructor

};

RTnetwork::RTnetwork() {
  
  debugPrintf("RTnetwork constructor\n");
  
  bandwidths = NULL;
  schedule = NULL;
  dynSchedule = NULL; // Dynamic Schedule (FlexRay)
  
  // Profinet
  irtMsgID = NULL;
  irtFrom = NULL;
  irtTo = NULL;
  irtStartTime = NULL;
  irtSendTime = NULL;
  irtTransTime = NULL;
  irtSchedLength = 0;  
  
  portDestination = NULL;
  nodeRouting = NULL;

  nwnodes = NULL;
    
  inputs = NULL;
  oldinputs = NULL;
  outputs = NULL;
  sendschedule = NULL;
  energyconsumption = NULL;
  
  nbrOfTransmissions = NULL;
  nbrOfSuccessfulReceptions = NULL;
  
  pathlossfun = NULL;

  // set TinyMT random number generator paramters
  randstate.mat1 = 0x8f7011ee;
  randstate.mat2 = 0xfc78ff1f;
  randstate.tmat = 0x3793fdff;

  debugPrintf("RTnetwork constructor done\n");
}

RTnetwork::~RTnetwork() {
  
  debugPrintf("RTnetwork destructor\n");
    
  if (inputs) {
    delete[] inputs;
    inputs = NULL;
  }

  if (sendschedule) {
    delete[] sendschedule;
    sendschedule = NULL;
  }

  if (energyconsumption) {
    delete[] energyconsumption;
    energyconsumption = NULL;
  }

  if (outputs) {
    delete[] outputs;
    outputs = NULL;
  }

  if (oldinputs) {
    delete[] oldinputs;
    oldinputs = NULL;
  }

  if (schedule) {
    delete[] schedule;
    schedule = NULL;
  }

  if (dynSchedule) {
    delete[] dynSchedule;
    dynSchedule = NULL;
  }

  if (bandwidths) {
    delete[] bandwidths;
    bandwidths = NULL;
  }
  //PROFINET
  if (irtMsgID) {
    delete[] irtMsgID;
    irtMsgID = NULL;
  }

  if (irtFrom) {
    delete[] irtFrom;
    irtFrom = NULL;
  }  

  if (irtTo) {
    delete[] irtTo;
    irtTo = NULL;
  }

  if (irtStartTime) {
    delete[] irtStartTime;
    irtStartTime = NULL;
  }

  if (irtTransTime) {
    delete[] irtTransTime;
    irtTransTime = NULL;
  }

  if (irtSendTime) {
    delete[] irtSendTime;
    irtSendTime = NULL;
  }
  
  if (nwnodes) {
    for (int i=0; i < nbrOfNodes; i++) {
      debugPrintf("Deleting nwnode %d\n", i);
      if (nwnodes[i]) {
        delete nwnodes[i];
        nwnodes[i] = NULL;
      }
    }
    delete[] nwnodes;
    nwnodes = NULL;
  }
  
  if (nodeRouting) {
    for (int i=0; i < nbrOfNodes; i++) {
      if (nodeRouting[i]) {
        delete[] nodeRouting[i];
        nodeRouting[i] = NULL;
      }
    }
    delete[] nodeRouting;
    nodeRouting = NULL;
  }
 
  if (portDestination) {
    for (int i=0; i < nbrOfNodes; i++) {
      if (portDestination[i]) {
        delete[] portDestination[i];
        portDestination[i] = NULL;
      }
    }
    delete[] portDestination;
    portDestination = NULL;
  }

  debugPrintf("RTnetwork destructor done\n");
}


/** 
 * random number generator functions
 */

#define unirand(X) tinymt32_generate_32double(& X ->randstate)

/* returns uniform integer number a <= x <= b */ 
static inline int urandint(RTnetwork *nwsys, int a, int b) {
  return a + (int)((1+b-a) * unirand(nwsys));
}
