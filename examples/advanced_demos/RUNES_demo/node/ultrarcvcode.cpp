double ultrarcvcode(int seg, void* data) {

  AODVrcv_Data *d = (AODVrcv_Data *) data;

  

  switch (seg) {

  case 1: {

    if (d->startTime != 0) {

      // Fetch ultrasound ping

      ttGetMsg(ULTRANETW);

      if (d->requestObtained == 1) {

	d->requestObtained = 0;

	// Fetch and reset time

	d->deltaTime = ttCurrentTime() - d->startTime;

	d->startTime = 0;

	//		 	mexPrintf("Ultra ping received in node %d at %f \n",d->nodeID,ttCurrentTime());

	// Create job for returning distance to bot

	ttCreateJob("ReturnDistance");

      }

    }

    return 0.0001;

  }

  case 2:

    return FINISHED; 

  }

  

  return FINISHED; 

}

