double hellocode(int seg, void* data) {



  Hello_Data *d = (Hello_Data*) data;



  int activeroute = 0;

  int k, m, n;

  double now;

  double period;



  HelloMsg *hellomsg;

  GenericNwkMsg *nwkmsg;

  RERRMsg *rerrmsg;

  DataNode *dn;

  DataNode *tmp;

  

  switch (seg) {

  case 1:

    now = ttCurrentTime();

  

    if (VERBOSE) {

      mexPrintf("Time: %f Node#%d running periodic HELLO task\n", now, d->nodeID);

    }

  

    // Determine if any active routes exist

    for (k=0; k<NBR_AODV; k++) {

      if (d->routing_table[k].valid) {

	activeroute = 1;

      }

    }

  

    period = ttGetPeriod();

    if (activeroute && (d->lastRREQ < now - period)) {

      if (VERBOSE) {

	mexPrintf("Broadcasting HELLO msg\n");

      }

      

      hellomsg = new HelloMsg;

      hellomsg->hopCnt = 0;

      hellomsg->dest = d->nodeID;

      hellomsg->destSeqNbr = d->seqNbrs[d->nodeID];

      hellomsg->src = 0;

      hellomsg->lifetime = DELETE_PERIOD; 

  

      nwkmsg = new GenericNwkMsg;

      nwkmsg->type = HELLO;

      nwkmsg->intermed = d->nodeID;

      nwkmsg->msg = hellomsg;

      ttSendMsg(1, 0, nwkmsg, 24);

    }

    

    // Determine local connectivity

    for (k=0; k<NBR_AODV; k++) {

      if (d->nbors[k]) {

	// Node k is a neighbor

	if (now - d->lastHello[k] > DELETE_PERIOD) {

	  mexPrintf("Node#%d lost connection to Node#%d\n", d->nodeID, k+1);

                

	  d->nbors[k] = 0; // remove from neighbor list

                

	  // Send RERRs

	  for (m=0; m<NBR_AODV; m++) {

	    // Find routes that use node k as next hop

	    if (d->routing_table[m].valid) {

	      if (d->routing_table[m].nextHop == k+1) {

		// Should send RERR to all nodes in precursor list

		for (n=0; n<NBR_AODV; n++) {

		  if (d->routing_table[m].prec[n]) {

		    // Node n uses this node as next hop towards node k

		    rerrmsg = new RERRMsg;

	                                

		    rerrmsg->dest = m + 1;

		    rerrmsg->destSeqNbr = d->routing_table[m].destSeqNbr;

		    rerrmsg->receiver = n + 1;

	                                

		    d->RERRlist->appendNode(new DataNode(rerrmsg, ""));

		  }

		}

	  

		// Invalidate route

		if (VERBOSE) {

		  mexPrintf("Node#%d invalidating route to Node#%d through unreachable Node#%d\n", d->nodeID, m+1, k+1);

		}

	    

		d->routing_table[m].valid = 0;

	      }

	    }

	  }

	} 

      }

    }

  

    return 0.0001;;

  case 2:

    ttSleep(0.001);

    return 0.0001;    

  case 3:

    // Send all RERRs

    dn = (DataNode*) d->RERRlist->getFirst(); 

    if (dn != NULL) {

      rerrmsg = (RERRMsg*) dn->data;



      nwkmsg = new GenericNwkMsg;

      nwkmsg->type = RERR;

      nwkmsg->intermed = d->nodeID;

      nwkmsg->msg = rerrmsg;

            

      ttSendMsg(1, rerrmsg->receiver, nwkmsg, 12); 

    

      tmp = dn;

      dn = (DataNode*) dn->getNext();

      tmp->remove();

      delete tmp;

        

      ttSleep(0.001);

      return 0.0001;    

    } else {

      return FINISHED; 

    } 

  case 4:    

    ttSetNextSegment(3);

    return 0.0001; 

  }

  

  return FINISHED; // to supress compilation warnings

}

