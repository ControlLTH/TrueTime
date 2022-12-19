/*
 * Copyright (c) 2016 Lund University
 *
 * Written by Anton Cervin, Dan Henriksson and Martin Ohlin,
 * Department of Automatic Control LTH, Lund University, Sweden.
 *   
 * This file is part of TrueTime 2.0.
 *
 * TrueTime 2.0 is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * TrueTime 2.0 is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TrueTime 2.0. If not, see <http://www.gnu.org/licenses/>
 */

#include "ttnetwork.h"

#include "tinymt32.c"


// Some function declarations
static int backoff_802_11(RTnetwork *nwsys, int nbr);
static void updateSignalLevels(RTnetwork *nwsys, int sender, double level);
static void send_message(RTnetwork *nwsys, NWmsg *m, double *nextHit);

// ------- Network functions -------

/**
 * Computes the remaining transmission time of the current frame
 */
static double remxtime(RTnetwork *nwsys, NWmsg *m) {
  return m->remaining / nwsys->datarate;
}


/**
 * This is really the polynomial log(normcdf(-sqrt(x)))
 * FIX use a better approximation.
 * (I have one implemented in a matlab script which is really good)
 */
double normcdf[] = {-7.68752641000273e-06,
		    0.000296583930755564,
		    -0.00465568357346315,
		    0.0382996005645967,
		    -0.17800427895166,
		    0.478799208304118,
		    -1.33239150909366,
		    -0.850535634159434};

/**
 * Computes the value of the polynomial poly evaluated in x
 * poly is of order order
 */
static double polyval(double poly[], int order, double x){
  int i;
  double value = 0;

  if ( x>20 ){
    return 0;
  }
    
  for (i=0;i<=order;i++){
    value += poly[i]*pow(x, (double)(order-i));
  }
  return exp(value);
}


/**
 * Decide if the message is lost due to to many bit errors.
 * The snr is computed using data from the worst moment during the 
 * message transmission time
 */
static int snr_ok(RTnetwork *nwsys, NWmsg *m){
  double snr = nwsys->nwnodes[m->receiver]->signallevels[m->sender]/(m->maximum_disturbance - nwsys->nwnodes[m->receiver]->signallevels[m->sender]);
  // BPSK on 1Mbit/s, QPSK on 2Mbit/s, CCK on higher...
  snr *= 2; //BPSK coding ==>2* QPSK ==>1* CCK ==>???
  // The noise from other transmitters is not white so the chip stuff is
  // commented out
  //snr = snr*11; // 11-chip

  debugPrintf("snr: %f at time % f\n", snr, nwsys->time);
  
  // integrate the normal distribution to get the probability
  double ber = polyval(normcdf, 7, snr);
  debugPrintf("BER: %f at time % f\n", ber, nwsys->time);

  if ( ber>0 ){
    // Calculate the probability that we have such a low bit error rate
    // that the coding can handle it
    // page 192 in Sannolikhetsteori med till?mpningar av Blom

    double temp = nwsys->error_threshold*m->length - m->length*ber;
    double success_probability;
    if (temp > 0){
      success_probability = 1-polyval(normcdf, 7, pow((temp)/sqrt(m->length*ber*(1-ber)), 2.0));
    }
    else{
      success_probability = polyval(normcdf, 7, pow((fabs(temp))/sqrt(m->length*ber*(1-ber)), 2.0));
    }

    debugPrintf("bit error rate is below %f with probability %f\n", nwsys->error_threshold, success_probability);

    if (unirand(nwsys) > success_probability ){
      return 0;
    }
  }
  return 1;
}


/**
 * Duplicate message and append to inputQ's of all nodes
 * There is no ACK from the receiver when using broadcast messages
 */
void broadcast(RTnetwork *nwsys, NWmsg *m, double *nextHit) {
  int j;
  NWmsg *m2;
  mxArray* data = NULL;

  m->type = BROADCAST;

  // important to do before calling send_message for broadcast
  updateSignalLevels(nwsys, m->sender, nwsys->nwnodes[m->sender]->transmitPower);

  if (m->dataMATLAB != NULL) {
    data = m->dataMATLAB;
  }

  for (j=0; j<nwsys->nbrOfNodes; j++) {
    if (j != m->sender) {
      // Duplicate message the message nbrOfNodes - 1 times
      m2 = new NWmsg();
      *m2 = *m;
      m2->receiver = j;
      if (m->dataMATLAB != NULL) {
	m2->dataMATLAB = mxDuplicateArray(data);
	mexMakeArrayPersistent(m2->dataMATLAB);
      }
      debugPrintf("broadcasting to node %d\n", m2->receiver+1);
      send_message(nwsys, m2, nextHit);
    }
  }
  
  nwsys->nwnodes[m->sender]->preprocQ->removeNode(m);
  
  delete m; // delete the original message
  m = NULL;
}


/**
 *
 */
static void send_message(RTnetwork *nwsys, NWmsg *m, double *nextHit){
  int j;
  NWmsg *m2;

  //double *values = mxGetPr(nwsys->nbrOfTransmissions);
  //values[m->sender + m->receiver*nwsys->nbrOfNodes] += 1;

  debugPrintf("moving message from node %d to node %d from preprocQ to inputQ at %f\n",m->sender+1, m->receiver+1, nwsys->time);
  m->remaining = fmax(m->length, nwsys->minsize);
  m->collided = 0;
  m->maximum_disturbance = 0;
  // The message is removed in broadcast(...) if of broadcast type
  if (m->type!=BROADCAST){
    nwsys->nwnodes[m->sender]->preprocQ->removeNode(m);
  }
  nwsys->nwnodes[m->receiver]->inputQ->appendNode(m);
  nwsys->nwnodes[m->sender]->state = W_SENDING;
  debugPrintf("remaining %f in node %d\n",m->remaining, m->sender+1);

  // can't send and listen at the same time
  // If the receiver is sending then he can not receive at the same time
  if ( nwsys->nwnodes[m->receiver]->state == W_SENDING ) {
    m->collided = 1;
    debugPrintf("can not send and receive at the same time (node %d), message from %d\n", m->receiver+1, m->sender+1);
  } 

  // Update the signal level in all nodes
  // This is done once in broadcast(...) for messages of broadcast type
  if (m->type!=BROADCAST){
    updateSignalLevels(nwsys, m->sender, nwsys->nwnodes[m->sender]->transmitPower);
  }


  // Does our sender have lower signal strength in the receiver
  // than another sender? Then our message is collided.
  // (no problem with broadcast here)
  for (j=0; j<nwsys->nbrOfNodes; j++) {
    if ( j!=m->sender && nwsys->nwnodes[m->receiver]->signallevels[j]>=nwsys->nwnodes[m->receiver]->signallevels[m->sender] ){
      m->collided = 1;
      debugPrintf("Collision with another message in receiver (node %d) at time %f\n", m->receiver+1, nwsys->time);
      debugPrintf("\t marking messages in %d from (node %d) as collided\n",j+1, m->sender+1);
      break;
    }
  }
	    	    
  // Check if our transmission interfers with message transmissions 
  // in other nodes.
  // We should _not_ mark our own message here
  for (j=0; j<nwsys->nbrOfNodes; j++) {
    m2 = (NWmsg *)nwsys->nwnodes[j]->inputQ->getFirst();
    while (m2 != NULL) {
      // (potential problem with broadcast is solved with 
      //    m->sender != m2->sender)
      // m->sender != m2->sender is also needed to make sure that we 
      //    do not mark message:1 as collided with message:1.
      if (m->sender != m2->sender && nwsys->nwnodes[j]->signallevels[m2->sender]<=nwsys->nwnodes[j]->signallevels[m->sender]){
	debugPrintf("marking messages in %d from (node %d) as collided at time %f\n",j+1, m2->sender+1, nwsys->time);
	m2->collided = 1;
      }

      // Update the maximum of total_received_power during
      // the message transmission
      if ( nwsys->nwnodes[m2->receiver]->total_received_power>=m2->maximum_disturbance ){
	m2->maximum_disturbance = 
	  nwsys->nwnodes[m2->receiver]->total_received_power;
      }
		
      m2 = (NWmsg *)m2->getNext();
    }
  }

  // when is the transmission finished?
  if (nwsys->time + remxtime(nwsys, m) < *nextHit) {
    *nextHit = nwsys->time + remxtime(nwsys, m);
  }
}


