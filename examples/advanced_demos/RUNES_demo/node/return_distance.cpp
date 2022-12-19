double return_distance(int seg, void* data) {

  AODVrcv_Data *d = (AODVrcv_Data *) data;

  

  switch (seg) {

  case 1:

    // Wait period depending on nodeID

    ttSleep(((double)(d->nodeID % 6))*0.025);

    return 0.0001;

  	

  case 2:

    // Broadcast message with distance

    GenericNwkMsg *msg = new GenericNwkMsg;

    msg->type = RANGE_RESPONSE;

    msg->intermed = 0;

  	

    RangeResponseMsg *rangeMsg = new RangeResponseMsg;

    msg->msg = rangeMsg;

  	

    rangeMsg->range = (unsigned int)(d->deltaTime * 72280.0);

    rangeMsg->x = ttAnalogIn(1);

    rangeMsg->y = ttAnalogIn(2);

    rangeMsg->nodeID = d->nodeID;

  	

    // Send the message

    ttSendMsg(ZIGBEENETW, 0, msg, 8*sizeof(*msg));

  	

    return FINISHED; 

  }



  return FINISHED; 

}

