/** Task for requesting wheel velocities from wheel avr */
double request_wheel_pos_code(int segment, void* t_d)
{
  AODVrcv_Data *d = (AODVrcv_Data *) t_d;

  MailboxMsg *mailboxmsgin;

  switch (segment){
  case 1: {
    generic* m;
    m = new generic;
    m->t = REQUEST_WHEEL_VEL;
    REQUEST_WHEEL_VEL_MSG* msg = new REQUEST_WHEEL_VEL_MSG;
    m->msg = msg;
	
    msg->hdr.sender = TMOTE;
    msg->hdr.receiver = LEFT_WHEEL;
	
    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, LEFT_WHEEL, m, 8);
    return 0.0001; 
  }
  case 2: {
    ttFetch("WheelRequestBox");
    return 0.0;
  }
  case 3: {
    mailboxmsgin = (MailboxMsg*) ttRetrieve("WheelRequestBox");
    d->leftVel = ((DataMsg*)mailboxmsgin->datamsg)->data;
    delete mailboxmsgin;
    return 0.0;
  }
  case 4: {
    generic* m2;
    m2 = new generic;
    m2->t = REQUEST_WHEEL_VEL;
    REQUEST_WHEEL_VEL_MSG* msg = new REQUEST_WHEEL_VEL_MSG;
    m2->msg = msg;
    msg->hdr.sender = TMOTE;
    msg->hdr.receiver = RIGHT_WHEEL;
	
    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, RIGHT_WHEEL, m2, 8);
    return 0.0001;
  }
  case 5: {
    ttFetch("WheelRequestBox");
    return 0.0;
  }
  default: {
    mailboxmsgin = (MailboxMsg*) ttRetrieve("WheelRequestBox");
    //		d->leftVel = ((DATAMSG*)mailboxmsgin->datamsg)->vel;
    //		datamsgin = (DataMsg*) ttRetrieve("WheelRequestBox");
    //		msgin = (SEND_WHEEL_VEL_MSG*) datamsgin->datamsg;
    generic* m1;
    m1 = new generic;
    m1->t = SEND_WHEEL_VELS;
    SEND_WHEEL_VELS_MSG* msg1 = new SEND_WHEEL_VELS_MSG;
    m1->msg = msg1;
    msg1->hdr.sender = TMOTE;
    msg1->hdr.receiver = LEFT_WHEEL;
    msg1->leftVel = d->leftVel;
    msg1->rightVel = ((DataMsg*)mailboxmsgin->datamsg)->data;
	
    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, MEGA_128, m1, 8+2*32);
    delete mailboxmsgin;
    return FINISHED;
  }
  }

  return FINISHED;
}

/** Task for sending range requests to nodes
 * This is not a periodic task but a task in an
 * infinite loop.
 *	#1 Check if blocked, if so add 50 ms delay
 *	#2 Send radio message
 *	#3 Request ultra sound transmission from Mega_Ultra
 */
double send_range_request(int segment, void* t_d)
{
  AODVrcv_Data *d = (AODVrcv_Data *)t_d;

  switch (segment){
  case 1: {
    if (ttCurrentTime() < d->snd_block) {
      // Request sending blocked, delay next send with 50 ms
      mexPrintf("Bot#%d dalying 50 ms\n", d->bot_nbr);
      ttSleep(0.050);
      ttSetNextSegment(4);
    } else if (d->rcv_timeout == 0.0) {
      // First time wait before sending first to avoid synced send
      d->rcv_timeout = ttCurrentTime();
      ttSleep(d->bot_nbr * 0.2);
      ttSetNextSegment(1);
    }

    return 0.0001;
  }
  case 2: {
    // Broadcast message
    GenericNwkMsg *msg = new GenericNwkMsg;
    msg->type = RANGE_REQUEST;
    msg->intermed = 0;	// Intermedian sending node?

			// Send the network message
    ttSendMsg(ZIGBEENETW, 0, msg, 8*sizeof(*msg));
    //			mexPrintf("Robot sends range request from x %f y %f at %f\n",ttAnalogIn(1),ttAnalogIn(2),ttCurrentTime());

    return 0.0001;
  }
  case 3: {
    // Tell ultra_mega to send ultra sound
    generic* m;
    m = new generic;
    m->t = SEND_ULTRA;
    m->msg = new SEND_ULTRA_MSG;

    d->rcv_timeout = ttCurrentTime() + 0.150;

    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, MEGA_ULTRA, m, 8);

    ttAnalogOut(1,d->responseCounter);
    d->responseCounter = 0;

    return 0.0001;
  }
  case 4: {
    // Loop back to #1
    ttSleep(1.2);
    ttSetNextSegment(1);

    return 0.0001;
  }
  }

  return FINISHED;
}