/**
 * Puts the message in the correct que with the correct delay
 * so that it later on can be sent with send_messag(...)
 */
static void resend_message(RTnetwork *nwsys, NWmsg *m, double *waituntil){
  NWmsg *m2;

  nwsys->nwnodes[m->sender]->nbrcollisions++;
	  
  if ( m->type!=BROADCAST && 
       nwsys->nwnodes[m->sender]->nbrcollisions <= nwsys->retrylimit ) {
    debugPrintf("message to (node %d) from (node %d) put in preProcQ again\n", m->receiver+1, m->sender+1);
    // put it in preprocQ with a suitable backoff time
    // FIX implement insertFirst in linkedList.cpp to get rid of this
    m2 = (NWmsg *)nwsys->nwnodes[m->sender]->preprocQ->getFirst();
    if ( m2 ) { // if there are messages in the queue
      nwsys->nwnodes[m->sender]->preprocQ->insertBefore(m, m2);
    } else {
      nwsys->nwnodes[m->sender]->preprocQ->appendNode(m);
    }
	  
    nwsys->nwnodes[m->sender]->state = W_WAITING;

    switch (nwsys->type) {
	
    case NCM_WIRELESS:
    case _802_11:
      // FIX put in a function
      nwsys->nwnodes[m->sender]->backoff =  
	SLOTTIME_802_11*backoff_802_11(nwsys,nwsys->nwnodes[m->sender]->nbrcollisions);
      break;
    case _802_15_4:
      // 802.15.4 does not have additional backoff because it is the second,
      // third (and so on) time we resend the message.
      nwsys->nwnodes[m->sender]->backoff = 0;
      break;
    default:
      mexPrintf("Protocol not implemented!\n");
      break;
    }

    debugPrintf("node %d chooses a backoff of %f slots at time %f\n",m->sender+1, nwsys->nwnodes[m->sender]->backoff/SLOTTIME_802_11, nwsys->time);
	    
    // Add the time we wait for an ACK message
    // Use the functionality normally used for the predelay
    m->waituntil = nwsys->time + nwsys->acktimeout; 

    // reset the remaining number of bits
    m->remaining = fmax(m->length, nwsys->minsize);
    *waituntil = nwsys->time + nwsys->nwnodes[m->sender]->backoff; 
  } else { // We have resent enough times already
    // or it is a broadcast message
    // Drop the message
    debugPrintf("The message from (node %d) to (node %d) has been dropped at time %f\n",m->sender+1, m->receiver+1, nwsys->time);
    nwsys->nwnodes[m->sender]->nbrcollisions = 0;
    nwsys->nwnodes[m->sender]->state = W_IDLE;
    // Delete message 
    delete m;
    m = NULL;
  }
}

/**
 * x1, x position of the sending node
 * y1, y position of the sending node
 * x2, x position of the receiving node
 * y2, y position of the receiving node
 * transmitPower, amount of Watts that the sending node is sending with
 */


static double c_pathloss(RTnetwork *nwsys, double transmitPower, int node1, double x1, double y1, int node2, double x2, double y2){

  double weight = 1.0;
  double distance = sqrt(pow(x1 - x2, 2.0) + pow(y1 - y2, 2.0));
  double rNbr = unirand(nwsys);
  if (rNbr < nwsys->lossprob) {
    weight = 0.0;
  }
  
  debugPrintf("distance is %f m\n",distance);

  return weight*transmitPower/pow(distance+1, nwsys->pathloss);
}


/**
 * x1, x position of the sending node
 * y1, y position of the sending node
 * x2, x position of the receiving node
 * y2, y position of the receiving node
 * transmitPower, amount of Watts that the sending node is sending with
 */
static double matlab_pathloss(RTnetwork *nwsys, double transmitPower, int node1, double x1, double y1, int node2, double x2, double y2){
  double retval;
  mxArray *lhs[1];
  mxArray *rhs[8];

  rhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
  *mxGetPr(rhs[0]) = transmitPower;
  rhs[1] = mxCreateDoubleMatrix(1,1,mxREAL);
  *mxGetPr(rhs[1]) = node1;
  rhs[2] = mxCreateDoubleMatrix(1,1,mxREAL);
  *mxGetPr(rhs[2]) = x1;
  rhs[3] = mxCreateDoubleMatrix(1,1,mxREAL);
  *mxGetPr(rhs[3]) = y1;
  rhs[4] = mxCreateDoubleMatrix(1,1,mxREAL);
  *mxGetPr(rhs[4]) = node2;
  rhs[5] = mxCreateDoubleMatrix(1,1,mxREAL);
  *mxGetPr(rhs[5]) = x2;
  rhs[6] = mxCreateDoubleMatrix(1,1,mxREAL);
  *mxGetPr(rhs[6]) = y2;
  rhs[7] = mxCreateDoubleMatrix(1,1,mxREAL);
  *mxGetPr(rhs[7]) = nwsys->time;

  lhs[0] = NULL;     // needed not to crash Matlab after an error
  if (mexCallMATLAB(1, lhs, 8, rhs, nwsys->codeName) != 0) {
    mexPrintf("Error while trying to call the user specified pathloss function\n");
    return 0.0;
  }
  
  retval = *mxGetPr(lhs[0]);

  mxDestroyArray(lhs[0]);
  
  return retval;
}


