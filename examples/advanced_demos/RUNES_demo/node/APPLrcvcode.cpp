double APPLrcvcode(int seg, void* data) {

  APPLrcv_Data* d = (APPLrcv_Data*) data;

  static DataMsg *msg;

  

  switch (seg) {

  case 1:

    ttFetch("AODVRcvBox");

    

    return 0.0;

  case 2:

    msg = (DataMsg*) ttRetrieve("AODVRcvBox");

    

    return 0.0002;

  case 3:    

//	  mexPrintf("(%.4f)Node#%d receiving data from #%d : %f\n", ttCurrentTime(), d->nodeID, msg->src,msg->data);

    ttAnalogOut(msg->src - 1,msg->data);

    delete msg;

    

    // Store received data

    d->received++;

    return FINISHED; 

  }

  

  return FINISHED; // to supress compilation warnings

}

