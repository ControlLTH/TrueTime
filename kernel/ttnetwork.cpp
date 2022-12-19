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
#include "matrix.h"
#include "ttnetwork.h"
#include "mex.h"

#include "tinymt32.c"

// ------- Network utility functions -------

// computes the remaining transmission time of the current frame
double remxtime(RTnetwork *nwsys, NWmsg *m) {
  switch (nwsys->type) {
  case FDMA:
    return m->remaining / (nwsys->bandwidths[m->sender] * nwsys->datarate + TT_TIME_RESOLUTION);
  default:
    return m->remaining / nwsys->datarate;
  }
}
// Compare message priorities
int msgHighPrioComp(Node* n1, Node* n2) {// Large prio number means high priority
  NWmsg *m1, *m2;
  m1 = (NWmsg *)n1;
  m2 = (NWmsg *)n2;
  if (m1->prio > m2->prio) {
    return 1;
  }  else return 0;
}
int msgLowPrioComp(Node* n1, Node* n2) {// Small prio number  means high priority
  NWmsg *m1, *m2;
  m1 = (NWmsg *)n1;
  m2 = (NWmsg *)n2;
  if (m1->prio < m2->prio) {
    return 1;
  }  else return 0;
}
// allocates switch memory, if possible
bool switchmalloc(RTnetwork *nwsys, int sender, int receiver, int length) {
  int i;
  if (nwsys->buftype == OUTPUTBUF) {
    if (receiver == -1) { // broadcast
      // make sure there is memory in each buffer first
      bool ok = true;
      for (i=0; i<nwsys->nbrOfNodes; i++) {
	if (sender != i) {
	  if (nwsys->nwnodes[i]->switchmem < length) {
	    ok = false;
	    break;
	  }
	}
      }
      if (!ok) return false;
      // allocate memory
      for (i=0; i<nwsys->nbrOfNodes; i++) {
	if (sender != i) {
	  nwsys->nwnodes[i]->switchmem -= length;
	  debugPrintf("Broadcast: %d bits allocated in output buffer %d, remaining %d\n", length, i, nwsys->nwnodes[i]->switchmem);
	}
      }
      return true;
    } else {
      if (nwsys->nwnodes[receiver]->switchmem < length) return false;
      nwsys->nwnodes[receiver]->switchmem -= length;
      debugPrintf("%d bits allocated in output buffer %d, remaining %d\n", length, receiver, nwsys->nwnodes[receiver]->switchmem);
      return true;
    }
  } else {
    if (receiver == -1) { // broadcast
      if (nwsys->switchmem < (nwsys->nbrOfNodes-1)*length) return false;
      nwsys->switchmem -= (nwsys->nbrOfNodes-1)*length;
      debugPrintf("Broadcast: %d bits allocated in common buffer, remaining %d\n", (nwsys->nbrOfNodes-1)*length, nwsys->switchmem);
      return true;
    } else {
      if (nwsys->switchmem < length) return false;
      nwsys->switchmem -= length;
      debugPrintf("%d bits allocated in common buffer, remaining %d\n", length, nwsys->switchmem);      
      return true;
    }
  }
}

// frees switch memory
void switchfree(RTnetwork *nwsys, int receiver, int length) {
  if (nwsys->buftype == OUTPUTBUF) {
    nwsys->nwnodes[receiver]->switchmem += length;
    debugPrintf("%d freed in output buffer %d, remaining %d\n", length, receiver, nwsys->nwnodes[receiver]->switchmem);    
  } else {
    nwsys->switchmem += length;
    debugPrintf("%d bits freed in common buffer, remaining %d\n", length, nwsys->switchmem);      
  }
}
bool nodeSwitchMalloc(RTnetwork *nwsys, int sender, int length) {
  // if there is enough memory in the node switch allocate memory and return true
  if (nwsys->nwnodes[sender]->switchmem >= length) {
    nwsys->nwnodes[sender]->switchmem -= length;
    return true;
  }
  return false;
}
int checkNodeDestination(RTnetwork *nwsys, int sender,  NWmsg *m) {
  // return the node to which the message should be sent to
  return nwsys->nodeRouting[sender][m->receiver];
}

int checkPortDestination(RTnetwork *nwsys, int sender, int receivingNode) {
  // return the number of the port that should transmit the message
  // if an appropriate port cant be found return -1
  for (int j=0; j < 4; j++) {
    if (nwsys->portDestination[sender][j] == receivingNode) {
      return j;
    }
  }
  return -1;
}

/**
 * Returns the smallest postdelay != 0
 */
double broadcast(RTnetwork *nwsys, NWmsg *m, int whichqueue) {
  int j;
  NWmsg *m2;
  mxArray* data = NULL;
  double temp = TT_MAX_TIMESTEP;

  if (m->dataMATLAB != NULL) {
    data = m->dataMATLAB;
  }

  for (j=0; j<nwsys->nbrOfNodes; j++) {
    if (j != m->sender) {
      // Duplicate message
      m2 = new NWmsg();
      *m2 = *m;
      if (data) {
        m2->dataMATLAB = mxDuplicateArray(data);
        mexMakeArrayPersistent(m2->dataMATLAB);
      }
      if (whichqueue == 1) {
        nwsys->nwnodes[j]->outputQ->appendNode(m2);
      } else if (whichqueue == 2) {
        nwsys->nwnodes[j]->switchQ->appendNode(m2);
      }
      
      m2->waituntil = nwsys->time + nwsys->nwnodes[j]->postdelay;
      // How far from now should we run _next_ time?
      if (nwsys->nwnodes[j]->postdelay != 0.0 && 
              temp > nwsys->nwnodes[j]->postdelay ){
        temp = nwsys->nwnodes[j]->postdelay;
      }
    }
  }
  
  delete m;
  m = NULL;
  
  return temp;
}  

// Simulates and returns the time of the next invocation (nextHit)