/**
 * updates the signal levels of the sending node in all other nodes.
 *
 * sender, the sending node.
 * level, amount of Watts that the sending node is sending with
 */
static void updateSignalLevels(RTnetwork *nwsys, int sender, double transmitPower){
  int i;
  double oldvalue;
  
  double x_sender = nwsys->nwnodes[sender]->xCoordinate;
  double y_sender = nwsys->nwnodes[sender]->yCoordinate;

  for (i=0; i<nwsys->nbrOfNodes; i++) {
    oldvalue = nwsys->nwnodes[i]->signallevels[sender];
    
    if (transmitPower == 0){
      nwsys->nwnodes[i]->signallevels[sender] = 0;
    } else {
      nwsys->nwnodes[i]->signallevels[sender] = nwsys->pathlossfun(nwsys, transmitPower, sender, x_sender, y_sender, i, nwsys->nwnodes[i]->xCoordinate, nwsys->nwnodes[i]->yCoordinate);
    }

    nwsys->nwnodes[i]->total_received_power += 
      nwsys->nwnodes[i]->signallevels[sender] - oldvalue;
  }
}

/**
 * 
 * Computes a backoff time in the interval [0,CW]
 *
 * [2^CWMIN-1, CWMAX]
 */
static int backoff_802_11(RTnetwork *nwsys, int nbr){
  // For some reason I have decided to increase nbr before this function is
  // called. Therefore CWMIN-1
  int collisionWindow = (1<<(nbr+CWMIN_802_11-1))-1;
  if ( collisionWindow>CWMAX_802_11 )
    collisionWindow = CWMAX_802_11;
  debugPrintf("\t using collision window of size %d at time %f\n", collisionWindow, nwsys->time);
  return urandint(nwsys, 0, collisionWindow);
}



static void MAC_802_11(RTnetwork *nwsys, double *nextHit){
  NWmsg *m;
  int i;

  // New transmissions
  // Check if messages have finished waiting in the preprocQ's
  // that is should we start sending them now?
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    debugPrintf("\t Checking preProcQ of node %d at %f\n",i+1, nwsys->time);
    m = (NWmsg *)nwsys->nwnodes[i]->preprocQ->getFirst();
    if (m != NULL) {
      // Have we waited the predelay? or in case of ACK
      // Have we waited the ACKTimeout?
      if (m->waituntil - nwsys->time < TT_TIME_RESOLUTION ) {
	debugPrintf("waited the preproc (ACKTimeout) time (node %d) at %f\n",m->sender+1, nwsys->time);	
	// Set the waiting flag if we are not already sending
	if (nwsys->nwnodes[m->sender]->state != W_SENDING){
	  debugPrintf("Already sending in (node %d) at time %f\n", m->sender+1, nwsys->time);
	  nwsys->nwnodes[m->sender]->state = W_WAITING;
	}

	// no signal during the last DIFS seconds?
	if ( nwsys->nwnodes[i]->lastused + DIFS_802_11 <= nwsys->time + TT_TIME_RESOLUTION) {
	  debugPrintf("the net is free in node %d\n", i+1);
	  // If collided before, countdown the backoff timer
	  // This will not count down the backoff of a message if the message has been marked as collided in this run as the network has then been active in that node during the DIFS last seconds, and that is correct.
	  if ( nwsys->nwnodes[i]->backoff > 0 ){
	    debugPrintf("counting down the backoff (node %d)\n",i+1);
	    // We must remember when we last counted down
	    if ( nwsys->nwnodes[i]->lastbackoffcount > 
		 nwsys->nwnodes[i]->lastused ){
	      nwsys->nwnodes[i]->backoff -= 
		nwsys->time - nwsys->nwnodes[i]->lastbackoffcount;
	    } else {
	      nwsys->nwnodes[i]->backoff -= 
		nwsys->time - nwsys->nwnodes[i]->lastused - DIFS_802_11;
	    }
	    // save the last time we decreased the backoff counter
	    nwsys->nwnodes[i]->lastbackoffcount = nwsys->time;
	  } // end backoff countdown

	  // Start sending
	  // If we have waited enough backoff time (or no collission)
	  if ( nwsys->nwnodes[i]->backoff < TT_TIME_RESOLUTION ){
	    if (m->receiver == -1){ // -1 <==> broadcast
	      broadcast(nwsys, m, nextHit);
	    } else{
	      send_message(nwsys, m, nextHit);
	    }
	  } else { // more backoff time to wait
	    if (nwsys->time + nwsys->nwnodes[i]->backoff < *nextHit) {
	      *nextHit = nwsys->time + nwsys->nwnodes[i]->backoff;
	    }
       	  }
	} else { // signal during the last DIFS seconds (maybe at this moment)
	  // If we sense transmissions, we must wait at least DIFS seconds
	  // before we can do anything.
	  // If no transmission is finished at this exact time, then we must
	  // wait until the next transmission is finished before we
	  // can decide what to do. When the next transmission is finished we will run anyway, so we do not have to think about that here.

	  // This if clause is labeled BETTER and is needed together with
	  // the other one where BETTER is mentioned
	  // Om det varit aktivitet inom de senaste DIFS sekunderna
	  // och den aktiviteten tagit slut nu s? ska vi snart k?ra igen
	  // eftersom det ?r m?jligt att vi kan f? s?nda
	  if ( (nwsys->nwnodes[i]->lastused < nwsys->time) && 
	       (nwsys->nwnodes[i]->lastused + DIFS_802_11 < *nextHit) ) {
  	    *nextHit = nwsys->nwnodes[i]->lastused + DIFS_802_11;
  	  }
	}

	// We have waited the predelay and we have waited the ACKTimeout
	// We may not have waited the backoff time though

	// According to the standard, we should choose a backoff
	// time if the net is busy at this moment and we want to send.
	// This is to avoid that all stations start to send at the same time
	// after a transmission.
	if ( nwsys->nwnodes[i]->lastused == nwsys->time &&
	     nwsys->nwnodes[i]->backoff < TT_TIME_RESOLUTION){
	  // Maybe we should increase the CWmax for every backoff
	  nwsys->nwnodes[i]->backoff = SLOTTIME_802_11*backoff_802_11(nwsys,1); // 1 is to get [0,31]
	  debugPrintf("node %d senses an active net and chooses to backoff at time %f, backoff=%f\n", i+1, nwsys->time, nwsys->nwnodes[i]->backoff);
	}


      } else { // not finished waiting the predelay
	// update nextHit?
	if (m->waituntil < *nextHit) {
	  *nextHit = m->waituntil;
	}
      }
    } // if (m != NULL)
  } //for (i=0; i<nwsys->nbrOfNodes; i++)

}

