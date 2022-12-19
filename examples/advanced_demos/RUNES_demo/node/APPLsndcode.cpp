double APPLsndcode(int seg, void* data) {

  APPLsnd_Data* d = (APPLsnd_Data*) data;

  

  static DataMsg *datamsg;

  MailboxMsg *mailboxmsg;

  

  switch (seg) {

  case 1:  

    // Data message to node 1 containing random number

    datamsg = new DataMsg;

    datamsg->dest = 1;

    datamsg->src = d->nodeID;

    datamsg->data = ((double)rand()/(double)RAND_MAX);

	ttAnalogOut(1,datamsg->data);

    datamsg->size = 2*32 + 32; // 4 bytes of data, 4 bytes for dest and src ID

    return 0.0002;

  case 2:    

    // Pass message to AODV layer using mailbox

    mailboxmsg = new MailboxMsg;

    mailboxmsg->type = APPL_DATA;

    mailboxmsg->dest = 0;

    mailboxmsg->datamsg = datamsg;

    ttPost("AODVSndBox", mailboxmsg);

    d->sent++;

//    mexPrintf("(%.4f)Message nbr %2d posted\n", ttCurrentTime(), d->sent);

    return FINISHED; 

  }

  

  return FINISHED; // to supress compilation warnings

}

