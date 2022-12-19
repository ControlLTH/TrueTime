#define INF 1.0E+15 

#define EPS 1.0E-15



void updateExpiryTimer(RouteEntry *table, Exp_Data *timerdata, int nodeID) {

  

  double mintime = INF; // defined in ttkernel.h

  double now;

  int i;

  

  if (timerdata->expTimer) {

    ttRemoveTimer("exptimer");

  }

  

  for (i=0; i<NBR_AODV; i++) {

    if (table[i].valid) {

      if (table[i].exptime < mintime) {

	mintime = table[i].exptime;

      }

    }

  }

  

  if (mintime < INF-100.0) {

    ttCreateTimer("exptimer", mintime, "timer_handler");

    timerdata->expTimer = 1;

    

    now = ttCurrentTime();

    

    if (VERBOSE) {

      mexPrintf("Time: %f Node#%d updating expiry timer to time: %f\n", now, nodeID, mintime);

    }

    

  } else {

    timerdata->expTimer = 0;

  }



}