static void MAC_802_11_NCM(RTnetwork *nwsys, double *nextHit){
  NWmsg *m;
  int i;
	
  // New transmissions
  // Check if messages have finished waiting in the preprocQ's
  // that is should we start sending them now?
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    debugPrintf("\t Checking preProcQ of node %d at %f\n",i+1, nwsys->time);
		
    if (nwsys->type == NCM_WIRELESS ) {
      if (nwsys->nextNode == -1) {
	printf ("ttnetwork: invalid next node %d\n",nwsys->nextNode); 
	nwsys->rrturn = 0;
      }  else {
	debugPrintf("ACCESSED");
	nwsys->rrturn = nwsys->nextNode;
      }
    } else {
      nwsys->rrturn = (nwsys->rrturn + 1) % nwsys->nbrOfNodes;
    }
		
		
    m = (NWmsg *)nwsys->nwnodes[i]->preprocQ->getFirst();
    if (m != NULL) {
      // Have we waited the predelay? or in case of ACK
      // Have we waited the ACKTimeout?
      if (m->waituntil - nwsys->time < TT_TIME_RESOLUTION ) {
	debugPrintf("waited the preproc (ACKTimeout) time (node %d) at %f\n",m->sender+1, nwsys->time);	
	// Set the waiting flag if we are not already sending
	if (nwsys->nwnodes[m->sender]->state != W_SENDING){
	  debugPrintf("Already sending in (node %d) at time %f\n", m->sender+1, nwsys->time);
	  nwsys->nwnodes[m->sender]->state = W_WAITING;
	}
				
	// no signal during the last DIFS seconds?
	if ( nwsys->nwnodes[i]->lastused + DIFS_802_11 <= nwsys->time + TT_TIME_RESOLUTION) {
	  debugPrintf("the net is free in node %d\n", i+1);
	  // If collided before, countdown the backoff timer
	  // This will not count down the backoff of a message if the message has been marked as collided in this run as the network has then been active in that node during the DIFS last seconds, and that is correct.
	  if ( nwsys->nwnodes[i]->backoff > 0 ){
	    debugPrintf("counting down the backoff (node %d)\n",i+1);
	    // We must remember when we last counted down
	    if ( nwsys->nwnodes[i]->lastbackoffcount > 
		 nwsys->nwnodes[i]->lastused ){
	      nwsys->nwnodes[i]->backoff -= 
		nwsys->time - nwsys->nwnodes[i]->lastbackoffcount;
	    } else {
	      nwsys->nwnodes[i]->backoff -= 
		nwsys->time - nwsys->nwnodes[i]->lastused - DIFS_802_11;
	    }
	    // save the last time we decreased the backoff counter
	    nwsys->nwnodes[i]->lastbackoffcount = nwsys->time;
	  } // end backoff countdown
					
	  // Start sending
	  // If we have waited enough backoff time (or no collission)
	  if ( nwsys->nwnodes[i]->backoff < TT_TIME_RESOLUTION ){
	    if (m->receiver == -1){ // -1 <==> broadcast
	      broadcast(nwsys, m, nextHit);
	    } else{
	      send_message(nwsys, m, nextHit);
	    }
	  } else { // more backoff time to wait
	    if (nwsys->time + nwsys->nwnodes[i]->backoff < *nextHit) {
	      *nextHit = nwsys->time + nwsys->nwnodes[i]->backoff;
	    }
	  }
	} else { // signal during the last DIFS seconds (maybe at this moment)
	  // If we sense transmissions, we must wait at least DIFS seconds
	  // before we can do anything.
	  // If no transmission is finished at this exact time, then we must
	  // wait until the next transmission is finished before we
	  // can decide what to do. When the next transmission is finished we will run anyway, so we do not have to think about that here.
					
	  // This if clause is labeled BETTER and is needed together with
	  // the other one where BETTER is mentioned
	  // Om det varit aktivitet inom de senaste DIFS sekunderna
	  // och den aktiviteten tagit slut nu s? ska vi snart k?ra igen
	  // eftersom det ?r m?jligt att vi kan f? s?nda
	  if ( (nwsys->nwnodes[i]->lastused < nwsys->time) && 
	       (nwsys->nwnodes[i]->lastused + DIFS_802_11 < *nextHit) ) {
	    *nextHit = nwsys->nwnodes[i]->lastused + DIFS_802_11;
	  }
	}
				
	// We have waited the predelay and we have waited the ACKTimeout
	// We may not have waited the backoff time though
				
	// According to the standard, we should choose a backoff
	// time if the net is busy at this moment and we want to send.
	// This is to avoid that all stations start to send at the same time
	// after a transmission.
	if ( nwsys->nwnodes[i]->lastused == nwsys->time &&
	     nwsys->nwnodes[i]->backoff < TT_TIME_RESOLUTION){
	  // Maybe we should increase the CWmax for every backoff
	  nwsys->nwnodes[i]->backoff =  
	    SLOTTIME_802_11*backoff_802_11(nwsys,1); // 1 is to get [0,31]
	  debugPrintf("node %d senses an active net and chooses to backoff at time %f, backoff=%f\n", i+1, nwsys->time, nwsys->nwnodes[i]->backoff);
	}
				
				
      } else { // not finished waiting the predelay
	// update nextHit?
	if (m->waituntil < *nextHit) {
	  *nextHit = m->waituntil;
	}
      }
    } // if (m != NULL)
  } //for (i=0; i<nwsys->nbrOfNodes; i++)
	
}


