#define S_FUNCTION_NAME tmote_init

#include "ttkernel.cpp"
#include "matrix.h"
#include "../network.h"
#include "defines.h"

// Code functions
#include "AODV/updateExpiryTimer.cpp"
#include "AODV/AODVsndcode.cpp"
#include "AODV/AODVrcvcode.cpp"
#include "AODV/APPLsndcode.cpp"
#include "AODV/APPLrcvcode.cpp"
#include "AODV/hellocode.cpp"
#include "AODV/timercode.cpp"
#include "AODV/expcode.cpp"
#include "AODV/nwrcvcode.cpp"
#include "tmote.cpp"
#include "printcode.cpp"
// The tmote functions

// Prios
#define NWHANDLER_PRIO		 2
#define AODVRCVTASK_PRIO	 3
#define TIMERHANDLER_PRIO	 9
#define HELLOTASK_PRIO		10
#define TIMERTASK_PRIO	 	11
#define AODVSNDTASK_PRIO	12


struct TMoteData {

  AODVsnd_Data *dataAODVsnd;  
  AODVrcv_Data *dataAODVrcv;	// Allso tmote data
  Hello_Data *dataHello;
  Exp_Data *dataExp;
  APPLsnd_Data *datasnd;
  APPLrcv_Data *datarcv;
  int *seqNbrs;
  RouteEntry *routing_table;
};


