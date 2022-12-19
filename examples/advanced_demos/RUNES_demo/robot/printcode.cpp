double printcode(int seg, void* data) {



  AODVrcv_Data *d = (AODVrcv_Data *) data;

  

  int nodeID;

  int i,j;

  

  

  nodeID = d->nodeID;

  

  if (ttCurrentTime() > 0.5) {

    

    mexPrintf("\n\nRouting table for robot node %d: \n\n",nodeID);

    

    for (i=0; i < NBR_AODV-2; i++) {

      if (d->routing_table[i].valid == 1) {

	mexPrintf("  To %d via %d in %d hops \n",d->routing_table[i].dest,d->routing_table[i].nextHop,

		  d->routing_table[i].hops);

	mexPrintf("  Expires at %f\n",d->routing_table[i].exptime);

	mexPrintf("  Used by ");

	for (j=0; j < NBR_AODV-2; j++) {

	  if (d->routing_table[i].prec[j] != 0) {

	    mexPrintf("%d ",j+1);

	  }

	}

	mexPrintf("\n");

      }

    }

    mexPrintf("\n");

    

    

    

    

    

  }

  

  return FINISHED; 

  

}