static void MAC_802_15_4(RTnetwork *nwsys, double *nextHit){
  NWmsg *m = NULL;
  int i;

#define macMinBE 0
#define aMaxBE 5
#define aUnitBackoffPeriod 20
#define macMaxCSMABackoffs 4

#define symbolTime 1.6e-5

#define SIFS 12
#define LIFS 40
#define IFS SIFS

  // NB, Number of Backoffs, initially set to 0
  // BE, Backoff Exponent, initially set to macMinBE=3
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    // Are we already sending something?
    if ( nwsys->nwnodes[i]->state == W_SENDING ){
      continue;
    }

    m = (NWmsg *)nwsys->nwnodes[i]->preprocQ->getFirst();
    // Do we have anything to send?
    if ( m == NULL ){
      continue;
    }

    // Have we waited the predelay, or in case of ACK
    // have we waited the ACKTimeout?
    if (m->waituntil - nwsys->time > TT_TIME_RESOLUTION ) {
      // When are we finished waiting?
      if ( m->waituntil < *nextHit) {
	*nextHit = m->waituntil;
      }     
      continue;
    }

    // Have we waited the backoff time?
    // In ZigBee as opposed to 802.11 the backoff time is always
    // decremented. Therefore we can (and do) store it as an absolute time.
    if ( nwsys->nwnodes[i]->backoff - nwsys->time > TT_TIME_RESOLUTION ){
      // When are we finished waiting?
      if ( nwsys->nwnodes[i]->backoff < *nextHit) {
	*nextHit = nwsys->nwnodes[i]->backoff;
      }
      continue;
    }
    
    // Ok, we have a message that we should try to send/resend now

    // Show that we want to send
    nwsys->nwnodes[m->sender]->state = W_WAITING;    

    // In ZigBee we shall always start by waiting a random time 
    // in the interval [0,2^BE-1]*aUnitBakoffPeriod*time_per_symbol
    // before we even try to access the net.
    if ( m->nbrOfBackoffs == -1 ){
      m->nbrOfBackoffs = 0;
      nwsys->nwnodes[i]->backoff = 
	urandint(nwsys, 0, (1<<macMinBE) - 1)*aUnitBackoffPeriod*symbolTime + nwsys->time;
      debugPrintf("Initial backoff: %f at time: %f in node: %d\n", nwsys->nwnodes[i]->backoff, nwsys->time, i+1);
      // update nextHit
      if ( nwsys->nwnodes[i]->backoff < *nextHit) {
	*nextHit = nwsys->nwnodes[i]->backoff;
      }
      continue;
    }

    // Perform CCA
    if ( nwsys->nwnodes[i]->lastused + IFS*symbolTime - nwsys->time <= TT_TIME_RESOLUTION) {
      m->nbrOfBackoffs = 0;     // Start all over again
      if (m->receiver == -1){ // -1 <==> broadcast
	broadcast(nwsys, m, nextHit);
      } else{
	send_message(nwsys, m, nextHit);
      }

      continue;
    }

    debugPrintf("%s:%d\n",__FUNCTION__,__LINE__);

    // The net is not free at this moment, we have to backoff
    m->nbrOfBackoffs++;

    // We have tried to send enough times
    // Drop the package
    if ( m->nbrOfBackoffs > macMaxCSMABackoffs ){
      nwsys->nwnodes[i]->nbrcollisions = 0;
      nwsys->nwnodes[i]->state = W_IDLE;
      debugPrintf("* Dropping message from node:%d to node:%d, at time %f (maximum CSMA backoffs reached)\n", m->sender+1, m->receiver+1, nwsys->time);
      nwsys->nwnodes[i]->preprocQ->deleteNode(m);
      continue;
    }

    // Choose a suitable backoff time
    int temp = fmin(macMinBE + m->nbrOfBackoffs, aMaxBE);
    nwsys->nwnodes[i]->backoff = 
      urandint(nwsys, 0, 1<<temp) * aUnitBackoffPeriod * symbolTime + nwsys->time;
    debugPrintf("\tBackoff: %f at time: %f in node: %d\n", nwsys->nwnodes[i]->backoff, nwsys->time, i+1);
    if ( nwsys->nwnodes[i]->backoff < *nextHit) {
      *nextHit = nwsys->nwnodes[i]->backoff;
    }
  } // for (i=0; i<nwsys->nbrOfNodes; i++) {
}

/**
 * Returns the time of the next invocation (nextHit)
 */