void init() {
  int i, nodeID;

  // Read the input argument from the block dialogue
  mxArray *initarg = ttGetInitArg();
  if (!mxIsDoubleScalar(initarg)) {
    TT_MEX_ERROR("The init argument must be a number!");
    return;
  }
  nodeID = (int)mxGetPr(initarg)[0];
  mexPrintf("Initializing Bot#%d\n", nodeID);

  // Allocate memory for the node and store pointer in UserData
  TMoteData *tMoteData = new TMoteData;
  ttSetUserData(tMoteData);
	
  // Initialize TrueTime kernel
	
  ttInitKernel(prioFP);
	
  ttCreateMailbox("AODVSndBox", 200);  // Data messages to AODV layer
  ttCreateMailbox("AODVRcvBox", 200);  // Data messages from AODV layer
  
  tMoteData->seqNbrs = new int[NBR_AODV];
  tMoteData->routing_table = new RouteEntry[NBR_AODV];
  for (i=0; i<NBR_AODV; i++) {
    tMoteData->seqNbrs[i] = 0;             // initialize sequence numbers
    tMoteData->routing_table[i].valid = 0; // mark all entries as invalid
  }
	
  // AODV task to send application messages
  // initiates route discovery if necessary
  tMoteData->dataAODVsnd = new AODVsnd_Data;
  tMoteData->dataAODVsnd->nodeID = nodeID + NBR_NODES;
  tMoteData->dataAODVsnd->RREQID = 0;   
  for (i=0; i<NBR_AODV; i++) {
    tMoteData->dataAODVsnd->buffer[i] = new List("MessageList", NULL);  // One message buffer per destination
  }
  tMoteData->dataAODVsnd->doEmptyBuffer = 0;
  tMoteData->dataAODVsnd->routing_table = tMoteData->routing_table;
  tMoteData->dataAODVsnd->seqNbrs = tMoteData->seqNbrs;
  tMoteData->dataAODVsnd->sendTo = 0; 
  tMoteData->dataAODVsnd->msg = NULL;
  tMoteData->dataAODVsnd->size = 0;
  tMoteData->dataAODVsnd->bufferDest = 0;
  ttCreateTask("AODVSendTask", 10, AODVsndcode, tMoteData->dataAODVsnd);
  ttSetPriority(AODVSNDTASK_PRIO, "AODVSendTask");
  ttCreateJob("AODVSendTask");
	
  // Task to process incoming messages
  // higher prio than AODVSend to allow faster routes to be
  // obtained while sending buffered data
  tMoteData->dataAODVrcv = new AODVrcv_Data;
  tMoteData->dataAODVrcv->nodeID = nodeID + NBR_NODES;
  for (i=0; i<NBR_AODV; i++) {
    tMoteData->dataAODVrcv->cache[i] = 0; // No cached RREQs
  }
  tMoteData->dataAODVrcv->RERRlist = new List("RERRList", NULL); // list of RERRs to propagate
  tMoteData->dataAODVrcv->routing_table = tMoteData->routing_table;
  tMoteData->dataAODVrcv->seqNbrs = tMoteData->seqNbrs;
  ttCreateTask("AODVRcvTask", 10, AODVrcvcode, tMoteData->dataAODVrcv);
  ttSetPriority(AODVRCVTASK_PRIO, "AODVRcvTask");
	
  // Periodic task to send HELLO messages and
  // discover/handle lost connections
  tMoteData->dataHello = new Hello_Data;
  tMoteData->dataHello->nodeID = nodeID + NBR_NODES;
  tMoteData->dataHello->lastRREQ = -100.0;
  tMoteData->dataHello->routing_table = tMoteData->routing_table;
  tMoteData->dataHello->seqNbrs = tMoteData->seqNbrs;
  for (i=0; i<NBR_AODV; i++) {
    tMoteData->dataHello->nbors[i] = 0; // no neighbors initially
    tMoteData->dataHello->lastHello[i] = -10.0; 
  }
  tMoteData->dataHello->RERRlist = new List("RERRList", NULL);
  tMoteData->dataAODVrcv->dataHello = tMoteData->dataHello;  // set pointer to HelloTask data structure
  double offset = 0.1*((double)rand()/(double)RAND_MAX); // to avoid collisions between HELLO msgs
  ttCreatePeriodicTask("HelloTask", offset, HELLO_INTERVAL, hellocode, tMoteData->dataHello);
  ttSetPriority(HELLOTASK_PRIO, "HelloTask");
	
  // Timer handler
  ttCreateHandler("timer_handler", TIMERHANDLER_PRIO, timercode);
	
  // Task to handle expiry of routing table entries
  tMoteData->dataExp = new Exp_Data;
  tMoteData->dataExp->nodeID = nodeID + NBR_NODES;
  tMoteData->dataExp->expTimer = 0; // flag to indicate timer set
  tMoteData->dataExp->routing_table = tMoteData->routing_table;
  tMoteData->dataAODVsnd->dataTimer = tMoteData->dataExp;  // set pointer to TimerTask data structure
  tMoteData->dataAODVrcv->dataTimer = tMoteData->dataExp;  // set pointer to TimerTask data structure
  ttCreateTask("TimerTask", 10, expcode, tMoteData->dataExp);
  ttSetPriority(TIMERTASK_PRIO, "TimerTask");
	
  // Application tasks
  // -----------------
  // Bot 1 periodically sends data to Bot 3
  // Jobs of RcvTask in node 3 created from AODV layer
  //if (nodeID == 1000) {
  //		mexPrintf("Bot 1:s send task initialized\n");
  //		datasnd = new APPLsnd_Data;
  //		datasnd->nodeID = NBR_NODES + nodeID;
  //		datasnd->sent = 0;
  //		ttCreatePeriodicTask("SendTask", 0, 2, 10, APPLsndcode, datasnd);
  //	}
  //	
  //	if (nodeID == 3000) {
  //		mexPrintf("Bot 3:s receive task initialized\n");
  //		datarcv = new APPLrcv_Data;
  //		datarcv->nodeID = NBR_NODES + nodeID;
  //		datarcv->received = 0;
  //		ttCreateTask("RcvTask", 1, 10, APPLrcvcode, datarcv);
  //	}
  
  // The Tmote init stuff
  tMoteData->dataAODVrcv->bot_nbr = nodeID;
  tMoteData->dataAODVrcv->rcv_timeout = 0.0;
  tMoteData->dataAODVrcv->snd_block = 0.0;
  tMoteData->dataAODVrcv->responseCounter = 0;
	
  // Offsets for periodic tasks
#define OS_0 0.0
#define OS_1 0.05
#define OS_2 0.1
#define OS_3 0.15
#define OS_4 0.2
#define OS_5 0.25
	  
  if (nodeID == 1) {

    // Tells each wheel to give its current angle
    ttCreateMailbox("WheelRequestBox",100);
    ttCreatePeriodicTask("request_wheel_pos", OS_0, 0.4, request_wheel_pos_code, tMoteData->dataAODVrcv);
    ttSetPriority(1, "request_wheel_pos");
	
    // Get ir information to mega_128
    ttCreatePeriodicTask("request_ir", OS_1, 0.4, request_ir_code, tMoteData->dataAODVrcv);
    ttSetPriority(2, "request_ir");
	

    // Sends radio and tells ultra to send for range-req
    ttCreateTask("send_range_req", 10, send_range_request, tMoteData->dataAODVrcv);
    ttSetPriority(1, "send_range_req");
    ttCreateJob("send_range_req");
	
    // Sends data from node to mega128 for kalman_update
    ttCreateTask("node_data_to_kalman", 5, send_data_to_kalman, tMoteData->dataAODVrcv);
    ttSetPriority(5, "node_data_to_kalman");
	
    // Tells mega128 to give new wheel velocities
    ttCreatePeriodicTask("request_wheel_vel", OS_3, 0.4, request_wheel_vel_code, tMoteData->dataAODVrcv);
    ttSetPriority(4, "request_wheel_vel");
  }
	
  // I2C network
  ttCreateHandler("msg_rcv_handler", 1, msg_rcv_handler, tMoteData->dataAODVrcv);
  ttAttachNetworkHandler(I2C_NETWORK + nodeID - 1, "msg_rcv_handler");

  // Initialize network
  ttCreateHandler("nw_handler", NWHANDLER_PRIO, nwrcvcode, tMoteData->dataAODVrcv);
  ttAttachNetworkHandler(ZIGBEENETW, "nw_handler");

  // Print routing table handler
  ttCreateHandler("print_handler",14,printcode,tMoteData->dataAODVrcv);
  ttAttachTriggerHandler(1, "print_handler",0.1);
}

void cleanup() {
  int i, nodeID;
  
  // Read the input argument from the block dialogue
  mxArray *initarg = ttGetInitArg();
  nodeID = (int)mxGetPr(initarg)[0];
  mexPrintf("Cleaning Bot#%d\n", nodeID);

  TMoteData *tMoteData = (TMoteData *)ttGetUserData();
   
  for (i=0; i<NBR_AODV; i++) {
    delete tMoteData->dataAODVsnd->buffer[i];
  }
  delete tMoteData->dataAODVsnd;
  
  delete tMoteData->dataAODVrcv->RERRlist;
  delete tMoteData->dataAODVrcv;
  
  delete tMoteData->dataHello->RERRlist;
  delete tMoteData->dataHello;
  
  delete tMoteData->dataExp;
        
  delete[] tMoteData->seqNbrs;
  delete[] tMoteData->routing_table;
}
