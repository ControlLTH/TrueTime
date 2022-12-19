#define S_FUNCTION_NAME node_init

#include "ttkernel.cpp"
#include "matrix.h"
#include "../network.h"

// Code functions
#include "updateExpiryTimer.cpp"
#include "AODVsndcode.cpp"
#include "AODVrcvcode.cpp"
#include "APPLsndcode.cpp"
#include "APPLrcvcode.cpp"
#include "hellocode.cpp"
#include "timercode.cpp"
#include "expcode.cpp"
#include "nwrcvcode.cpp"
#include "ultrarcvcode.cpp"
#include "return_distance.cpp"
#include "printcode.cpp"

// Prios
#define ULTRAHANDLER_PRIO	 1.0
#define NWHANDLER_PRIO		 2.0
#define AODVRCVTASK_PRIO	 3.0
#define TIMERHANDLER_PRIO	 9.0
#define HELLOTASK_PRIO		10.0
#define TIMERTASK_PRIO	 	11.0
#define AODVSNDTASK_PRIO	12.0
#define RETDIST_PRIO		13.0

#define NBROFINPUTS 2
#define NBROFOUTPUTS 0

// NBR_NODES defined in ../network.h

// NBR_NODES instances of these task data structures

struct NodeData {
  
  AODVsnd_Data *dataAODVsnd;  
  AODVrcv_Data *dataAODVrcv;
  Hello_Data *dataHello;
  Exp_Data *dataExp;
  APPLsnd_Data *datasnd;
  APPLrcv_Data *datarcv;
  int *seqNbrs;
  RouteEntry *routing_table;

};