static double runNetwork(RTnetwork *nwsys) {
  int i, j;
  NWmsg *m, *m3;  // m is our message
  NWmsg *next;
  double timeElapsed;
  double nextHit = nwsys->time + TT_MAX_TIMESTEP;
  double waituntil;

  debugPrintf("%s time:%f\n", __FUNCTION__, nwsys->time);

  timeElapsed = nwsys->time - nwsys->prevHit; // time since last invocation
  nwsys->prevHit = nwsys->time;

  debugPrintf("Running network at %f\n", nwsys->time);

  // Update last usage time of the network in every node (802.11)
  // The nodes own signal strength is also taken into account
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    for (j=0; j<nwsys->nbrOfNodes; j++) {
      if ( nwsys->nwnodes[i]->signallevels[j] >= nwsys->receiverThreshold ){
	//This printout could be misguiding, 
	//there could be more sending stations we only print one
	debugPrintf("action in node %d from node %d at time %f (%f Watt)\n", i+1, j+1, nwsys->time, nwsys->nwnodes[i]->signallevels[j]);
	nwsys->nwnodes[i]->lastused = nwsys->time;
	debugPrintf("Node %d senses %f Watt %f dbm\n", i+1, nwsys->nwnodes[i]->total_received_power, 10*log10(nwsys->nwnodes[i]->total_received_power*1000));
	break;
      }
    }
  }

  // Check for finished transmisssions in every node
  // Transmissions to a node are in parallell, therefore
  // we must check everything in every nodes inputQ
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst();
    while (m != NULL) {
      m3 = (NWmsg *)m->getNext(); //very important because we may remove m
      //from the appropriate list below...
      // decrease remaining number of bits in current frame
      m->remaining -= (nwsys->datarate * timeElapsed);
      debugPrintf("Counting down transmission time of node %d\n",m->sender+1);
      debugPrintf("\t remaining %f\n",m->remaining);
      // frame finished?
      if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {
	debugPrintf("Transmission finished\n");
	nwsys->nwnodes[m->receiver]->inputQ->removeNode(m);
	// If the message has collided, then resend
	// or if the message has not reached the receiver, also resend
	// from the senders perspective they do not differ
	if (m->collided || nwsys->nwnodes[i]->signallevels[m->sender] < 
	    nwsys->receiverThreshold){
	  debugPrintf("want to resend message from node %d to node %d at time %f\n",m->sender+1, m->receiver+1, nwsys->time);
	  resend_message(nwsys, m, &waituntil);
	} else { // no collision and the signal reached us
	  // is the snr good enough to decode the message?
	  if ( snr_ok(nwsys, m) ){
	    // Save the signalPower so that the node can see it
	    m->signalPower = 10*log10(nwsys->nwnodes[m->receiver]->signallevels[m->sender]*1000);
	    // transmission has finished successfully, move to outputQ
	    nwsys->nwnodes[m->receiver]->outputQ->appendNode(m);
	    debugPrintf("* message put in outputQ (node %d) from (node %d) at time %f\n", m->receiver+1, m->sender+1, nwsys->time);
	    m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay;
	    waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay;
	    // this should maybe only be done once when broadcasting
	    nwsys->nwnodes[m->sender]->state = W_IDLE;
	    debugPrintf("max distu %f\n",m->maximum_disturbance);
	    nwsys->nwnodes[m->sender]->nbrcollisions = 0;
	  } else{
	    resend_message(nwsys, m, &waituntil);
	    debugPrintf("packet lost due to to large BER in node %d from node %d at time\n", m->receiver, m->sender+1, nwsys->time);
	  }
	} // end of no collision and the signal reached us

	// update nextHit?
	if ( waituntil < nextHit ) {
	  nextHit = waituntil;
	}

	// This should only be done once for every broadcast message
	// m->receiver <==> i
	// if broadcast, only decrement the signalpower for the last message
	// that is the receiver == last node or if the sender == last node then 
	// it is the second last node.
	if (m->type!=BROADCAST || m->receiver==nwsys->nbrOfNodes-1 ||
	    (m->sender == nwsys->nbrOfNodes-1 && 
	     m->receiver == nwsys->nbrOfNodes-2) ){
	  updateSignalLevels(nwsys, m->sender, 0);
	}

	switch (nwsys->type) {
	
	case NCM_WIRELESS:
	case _802_11:
	  // If a transmission is finished, then there _may_ be someone waiting 
	  // to send after DIFS seconds
	  // this if clause is needed together with the one labeled BETTER in
	  // the MAC_802_11
	  if ( nextHit > nwsys->time + DIFS_802_11) {
	    nextHit = nwsys->time + DIFS_802_11;
	  }
	  break;
	default:
	  break;
	}
      } else { // compute when the transmission is finished
	if (nwsys->time + remxtime(nwsys, m) < nextHit) {
	  nextHit = nwsys->time + remxtime(nwsys, m);
	  debugPrintf("Transmission from (node %d) to (node %d) is finished at %f\n",m->sender,m->receiver,nwsys->time + remxtime(nwsys, m));
	}	  
      }
      
      m = m3; //See the comment above
    } //while (m != NULL)
  } //for (i=0; i<nwsys->nbrOfNodes; i++)
  
  switch (nwsys->type) {
  case _802_11:
    MAC_802_11(nwsys, &nextHit);
    break;
  case _802_15_4:
    MAC_802_15_4(nwsys, &nextHit);
    break;
  case NCM_WIRELESS:
    MAC_802_11_NCM(nwsys, &nextHit);
    break;
  default:
    mexPrintf("Protocol not implemented!\n");
    break;
  }

  // Check if messages have finished waiting in the outputQ's
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    m = (NWmsg *)nwsys->nwnodes[i]->outputQ->getFirst();
    while (m != NULL) {
      next = (NWmsg *)m->getNext();
      if (m->waituntil - nwsys->time < TT_TIME_RESOLUTION) {
	// finished waiting, move to postprocQ
	debugPrintf("*** moving message from outputQ (node %d) to postprocQ at %f\n", i+1, nwsys->time);
	nwsys->nwnodes[i]->outputQ->removeNode(m);
	nwsys->nwnodes[i]->postprocQ->appendNode(m);
	if (nwsys->outputs[i] == 0.0) {
	  nwsys->outputs[i] = 1.0; // trigger rcv output
	} else {
	  nwsys->outputs[i] = 0.0; // trigger rcv output
	}
	//	mexPrintf("\tmessage->receiver=%d, i=%d\n",m->receiver,i);
      } else {
	// update nextHit?
	if (m->waituntil < nextHit) {
	  nextHit = m->waituntil;
	}
      }
      m = next; // get next
    }
  }

  // done

  debugPrintf("Next hit scheduled for %f\n", nextHit);

  // produce output graph
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    if (nwsys->nwnodes[i]->state == W_SENDING) {
      nwsys->sendschedule[i] = i+1.5;  // sending
      nwsys->energyconsumption[i] = nwsys->nwnodes[i]->transmitPower; // sending
    } else if (nwsys->nwnodes[i]->state == W_WAITING) {
      nwsys->sendschedule[i] = i+1.25; // waiting
      nwsys->energyconsumption[i] = 0; // waiting
    } else if (nwsys->nwnodes[i]->state == W_IDLE){
      nwsys->sendschedule[i] = i+1.0;     // idle
      nwsys->energyconsumption[i] = 0;  // idle
    } else {
      //Something is wrong
      mexPrintf("Error creating schedule file:%s line:%d\n",__FILE__,__LINE__);
    }
  }

  return nextHit;
}

// ------- Simulink callback functions ------- 

