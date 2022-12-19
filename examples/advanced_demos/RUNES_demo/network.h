#define NBR_NODES 6

#define NBR_BOTS 3

#define NBR_AODV 9



#define ACTIVE_ROUTE_TIMEOUT 3 // 3000 msec

#define MY_ROUTE_TIMEOUT (2 * ACTIVE_ROUTE_TIMEOUT)

#define HELLO_INTERVAL 1 // 1000 msec

#define ALLOWED_HELLO_LOSS 2

#define DELETE_PERIOD (ALLOWED_HELLO_LOSS * HELLO_INTERVAL)



#define VERBOSE 0 // Many printouts or just a few?



// Network numbers

#define ZIGBEENETW 1	// Zigbee network

#define ULTRANETW 2		// Ultrasound network



// types of network messages

enum { RREQ, RREP, RERR, HELLO, DATA, RANGE_REQUEST, RANGE_RESPONSE}; 

enum { APPL_DATA, ROUTE_EST };         // types of mailbox messages



// Route table entry

class RouteEntry {

public:

    int dest;

    int nextHop;

    int hops;

    int destSeqNbr;

    double exptime;

    int prec[NBR_AODV]; // precursor list, element i is non-zero is node i uses this node

                         // node as next hop towards dest.

    int valid;

};



// Data structure for application data messages

class DataMsg {

public:

    int dest;

    int src;

    double data;

    int size;

};



// Data structure for mailbox messages

// Used for communication between application and AODV layers (data)

//  and for communication between AODVsend and AODVrcv tasks  (notification of established routes)

class MailboxMsg {

public:

    int type;

    int dest; // destination for est. route, 0 in data messages

    DataMsg* datamsg; // NULL if comm. est. route

};



// Generic data structure for network messages

class GenericNwkMsg {

public:

    int type;      // RREQ, RREP, RERR, HELLO, DATA, RANGE_REQUEST, RANGE_RESPONSE

    int intermed;  // intermediate sending node 

    void* msg;

};



// Data structure for AODV RREQ messages

class RREQMsg {

public:

    int hopCnt;

    int RREQID;

    int dest;

    int destSeqNbr;

    int src;

    int srcSeqNbr;

};



// Data structure for AODV RREP messages

class RREPMsg {

public:

    int hopCnt;

    int dest;

    int destSeqNbr;

    int src;

    double lifetime;

};



// Data structure for AODV RERR messages

class RERRMsg {

public:

    int dest;

    int destSeqNbr;

    int receiver; // RERRs sent as a sequence of unicasts

};



// Data structure for AODV Hello messages

class HelloMsg {

public:

    int hopCnt;

    int dest;

    int destSeqNbr;

    int src;

    double lifetime;

};





// Task data structures



// Data structure for task handling Hello messages

class Hello_Data {

public:

    int nodeID;

    double lastRREQ; // time for last received RREQ

    RouteEntry *routing_table;

    int *seqNbrs;

    int nbors[NBR_AODV]; // element i is 1 if node i is a neighbor

    double lastHello[NBR_AODV]; // last Hello from neighbor nodes

    List* RERRlist;   // list of RERRs to send

};



// Data structure for timer expiry task

class Exp_Data {

public:

    int nodeID;

    int expTimer;

    RouteEntry *routing_table;

};



// Data structure for AODV snd task

class AODVsnd_Data {

public:

    int nodeID;

    int RREQID;

    List* buffer[NBR_AODV];   // message buffer for each destination

    int doEmptyBuffer;         // flag to indicate that the buffer should be flushed

    int bufferDest;

    RouteEntry *routing_table;

    int *seqNbrs;

    int sendTo;

    GenericNwkMsg* msg;

    int size;

    Exp_Data *dataTimer;  // to be able to update the expiry timer    

};



// Data structure for AODV rcv task

class AODVrcv_Data {

public:

    int nodeID;

    int cache[NBR_AODV];   // last RREQID from each node

    List* RERRlist;         // list of RERRs to send

    RouteEntry *routing_table;

    int *seqNbrs;

    Hello_Data *dataHello;  // to be able to update lastHello of neighbor nodes 

    Exp_Data *dataTimer;    // to be able to update the expiry timer   

	double startTime;

	double deltaTime;

	// Added for bot only

	int bot_nbr;

    double rcv_timeout;

    double snd_block;

    double node_x;

    double node_y;

    double node_dist;

    unsigned int node_nodeID;	// DEBUG

	double leftVel;

	int requestObtained;

	int responseCounter;

};  



// Data structure for application send task

class APPLsnd_Data {

public:

    int nodeID;

    int sent;

};



// Data structure for application receive task

class APPLrcv_Data {

public:

    int nodeID;

    int received;

    

};



class RangeResponseMsg {

public:

	/* not the actual range, but the index at

	which the pulse is detected */

	unsigned int range;

	/* Fixed-pint, 9-bit decimal

	=> resolution 1/512 m, which is sufficient

	since we got a sdev of 0.02 on the range

	measurements anyhow */

	double x;

	double y;

	// DEBUG

	unsigned int nodeID;

};



class UltraSoundMsg {

public:

	int variable;

};