void init() {
  int i, nodeID;
  
  // Initialize TrueTime kernel

   // Read the input argument from the block dialogue
  mxArray *initarg = ttGetInitArg();
  if (!mxIsDoubleScalar(initarg)) {
    TT_MEX_ERROR("The init argument must be a number!");
    return;
  }
  nodeID = (int)mxGetPr(initarg)[0];
  mexPrintf("Initializing Node#%d\n", nodeID);

  // Allocate memory for the node and store pointer in UserData
  NodeData *nodeData = new NodeData;
  ttSetUserData(nodeData);
  nodeData->dataAODVsnd = NULL;
  nodeData->dataAODVrcv= NULL;
  nodeData->dataHello= NULL;
  nodeData->dataExp= NULL;
  nodeData->datasnd= NULL;
  nodeData->datarcv= NULL;
  nodeData->seqNbrs= NULL;
  nodeData->routing_table= NULL;

  ttInitKernel(prioFP);

  ttCreateMailbox("AODVSndBox", 200);  // Data messages to AODV layer
 
  ttCreateMailbox("AODVRcvBox", 200);  // Data messages from AODV layer
  
  nodeData->seqNbrs = new int[NBR_AODV];
  nodeData->routing_table = new RouteEntry[NBR_AODV];
  for (i=0; i<NBR_AODV; i++) {
    nodeData->seqNbrs[i] = 0;             // initialize sequence numbers
    nodeData->routing_table[i].valid = 0; // mark all entries as invalid
  }
  
  // AODV task to send application messages
  // initiates route discovery if necessary
  nodeData->dataAODVsnd = new AODVsnd_Data;
  nodeData->dataAODVsnd->nodeID = nodeID;
  nodeData->dataAODVsnd->RREQID = 0;   
  for (i=0; i<NBR_AODV; i++) {
    nodeData->dataAODVsnd->buffer[i] = new List("MessageList", NULL);  // One message buffer per destination
  }
  nodeData->dataAODVsnd->doEmptyBuffer = 0;
  nodeData->dataAODVsnd->routing_table = nodeData->routing_table;
  nodeData->dataAODVsnd->seqNbrs = nodeData->seqNbrs;
  nodeData->dataAODVsnd->sendTo = 0; 
  nodeData->dataAODVsnd->msg = NULL;
  nodeData->dataAODVsnd->size = 0;
  nodeData->dataAODVsnd->bufferDest = 0;
  ttCreateTask("AODVSendTask", 10.0, AODVsndcode, nodeData->dataAODVsnd);
  ttSetPriority(AODVSNDTASK_PRIO, "AODVSendTask");
  ttCreateJob("AODVSendTask");
  
  // Task to process incoming messages
  // higher prio than AODVSend to allow faster routes to be
  // obtained while sending buffered data
  nodeData->dataAODVrcv = new AODVrcv_Data;
  nodeData->dataAODVrcv->nodeID = nodeID;
  nodeData->dataAODVrcv->requestObtained = 0;
  for (i=0; i<NBR_AODV; i++) {
    nodeData->dataAODVrcv->cache[i] = 0; // No cached RREQs
  }
  nodeData->dataAODVrcv->RERRlist = new List("RERRList", NULL); // list of RERRs to propagate
  nodeData->dataAODVrcv->routing_table = nodeData->routing_table;
  nodeData->dataAODVrcv->seqNbrs = nodeData->seqNbrs;
  ttCreateTask("AODVRcvTask", 10,  AODVrcvcode, nodeData->dataAODVrcv);
  ttSetPriority(AODVRCVTASK_PRIO, "AODVRcvTask");
  
  // Periodic task to send HELLO messages and
  // discover/handle lost connections
  nodeData->dataHello = new Hello_Data;
  nodeData->dataHello->nodeID = nodeID;
  nodeData->dataHello->lastRREQ = -100.0;
  nodeData->dataHello->routing_table = nodeData->routing_table;
  nodeData->dataHello->seqNbrs = nodeData->seqNbrs;
  for (i=0; i<NBR_AODV; i++) {
    nodeData->dataHello->nbors[i] = 0; // no neighbors initially
    nodeData->dataHello->lastHello[i] = -10.0; 
  }
  nodeData->dataHello->RERRlist = new List("RERRList", NULL);
  nodeData->dataAODVrcv->dataHello = nodeData->dataHello;  // set pointer to HelloTask data structure
  double offset = 0.1*((double)rand()/(double)RAND_MAX); // to avoid collisions between HELLO msgs
  ttCreatePeriodicTask("HelloTask", offset, HELLO_INTERVAL, hellocode, nodeData->dataHello);
  ttSetPriority(HELLOTASK_PRIO, "HelloTask");

  // Timer handler
  ttCreateHandler("timer_handler", TIMERHANDLER_PRIO, timercode);
  // Task to handle expiry of routing table entries
  nodeData->dataExp = new Exp_Data;
  nodeData->dataExp->nodeID = nodeID;
  nodeData->dataExp->expTimer = 0; // flag to indicate timer set
  nodeData->dataExp->routing_table = nodeData->routing_table;
  nodeData->dataAODVsnd->dataTimer = nodeData->dataExp;  // set pointer to TimerTask data structure
 nodeData-> dataAODVrcv->dataTimer = nodeData->dataExp;  // set pointer to TimerTask data structure
  ttCreateTask("TimerTask", 10, expcode, nodeData->dataExp);
  ttSetPriority(TIMERTASK_PRIO, "TimerTask");

  // Initialize network
  ttCreateHandler("nw_handler", NWHANDLER_PRIO, nwrcvcode, nodeData->dataAODVrcv);
  ttAttachNetworkHandler(ZIGBEENETW, "nw_handler"); 

  // Initialize ultrasound network
  ttCreateHandler("ultra_handler", ULTRAHANDLER_PRIO, ultrarcvcode, nodeData->dataAODVrcv);
  ttAttachNetworkHandler(ULTRANETW, "ultra_handler");

  // Return distance measurement task
  ttCreateTask("ReturnDistance", 10, return_distance, nodeData->dataAODVrcv);
  ttSetPriority(RETDIST_PRIO, "ReturnDistance");
  
  // Print routing table handler
  ttCreateHandler("print_handler",14,printcode,nodeData->dataAODVrcv);
  ttAttachTriggerHandler(1,"print_handler",0.1);
  
  // Application tasks
  // -----------------
  // Node 6 periodically sends data to node 1
  // Jobs of RcvTask in node 1 created from AODV layer
  if (nodeID == 6) {
    mexPrintf("Node #%d send task initialized\n",nodeID);
    nodeData->datasnd = new APPLsnd_Data;
    nodeData->datasnd->nodeID = nodeID;
    nodeData->datasnd->sent = 0;
    ttCreatePeriodicTask("SendTask", 0.5, 2, APPLsndcode, nodeData->datasnd);
    ttSetPriority(10, "SendTask");
  }
	
  if (nodeID == 1) {
    mexPrintf("Node 1:s receive task initialized\n");
    nodeData->datarcv = new APPLrcv_Data;
    nodeData->datarcv->nodeID = nodeID;
    nodeData->datarcv->received = 0;
    ttCreateTask("RcvTask", 1, APPLrcvcode, nodeData->datarcv);
    ttSetPriority(10.0, "RcvTask");
  }
  

}

void cleanup() {
  int i, nodeID;
  
  // Read the input argument from the block dialogue
  mxArray *initarg = ttGetInitArg();
  nodeID = (int)mxGetPr(initarg)[0];
  mexPrintf("Cleaning Node#%d\n", nodeID);

  NodeData *nodeData = (NodeData *)ttGetUserData();
  
  for (i=0; i<NBR_AODV; i++) {
    delete nodeData->dataAODVsnd->buffer[i];
  }
  delete nodeData->dataAODVsnd;
  delete nodeData->dataAODVrcv->RERRlist;
  delete nodeData->dataAODVrcv;
  delete nodeData->dataHello->RERRlist;
  delete nodeData->dataHello;
  delete nodeData->dataExp;
  delete[] nodeData->seqNbrs;
  delete[] nodeData->routing_table;
  delete nodeData->datasnd;
}