#ifdef __cplusplus
extern "C" { // use the C fcn-call standard for all functions  
#endif       // defined within this scope   

#define S_FUNCTION_NAME ttwnetwork
#define S_FUNCTION_LEVEL 2

#include "simstruc.h"
  static void mdlInitializeSizes(SimStruct *S)
  {
    int nbrOfNodes;
    const mxArray *arg;

    ssSetNumSFcnParams(S, 15);  /* Number of expected parameters */
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
      return; /* Parameter mismatch will be reported by Simulink */
    }
  
    // Parse second argument only, to determine nbrOfNodes

    // 2 - Number of nodes
    arg = ssGetSFcnParam(S, 2);
    if (mxIsDoubleScalar(arg)) {
      nbrOfNodes = (int) *mxGetPr(arg);
    } else {
      ssSetErrorStatus(S, "TrueTime Network: The number of nodes must be an integer > 0");
      return;
    }
    
    if (nbrOfNodes < 1) {
      ssSetErrorStatus(S, "TrueTime Network: The number of nodes must be an integer > 0");
      return;
    }

    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);
  
    ssSetNumInputPorts(S, 3);
    ssSetInputPortDirectFeedThrough(S, 0, 0);
    ssSetInputPortWidth(S, 0, nbrOfNodes);
    ssSetInputPortWidth(S, 1, nbrOfNodes);
    ssSetInputPortWidth(S, 2, nbrOfNodes);
  
    ssSetNumOutputPorts(S, 3);
    ssSetOutputPortWidth(S, 0, nbrOfNodes);
    ssSetOutputPortWidth(S, 1, nbrOfNodes);
    ssSetOutputPortWidth(S, 2, nbrOfNodes); // Energy consumption

    ssSetNumSampleTimes(S, 1);
  
    ssSetNumRWork(S, 0);
    ssSetNumIWork(S, 0);
    ssSetNumPWork(S, 0); 
    ssSetNumModes(S, 0);
    ssSetNumNonsampledZCs(S, 1);

    // Make sure cleanup is performed even if errors occur
    ssSetOptions(S, SS_OPTION_CALL_TERMINATE_ON_EXIT);

    int i;

    // Create new network struct
    RTnetwork *nwsys = new RTnetwork;
    ssSetUserData(S, nwsys); // save pointer in UserData

    // 1 - Network type
    arg = ssGetSFcnParam(S, 0);
    if (mxIsDoubleScalar(arg)) {
      nwsys->type = (int)(*mxGetPr(arg))-1;
      if ( nwsys->type==0 )
	nwsys->type = _802_11;
      else if (nwsys->type==1)
	nwsys->type = _802_15_4;
      else if (nwsys->type==2)
	nwsys->type = NCM_WIRELESS;	  
      else
	mexPrintf("Protocol not implemented!\n");
    }
    if (nwsys->type < 0) {
      ssSetErrorStatus(S, "Error in type argument");
      return;
    }
    debugPrintf("type = %d\n", nwsys->type);

    // 2 - Network Number
    arg = ssGetSFcnParam(S, 1);
    if (mxIsDoubleScalar(arg)) {
      nwsys->networkNbr = (int) *mxGetPr(arg);
    }
    debugPrintf("networkNbr = %d\n", nwsys->nbrOfNodes);

    // 3 - Number of nodes
    arg = ssGetSFcnParam(S, 2);
    if (mxIsDoubleScalar(arg)) {
      nwsys->nbrOfNodes = (int) *mxGetPr(arg);
    }
    debugPrintf("nbrOfNodes = %d\n", nwsys->nbrOfNodes);

    // 4 - Data rate (bits/s)
    arg = ssGetSFcnParam(S, 3);
    if (mxIsDoubleScalar(arg)) {
      nwsys->datarate = *mxGetPr(arg);
    }
    if (nwsys->datarate < 0.0) {
      ssSetErrorStatus(S, "The data rate must be > 0");
      return;
    }
    debugPrintf("datarate = %f\n", nwsys->datarate);

    // 5 - Minimum frame size
    arg = ssGetSFcnParam(S, 4);
    if (mxIsDoubleScalar(arg)) {
      nwsys->minsize = (int) *mxGetPr(arg);
    }
    if (nwsys->minsize < 0) {
      ssSetErrorStatus(S, "The minimum frame size must be >= 0");
      return;
    }

    double temp1 = 0.0;
    double transmitPowerWatt = 0.0;
    // 6 - Transmit Power
    arg = ssGetSFcnParam(S, 5);
    if (mxIsDoubleScalar(arg)) {
      transmitPowerWatt = pow(10.0, *mxGetPr(arg)/10.0)/1000.0;
      temp1 = *mxGetPr(arg);
    }

    double temp2 = 0.0;
    // 7 - Receiver Signal Threshold
    arg = ssGetSFcnParam(S, 6);
    if (mxIsDoubleScalar(arg)) {
      nwsys->receiverThreshold = pow(10.0, *mxGetPr(arg)/10.0)/1000.0;
      temp2 = *mxGetPr(arg);
    }

    // 8 - Path Loss Exponent
    arg = ssGetSFcnParam(S, 7);
    if (mxIsDoubleScalar(arg)) {
      nwsys->pathloss = *mxGetPr(arg);
    }
    if (nwsys->pathloss <= 0.0 ) {
      ssSetErrorStatus(S, "The path loss exponent must be larger than 0");
      return;
    }

    static int printed2 = 0;
    if (!printed2) {
      printed2 = 1;
      mexPrintf("Wireless network data:\n");
      mexPrintf("Transmit power is:\t%.2f dbm \t<==> %.2f mW\n", 
		temp1, transmitPowerWatt * 1000.0);
      mexPrintf("Receiver threshold is:\t%.2f dbm \t<==> %.2e mW\n",
		temp2, nwsys->receiverThreshold * 1000.0);
      mexPrintf("Maximum signal reach is calculated to:\t%.2f m\n", 
		pow(transmitPowerWatt/nwsys->receiverThreshold,
		    1.0/nwsys->pathloss)-1.0);
      mexPrintf("\n");
    }
    
    // 9 - Pathloss function popup
    arg = ssGetSFcnParam(S, 8);
    if (mxIsChar(arg)) {
      char buf[MAXCHARS];
      mxGetString(arg, buf, MAXCHARS);
      if (strcmp(buf, "custom")==0){
	// 10 - Special pathloss function if "custom"
	arg = ssGetSFcnParam(S, 9);
	if (mxIsChar(arg)) {
	  mxGetString(arg, nwsys->codeName, MAXCHARS);
	  nwsys->pathlossfun = &matlab_pathloss;
	} else {
	  ssSetErrorStatus(S, "No pathloss function defined");
	  return;
	}
      } else { // off
	nwsys->pathlossfun = &c_pathloss;
      }
    } else {
      ssSetErrorStatus(S, "Illegal pathloss argument");
      return;
    }

    // 11 - ACK Timeout
    arg = ssGetSFcnParam(S, 10);
    if (mxIsDoubleScalar(arg)) {
      nwsys->acktimeout = *mxGetPr(arg);
    }
    if (nwsys->acktimeout < 0 ) {
      ssSetErrorStatus(S, "The acktimeout must be larger than or equal to 0");
      return;
    }
      
    // 12 - Retrylimit
    arg = ssGetSFcnParam(S, 11);
    if (mxIsDoubleScalar(arg)) {
      nwsys->retrylimit = *mxGetPr(arg);
    }
    if (nwsys->retrylimit < 0 ) {
      ssSetErrorStatus(S, "The retrylimit must be larger than or equal to 0");
      return;
    }

    // 13 - Error coding threshold
    arg = ssGetSFcnParam(S, 12);
    if (mxIsDoubleScalar(arg)) {
      nwsys->error_threshold = *mxGetPr(arg);
    }
    if (nwsys->error_threshold < 0 || nwsys->error_threshold > 1 ) {
      ssSetErrorStatus(S, "The error coding threshold must be in the interval [0,1]");
      return;
    }
    debugPrintf("Error coding threshold=%f %f\n",nwsys->error_threshold, *mxGetPr(arg));
    
    // 14 - Loss probability
    arg = ssGetSFcnParam(S, 13);
    if (mxIsDoubleScalar(arg)) {
      nwsys->lossprob = *mxGetPr(arg);
    }
    if (nwsys->lossprob < 0 || nwsys->lossprob > 1 ) {
      ssSetErrorStatus(S, "The loss probability threshold must be in the interval [0 1]");
      return;
    }

    // 15 - Seed for random number generator
    arg = ssGetSFcnParam(S, 14);
    unsigned int seed = 0;
    if (mxIsDoubleScalar(arg)) {
      seed = (unsigned int)*mxGetPr(arg);
      tinymt32_init(&nwsys->randstate, seed);
    }

    /* Write pointer to MATLAB global workspace */
    mxArray* var = mxCreateDoubleScalar(0.0);
    mexMakeArrayPersistent(var);
    *((void **)mxGetData(var)) = nwsys;
    char nwsysbuf[MAXCHARS];
    sprintf(nwsysbuf, "_nwsys_%d", nwsys->networkNbr);
    mexPutVariable("global", nwsysbuf, var);
    
    nwsys->inputs = new double[nwsys->nbrOfNodes];
    nwsys->oldinputs = new double[nwsys->nbrOfNodes];
    nwsys->outputs   = new double[nwsys->nbrOfNodes];
    nwsys->sendschedule = new double[nwsys->nbrOfNodes];
    nwsys->energyconsumption = new double[nwsys->nbrOfNodes];

    for (i=0; i<nwsys->nbrOfNodes; i++) {
      nwsys->inputs[i] = 0.0;
      nwsys->oldinputs[i] = 0.0;
      nwsys->outputs[i] = 0.0;
      nwsys->sendschedule[i] = i+1;
      nwsys->energyconsumption[i] = 0;
    }

    nwsys->time = 0.0;
    nwsys->prevHit = 0.0;
  
    nwsys->nwnodes = new NWnode*[nwsys->nbrOfNodes];
    for (i=0; i<nwsys->nbrOfNodes; i++) {
      int j;

      nwsys->nwnodes[i] = new NWnode();
      nwsys->nwnodes[i]->transmitPower = transmitPowerWatt;

      nwsys->nwnodes[i]->signallevels = new double[nwsys->nbrOfNodes]; //802.11
      for (j=0; j<nwsys->nbrOfNodes; j++) {
	nwsys->nwnodes[i]->signallevels[j] = 0;
      }
      
    }

    nwsys->waituntil = 0.0;
    nwsys->sending = -1;  // Note! -1 means nobody is sending
    nwsys->rrturn = nwsys->nbrOfNodes - 1; // want to start at 0
    nwsys->lasttime = -1.0;

    nwsys->slotcount = nwsys->schedsize - 1; // want to start at 0
    nwsys->currslottime = -nwsys->slottime;  // same here

    // rad, kolum, reella tal
    //nwsys->nbrOfTransmissions = mxCreateDoubleMatrix(nwsys->nbrOfNodes, nwsys->nbrOfNodes, mxREAL);
    //mexMakeArrayPersistent(nwsys->nbrOfTransmissions);
  }


  static void mdlInitializeSampleTimes(SimStruct *S)
  {
    ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, FIXED_IN_MINOR_STEP_OFFSET);
  }

  
