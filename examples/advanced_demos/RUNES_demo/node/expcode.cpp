double expcode(int seg, void* data) {

  Exp_Data *d = (Exp_Data*) data;

  double mintime = INF; // defined in ttkernel.h

  double now = ttCurrentTime();

  

  int k;

  

  for(k=0; k<NBR_AODV; k++) {

    // Invalidate timed out entry(ies) and compute new timer

    if (d->routing_table[k].valid) {

      if ((d->routing_table[k].exptime - now) < EPS) {

	d->routing_table[k].valid = 0;

      } else {

          mintime = min(mintime, d->routing_table[k].exptime);

      }

    }

  }



  if (VERBOSE) {

    mexPrintf("Time: %f timer expiry in Node#%d\n", now, d->nodeID);

  }

    

  // Create new timer if mintime != INF

  if (mintime < INF-100.0) {

    ttCreateTimer("exptimer", mintime, "timer_handler");

    d->expTimer = 1;

    

    if (VERBOSE) {

      mexPrintf("Time: %f Node#%d updating expiry timer to time: %f\n", now, d->nodeID, mintime);

    }

  } else {

    d->expTimer = 0;

  }

     

  return FINISHED;  

}

