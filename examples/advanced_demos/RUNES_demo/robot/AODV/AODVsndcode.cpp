double AODVsndcode(int seg, void* data) {

  AODVsnd_Data* d = (AODVsnd_Data*) data;

  int myID = d->nodeID;

  

  MailboxMsg *mailboxmsg; 

  GenericNwkMsg *nwkmsg;

  DataMsg *datamsg;  

  RREQMsg *rreqmsg;  

  

  int dest;

  double now, etime;



  RouteEntry *dest_entry;

  DataNode *dn;

  

  switch (seg) {

  case 1:  

    ttFetch("AODVSndBox");

    return 0.0001;

    

  case 2:

    mailboxmsg = (MailboxMsg*) ttRetrieve("AODVSndBox");

    now = ttCurrentTime();

    

    switch (mailboxmsg->type) {

    case APPL_DATA:

      // Data message from application

      datamsg = (DataMsg*) mailboxmsg->datamsg;

      delete mailboxmsg;

        

      mexPrintf("(%.4f)Node# %d wants to send to Node# %d Data: %.4f\n", now, myID, datamsg->dest, datamsg->data);

        

      dest_entry = &(d->routing_table[datamsg->dest-1]);

        

      // Is the entry valid?

      if (!dest_entry->valid) {

	// Not found in routing table

	mexPrintf("No (valid) route exists\n");

            

	// Increment broadcast ID and sequence number

	d->RREQID++;

	d->seqNbrs[myID-1]++;

                  

	// Create RREQ

	rreqmsg = new RREQMsg;

	rreqmsg->hopCnt = 0;

	rreqmsg->RREQID = d->RREQID;

	rreqmsg->dest = datamsg->dest;

	rreqmsg->destSeqNbr = d->seqNbrs[datamsg->dest-1];

	rreqmsg->src = myID;

	rreqmsg->srcSeqNbr = d->seqNbrs[myID-1];

        

	d->sendTo = 0; // broadcast

	d->msg = new GenericNwkMsg;

	d->msg->type = RREQ;

	d->msg->intermed = myID;

	d->msg->msg = rreqmsg;

	d->size = 24*8; // 24 bytes

      

	// buffer data until route has been established

	mexPrintf("Buffering message %d\n", d->buffer[datamsg->dest-1]->length()+1);

            

	d->buffer[datamsg->dest-1]->appendNode(new DataNode(datamsg, ""));

            

	// exectime

	etime = 0.0001;;

      } else {

	// Route to destination exists in table

	mexPrintf("Route exists in table ");

	mexPrintf("nextHop: %d #Hops: %d\n", dest_entry->nextHop, dest_entry->hops);

            

	// Send data to first node on route to destination

	d->sendTo = dest_entry->nextHop;

	d->msg = new GenericNwkMsg;

	d->msg->type = DATA;

	d->msg->intermed = myID;

	d->msg->msg = datamsg;

	d->size = datamsg->size;

     

	// Update expiry time

	d->routing_table[datamsg->dest-1].exptime = now + ACTIVE_ROUTE_TIMEOUT;

	updateExpiryTimer(d->routing_table, d->dataTimer, myID);

      

	etime = 0.0001;;			

      }

      return etime;

        

    case ROUTE_EST:

      // Message from AODVRcv that a route has been established

      dest = mailboxmsg->dest;

      delete mailboxmsg;

        

      mexPrintf("Time: %f A new route has been established between Node#%d and Node#%d\n", now, myID, dest);

        

      dest_entry = &(d->routing_table[dest-1]);

      mexPrintf("nextHop: %d #Hops: %d\n", dest_entry->nextHop, dest_entry->hops);

        

      mexPrintf("%d data messages in buffer\n", d->buffer[dest-1]->length());

        

      d->bufferDest = dest;

      d->doEmptyBuffer = 1; // Buffer should be emptied



      // Update expiry timer

      d->routing_table[dest-1].exptime = now + ACTIVE_ROUTE_TIMEOUT;

      updateExpiryTimer(d->routing_table, d->dataTimer, myID);

      

      return 0.0001;;

    }

    

  case 3:

    // Send buffered data messages?

    if (d->doEmptyBuffer) {

    	

      dn = (DataNode*) d->buffer[d->bufferDest-1]->getFirst(); 

      if (dn != NULL) {

	// Retrieve next message in buffer

	datamsg = (DataMsg*) dn->data;

            

	mexPrintf("Buffer Size: %d Sending buffered message towards Node#%d\n", d->buffer[d->bufferDest-1]->length(), d->bufferDest);

            

	dn->remove();

	delete dn;

            

	// Retrieve route entry

	dest_entry = &(d->routing_table[datamsg->dest-1]);



	nwkmsg = new GenericNwkMsg;

	nwkmsg->type = DATA;

	nwkmsg->intermed = myID;

	nwkmsg->msg = datamsg;

	ttSendMsg(1, dest_entry->nextHop, nwkmsg, datamsg->size); 

	ttSleep(0.001);



	return 0.0001;

      } else {

	// Finished sending data messages

	mexPrintf("Buffer emptied\n");

            

	d->doEmptyBuffer = 0;

	ttSetNextSegment(1); // Loop back

	return 0.0001;

      }

  

    } else {

      ttSendMsg(1, d->sendTo, d->msg, d->size); 

      ttSetNextSegment(1); // Loop back

        

      return 0.0001;

    }

 

  case 4:

    ttSetNextSegment(3);

    return 0.0001;

  }

 

  return FINISHED; // to supress compilation warnings

}

