#define PRINTLN //mexPrintf("Reached line %3d in %s\n", __LINE__, __FILE__)



#define TMOTE 1

#define LEFT_WHEEL 2

#define RIGHT_WHEEL 3

#define MEGA_128 4

#define MEGA_ULTRA 5

#define I2C_NETWORK 3



struct BOT_MSG_HEADER {

    /* Id is the same as last 8-bits of ip address */

    int sender;

    int receiver;

    int type;

};



/* Message types */

enum type{

    REQUEST_WHEEL_VEL,

    RETURN_WHEEL_VEL,

    SEND_WHEEL_VEL,

	SEND_WHEEL_VELS,

    SEND_ULTRA,

    SEND_REF_POS,

    SEND_ULTRA_POS,

    REQUEST_IR,

    SEND_IR

};



struct generic {

    type t;

    void *msg;

};



struct REQUEST_IR_MSG {

    BOT_MSG_HEADER hdr;

};



struct SEND_IR_MSG {

    BOT_MSG_HEADER hdr;

    int ir_result;

};



struct REQUEST_WHEEL_VEL_MSG {

    BOT_MSG_HEADER hdr;

};



struct SEND_WHEEL_VEL_MSG {

    BOT_MSG_HEADER hdr;

    double vel;

};



struct SEND_WHEEL_VELS_MSG {

	BOT_MSG_HEADER hdr;

	double leftVel;

	double rightVel;

};



struct SEND_ULTRA_MSG{

    BOT_MSG_HEADER hdr;

};



struct SEND_REF_POS_MSG{

    BOT_MSG_HEADER hdr;

    double x,y;

};



struct SEND_ULTRA_POS_MSG{

    BOT_MSG_HEADER hdr;

    double x;

    double y;

    double dist;

    unsigned int nodeID;	// DEBUG

};

