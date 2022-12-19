double AODVrcvcode(int seg, void* data) {



  AODVrcv_Data* d = (AODVrcv_Data*) data;

  int myID = d->nodeID;

  

  MailboxMsg *mailboxmsg; 

  GenericNwkMsg *nwkmsg;

  DataMsg *datamsg;  

  RREQMsg *rreqmsg;  

  RREPMsg *rrepmsg; 

  RERRMsg *rerrmsg, *rerrmsg2; 

  HelloMsg *hellomsg; 

  

  int i, dest, seqNbr, hopCnt, cond1, cond2;

  int intermed, drop, cached_RREQID, propagate, nextHop;

  double now, etime1, etime2, lifetime;

  double etime = 0.0001;

  

  RouteEntry *dest_entry, *src_entry;

  DataNode *dn;

  

  switch (seg) {

  case 1:

    nwkmsg = (GenericNwkMsg*) ttGetMsg(ZIGBEENETW);

    if (nwkmsg == NULL) {

      mexPrintf("ERROR: Received empty message!\n");

      ttAbortSimulation();

      return -1;

    }



    now = ttCurrentTime();

    

    if (nwkmsg->type == RANGE_REQUEST) {

      d->requestObtained = 1;

      now = ttCurrentTime();

      d->startTime = now;

      //	mexPrintf("Range request received in node %d at %f \n",d->nodeID,now);

      

      return FINISHED;

    } else if (nwkmsg->type == DATA) {

      // Data message

      datamsg = (DataMsg*) nwkmsg->msg;

      delete nwkmsg;

      

      if (datamsg->dest == myID) { 

	// Data message arrived at destination

	if (VERBOSE) {

	  mexPrintf("Time: %f Data message arrived at Node#%d\n", now, myID);

	}

	

	// Update expiry timer for route to source

	d->routing_table[datamsg->src - 1].exptime = now + ACTIVE_ROUTE_TIMEOUT;

	

	// Notify application

	ttTryPost("AODVRcvBox", datamsg);

	ttCreateJob("RcvTask");

	

	etime = 0.0001;

      } else {

	

	// Forward data message

	if (VERBOSE) {

	  mexPrintf("Time: %f Node#%d about to forward data to Node#%d Data: %f\n", now, myID, datamsg->dest, datamsg->data);  

	}

	

	// Update expiry timer for route to source

	d->routing_table[datamsg->src - 1].exptime = now + ACTIVE_ROUTE_TIMEOUT;

	

	// Find route to destination and update expiry timer

	dest_entry = &(d->routing_table[datamsg->dest - 1]);

	dest_entry->exptime = now + ACTIVE_ROUTE_TIMEOUT;

	

	// Forward data message to next hop

	nwkmsg = new GenericNwkMsg;

	nwkmsg->type = DATA;

	nwkmsg->msg = datamsg;

	ttSendMsg(1, dest_entry->nextHop, nwkmsg, datamsg->size); 

	

	etime = 0.0001;;

      }	   

      updateExpiryTimer(d->routing_table, d->dataTimer, myID);

      

    } else {

      // AODV control message (RREQ, RREP, RERR, or HELLO)

      if (VERBOSE) {

	mexPrintf("Time: %f Node#%d processing AODV message type: %d from Node#%d\n", now, myID, nwkmsg->type, nwkmsg->intermed);

      }

      

      switch (nwkmsg->type) {

      case RREQ:  

	

	rreqmsg = (RREQMsg*) nwkmsg->msg;

	intermed = nwkmsg->intermed;

	

	if (rreqmsg->src == myID) {

	  // Skip RREQs received by the original source	

	  etime = 0.00001; 

	} else { 

	  // Have this RREQ already been processed?

	  cached_RREQID = d->cache[rreqmsg->src - 1];

	  

	  drop = 0;

	  if (rreqmsg->RREQID <= cached_RREQID) {

	    // Found in cache, drop redundant RREQ

	    if (VERBOSE) {

	      mexPrintf("Time: %f Node#%d dropping redundant RREQ from Node#%d\n", now, myID, intermed);

	    }

	    drop = 1;

	  }

	  

	  if (!drop) { 

	    // process RREQ

	    if (VERBOSE) {

	      mexPrintf("Time: %f Node#%d caching RREQ with Src: %d RREQID: %d\n", now, myID, rreqmsg->src, rreqmsg->RREQID);

	    }

	    

	    // Enter RREQID in cache

	    d->cache[rreqmsg->src - 1] = rreqmsg->RREQID;

	  

	    // Create or update route entry to source

	    src_entry = &(d->routing_table[rreqmsg->src - 1]);

	    

	    if (!src_entry->valid) {

	      // No entry exists or invalid

	      src_entry->destSeqNbr = rreqmsg->srcSeqNbr;

	      for (i=0; i<NBR_AODV; i++) {

		src_entry->prec[i] = 0; // empty precursor list

	      }

	    } else {

	      // Update destination sequence number

	      src_entry->destSeqNbr = max(src_entry->destSeqNbr, rreqmsg->srcSeqNbr);

	    }

	    src_entry->dest = rreqmsg->src;

	    src_entry->nextHop = intermed;

	    src_entry->hops = rreqmsg->hopCnt + 1;

	    src_entry->exptime = now + ACTIVE_ROUTE_TIMEOUT;

	    src_entry->valid = 1;

	    

	    updateExpiryTimer(d->routing_table, d->dataTimer, myID);

			

	    // Check if we have a route to destination

	    dest_entry = &(d->routing_table[rreqmsg->dest - 1]);

	    

	    if (rreqmsg->dest==myID || dest_entry->valid) {

	      // We are the destination or we have a route to it

	      if (VERBOSE) {

		mexPrintf("Node#%d has a route to destination#%d\n", myID, rreqmsg->dest);

	      }

	      

	      if (rreqmsg->dest!=myID) {

		// I am not the destination, but have a route to it

		if (VERBOSE) {

		  mexPrintf("Sending first RREP from node with route\n");

		}

		

		dest = dest_entry->dest;

		seqNbr = dest_entry->destSeqNbr;

		hopCnt = dest_entry->hops;

		lifetime = dest_entry->exptime - now;

		dest_entry->prec[intermed] = 1;

	      } else {

		// I am the destination itself

		if (VERBOSE) {

		  mexPrintf("Sending first RREP from destination itself\n");

		}

		

		dest = myID;

		if (d->seqNbrs[myID - 1] + 1 == rreqmsg->srcSeqNbr) {

		  d->seqNbrs[myID - 1]++;

		}

		seqNbr = max(d->seqNbrs[myID - 1], rreqmsg->destSeqNbr);

		hopCnt = 0;

		lifetime = ACTIVE_ROUTE_TIMEOUT; 

	      }

	    

	      // Create RREP

	      rrepmsg = new RREPMsg;

	      rrepmsg->hopCnt = hopCnt;		   

	      rrepmsg->dest = dest;

	      rrepmsg->destSeqNbr = seqNbr;

	      rrepmsg->src = rreqmsg->src;

	      rrepmsg->lifetime = lifetime;

		    

	      // Send RREP to previous hop

	      nwkmsg = new GenericNwkMsg;

	      nwkmsg->type = RREP;

	      nwkmsg->intermed = myID;

	      nwkmsg->msg = rrepmsg;	 

	      ttSendMsg(1, intermed, nwkmsg, 20*8);	// 20 bytes

	    

	      etime = 0.0001;;

	    

	    } else {

	      // We do not have a route to the destination

	      if (VERBOSE) {

		mexPrintf("Time: %f Node#%d sending new broadcast\n", now, myID);

	      }

			    

	      // Update RREQ

	      rreqmsg->hopCnt++;

	      rreqmsg->destSeqNbr = max(d->seqNbrs[rreqmsg->dest - 1], rreqmsg->destSeqNbr);

			    

	      // Rebroadcast RREQ

	      nwkmsg = new GenericNwkMsg;

	      nwkmsg->type = RREQ;

	      nwkmsg->intermed = myID;

	      nwkmsg->msg = rreqmsg;	 

	      ttSendMsg(1, 0, nwkmsg, 24*8);	 // 24 bytes

	    

	      etime = 0.0001;

	    }

	  } else {

	    etime = 0.00001; // used if RREQ is dropped

	  }

	

	}

	break;

	    

      case RREP:

	rrepmsg = (RREPMsg*) nwkmsg->msg;

	intermed = nwkmsg->intermed;

	    

	if (VERBOSE) {

	  mexPrintf("Node#%d got an RREP from Node#%d for destination#%d\n", myID, intermed, rrepmsg->dest);

	}

	

	// Initialize or update forward route entry

	dest_entry = &(d->routing_table[rrepmsg->dest - 1]);

      

	if (!dest_entry->valid) {

	  // No valid entry exists, initialize new

	  if (VERBOSE) {

	    mexPrintf("Initializing new forward entry from Node#%d to Node#%d\n", myID, rrepmsg->dest);

	  }

	  

	  dest_entry->dest = rrepmsg->dest;

	  dest_entry->nextHop = intermed;

	  dest_entry->hops = rrepmsg->hopCnt + 1; 

	  dest_entry->destSeqNbr = rrepmsg->destSeqNbr;

	  dest_entry->exptime = now + rrepmsg->lifetime;

	  for (i=0; i<NBR_AODV; i++) {

	    dest_entry->prec[i] = 0; // empty precursor list

	  }

	  dest_entry->valid = 1;

	

	  propagate = 1;

	

	  etime2 = 0.0001;;

	} else {

	  // Valid forward entry already exists in table

	  // Should it be updated?

	  cond1 = (rrepmsg->destSeqNbr > dest_entry->destSeqNbr);

	  cond2 = ((rrepmsg->destSeqNbr == dest_entry->destSeqNbr) && (rrepmsg->hopCnt+1 < dest_entry->hops));

	

	  if (cond1 || cond2) {

	    // Update existing entry

	    if (VERBOSE) {

	      mexPrintf("Updating existing forward entry from Node#%d to Node#%d\n", myID, rrepmsg->dest);

	    }

	  

	    dest_entry->nextHop = intermed;

	    dest_entry->hops = rrepmsg->hopCnt + 1; 

	    dest_entry->destSeqNbr = rrepmsg->destSeqNbr;

	    dest_entry->exptime = now + rrepmsg->lifetime;



	    propagate = 1;



	    etime2 = 0.0001;;

	  } else {

	    // Existing entry should be kept

	    // Do not propagate RREP 

	    if (VERBOSE) {

	      mexPrintf("No entry updated in Node#%d\n", myID);

	    }

	    

	    propagate = 0;

		

	    etime2 = 0.0001;;

	  }

	}

      

	etime1 = 0;

      

	if (rrepmsg->src == myID) {

	  // Original source, no reverse entry exists

	  if (VERBOSE) {

	    mexPrintf("Node#%d got final RREP for route to Node#%d\n", myID, rrepmsg->dest); 

	  }

	  

	  // Inform AODVSend to send buffered data messages

	  mailboxmsg = new MailboxMsg;

	  mailboxmsg->type = ROUTE_EST;

	  mailboxmsg->dest = rrepmsg->dest;

	  mailboxmsg->datamsg = NULL;

	  ttTryPost("AODVSndBox", mailboxmsg);

	

	  etime1 = 0.0001;

	} else if (propagate) {

	  // Update reverse entry from info in RREP

	  // and get next hop towards source

	  src_entry = &(d->routing_table[rrepmsg->src - 1]);

	  src_entry->exptime = now + rrepmsg->lifetime;

	  src_entry->prec[intermed-1] = 1; // the node that sent the RREP is a precursor towards the src

	  nextHop = src_entry->nextHop;

		    

	  // Update precursor list for forward entry

	  dest_entry->prec[nextHop-1] = 1; // the node that will rcv next RREP is a precursor towards the dest

	

	  // Update RREP to continue back propagation

	  rrepmsg->hopCnt++;

			   

	  nwkmsg = new GenericNwkMsg;

	  nwkmsg->type = RREP;

	  nwkmsg->intermed = myID;

	  nwkmsg->msg = rrepmsg;   

		    

	  ttSendMsg(1, nextHop, nwkmsg, 20*8);  // 20 bytes

	

	  etime1 = 0.0001;

	}

	updateExpiryTimer(d->routing_table, d->dataTimer, myID);

	    

	etime = etime1 + etime2;

	break;

      case RERR:

	rerrmsg = (RERRMsg*) nwkmsg->msg;

	intermed = nwkmsg->intermed;

	    

	if (VERBOSE) {

	  mexPrintf("Node#%d got an RERR from Node#%d for destination#%d\n", myID, intermed, rerrmsg->dest);

	}

      

	// Propagate RERR?

	dest_entry = &(d->routing_table[rerrmsg->dest - 1]);

	    

	if (dest_entry->valid && dest_entry->nextHop == intermed) {

	  // Should send RERR to all nodes in precursor list (neighbors)

	  for (i=0; i<NBR_AODV; i++) {

	    if (dest_entry->prec[i]) {

	      // Node i+1 uses this node as next hop towards dest, send RERR

	      rerrmsg2 = new RERRMsg;	       

	      rerrmsg2->dest = dest_entry->dest;

	      rerrmsg2->destSeqNbr = dest_entry->destSeqNbr;

	      rerrmsg2->receiver = i+1;

					

	      d->RERRlist->appendNode(new DataNode(rerrmsg2, ""));   

	    }

	  }

			

	  // Invalidate route

	  if (VERBOSE) {

	    mexPrintf("Node#%d invalidating route to Node#%d through Node#%d\n", myID, rerrmsg->dest, intermed);

	  }

	    

	  dest_entry->valid = 0;

	}

      

	etime = 0.0001;;

	break;

	    

      case HELLO:

	hellomsg = (HelloMsg*) nwkmsg->msg;

	intermed = nwkmsg->intermed;

	//delete nwkmsg;

      

	if (VERBOSE) {

	  mexPrintf("Time: %f Node#%d got a hello message from Node#%d\n", now, myID, hellomsg->dest); 

	}

		   

	// Update time stamp for last HELLO msg

	d->dataHello->nbors[hellomsg->dest - 1] = 1;

	d->dataHello->lastHello[hellomsg->dest - 1] = now;

	

	etime = 0.0001;;

      }

    

    }  

    return etime; 

    

  case 2:

  

    // Send next RERR, if any

    dn = (DataNode*) d->RERRlist->getFirst(); 

    if (dn != NULL) {

      // Retrieve next RERR in list

      rerrmsg = (RERRMsg*) dn->data;

	

      dn->remove();

      delete dn;



      // Send RERR to receiver

      nwkmsg = new GenericNwkMsg;

      nwkmsg->type = RERR;

      nwkmsg->intermed = myID;

      nwkmsg->msg = rerrmsg;

	

      if (VERBOSE) {

	mexPrintf("Node#%d sending RERR to Node#%d\n", myID, rerrmsg->receiver);

      }

	

      ttSendMsg(1, rerrmsg->receiver, nwkmsg, 12*8);  // 12 bytes 

      ttSleep(0.001);

      return 0.0001;

    } else {

      return FINISHED;    

    }

    

  case 3:

    ttSetNextSegment(2);

    return 0.0001;

  }

 

  return FINISHED; // to supress compilation warnings

}