#define MDL_START
  static void mdlStart(SimStruct *S)
  {
    // Display the TrueTime splash if global variable TTSPLASH not defined
    mxArray* splvar = (mxArray*)mexGetVariablePtr("global", "TTSPLASH");
    if (splvar == NULL) {
      splvar = mxCreateDoubleMatrix(0, 0, mxREAL);
      mexMakeArrayPersistent(splvar);
      mexPutVariable("global", "TTSPLASH", splvar);
      mexPrintf("\n"
		"--------------------------------------------------------------\n"
		"                         TrueTime 2.0                         \n"
		"              Copyright (c) 2016 Lund University              \n"
		"   Written by Anton Cervin, Dan Henriksson and Martin Ohlin,  \n"
		" Department of Automatic Control LTH, Lund University, Sweden \n"
		"--------------------------------------------------------------\n"
		);
    }
  }
  

#define MDL_INITIALIZE_CONDITIONS
  static void mdlInitializeConditions(SimStruct *S)
  {

  }

  static void mdlOutputs(SimStruct *S, int_T tid)
  {
    int i, exttrig = 0;

    RTnetwork *nwsys = (RTnetwork*)ssGetUserData(S);
    nwsys->time = ssGetT(S);
    real_T* output_0 = ssGetOutputPortRealSignal(S,0);
    real_T* output_1 = ssGetOutputPortRealSignal(S,1);
    real_T* output_2 = ssGetOutputPortRealSignal(S,2);

    for (i=0; i < nwsys->nbrOfNodes; i++) {
      if (fabs(nwsys->inputs[i]-nwsys->oldinputs[i]) > 0.1) {
	nwsys->oldinputs[i] = nwsys->inputs[i];
	exttrig = 1;
      }
    }
    if (exttrig == 1) {
      // Triggered on external events
      nwsys->nextHit = runNetwork(nwsys);
    } else {
      // Triggered on internal events
      if (nwsys->time >= nwsys->nextHit) {
	nwsys->nextHit = runNetwork(nwsys);
      }
    }

    for (i=0; i<nwsys->nbrOfNodes; i++) {
      output_0[i] = nwsys->outputs[i];
      output_1[i] = nwsys->sendschedule[i];
      output_2[i] = nwsys->energyconsumption[i];
    }
  } 


#define MDL_ZERO_CROSSINGS

  static void mdlZeroCrossings(SimStruct *S)
  {
    int i;
    double now = ssGetT(S);
    RTnetwork *nwsys = (RTnetwork*)ssGetUserData(S);
    InputRealPtrsType input_0 = ssGetInputPortRealSignalPtrs(S,0);
    InputRealPtrsType input_1 = ssGetInputPortRealSignalPtrs(S,1);
    InputRealPtrsType input_2 = ssGetInputPortRealSignalPtrs(S,2);

    /* Check for external events */
    for (i=0; i < nwsys->nbrOfNodes; i++) {
      if (fabs(*input_0[i] - nwsys->inputs[i]) > 0.1) {
	nwsys->nextHit = now;
	break;
      }
    }
    /* Copy inputs */
    for (i=0; i < nwsys->nbrOfNodes; i++) {
      nwsys->inputs[i] = *input_0[i];
      nwsys->nwnodes[i]->xCoordinate = *input_1[i];
      nwsys->nwnodes[i]->yCoordinate = *input_2[i];
    }
    ssGetNonsampledZCs(S)[0] = nwsys->nextHit - now;
  }


  static void mdlTerminate(SimStruct *S)
  {
    RTnetwork *nwsys = (RTnetwork*) ssGetUserData(S);
    if (nwsys == NULL) {
      return;
    }

    char nwsysbuf[MAXCHARS];
    mxArray* rhs[2];
    sprintf(nwsysbuf, "_nwsys_%d", nwsys->networkNbr);
    rhs[0] = mxCreateString("global");
    rhs[1] = mxCreateString(nwsysbuf);
    mexCallMATLAB(0, NULL, 2, rhs, "clear");

    delete nwsys;
    nwsys = NULL;
  }


#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif

#ifdef __cplusplus
} // end of extern "C" scope
#endif