double runNetwork(RTnetwork *nwsys) {
  int i, j;
  NWmsg *m = NULL, *m2 = NULL;  // m is our message, m2 is contending message
  NWmsg *next;
  double timeElapsed;
  double nextHit = nwsys->time + TT_MAX_TIMESTEP;
  double waittime;
  double waituntil;

  timeElapsed = nwsys->time - nwsys->prevHit; // time since last invocation
  nwsys->prevHit = nwsys->time;

  debugPrintf("Running network %d at %f\n", nwsys->networkNbr, nwsys->time);

  // Check if messages have finished waiting in the preprocQ's
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    m = (NWmsg *)nwsys->nwnodes[i]->preprocQ->getFirst();
    while (m != NULL) {
      if (m->waituntil - nwsys->time < TT_TIME_RESOLUTION) {
        debugPrintf("moving message from preprocQ to inputQ at %f\n", nwsys->time);
        m->remaining = fmax(m->length, nwsys->minsize);
        nwsys->nwnodes[i]->preprocQ->removeNode(m);
        nwsys->nwnodes[i]->inputQ->insertSorted(m);
        if (nwsys->type == PROFINET) {
          *nwsys->nwnodes[i]->lastMsg = *m;
          if (m->dataMATLAB != NULL) {
            nwsys->nwnodes[i]->lastMsg->dataMATLAB = mxDuplicateArray(m->dataMATLAB);
            mexMakeArrayPersistent(nwsys->nwnodes[i]->lastMsg->dataMATLAB);
          }
        }
      } else {
        // update nextHit?
        if (m->waituntil < nextHit) {
          nextHit = m->waituntil;
        }
      }
      m = (NWmsg *)m->getNext();
    }
    if (nwsys->type == FLEXRAY) {//Also move messges from statpreprocQ to statinputQ
      m = (NWmsg *)nwsys->nwnodes[i]->statpreprocQ->getFirst();
      while (m != NULL) {
        if (m->waituntil - nwsys->time < TT_TIME_RESOLUTION) {
          debugPrintf("moving message from statpreprocQ to statinputQ at %f\n", nwsys->time);
          m->remaining = fmax(m->length, nwsys->minsize);
          nwsys->nwnodes[i]->statpreprocQ->removeNode(m);
          nwsys->nwnodes[i]->statinputQ->insertSorted(m);
        } else {
          // update nextHit?
          if (m->waituntil < nextHit) {
            nextHit = m->waituntil;
          }
        }
        m = (NWmsg *)m->getNext();
      }
    }
  }
  
  debugPrintf("network type is %d\n", nwsys->type);
  // Do the main processing of different protocols
  switch (nwsys->type) {
    
    case CSMACD:
      if (nwsys->sending != -1) { // not been idle
        debugPrintf("Node %d has been sending\n", nwsys->sending);
        m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst();
        // decrease remaining number of bits in current frame
        m->remaining -= (nwsys->datarate * timeElapsed);
        // frame finished?
        if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {
          nwsys->nwnodes[nwsys->sending]->nbrcollisions = 0; // reset coll. counter
          // transmission is finished, move to outputQ
          debugPrintf("Transmission finished\n");
          nwsys->nwnodes[nwsys->sending]->inputQ->removeNode(m);
          if (m->receiver == -1) {
            // Broadcast
            // Duplicate message and append to outputQ's of all nodes
            waituntil = nwsys->time + broadcast(nwsys, m, 1);
          } else {
            nwsys->nwnodes[m->receiver]->outputQ->appendNode(m);
            m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay;
            waituntil = m->waituntil;
          }
          // update nextHit?
          if ( waituntil != nwsys->time && waituntil < nextHit ) {
            nextHit = waituntil;
          }
          
          nwsys->nwnodes[nwsys->sending]->state = 0; // idle
          nwsys->sending = -1;
          
        } else {
          if (nwsys->time + remxtime(nwsys, m) < nextHit) {
            nextHit = nwsys->time + remxtime(nwsys, m);
          }
        }
      }
      
      // check if some node has finished waiting after a collision
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        if (nwsys->nwnodes[i]->state == 3) {
          if (nwsys->time > nwsys->nwnodes[i]->waituntil - TT_TIME_RESOLUTION) {
            debugPrintf("Node %d has finished waiting\n", i);
            nwsys->nwnodes[i]->state = 0; // idle again
          } else if (nwsys->nwnodes[i]->waituntil < nextHit) {
            nextHit = nwsys->nwnodes[i]->waituntil;
          }
        }
        
      }
      
      // if network appears idle, check if any new nodes want to transmit
      if (nwsys->sending == -1 || nwsys->time < nwsys->lasttime + COLLISION_WINDOW) {
        debugPrintf("The network *appears* to be idle\n");
        // do any nodes want to transmit?
        for (i=0; i<nwsys->nbrOfNodes; i++) {
          // not sending already and anything to send?
          if (nwsys->nwnodes[i]->state == 0 && (m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) {
            debugPrintf("Node %d wants to transmit...\n", i);
            if (nwsys->sending == -1) { // really idle?
              debugPrintf("Medium is idle at time %f, node %d starts to transmit...\n", nwsys->time, i);
              debugPrintf("Remaining: %f\n", m->remaining);
              nwsys->lasttime = nwsys->time;
              nwsys->nwnodes[i]->state = 1; // sending
              nwsys->sending = i;
              // update nextHit?
              if (nwsys->time + remxtime(nwsys, m) < nextHit) {
                nextHit = nwsys->time + remxtime(nwsys, m);
              }
            } else { // collision!
              debugPrintf("Collision when node %d tried to send at time %f!\n", i, nwsys->time);
              // mark both nodes as colliding
              nwsys->nwnodes[i]->state = 2; // colliding
              nwsys->nwnodes[nwsys->sending]->state = 2; // colliding
              // abort the currently transmitted frame
              m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst();
              m->remaining = fmax(m->length, nwsys->minsize);  // restore remaining
            }
          }
        }
      }
      
      // move nodes from colliding to waiting state
      for  (i=0; i<nwsys->nbrOfNodes; i++) {
        if (nwsys->nwnodes[i]->state == 2) {
          nwsys->sending = -1;
          nwsys->nwnodes[i]->state = 3;
          nwsys->nwnodes[i]->nbrcollisions += 1;
          debugPrintf("node %d has now collided %d times in a row\n", i, nwsys->nwnodes[i]->nbrcollisions);
          if (nwsys->nwnodes[i]->nbrcollisions > 10) {
            nwsys->nwnodes[i]->nbrcollisions = 10;
            debugPrintf("max number of collisions reached!\n");
          }
          // compute random back-off time
          waittime = (double)urandint(nwsys,0,(1 << nwsys->nwnodes[i]->nbrcollisions) - 1) * nwsys->minsize / nwsys->datarate;
          nwsys->nwnodes[i]->waituntil = nwsys->time + fmax(waittime, 2.0*COLLISION_WINDOW);
          debugPrintf("Will reattempt at %f\n", nwsys->nwnodes[i]->waituntil);
          if (nwsys->nwnodes[i]->waituntil < nextHit) {
            nextHit = nwsys->nwnodes[i]->waituntil;
          }
        }
      }
      break;
      
  case CSMAAMP:
    if (nwsys->sending != -1) { // not been idle
      debugPrintf("Node %d has been sending\n", nwsys->sending);
      m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst();
      // decrease remaining number of bits in current frame
      m->remaining -= (nwsys->datarate * timeElapsed);
      // frame finished?
      if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {
        // transmission is finished, move to outputQ
        debugPrintf("Transmission finished\n");
        nwsys->nwnodes[nwsys->sending]->inputQ->removeNode(m);
        
        if (m->receiver == -1) {
          // Broadcast
          // Duplicate message and append to outputQ's of all nodes
          waituntil = nwsys->time + broadcast(nwsys, m, 1);
        } else {
          nwsys->nwnodes[m->receiver]->outputQ->appendNode(m);
          m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay;
          waituntil = m->waituntil;
        }
        // update nextHit?
        if ( waituntil != nwsys->time && waituntil < nextHit ) {
          nextHit = waituntil;
        }
        
        nwsys->nwnodes[nwsys->sending]->state = 0; // idle
        nwsys->sending = -1;
        
      } else {
        if (nwsys->time + remxtime(nwsys, m) < nextHit) {
          nextHit = nwsys->time + remxtime(nwsys, m);
        }
      }
    }
    
    if (nwsys->sending == -1 || nwsys->time < nwsys->lasttime + COLLISION_WINDOW) {
      debugPrintf("The network *appears* to be idle\n");
      // do any nodes want to transmit?
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        // not sending already and anything to send?
        if (nwsys->nwnodes[i]->state == 0 && (m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) {
          debugPrintf("Node %d wants to transmit...\n", i);
          if (nwsys->sending == -1) { // still idle?
            debugPrintf("Medium is idle, node %d starts to transmit...\n", i);
            nwsys->lasttime = nwsys->time;
            nwsys->nwnodes[i]->state = 1; // sending
            nwsys->sending = i;
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          } else { // collision!
            debugPrintf("Medium was not idle - collision!\n");
            m2 = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst();
            debugPrintf("Sending message has priority %f, our message has priority %f\n", m2->prio, m->prio);
            if (m->prio > m2->prio) { // we have lower prio?
              debugPrintf("We lose the contention, will try again later\n");
              if (nwsys->time + remxtime(nwsys, m2) < nextHit) {
                nextHit = nwsys->time + remxtime(nwsys, m2);
              }
            } else { // we have higher prio
              debugPrintf("We win the contention, other message aborted!\n");
              m2->remaining = fmax(m2->length, nwsys->minsize); // restore remaining
              nwsys->nwnodes[nwsys->sending]->state = 0;
              nwsys->nwnodes[i]->state = 1;
              nwsys->sending = i;
              if (nwsys->time + remxtime(nwsys, m) < nextHit) {
                nextHit = nwsys->time + remxtime(nwsys, m);
              }
            }
          }
        }
      }
    }
    break;
    
    case RR:
      if (nwsys->sending == -1) {
        debugPrintf("The network has been idle\n");
        // ready for the next transmission?
        if (nwsys->time > nwsys->waituntil - TT_TIME_RESOLUTION) {
          if (nwsys->type == NCM ) {
            if (nwsys->nextNode == -1) {
              printf ("ttnetwork: invalid next node %d\n",nwsys->nextNode);
              nwsys->rrturn = 0;
            }  else {
              nwsys->rrturn = nwsys->nextNode;
            }
          } else {
            nwsys->rrturn = (nwsys->rrturn + 1) % nwsys->nbrOfNodes;
          }
          debugPrintf("Token passed to node %d at time %f,\n", nwsys->rrturn, nwsys->time);
          if ((m = (NWmsg *)nwsys->nwnodes[nwsys->rrturn]->inputQ->getFirst()) != NULL) {
            debugPrintf("  node %d starts to transmit\n", nwsys->rrturn);
            nwsys->sending = nwsys->rrturn;
            nwsys->nwnodes[nwsys->rrturn]->state = 1; // we're sending
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          } else {
            debugPrintf("  node %d has nothing to send, will idle...\n", nwsys->rrturn);
            nwsys->waituntil = nwsys->time + nwsys->minsize / nwsys->datarate;
            // update nextHit?
            if (nwsys->waituntil > nwsys->time && nwsys->waituntil < nextHit) {
              nextHit = nwsys->waituntil;
            }
          }
        } else {// wait some more
          // update nextHit?
          if (nwsys->waituntil < nextHit) {
            nextHit = nwsys->waituntil;
          }
        }
      } else {
        debugPrintf("Node %d has been sending\n", nwsys->sending);
        m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst();
        // count down remaining transmission length
        m->remaining -= (nwsys->datarate * timeElapsed);
        // frame finished?
        if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) { // yes
          // finished transmission, move to outputQ
          debugPrintf("Transmission finished\n");
          nwsys->nwnodes[nwsys->sending]->inputQ->removeNode(m);
          
          if (m->receiver == -1) {
            // Broadcast
            // Duplicate message and append to outputQ's of all nodes
            waituntil = nwsys->time + broadcast(nwsys, m, 1);
          } else {
            nwsys->nwnodes[m->receiver]->outputQ->appendNode(m);
            m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay;
            waituntil = m->waituntil;
          }
          // update nextHit?
          if ( waituntil != nwsys->time && waituntil < nextHit ) {
            nextHit = waituntil;
          }
          
          nwsys->nwnodes[nwsys->sending]->state = 0;
          nwsys->sending = -1;  // idle and hand over to next node
          nwsys->waituntil = nwsys->time + nwsys->minsize / nwsys->datarate;
          if (nwsys->waituntil < nextHit) {
            nextHit = nwsys->waituntil;
          }
        } else { // more to transmit in this frame
          if (nwsys->time + remxtime(nwsys, m) < nextHit) {
            nextHit = nwsys->time + remxtime(nwsys, m);
          }
        }
      }
      
      break;
      
    case NCM:
      //case RR:
      if (nwsys->sending == -1) {
        debugPrintf("The network has been idle\n");
        // ready for the next transmission?
        if (nwsys->time > nwsys->waituntil - TT_TIME_RESOLUTION) {
          if (nwsys->type == NCM ) {
            if (nwsys->nextNode == -1) {
              printf ("ttnetwork: invalid next node %d\n",nwsys->nextNode);
              nwsys->rrturn = 0;
            }  else {
              nwsys->rrturn = nwsys->nextNode;
            }
          } else {
            nwsys->rrturn = (nwsys->rrturn + 1) % nwsys->nbrOfNodes;
          }
          debugPrintf("Token passed to node %d at time %f,\n", nwsys->rrturn, nwsys->time);
          if ((m = (NWmsg *)nwsys->nwnodes[nwsys->rrturn]->inputQ->getFirst()) != NULL) {
            debugPrintf("  node %d starts to transmit\n", nwsys->rrturn);
            nwsys->sending = nwsys->rrturn;
            nwsys->nwnodes[nwsys->rrturn]->state = 1; // we're sending
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          } else {
            debugPrintf("  node %d has nothing to send, will idle...\n", nwsys->rrturn);
            nwsys->waituntil = nwsys->time + nwsys->minsize / nwsys->datarate;
            // update nextHit?
            if (nwsys->waituntil > nwsys->time && nwsys->waituntil < nextHit) {
              nextHit = nwsys->waituntil;
            }
          }
        } else {// wait some more
          // update nextHit?
          if (nwsys->waituntil < nextHit) {
            nextHit = nwsys->waituntil;
          }
        }
      } else {
        debugPrintf("Node %d has been sending\n", nwsys->sending);
        m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst();
        // count down remaining transmission length
        m->remaining -= (nwsys->datarate * timeElapsed);
        // frame finished?
        if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) { // yes
          // finished transmission, move to outputQ
          debugPrintf("Transmission finished\n");
          nwsys->nwnodes[nwsys->sending]->inputQ->removeNode(m);
          
          if (m->receiver == -1) {
            // Broadcast
            // Duplicate message and append to outputQ's of all nodes
            waituntil = nwsys->time + broadcast(nwsys, m, 1);
          } else {
            nwsys->nwnodes[m->receiver]->outputQ->appendNode(m);
            m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay;
            waituntil = m->waituntil;
          }
          // update nextHit?
          if ( waituntil != nwsys->time && waituntil < nextHit ) {
            nextHit = waituntil;
          }
          
          nwsys->nwnodes[nwsys->sending]->state = 0;
          nwsys->sending = -1;  // idle and hand over to next node
          nwsys->waituntil = nwsys->time + nwsys->minsize / nwsys->datarate;
          if (nwsys->waituntil < nextHit) {
            nextHit = nwsys->waituntil;
          }
        } else { // more to transmit in this frame
          if (nwsys->time + remxtime(nwsys, m) < nextHit) {
            nextHit = nwsys->time + remxtime(nwsys, m);
          }
        }
      }
      
      break;
      
    case FDMA:
      // go through all nodes and count down transmission times
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        if (nwsys->nwnodes[i]->state == 1) { // node has been sending
          m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst();
          // decrease remaining number of bits in current frame
          m->remaining -= (nwsys->bandwidths[i] * nwsys->datarate * timeElapsed);
          // frame finished?
          if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {
            // transmission is finished, move to outputQ
            debugPrintf("transmission finished\n");
            nwsys->nwnodes[i]->inputQ->removeNode(m);
            
            if (m->receiver == -1) {
              // Broadcast
              // Duplicate message and append to outputQ's of all nodes
              waituntil = nwsys->time + broadcast(nwsys, m, 1);
            } else {
              nwsys->nwnodes[m->receiver]->outputQ->appendNode(m);
              m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay;
              waituntil = m->waituntil;
            }
            // update nextHit?
            if ( waituntil != nwsys->time && waituntil < nextHit ) {
              nextHit = waituntil;
            }
            
            nwsys->nwnodes[i]->state = 0;
            
          } else { // frame not finished
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          }
        }
      }
      
      // check if any new transmissions should be started
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        if (nwsys->nwnodes[i]->state == 0) { // idle?
          // check if we should start a new transmission
          if ((m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) {
            debugPrintf("Node %d starting transmission of frame at %f\n", i, nwsys->time);
            nwsys->nwnodes[i]->state = 1; // we're sending
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          }
        }
      }
      break;
      
    case TDMA:
      if (nwsys->sending != -1) { // someone has been sending
        m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst();
        // decrease remaining number of bits in current frame
        m->remaining -= (nwsys->datarate * timeElapsed);
        if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {
          // transmission is finished, move to outputQ
          debugPrintf("transmission finished\n");
          nwsys->nwnodes[nwsys->sending]->inputQ->removeNode(m);
          
          if (m->receiver == -1) {
            // Broadcast
            // Duplicate message and append to outputQ's of all nodes
            waituntil = nwsys->time + broadcast(nwsys, m, 1);
          } else {
            nwsys->nwnodes[m->receiver]->outputQ->appendNode(m);
            m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay;
            waituntil = m->waituntil;
          }
          // update nextHit?
          if ( waituntil != nwsys->time && waituntil < nextHit ) {
            nextHit = waituntil;
          }
          
          // will we have time and are there more messages?
          if (nwsys->time < nwsys->currslottime + nwsys->slottime && (m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst()) != NULL) {
            debugPrintf("Node %d starting transmission of new message at %f\n", nwsys->sending, nwsys->time);
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          } else {
            nwsys->nwnodes[nwsys->sending]->state = 0; // idle
            nwsys->sending = -1;
          }
        } else { // frame not finished
          // update nextHit?
          if (nwsys->time + remxtime(nwsys, m) < nextHit) {
            nextHit = nwsys->time + remxtime(nwsys, m);
          }
        }
      }
      
      // time for new slot?
      if (nwsys->time > nwsys->currslottime + nwsys->slottime - TT_TIME_RESOLUTION) {
        nwsys->slotcount = (nwsys->slotcount + 1) % nwsys->schedsize;
        debugPrintf("New slot: index %d, sender %d\n", nwsys->slotcount, nwsys->schedule[nwsys->slotcount]);
        nwsys->currslottime += nwsys->slottime;
        
        // preempt current transmission?
        if (nwsys->sending != -1 && nwsys->schedule[nwsys->slotcount] != nwsys->sending) {
          debugPrintf("current transmission by %d interrupted!\n", nwsys->sending);
          nwsys->nwnodes[nwsys->sending]->state = 2; // wait state
          nwsys->sending = -1;
        }
      }
      
      // start/resume a transmission?
      if (nwsys->schedule[nwsys->slotcount] != nwsys->sending) {
        i = nwsys->schedule[nwsys->slotcount];
        // resuming an old transmission?
        if (nwsys->nwnodes[i]->state == 2) {
          debugPrintf("Node %d resuming its transmission\n", i);
          nwsys->nwnodes[i]->state = 1;
          nwsys->sending = i;
          m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst();
          if (nwsys->time + remxtime(nwsys, m) < nextHit) {
            nextHit = nwsys->time + remxtime(nwsys, m);
          }
        } else { // new transmission
          debugPrintf("node %d may start new transmission\n", i);
          if ((m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) {
            debugPrintf("Node %d starting transmission of frame at %f\n", i, nwsys->time);
            nwsys->nwnodes[i]->state = 1; // we're sending
            nwsys->sending = i;
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          } else {
            debugPrintf("Node %d has nothing to send...\n", i);
          }
        }
      }
      
      if (nwsys->currslottime + nwsys->slottime < nextHit) {
        nextHit = nwsys->currslottime + nwsys->slottime;
      }
      
      break;
      
    case FLEXRAY:
      
      // The FlexRay protocol's static schedule uses the same variable name as the TDMA schedule
      if (nwsys->sending != -1) { //Someone is sending
        debugPrintf("Node %d has been sending\n", nwsys->sending);
        if (nwsys->slotcount <= nwsys->schedsize-1) {//static segment
          debugPrintf("Static Segment\n");
          m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->statinputQ->getFirst(); //Get the first message from the static-segment input queue of the sending node
          m->remaining -= (nwsys->datarate * timeElapsed); //Decrese remaining number of bits in current frame
          debugPrintf("m->remaining= %.15f\n",m->remaining);
          if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {// transmission is finished
            debugPrintf("Node %d has finished sending at time %.15f\n", nwsys->sending, nwsys->time);
            nwsys->nwnodes[nwsys->sending]->statinputQ->removeNode(m); // remove from static-segment input queue
            
            if (m->receiver == -1) {
              // Broadcast
              // Duplicate message and append to outputQ's of all nodes
              waituntil = nwsys->time + broadcast(nwsys, m, 1);
            } else {
              nwsys->nwnodes[m->receiver]->outputQ->appendNode(m); // add to receiver output queue
              m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay; // update m->waituntil, used to determine if msg should be moved to postprocQ
              waituntil = m->waituntil;
            }
            if (waituntil != nwsys->time && waituntil < nextHit){
              nextHit = waituntil;
            }
            if (nwsys->time < nwsys->currslottime + nwsys->slottime && nwsys->currslottime + nwsys->slottime < nextHit) {
              //set nextHit to the begining of next slot
              nextHit = nwsys->currslottime + nwsys->slottime;
              debugPrintf("nextHit= %.15f\n",nextHit);
            }
            if ((m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->statinputQ->getFirst()) != NULL) {
              nwsys->nwnodes[nwsys->sending]->state = 2;
              nwsys->sending = -1;
            } else {
              nwsys->nwnodes[nwsys->sending]->state = 0;
              nwsys->sending = -1;
            }
          } else {
            debugPrintf("Node %d continues to send. Message left = %.15f\n",nwsys->sending, m->remaining);
            if (nwsys->time + remxtime(nwsys,m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys,m);
            }
          }
        } else if (nwsys->slotcount > nwsys->schedsize-1 && nwsys->miniSlotCount < nwsys->dynSchedSize){ //Dymanic Segment
          debugPrintf("Dynamic Segment");
          m = (NWmsg *)nwsys->nwnodes[nwsys->sending]->inputQ->getFirst(); // Get the first message from the dynamic-segment input queue of the sending node
          m->remaining -= (nwsys->datarate * timeElapsed); // Decrease remaining nbr of bits in current frame
          if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {// transmission is finished
            debugPrintf("Node %d has finished sending\n", nwsys->sending);
            nwsys->nwnodes[nwsys->sending]->inputQ->removeNode(m); // remove from dynamic-segment input queue
            
            if (m->receiver == -1) {
              // Broadcast
              // Duplicate message and append to outputQ's of all nodes
              waituntil = nwsys->time + broadcast(nwsys, m, 1);
            } else {
              nwsys->nwnodes[m->receiver]->outputQ->appendNode(m); // add to receiver output queue
              m->waituntil = nwsys->time + nwsys->nwnodes[m->receiver]->postdelay; // update m->waituntil, used to determine if msg should be moved to postprocQ
              waituntil = m->waituntil;
            }
            if (waituntil != nwsys->time && waituntil < nextHit){
              nextHit = waituntil;
            }
            nwsys->miniSlotCount += nwsys->msgMiniSlots;
            nwsys->msgMiniSlots = 0; // reset msgMiniSlots
            nwsys->nwnodes[nwsys->sending]->state = 0; //node idle
            nwsys->sending = -1; // no node is sending
          } else {
            debugPrintf("Node %d continues to send. Message left = %.15f\n",nwsys->sending, m->remaining);
            if (nwsys->time + remxtime(nwsys,m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys,m);
            }
          }
        }
      } else {
        debugPrintf("No node has been sending\n");
        if (nwsys->slotcount == nwsys->schedsize && nwsys->time > nwsys->networkIdleUntil - TT_TIME_RESOLUTION) {
          nwsys->slotcount = -1;
          debugPrintf("Network Idle Time is finished\n");
        }
      }
      
      // If no node is sending we start here
      if (nwsys->slotcount <= nwsys->schedsize-1) {//static segment
        debugPrintf("Static Segment:\n");
        if (nwsys->currslottime < 0) {
          nwsys->currslottime = nwsys->time - nwsys->slottime;
        }
        // time for the next slot?
        debugPrintf("currslottime + slottime= %.15f\n", nwsys->currslottime+nwsys->slottime);
        if (nwsys->time > nwsys->currslottime + nwsys->slottime -TT_TIME_RESOLUTION ) { // If slot-changing time is passed:
          nwsys->slotcount += 1;
          debugPrintf("Updated slotcount = %d\n", nwsys->slotcount);
          if (nwsys->slotcount == nwsys->schedsize) {
            nwsys->miniSlotCount = 0;
          }
          nwsys->currslottime += nwsys->slottime;
          
          //preempt current transmission?
          if (nwsys->sending != -1 && nwsys->schedule[nwsys->slotcount] != nwsys->sending) {
            debugPrintf("  Preempt\n");
            nwsys->nwnodes[nwsys->sending]->state = 2; // wait state
            nwsys->sending = -1;
          }
        }
        // start/resume transmission
        if (nwsys->slotcount <= nwsys->schedsize-1) {//check to see if we are still in the static segment
          if (nwsys->schedule[nwsys->slotcount] != nwsys->sending && nwsys->slotcount > -1) {
            debugPrintf("Slot change: %d ", nwsys->slotcount);
            i = nwsys->schedule[nwsys->slotcount];
            
            debugPrintf("Try new transmission: ");
            if ((m = (NWmsg *)nwsys->nwnodes[i]->statinputQ->getFirst()) != NULL) {
              if (remxtime(nwsys,m) <= nwsys->currslottime + nwsys->slottime - nwsys->time+TT_TIME_RESOLUTION) { //check if there is enough time to send message
                if (remxtime(nwsys,m) > nwsys->currslottime + nwsys->slottime - nwsys->time) {//numerical issue fix
                  m->remaining =  (nwsys->currslottime + nwsys->slottime - nwsys->time)*nwsys->datarate;
                }
                debugPrintf("A new message is available and there is enough time to send it. Node %d is sending\n", i);
                nwsys->nwnodes[i]->state = 1; //sending state
                nwsys->sending = i;
                // update nextHit?
                if (nwsys->time + remxtime(nwsys,m) < nextHit) {
                  nextHit = nwsys->time + remxtime(nwsys,m);
                }
              } else {
                debugPrintf("Not enough time to start new transmission\n");
                if (nwsys->currslottime + nwsys->slottime < nextHit) {
                  nextHit = nwsys->currslottime + nwsys->slottime;
                  nwsys->nwnodes[i]->state = 2; //waiting state
                  nwsys->sending = -1;
                }
              }
            } else {
              debugPrintf("No new message is available\n");
              if (nwsys->currslottime + nwsys->slottime < nextHit) {
                nextHit = nwsys->currslottime+nwsys->slottime;
              }
            }
          }
        }
      }
      
    if (nwsys->slotcount > nwsys->schedsize-1 && nwsys->miniSlotCount >= 0 && nwsys->miniSlotCount < nwsys->dynSchedSize ) { //dynamic segment   
      debugPrintf("Dynamic Segment:\n");
      
      if (nwsys->sending == -1) { // If no one is sending, check dynamic schedule to see who is next.
        i = nwsys->dynSchedule[nwsys->miniSlotCount];
        debugPrintf("  Dynamical Schedule node: %d \n",i);
        
        if ((m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) { // Check if there is enough time to send the message
          if (m->remaining <= (double)(nwsys->dynSchedSize - nwsys->miniSlotCount) * (double)nwsys->miniSlotSize) {
            // calculate the number of mini slots that will be used to send the message (rounded upwards)
            nwsys->msgMiniSlots = ceil(remxtime(nwsys,m) * nwsys->datarate / nwsys->miniSlotSize);
            if(nwsys->msgMiniSlots == 0) nwsys->msgMiniSlots = 1;   // if the message is less than one minislot it still occupies one
            nwsys->waituntil = nwsys->time + nwsys->msgMiniSlots*nwsys->miniSlotSize/nwsys->datarate;
            
            nwsys->nwnodes[i]->state = 1; //sending
            nwsys->sending = i; //set sending node
            // Update nextHit?
            if (nwsys->waituntil < nextHit) {
              nextHit = nwsys->waituntil;
            }
          } else {
            // There is not time enough to send the message, move on to next slot
            nwsys->nwnodes[i]->state = 2; //waiting
            nwsys->sending = -1; //No one is sending
            nwsys->miniSlotCount += 1;
            if (nwsys->time + nwsys->miniSlotSize/nwsys->datarate < nextHit) {
              nextHit = nwsys->time + nwsys->miniSlotSize/nwsys->datarate;
            }
          }
        } else {
          //There were no messsages in the inputQ, move on to the next slot
          nwsys->miniSlotCount += 1;
          nwsys->nwnodes[i]->state = 0; //idle
          nwsys->sending = -1;
          if (nwsys->time + nwsys->miniSlotSize/nwsys->datarate < nextHit) {
            nextHit = nwsys->time + nwsys->miniSlotSize/nwsys->datarate;
          }
        }
      } else {
        debugPrintf("  Node %d is sending in the dynamic segement\n", nwsys->sending);
        if (nwsys->time + remxtime(nwsys,m) < nextHit) {
          nextHit = nwsys->time + remxtime(nwsys,m);
          debugPrintf("nextHit7= %f\n",nextHit);
        }
      }
    }
      
      // Network idle time
      if (nwsys->miniSlotCount >= nwsys->dynSchedSize) {// Network Idle Time and then reseting counters etc.
        debugPrintf("Network Idle Time starting\n");
        nwsys->miniSlotCount = -1;
        nwsys->networkIdleUntil = nwsys->time + nwsys->NIT/nwsys->datarate;
        nwsys->currslottime = -nwsys->slottime;
        if (nwsys->networkIdleUntil < nextHit) {
          nextHit = nwsys->networkIdleUntil;
        }
      }
      
      break;
      
    case SFDSE: // Symmetric Full Duplex Switched Ethernet
      // FROM SENDER TO SWITCH

      // go through all nodes and count down transmission times
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        if (nwsys->nwnodes[i]->state == 1) { // node has been sending
          m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst();
          // decrease remaining number of bits in current frame
          m->remaining -= (nwsys->datarate * timeElapsed);
          // frame finished?
          if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {
            // transmission is finished, move to switchQ
            debugPrintf("transmission finished, move to switchQ %d\n", i);
            m->remaining = fmax(m->length, nwsys->minsize); // restore remaining
            if (switchmalloc(nwsys, i,m->receiver,m->length)) {
              nwsys->nwnodes[i]->inputQ->removeNode(m);
              if (m->receiver == -1) {
                // Broadcast
                // Duplicate message and append to switchQ's of all nodes
                waituntil = nwsys->time + broadcast(nwsys, m, 2);
              } else {
                nwsys->nwnodes[m->receiver]->switchQ->appendNode(m);
              }
            } else {
              if (nwsys->overflow == BUFFULLRETRY) {
                debugPrintf("Switch buffer full, retransmitting!\n");
              } else {
                debugPrintf("Switch buffer full, dropping!\n");
                nwsys->nwnodes[i]->inputQ->deleteNode(m);
              }
            }
            
            nwsys->nwnodes[i]->state = 0;
            
          } else { // frame not finished
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          }
        }
      }
      
      // check if any new transmissions should be started
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        if (nwsys->nwnodes[i]->state == 0) { // idle?
          // check if we should start a new transmission
          if ((m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) {
            debugPrintf("Node %d starting transmission of frame at %f\n", i, nwsys->time);
            nwsys->nwnodes[i]->state = 1; // we're sending
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          }
        }
      }
      
      // FROM SWITCH TO RECEIVER
      
      // go through all nodes and count down transmission times
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        if (nwsys->nwnodes[i]->swstate == 1) { // node has been sending
          m = (NWmsg *)nwsys->nwnodes[i]->switchQ->getFirst();
          // decrease remaining number of bits in current frame
          m->remaining -= (nwsys->datarate * timeElapsed);
          // frame finished?
          if (m->remaining < nwsys->datarate * TT_TIME_RESOLUTION) {
            // transmission is finished, move to outputQ
            debugPrintf("transmission finished, move to outputQ %d\n", i);
            m->remaining = fmax(m->length, nwsys->minsize); // restore remaining
            nwsys->nwnodes[i]->switchQ->removeNode(m);
            nwsys->nwnodes[i]->outputQ->appendNode(m);
            nwsys->nwnodes[i]->swstate = 0;
            switchfree(nwsys, i, m->length);
          } else { // frame not finished
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          }
        }
      }
      
      // check if any new transmissions should be started
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        if (nwsys->nwnodes[i]->swstate == 0) { // idle?
          // check if we should start a new transmission
          if ((m = (NWmsg *)nwsys->nwnodes[i]->switchQ->getFirst()) != NULL) {
            debugPrintf("Switch output %d starting transmission of frame at %f\n", i, nwsys->time);
            nwsys->nwnodes[i]->swstate = 1; // we're sending
            // update nextHit?
            if (nwsys->time + remxtime(nwsys, m) < nextHit) {
              nextHit = nwsys->time + remxtime(nwsys, m);
            }
          }
        }
      }
      
      break;
      
    case PROFINET:
      int sendPort, sendToNode, fromNode;
      fromNode = -1;
      // Deliver IRT messages:
      if (nwsys->interval == IRT) {
        for (i = 0; i < nwsys->irtSchedLength; i++) {
          fromNode = nwsys->irtFrom[i];
          if (nwsys->irtSendTime[i] > 0) { // message has been sent
            debugPrintf("Transmission started at %f\n",nwsys->irtSendTime[i]);
            if (nwsys->time >  nwsys->irtSendTime[i] + nwsys->irtTransTime[i] - TT_TIME_RESOLUTION) { // arrival time is passed
              debugPrintf("Arrival time is passed at %f\n",nwsys->time);
              for (j = 0; j < nwsys->nwnodes[fromNode]->inputQ->length(); j++) { //
                if ((m = (NWmsg *)nwsys->nwnodes[fromNode]->inputQ->getFirst()) != NULL) { // there is a message in the inputQ
                  if (nwsys->irtMsgID[i] == m->msgID) { // message with correct ID found
                    nwsys->nwnodes[fromNode]->inputQ->removeNode(m);
                    nwsys->nwnodes[nwsys->irtTo[i]]->outputQ->appendNode(m);
                    debugPrintf("Msg delivered\n");
                    if (nwsys->nwnodes[fromNode]->inputQ->getFirst() != NULL) { // inputQ is not empty
                      nwsys->nwnodes[fromNode]->state = 2; // waiting
                    } else { // inputQ is empty
                      nwsys->nwnodes[fromNode]->state = 0; // idle
                      debugPrintf("Node state idle\n");
                    }
                    debugPrintf("Nbr of msgs in inputQ: %d\n",nwsys->nwnodes[fromNode]->inputQ->length());
                    nwsys->irtSendTime[i] = -TT_MAX_TIMESTEP; // reset irtSendTime[i]
                    if (nwsys->currslottime + nwsys->syncTime + nwsys->irtLength < nextHit) { // update nextHit
                      nextHit = nwsys->currslottime + nwsys->syncTime + nwsys->irtLength;
                    }
                    break;
                  } else { // wrong message ID, try next message
                    m = (NWmsg *)m->pNext;
                  }
                }
              }
            } else { // arrival time is not passed
              if (nwsys->irtSendTime[i] + nwsys->irtTransTime[i] < nextHit) { // update next hit
                nextHit = nwsys->irtSendTime[i] + nwsys->irtTransTime[i];
              }
            }
          }
        }
      }
      // Delivery RT 1/NRT messages
      if (nwsys->interval == NRT) {
        for (i = 0; i < nwsys->nbrOfNodes; i++) { // for all nodes
          for (j = 0; j < 4; j++) { // for all ports
            if ((m = (NWmsg *)nwsys->nwnodes[i]->port[j]->getFirst()) != NULL) { // message in port
              // Count down message remaining and check destination
              m->remaining -= (timeElapsed*nwsys->datarate);
              sendToNode = checkNodeDestination(nwsys, i, m);
              double bitsSent;
              bitsSent = m->length - m->remaining;
              if (nwsys->nwnodes[sendToNode]->switchmem >= nwsys->nwnodes[sendToNode]->switchMemLimit) { // the switch is empty
                if (bitsSent > ADRESSBITS && bitsSent < ADRESSBITS+1) { // address is read
                  if (m->receiver != sendToNode) {
                    m2 = new NWmsg(); // Create a new message
                    *m2 = *m;
                    m2->remaining = fmax(m2->length,nwsys->minsize); // restore remaining
                    m->cutthrough = 1; // Indicate that the message left in port is cutting through
                    nwsys->nwnodes[sendToNode]->inputQ->insertSorted(m2);
                    if (nwsys->time + remxtime(nwsys,m) < nextHit) { // update nextHit
                      nextHit = nwsys->time + remxtime(nwsys,m);
                    }
                  }
                }
              }
              if (m->remaining < (nwsys->datarate * TT_TIME_RESOLUTION)) { // message transmission complete
                switchfree(nwsys,i,m->allocMem); // free switch mem
                if (m->receiver == sendToNode) { // message has arrived at the final destination
                  nwsys->nwnodes[i]->port[j]->removeNode(m); // remove message from port
                  nwsys->nwnodes[m->receiver]->outputQ->appendNode(m); // add message to the outputQ
                } else { // message should be put in the node switch for further transmission
                  m->remaining = fmax(m->length,nwsys->minsize); // restore remaining
                  nwsys->nwnodes[i]->port[j]->removeNode(m);
                  if (m->cutthrough != 1) { // message did not cut-through
                    nwsys->nwnodes[sendToNode]->inputQ->insertSorted(m);
                  }
                }
              } else { // transmission not finished
                if (bitsSent < ADRESSBITS) { // less than ADRESSBITS bits are transmitted
                  if (nwsys->time + (ADRESSBITS - bitsSent)/nwsys->datarate < nextHit) {
                    nextHit = nwsys->time +  (ADRESSBITS - bitsSent)/nwsys->datarate;
                  }
                }
                if (nwsys->time + remxtime(nwsys, m) < nextHit) { // update nextHit
                  nextHit = nwsys->time + remxtime(nwsys, m);
                }
              }
            }
          }
        }
      }
      // Slottime management:
      if (nwsys->time > nwsys->currslottime + nwsys->slottime - TT_TIME_RESOLUTION) { // If a new cycle is entered increase nwsys->currslottime to the start of the new cycle
        nwsys->currslottime += nwsys->slottime;
      }
      if (nwsys->currslottime + nwsys->slottime < nextHit) { // set nextHit to end of cycle
        nextHit = nwsys->currslottime + nwsys->slottime;
      }
      
      //Interval selection:
      if (nwsys->time > nwsys->currslottime - TT_TIME_RESOLUTION && nwsys->time < nwsys->currslottime + nwsys->syncTime) {
        nwsys->interval = SYNC;
        debugPrintf("SYNC\n");
      } else if (nwsys->irtSchedLength > 0 && nwsys->currslottime + nwsys->syncTime <= nwsys->time && nwsys->time <= nwsys->currslottime + nwsys->syncTime + nwsys->irtLength) {
        nwsys->interval = IRT;
        debugPrintf("IRT\n");
      } else {
        nwsys->interval = NRT;
        debugPrintf("NRT\n");
      }
      
      if (nwsys->interval == SYNC) { // Synchronization interval
        if (nwsys->currslottime + nwsys->syncTime < nextHit) { // set nextHit
          nextHit = nwsys->currslottime + nwsys->syncTime;
        }
        
      } else if (nwsys->interval == IRT) { // RT Class 3 / IRT interval
        for (i = 0; i < nwsys->irtSchedLength; i++) {// go through IRT schedule
          if (nwsys->time > nwsys->currslottime + nwsys->syncTime + nwsys->irtStartTime[i] - TT_TIME_RESOLUTION) { // start transmission time is passed
            fromNode = nwsys->irtFrom[i];
            // Resend last incoming msg:
            if (nwsys->nwnodes[fromNode]->inputQ->getFirst() == NULL && nwsys->nwnodes[fromNode]->lastMsg != NULL) {
              if (nwsys->time < nwsys->currslottime + nwsys->syncTime + nwsys->irtStartTime[i] + nwsys->irtTransTime[i]-TT_TIME_RESOLUTION ) {
                if (nwsys->nwnodes[fromNode]->lastMsg->msgID == nwsys->irtMsgID[i]) {
                  m2 = new NWmsg();
                  *m2 = *nwsys->nwnodes[fromNode]->lastMsg;
                  if (nwsys->nwnodes[fromNode]->lastMsg->dataMATLAB != NULL) {
                    m2->dataMATLAB = mxDuplicateArray(nwsys->nwnodes[fromNode]->lastMsg->dataMATLAB);
                    mexMakeArrayPersistent(m2->dataMATLAB);
                  }
                  nwsys->nwnodes[fromNode]->inputQ->appendNode(m2);
                }
              }
            }
            //Start transmission:
            if ((m = (NWmsg *)nwsys->nwnodes[fromNode]->inputQ->getFirst()) != NULL) { // there is a message in the inputQ
              if (nwsys->irtSendTime[i] < 0) { // message has not been sent
                if (nwsys->irtTransTime[i] < nwsys->currslottime + nwsys->syncTime + nwsys->irtStartTime[i] + nwsys->irtTransTime[i] + TT_TIME_RESOLUTION - nwsys->time) { // There is enough time to transmit message
                  for (j = 0; j < nwsys->nwnodes[fromNode]->inputQ->length(); j++) {
                    if (nwsys->irtMsgID[i] == m->msgID) { // message with correct ID found, start sending message
                      nwsys->nwnodes[fromNode]->state = 1; // set state = sending
                      nwsys->irtSendTime[i] = nwsys->time; // set time stamp
                      if (nwsys->time + nwsys->irtTransTime[i] < nextHit) { // update nextHit
                        nextHit = nwsys->time + nwsys->irtTransTime[i];
                      }
                      break;
                    } else { // Check next message
                      m = (NWmsg *)m->pNext;
                      if (m == NULL) {
                        break;
                      }
                    }
                  }
                }
              }
            } else { // no message available - find next start time
              for (j = 0; j < nwsys->irtSchedLength; j++) {
                if (nwsys->time < nwsys->currslottime + nwsys->syncTime + nwsys->irtStartTime[j]) { // start time not passed
                  if (nwsys->currslottime + nwsys->syncTime + nwsys->irtStartTime[j] < nextHit) { // update nextHit
                    nextHit = nwsys->currslottime + nwsys->syncTime + nwsys->irtStartTime[j];
                  }
                }
              }
            }
          } else { // transmission time is not passed
            if (nwsys->currslottime + nwsys->syncTime + nwsys->irtStartTime[i] < nextHit) {
              nextHit = nwsys->currslottime + nwsys->syncTime + nwsys->irtStartTime[i];
            }
          }
        }
        // Set nextHit to the end of IRT interval
        if (nwsys->currslottime + nwsys->syncTime + nwsys->irtLength < nextHit) { // update nextHit
          nextHit  = nwsys->currslottime + nwsys->syncTime + nwsys->irtLength;
        }
        
      } else if (nwsys->interval == NRT) { // RT Class 1 /NRT interval
        for (i=0; i < nwsys->nbrOfNodes; i++) { // for all nodes
          int temp = nwsys->nwnodes[i]->inputQ->length();
          if ((m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) { // inputQ is not empty
            for (j = 0; j < temp; j++) { // Move all messages with msgId == 0 in inputQ to switchQ
              if (m->msgID == 0) {
                if (nwsys->nwnodes[i]->switchmem < nwsys->nwnodes[i]->switchMemLimit) { // switchQ is non-empty
                  if (nodeSwitchMalloc(nwsys, i, m->length)) { // store-and-forward
                    nwsys->nwnodes[i]->inputQ->removeNode(m);
                    nwsys->nwnodes[i]->switchQ->insertSorted(m);
                    m->allocMem = m->length;
                  } else { // switch overflow
                    if (nwsys->overflow == BUFFULLRETRY) {
                      debugPrintf("Switch buffer full, retransmitting!\n");
                    } else {
                      debugPrintf("Switch buffer full, dropping!\n");
                      nwsys->nwnodes[i]->inputQ->deleteNode(m);
                    }
                  }
                } else { // switchQ is empty, cut-through
                  if(nodeSwitchMalloc(nwsys, i, ADRESSBITS)) {
                    nwsys->nwnodes[i]->inputQ->removeNode(m);
                    nwsys->nwnodes[i]->switchQ->insertSorted(m);
                    m->allocMem = ADRESSBITS;
                  }
                }
              }
              m = (NWmsg *)m->pNext;
              if (m == NULL) {
                break;
              }
            }
          }
          // Move messages from switchQ to port
          for (j = 0; j < 4; j++) { // can max move 4 messages from switchQ to the ports
            if ((m = (NWmsg *)nwsys->nwnodes[i]->switchQ->getFirst()) != NULL) { // switchQ is non-empty
              if (m->length <= (nwsys->currslottime + nwsys->slottime - nwsys->time + TT_TIME_RESOLUTION)*nwsys->datarate) {
                sendToNode = checkNodeDestination(nwsys, i, m);
                sendPort = checkPortDestination(nwsys, i, sendToNode);
                if (sendPort != -1) {
                  if(nwsys->nwnodes[i]->port[sendPort]->getFirst() == NULL) { // port is free;
                    nwsys->nwnodes[i]->switchQ->removeNode(m); // remove from switchQ
                    nwsys->nwnodes[i]->port[sendPort]->appendNode(m); // put message in port
                    if (nwsys->time + ADRESSBITS/nwsys->datarate < nextHit) { // update nextHit
                      // Set nextHit to time when the address bits are read at receiving node
                      nextHit = nwsys->time + ADRESSBITS/nwsys->datarate;
                    }
                  }
                }
              }
            } else { // swicthQ is empty, no messages to move
              if (nwsys->currslottime + nwsys->slottime < nextHit) { // update nextHit
                nextHit = nwsys->currslottime + nwsys->slottime;
              }
              break;
            }
          }
          //Test: f?rs?ka att f? till fler hits f?r att undvika problem
          if (nwsys->currslottime + nwsys->slottime < nextHit) {
            nextHit = nwsys->currslottime + nwsys->slottime;
          }
          // Set state for plotting purposes
          if ((m = (NWmsg *)nwsys->nwnodes[i]->switchQ->getFirst()) != NULL) { // The switchQ is not empty
            nwsys->nwnodes[i]->state = 2;
          } else {
            nwsys->nwnodes[i]->state = 0;
          }
          for (int j = 0; j < 4; j++) {
            if ((m = (NWmsg *)nwsys->nwnodes[i]->port[j]->getFirst()) != NULL) { // a port contains a message i.e., node is sending
              nwsys->nwnodes[i]->state = 1;
              break;
            }
          }
        }
      }
      break;
    default:
      debugPrintf("Protocol not implemented!\n");
      break;
  }
  
  // Check if messages have finished waiting in the outputQ's
  for (i=0; i<nwsys->nbrOfNodes; i++) {
    m = (NWmsg *)nwsys->nwnodes[i]->outputQ->getFirst();
    while (m != NULL) {
      next = (NWmsg *)m->getNext();
      if (m->waituntil - nwsys->time < TT_TIME_RESOLUTION) {
        // finished waiting, move to postprocQ
        debugPrintf("*** moving message from outputQ to postprocQ %d at %f\n", i, nwsys->time);
        nwsys->nwnodes[i]->outputQ->removeNode(m);
        double rNbr = unirand(nwsys);
        if (rNbr < nwsys->lossprob) {
          // packet lost, do not forward to post-proc
          debugPrintf("Network: A packet headed for node # %d, was lost at time %f\n", i , nwsys->time);
          delete m;
          m = NULL;
        } else {
          nwsys->nwnodes[i]->postprocQ->appendNode(m);
          if (nwsys->outputs[i] == 0.0) {
            debugPrintf("trigger rcv output %d\n", i);
            nwsys->outputs[i] = 1.0; // trigger rcv output
          } else {
            nwsys->outputs[i] = 0.0; // trigger rcv output
          }
        }
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
  
  debugPrintf("Next hit scheduled for %f\n\n", nextHit);
  
  // produce output graph
  if (nwsys->type == PROFINET) {
    for (i=0; i<nwsys->nbrOfNodes; i++) {
      if (nwsys->nwnodes[i]->state == 1) {
        nwsys->sendschedule[i] = i+1.5;  // sending
      } else if ((m = (NWmsg *)nwsys->nwnodes[i]->switchQ->getFirst()) != NULL || (m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL) {
        nwsys->sendschedule[i] = i+1.25; // waiting
      } else {
        nwsys->sendschedule[i] = i+1.0;     // idle
      }
    }
  } else {
    for (i=0; i<nwsys->nbrOfNodes; i++) {
      if (nwsys->nwnodes[i]->state == 1) {
        nwsys->sendschedule[i] = i+1.5;  // sending
      } else if ((m = (NWmsg *)nwsys->nwnodes[i]->inputQ->getFirst()) != NULL || (m = (NWmsg *)nwsys->nwnodes[i]->statinputQ->getFirst()) != NULL) {
        nwsys->sendschedule[i] = i+1.25; // waiting
      } else {
        nwsys->sendschedule[i] = i+1.0;     // idle
      }
    }
  }
  return nextHit;
}

// ------- Simulink callback functions -------

#ifdef __cplusplus
extern "C" { // use the C fcn-call standard for all functions
#endif       // defined within this scope
  
#define S_FUNCTION_NAME ttnetwork
#define S_FUNCTION_LEVEL 2
  
#include "simstruc.h"
  
  static void mdlInitializeSizes(SimStruct *S)
  {
    debugPrintf("'%s': mdlInitializeSizes\n", S->path);
    
    int nbrOfNodes = 0;
    const mxArray *arg;
    
    ssSetNumSFcnParams(S, 21);  /* Number of expected parameters */
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
      return; /* Parameter mismatch will be reported by Simulink */
    }
    
    // Parse second argument only to determine nbrOfNodes
    
    // Arg 3 - Number of nodes
    arg = ssGetSFcnParam(S, 2);
    if (mxIsDoubleScalar(arg)) {
      nbrOfNodes = (int) *mxGetPr(arg);
    }
    if (nbrOfNodes < 1) {
      ssSetErrorStatus(S, "TrueTime Network: The number of nodes must be an integer > 0");
      return;
    }
    
    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);
    
    ssSetNumInputPorts(S, 1);
    ssSetInputPortDirectFeedThrough(S, 0, 0);
    ssSetInputPortWidth(S, 0, nbrOfNodes);
    
    
    ssSetNumOutputPorts(S, 2);
    ssSetOutputPortWidth(S, 0, nbrOfNodes);
    ssSetOutputPortWidth(S, 1, nbrOfNodes);
    
    ssSetNumSampleTimes(S, 1);
    
    ssSetNumRWork(S, 0);
    ssSetNumIWork(S, 0);
    ssSetNumPWork(S, 0);
    ssSetNumModes(S, 0);
    ssSetNumNonsampledZCs(S, 1);
    
    // Make sure cleanup is performed even if errors occur
    ssSetOptions(S, SS_OPTION_CALL_TERMINATE_ON_EXIT);
    
  }
  
  
  static void mdlInitializeSampleTimes(SimStruct *S)
  {
    ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, FIXED_IN_MINOR_STEP_OFFSET);
  }
  
  
#define MDL_START
  static void mdlStart(SimStruct *S)
  {
    debugPrintf("'%s': mdlStart\n", S->path);
    
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
    
    // allocate data
    
    const mxArray *arg;
    int i;
    
    // Create new network struct
    
    // ----- Main data structure ------
    
    RTnetwork *nwsys = new RTnetwork;

    ssSetUserData(S, nwsys); // save pointer in UserData
    
    // Arg 1 - Network type
    arg = ssGetSFcnParam(S, 0);
    if (mxIsDoubleScalar(arg)) {
      nwsys->type = (int)(*mxGetPr(arg))-1;
    }
    if (nwsys->type < 0) {
      ssSetErrorStatus(S, "Error in type argument");
      return;
    }
    debugPrintf("  type: %d\n", nwsys->type);
    
    // Arg 2 - Network Number
    arg = ssGetSFcnParam(S, 1);
    if (mxIsDoubleScalar(arg)) {
      nwsys->networkNbr = (int) *mxGetPr(arg);
    }
    debugPrintf("  Network Number: %d\n", nwsys->networkNbr);
    
    // Arg 3 - Number of nodes
    arg = ssGetSFcnParam(S, 2);
    if (mxIsDoubleScalar(arg)) {
      nwsys->nbrOfNodes = (int) *mxGetPr(arg);
    }
      
    debugPrintf("  nbrOfNodes: %d\n", nwsys->nbrOfNodes);
    
    // Arg 4 - Data rate (bits/s)
    arg = ssGetSFcnParam(S, 3);
    if (mxIsDoubleScalar(arg)) {
      nwsys->datarate = *mxGetPr(arg);
    }
    if (nwsys->datarate < 0.0) {
      ssSetErrorStatus(S, "The data rate must be > 0");
      return;
    }
    debugPrintf("Datarate: %f\n", nwsys->datarate);
    
    // Arg 5 - Minimum frame size
    arg = ssGetSFcnParam(S, 4);
    if (mxIsDoubleScalar(arg)) {
      nwsys->minsize = (int) *mxGetPr(arg);
    }
    if (nwsys->minsize <= 0) {
      ssSetErrorStatus(S, "The minimum frame size must be > 0");
      return;
    }
    
    // Arg 6 - Loss Probability
    arg = ssGetSFcnParam(S, 5);
    if (mxIsDoubleScalar(arg)) {
      nwsys->lossprob = *mxGetPr(arg);
    }
    if (nwsys->lossprob < 0.0 || nwsys->lossprob > 1.0) {
      ssSetErrorStatus(S, "The loss probability must be between 0 and 1");
      return;
    }
    debugPrintf("Loss Probability: %f\n", nwsys->lossprob);
    
    if (nwsys->type == FDMA) {
      
      // Arg 7 - Bandwidth allocations for FDMA
      arg = ssGetSFcnParam(S, 6);
      if (mxIsDouble(arg)) {
        if ((size_t)mxGetNumberOfElements(arg) != (size_t)nwsys->nbrOfNodes) {
          ssSetErrorStatus(S, "The number of bandwidth allocations must equal the number of nodes");
          return;
        }
        nwsys->bandwidths = new double[nwsys->nbrOfNodes];
        for (i=0; i<nwsys->nbrOfNodes; i++) {
          nwsys->bandwidths[i] = mxGetPr(arg)[i];
          debugPrintf("bandwidth %d: %f\n", i, nwsys->bandwidths[i]);
        }
      } else {
        ssSetErrorStatus(S, "The bandwidth allocations must be a vector of doubles");
      }
      
    }
    
    if (nwsys->type == TDMA || nwsys->type == FLEXRAY) {
      
      // Arg 8 - Slotsize (translated into slottime) for TDMA and FLEXRAY
      arg = ssGetSFcnParam(S, 7);
      if (mxIsDoubleScalar(arg)) {
        nwsys->slottime = *mxGetPr(arg)/nwsys->datarate;
      }
      if (nwsys->slottime <= 0.0) {
        ssSetErrorStatus(S, "The slot size must be > 0");
        return;
      }
      
      // Arg 9 - Static schedule for TDMA and FLEXRAY
      arg = ssGetSFcnParam(S, 8);
      if (mxIsDouble(arg)) {
        nwsys->schedsize = mxGetNumberOfElements(arg);
        debugPrintf("  Static scheduler size: %d\n",nwsys->schedsize);
      }
      if (nwsys->schedsize < 1) {
        ssSetErrorStatus(S, "A schedule must be entered");
        return;
      }
      nwsys->schedule = new int[nwsys->schedsize];
      debugPrintf("schedule: ");
      for (i=0; i<nwsys->schedsize; i++) {
        nwsys->schedule[i] = ((int)mxGetPr(arg)[i])-1;
        if (nwsys->schedule[i] < -1 || nwsys->schedule[i] > nwsys->nbrOfNodes-1) {
          ssSetErrorStatus(S, "Illegal node (< 0 or > nbrOfNodes) in schedule");
          return;
        }
        debugPrintf("%d ", nwsys->schedule[i]);
      }
      debugPrintf("\n");
    }
    
    if (nwsys->type == SFDSE) {
      
      // Arg 10 - Switch memory size for Swithed Ethernet
      arg = ssGetSFcnParam(S, 9);
      if (mxIsDoubleScalar(arg)) {
        nwsys->memsize = (int) *mxGetPr(arg);
      }
      if (nwsys->memsize <= 0) {
        ssSetErrorStatus(S, "The switch memory size must be > 0");
        return;
      }
      nwsys->switchmem = nwsys->memsize;
      
      // Arg 11 - Switch buffer type for Swithed Ethernet
      arg = ssGetSFcnParam(S, 10);
      if (mxIsDoubleScalar(arg)) {
        nwsys->buftype = (int)(*mxGetPr(arg))-1;
      }
      if (nwsys->buftype < 0) {
        ssSetErrorStatus(S, "Error in buffer type argument");
        return;
      }
      
      // Arg  12 - Switch overflow behavior for Swithed Ethernet
      arg = ssGetSFcnParam(S, 11);
      if (mxIsDoubleScalar(arg)) {
        nwsys->overflow = (int)(*mxGetPr(arg))-1;
      }
      if (nwsys->overflow < 0) {
        ssSetErrorStatus(S, "Error in buffer type argument");
        return;
      }
    }
    if (nwsys->type == PROFINET) {
      // Arg 10 - Switch memory size for PROFINET
      arg = ssGetSFcnParam(S, 9);
      if (mxIsDoubleScalar(arg)) {
        nwsys->memsize = (int) *mxGetPr(arg);
      }
      if (nwsys->memsize <= 0) {
        ssSetErrorStatus(S, "The switch memory size must be > 0");
        return;
      }
      nwsys->switchmem = nwsys->memsize;
      
      // Arg 11 - Switch buffer type for PROFINET
      nwsys->buftype = OUTPUTBUF;
      
      // Arg  12 - Switch overflow behavior for PROFINET
      arg = ssGetSFcnParam(S, 11);
      if (mxIsDoubleScalar(arg)) {
        nwsys->overflow = (int)(*mxGetPr(arg))-1;
      }
      if (nwsys->overflow < 0) {
        ssSetErrorStatus(S, "Error in buffer type argument");
        return;
      }
    }

    // Arg 13 - Seed for random number generator
    arg = ssGetSFcnParam(S, 12);
    unsigned int seed = 0;
    if (mxIsDoubleScalar(arg)) {
      seed = (unsigned int)*mxGetPr(arg);
      tinymt32_init(&nwsys->randstate, seed);
      debugPrintf("random number generator seed = %d\n", seed);
    }

    if (nwsys->type == FLEXRAY) {
      // Arg 14 - Schedule for the dynamic segment
      arg = ssGetSFcnParam(S,13);
      if (mxIsDouble(arg)) {
        nwsys->dynSchedSize = mxGetNumberOfElements(arg);
      }
      debugPrintf("  Dynamic scheduler size: %d \n", nwsys->dynSchedSize);
      if (nwsys->dynSchedSize >= 1) {
        nwsys->dynSchedule = new int[nwsys->dynSchedSize];
        debugPrintf("  Dynamic Schedule (node): ");
        for (i=0; i < nwsys->dynSchedSize; i++) {
          nwsys->dynSchedule[i] = ((int)mxGetPr(arg)[i])-1;
          debugPrintf("%d ", nwsys->dynSchedule[i]+1);
          if (nwsys->dynSchedule[i] <= -1 || nwsys->dynSchedule[i] > nwsys->nbrOfNodes-1) {
            ssSetErrorStatus(S, "Illegal node (<= 0 or > nbrOfNodes) in dynamic segment schedule");
            return;
          }
        }
      }
      

      // Arg 15 - Size of mini slots
      arg = ssGetSFcnParam(S,14);
      if (mxIsDoubleScalar(arg)) {
        nwsys->miniSlotSize = *mxGetPr(arg);
      }
      if (nwsys->miniSlotSize <= 0.0) {
        ssSetErrorStatus(S, "Mini slot size must be > 0");
        return;
      }
      
      // Arg 16 - Network Idle Time
      arg = ssGetSFcnParam(S,15);
      if (mxIsDoubleScalar(arg)) {
        nwsys->NIT = *mxGetPr(arg);
      }
      if (nwsys->NIT < 0.0) {
        ssSetErrorStatus(S, "Network Idle Time must be >= 0");
        return;
      }
    }
    // Arg 17 - InputQs sort mode
    arg = ssGetSFcnParam(S,16);
    if(mxIsDoubleScalar(arg)) {
      nwsys->QSortMode = (int)*mxGetPr(arg)-1; // QSortMode = 1 => -FIFO, QSortMode = 2 => Priotity
    }
    if(nwsys->type == PROFINET) {// Set correct priority setting for PROFINET
      nwsys->QSortMode = PRIORITYHIGH;
    }

 
     if (nwsys->type == PROFINET) {
      // Arg 18 - Synchronization length
      arg = ssGetSFcnParam(S,17);
      if(mxIsDoubleScalar(arg)) {
        nwsys->syncTime = *mxGetPr(arg)/nwsys->datarate;
        if (nwsys->syncTime < 0.0) {
          ssSetErrorStatus(S, "Synchronization interval must be >= 0");
          return;
        }
        debugPrintf("SyncTime = %f s\n", nwsys->syncTime);
      }
      // Arg 19 - IRT schedule
      arg = ssGetSFcnParam(S,18);
      if (!mxIsDoubleScalar(arg)) {
        if (mxIsDouble(arg)) {
          if (5 == (int)mxGetN(arg)) {
            nwsys->irtSchedLength = (int)mxGetM(arg);
            debugPrintf("irtSchedLength = %d \n",nwsys->irtSchedLength);
            
            nwsys->irtMsgID = new int[nwsys->irtSchedLength];
            nwsys->irtFrom = new int[nwsys->irtSchedLength];
            nwsys->irtTo = new int[nwsys->irtSchedLength];
            nwsys->irtStartTime = new double[nwsys->irtSchedLength];
            nwsys->irtTransTime = new double[nwsys->irtSchedLength];
            nwsys->irtSendTime =  new double[nwsys->irtSchedLength];
            
            nwsys->irtLength = 0.0;
            for (i=0; i < nwsys->irtSchedLength; i++) {
              nwsys->irtMsgID[i] = ((int)mxGetPr(arg)[i]);
              nwsys->irtFrom[i] = ((int)mxGetPr(arg)[nwsys->irtSchedLength + i])-1;
              if (nwsys->irtFrom[i] <= -1 || nwsys->irtFrom[i] > nwsys->nbrOfNodes-1) {
                ssSetErrorStatus(S, "Illegal sending node (<= 0 or > nbrOfNodes) in IRT-schedule");
                return;
              }
              nwsys->irtTo[i] = ((int)mxGetPr(arg)[nwsys->irtSchedLength*2 + i])-1;
              if (nwsys->irtTo[i] <= -1 || nwsys->irtTo[i] > nwsys->nbrOfNodes-1) {
                ssSetErrorStatus(S, "Illegal receiving node (<= 0 or > nbrOfNodes) in IRT-schedule");
                return;
              }
              nwsys->irtStartTime[i] = ((double)mxGetPr(arg)[nwsys->irtSchedLength*3 + i]);
              if (nwsys->irtStartTime[i] < 0) {
                ssSetErrorStatus(S, "Illegal start time (< 0) in IRT-schedule");
                return;
              }
              nwsys->irtTransTime[i] = mxGetPr(arg)[nwsys->irtSchedLength*4 + i];
              if (nwsys->irtTransTime[i] < 0) {
                ssSetErrorStatus(S, "Illegal transmission time (< 0) in IRT-schedule");
                return;
              }
              nwsys->irtSendTime[i] = -TT_MAX_TIMESTEP;
              /*
               * debugPrintf("irtMsgID[%d] = %d\n", i, nwsys->irtMsgID[i]);
               * debugPrintf("irtFrom[%d] = %d\n", i, nwsys->irtFrom[i]);
               * debugPrintf("irtTo[%d] = %d\n", i, nwsys->irtTo[i]);
               * debugPrintf("irtStartTime[%d] = %f\n", i, nwsys->irtStartTime[i]);
               * debugPrintf("irtTransTime[%d] = %f\n", i, nwsys->irtTransTime[i]);
               */
              if (nwsys->irtStartTime[i] +  nwsys->irtTransTime[i] > nwsys->irtLength) {
                nwsys->irtLength = nwsys->irtStartTime[i] +  nwsys->irtTransTime[i];
              }
            }
            if (nwsys->irtLength < 0.0) {
              ssSetErrorStatus(S, "The length of the IRT interval is < 0");
              return;
            }
          }
        } else {
          ssSetErrorStatus(S, "The IRT schedule does not have 5 columns");
        }
      } else {
        nwsys->irtLength = 0.0;
      }
      // Arg 20 - RT Class 1 / NRT length
      arg = ssGetSFcnParam(S,19);
      if (mxIsDoubleScalar(arg)) {
        nwsys->nrtLength = *mxGetPr(arg)/nwsys->datarate;
        if (nwsys->nrtLength < 0.0) {
          ssSetErrorStatus(S, "RT Class 1 / NRT interval must be >= 0");
          return;
        }
        debugPrintf("NRT length = %f s\n",nwsys->nrtLength);
      }
      nwsys->slottime = nwsys->irtLength + nwsys->nrtLength + nwsys->syncTime;
      debugPrintf("Cycle time = %f s\n",nwsys->slottime);
      
      // Arg 21 - Node graph
      arg = ssGetSFcnParam(S,20);
      if (mxGetNumberOfElements(arg) != nwsys->nbrOfNodes*nwsys->nbrOfNodes) {
        ssSetErrorStatus(S,"Node graph: number of elements is not nbrOfNodes^2");
        return;
      }
      if (mxGetM(arg) != mxGetN(arg)) {
        ssSetErrorStatus(S, "Node graph dimension mismatch. Argument has to be a (n x n) matrix");
        return;
      }
      nwsys->nodeRouting = new int *[nwsys->nbrOfNodes];
      nwsys->portDestination =  new int *[nwsys->nbrOfNodes];
      int flag = 0;
      for (i=0; i < nwsys->nbrOfNodes; i++) {
        nwsys->nodeRouting[i] = new int [nwsys->nbrOfNodes];
        nwsys->portDestination[i] = new int [4];
        
        for (int j=0; j < nwsys->nbrOfNodes; j++) {
          nwsys->nodeRouting[i][j] = (int)mxGetPr(arg)[i + nwsys->nbrOfNodes*j]-1;
          
        }
        for (int j=0; j < 4; j++) {
          nwsys->portDestination[i][j] = -1;
        }
        for (int j=0; j < nwsys->nbrOfNodes; j++) {
          if (nwsys->nodeRouting[i][j] != i) {
            for (int k=0; k < 4; k++) {
              if (nwsys->portDestination[i][k] == nwsys->nodeRouting[i][j]) {
                flag = 1;
                break;
              }
            }
            if (flag == 0) {
              for (int l=0; l < 4; l++) {
                if (nwsys->portDestination[i][l] == -1) {
                  nwsys->portDestination[i][l] = nwsys->nodeRouting[i][j];
                  break;
                }
              }
            } else {
              flag = 0;
            }
          }
        }
      }
      for(i=0; i < nwsys->nbrOfNodes; i++) {
        for(int j=0; j < nwsys->nbrOfNodes; j++) {
        }
      }
      for(i=0; i < nwsys->nbrOfNodes; i++) {
        for(int j=0; j < 4; j++) {
        }
      }
    }
    
    /* Write nwsys pointer to global workspace so that the kernel
     * blocks can access it */

    debugPrintf(" ttnw 1\n");
    mxArray* var = mxCreateDoubleScalar(0.0);
    debugPrintf(" ttnw 2\n");
    mexMakeArrayPersistent(var);
    debugPrintf(" ttnw 3\n");
    *((void **)mxGetData(var)) = nwsys;
    debugPrintf(" ttnw 4\n");
    char nwsysbuf[MAXCHARS];
    sprintf(nwsysbuf, "_nwsys_%d", nwsys->networkNbr);
    mexPutVariable("global", nwsysbuf, var);
    debugPrintf(" ttnw 5\n");

    nwsys->inputs = new double[nwsys->nbrOfNodes];
    nwsys->oldinputs = new double[nwsys->nbrOfNodes];
    nwsys->outputs   = new double[nwsys->nbrOfNodes];
    nwsys->sendschedule = new double[nwsys->nbrOfNodes];
    
    for (i=0; i<nwsys->nbrOfNodes; i++) {
      nwsys->inputs[i] = 0.0;
      nwsys->oldinputs[i] = 0.0;
      nwsys->outputs[i] = 0.0;
      nwsys->sendschedule[i] = i+1;
    }
    
    nwsys->time = 0.0;
    nwsys->prevHit = 0.0;
    
    debugPrintf(" ttnw 6\n");

    nwsys->nwnodes = new NWnode*[nwsys->nbrOfNodes];
    if (nwsys->QSortMode == FIFO) { //FIFO-sorted queues
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        nwsys->nwnodes[i] = new NWnode();
        nwsys->nwnodes[i]->switchmem = nwsys->switchmem / nwsys->nbrOfNodes;
        nwsys->nwnodes[i]->switchMemLimit = nwsys->nwnodes[i]->switchmem;
      }
    } else if(nwsys->QSortMode == PRIORITYLOW) { //Prioity-sorted queues. Low priority number = high priority
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        nwsys->nwnodes[i] = new NWnode(&msgLowPrioComp);
        nwsys->nwnodes[i]->switchmem = nwsys->switchmem / nwsys->nbrOfNodes;
        nwsys->nwnodes[i]->switchMemLimit = nwsys->nwnodes[i]->switchmem;
      }
    } else { // Prioity-sorted queues. High priority number = high priority
      for (i=0; i<nwsys->nbrOfNodes; i++) {
        nwsys->nwnodes[i] = new NWnode(&msgHighPrioComp);
        nwsys->nwnodes[i]->switchmem = nwsys->switchmem / nwsys->nbrOfNodes;
        nwsys->nwnodes[i]->switchMemLimit = nwsys->nwnodes[i]->switchmem;
      }
    }
 
    debugPrintf(" ttnw 7\n");
    
    nwsys->waituntil = 0.0;
    nwsys->sending = -1;  // Note! -1 means nobody is sending
    nwsys->rrturn = nwsys->nbrOfNodes - 1; // want to start at 0
    nwsys->lasttime = -1.0;
    
    nwsys->slotcount = -1;       // want to start at 0
    nwsys->miniSlotCount = -1;
    nwsys->currslottime = -nwsys->slottime;  // same here
    nwsys->msgMiniSlots = 0;
    
    nwsys->nextNode=0;
    
    nwsys->networkIdleUntil = TT_MAX_TIMESTEP;

    debugPrintf(" ttnw 8\n");

  }
  
  static void mdlOutputs(SimStruct *S, int_T tid)
  {
    debugPrintf("'%s': mdlOutputs at %.14f\n", S->path, ssGetT(S));
    
    int i, exttrig = 0;
    RTnetwork *nwsys = (RTnetwork*)ssGetUserData(S);
    if (nwsys == NULL) {
      debugPrintf("ERROR: NULL pointer in nwsys\n");
      return;
    }
   
    nwsys->time = ssGetT(S);
    
    for (i=0; i < nwsys->nbrOfNodes; i++) {
      
      if (fabs(nwsys->inputs[i]-nwsys->oldinputs[i]) > 0.1) {
        nwsys->oldinputs[i] = nwsys->inputs[i];
        exttrig = 1;
      }
    }
    if (exttrig == 1) {
      nwsys->nextHit = runNetwork(nwsys);
    } else {
      if (nwsys->time >= nwsys->nextHit) {
        nwsys->nextHit = runNetwork(nwsys);
      }
    }
    
    for (i=0; i<nwsys->nbrOfNodes; i++) {
      ssGetOutputPortRealSignal(S,0)[i] = nwsys->outputs[i];
    }
    
    for (i=0; i<nwsys->nbrOfNodes; i++) {
      ssGetOutputPortRealSignal(S,1)[i] = nwsys->sendschedule[i];
    }
  }
  
