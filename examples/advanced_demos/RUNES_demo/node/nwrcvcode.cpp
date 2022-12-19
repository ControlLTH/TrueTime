double nwrcvcode(int seg, void* data) {
  switch (seg) {
  case 1:
    // Let AODV layer deal with network message
    ttCreateJob("AODVRcvTask");		
    return 0.0001;
	
  case 2:
    return FINISHED; 
  }
	
  return FINISHED; 
}