/**	Task for sending node data to Mega_128
 *	A job of this task i created in the 
 * message receive handler when a node
 * answers a range request.
 */
double send_data_to_kalman(int segment, void* t_d)
{
  AODVrcv_Data *d = (AODVrcv_Data *)t_d;

  switch (segment){
  case 1: {
    // Tell mega128 to make a kalman_update
    generic* m = new generic;
    m->t = SEND_ULTRA_POS;
    SEND_ULTRA_POS_MSG* msg = new SEND_ULTRA_POS_MSG;

    msg->hdr.sender = TMOTE;
    msg->hdr.receiver = MEGA_128;

    msg->x = d->node_x;
    msg->y = d->node_y;
    msg->dist = d->node_dist;
    msg->nodeID = d->node_nodeID;

    m->msg = msg;

    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, MEGA_128, m, 8+3*32+16);

    return 0.0001;
  }
  default:
    return FINISHED;
  }
}

/**	Periodc task that requests the Mega_128 to send
 * reference velocities to the wheel avr:s.
 */
double request_wheel_vel_code(int segment, void* t_d)
{
  AODVrcv_Data *d = (AODVrcv_Data *) t_d;

  switch (segment){
  case 1:{
    generic* m;
    m = new generic;
    m->t = REQUEST_WHEEL_VEL;
    REQUEST_WHEEL_VEL_MSG* msg = new REQUEST_WHEEL_VEL_MSG;
    m->msg = msg;

    msg->hdr.sender = TMOTE;
    msg->hdr.receiver = MEGA_128;

    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, MEGA_128, m, 8);
    return 0.0001;
  }
  default:
    return FINISHED;
  }
}

/**	Periodic task that tells the Mega_Ultra to 
 * send obstacle avoidance information collected
 * from the IR-sensor.
 */
double request_ir_code(int segment, void* t_d)
{
  AODVrcv_Data *d = (AODVrcv_Data *) t_d;

  switch (segment){
  case 1:{
    generic* m;
    m = new generic;
    m->t = REQUEST_IR;
    REQUEST_IR_MSG* msg = new REQUEST_IR_MSG;
    m->msg = msg;

    msg->hdr.sender = TMOTE;
    msg->hdr.receiver = MEGA_ULTRA;

    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, MEGA_ULTRA, m, 8);
    return 0.0001;
  }
  default:
    return FINISHED;
  }
}

/**	Task, executed upon receiving a network message
 * on the I2C-bus.
 */
double msg_rcv_handler(int seg, void* t_d)
{
  AODVrcv_Data *d = (AODVrcv_Data *) t_d;
  MailboxMsg *mailboxmsg;
  DataMsg *dataMsg;
  
  generic *m;
  m = (generic *) ttGetMsg(I2C_NETWORK + d->bot_nbr - 1);
  switch (seg) {
  case 1:
    switch (m->t){
      // Forwarding velocity references from Mega_128 to wheel avr:s
      // or forwarding velocities from wheel avr:s to Mega_128.
    case SEND_WHEEL_VEL:
      {
	if (((SEND_WHEEL_VEL_MSG*) m->msg)->hdr.sender == MEGA_128) {
	  if (((SEND_WHEEL_VEL_MSG*) m->msg)->hdr.receiver == LEFT_WHEEL)
	    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, LEFT_WHEEL, m, 8+32);
	  else
	    ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, RIGHT_WHEEL, m, 8+32);
	} else { // This else needs to be modified
	  mailboxmsg = new MailboxMsg;
	  dataMsg = new DataMsg;
	  dataMsg->data = ((SEND_WHEEL_VEL_MSG*) m->msg)->vel;
	  mailboxmsg->datamsg = dataMsg;
	  ttPost("WheelRequestBox",mailboxmsg);
	  //   ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, MEGA_128, m, 10);
	  delete m;
	}
	
	return 0.0001;
      }
      
      // Forwarding obstacle avoidance information from Mega_Ultra to
      // to Mega_128.
    case SEND_IR:
      {
	ttSendMsg(I2C_NETWORK + d->bot_nbr - 1, MEGA_128, m, 8+32);
	return 0.0001;
      }

    default: {}

    }

    
    return 0.0001;
    
  default:
    return FINISHED;
  }
}