#define MDL_ZERO_CROSSINGS
  static void mdlZeroCrossings(SimStruct *S)
  {
    //debugPrintf("'%s': mdlZeroCrossings at %.14f\n", S->path, ssGetT(S));
    
    RTnetwork *nwsys = (RTnetwork*)ssGetUserData(S);
    if (nwsys == NULL) {
      mexPrintf("ERROR: NULL pointer in nwsys\n");
      return;
    }
    
    /* Copy inputs and check for events */
    for (int i=0; i < nwsys->nbrOfNodes; i++) {
      if (fabs(*ssGetInputPortRealSignalPtrs(S,0)[i] - nwsys->inputs[i]) > 0.1) {
        nwsys->nextHit = ssGetT(S);
      }
      nwsys->inputs[i] = *ssGetInputPortRealSignalPtrs(S,0)[i];
    }
    ssGetNonsampledZCs(S)[0] = nwsys->nextHit - ssGetT(S);
    
  }
  
  
  
  static void mdlTerminate(SimStruct *S)
  {
    debugPrintf("ttnetwork mdlTerminate\n");
    
    if (ssGetUserData(S) == NULL) return;
    
    RTnetwork *nwsys = (RTnetwork*) ssGetUserData(S);
    if (nwsys == NULL) {
      mexPrintf("ERROR: NULL pointer in nwsys\n");
      return;
    }
    
    int nwNbr = nwsys->networkNbr;
    
    delete nwsys;
    nwsys = NULL;
    
    char nwsysbuf[MAXCHARS];
    mxArray* rhs[2];
    sprintf(nwsysbuf, "_nwsys_%d", nwNbr);
    rhs[0] = mxCreateString("global");
    rhs[1] = mxCreateString(nwsysbuf);
    mexCallMATLAB(0, NULL, 2, rhs, "clear");
    
  }
  
  
#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
  
#ifdef __cplusplus
} // end of extern "C" scope
#endif
