function [exectime, data] = AODVrcvcode(seg, data)

global routing_table
global seqNbrs
global AODVparams
global verbose

myID = data.nodeID;

switch seg,
  
 case 1,
  msg = ttGetMsg;
  noww = ttCurrentTime;
  
  if (isfield(msg, 'data'))
    % Data message
    
    if (msg.dest==myID) % Data message arrived at destination
      
      if verbose
	disp(['Time: ' num2str(noww) ' Data message arrived at Node#' num2str(myID)]);
      end
      
      % Find reverse route to original source
      prev_entry = findEntry(msg.src, myID);
      
      % Update expiry timer
      prev_entry.exptime = ttCurrentTime + AODVparams.ACTIVE_ROUTE_TIMEOUT;
      updateEntry(prev_entry, myID);
      
      % Notify application
      ttTryPost('AODVRcvBox', msg.data);
      ttCreateJob('RcvTask');
      
      etime = 0.0001;
      
    else % Forward data message
      
      if verbose
	disp(['Time: ' num2str(noww) ' Node#' num2str(myID) ' about to forward data to Node#' num2str(msg.dest) ' Data: ' num2str(msg.data) ' Size: ' num2str(msg.size)]);
      end
      
      % Find reverse route to original source
      prev_entry = findEntry(msg.src, myID);
      
      % Update expiry timer
      prev_entry.exptime = ttCurrentTime + AODVparams.ACTIVE_ROUTE_TIMEOUT;
      updateEntry(prev_entry, myID);
      
      % Find next hop on route to destination
      dest_entry = findEntry(msg.dest, myID);
      
      % Update expiry timer
      dest_entry.exptime = ttCurrentTime + AODVparams.ACTIVE_ROUTE_TIMEOUT;
      updateEntry(dest_entry, myID);
      
      % Forward data message to next hop
      msg.intermed = myID;
      ttSendMsg(dest_entry.nextHop, msg, msg.size + 4);
      
      etime = 0.0001;
      
    end
    
  else % AODV control message (RREQ, RREP, RERR, or HELLO)
    
    if verbose
      disp(['Time: ' num2str(noww) ' Node#' num2str(myID) ' processing AODV message type: ' num2str(msg.type) ' from Node#' num2str(msg.intermed)]); 
    end
    
    switch msg.type,
     case 1, % RREQ
      
      if (msg.src == myID) % Skip RREQs received by the original source	
	
	etime = 0.00001; 
	
      else 
	
	% Have this RREQ already been processed?
	cache_RREQ  = findInCache(msg.src, msg.RREQID, data.cache);
	
	drop = 0;
	if (~isempty(cache_RREQ))
	  % Found in cache, drop redundant RREQ
	  
	  if verbose
	    disp(['Time: ' num2str(noww) ' Node#' num2str(myID) ' dropping redundant RREQ from Node#' num2str(msg.intermed)]);
	  end
	  drop = 1;
	end
	
	if (drop==0) % process RREQ
	    
	  if verbose
	    disp(['Time: ' num2str(noww) ' Node#' num2str(myID) ' caching RREQ with Src: ' num2str(msg.src) ' RREQID: ' num2str(msg.RREQID)]);
	  end
	  
	  % Enter RREQ in cache
	  cache_entry.src = msg.src;
	  cache_entry.RREQID = msg.RREQID;
	  l = length(data.cache);
	  data.cache{l+1} = cache_entry;
	  
	  % Create or update route entry to source
	  src_entry = findEntry(msg.src, myID);
	  
	  if (isempty(src_entry))
	    % No entry exists, create new
	    rev_entry.dest = msg.src;
	    rev_entry.nextHop = msg.intermed;
	    rev_entry.hops = msg.hopCnt + 1;
	    rev_entry.destSeqNbr = msg.srcSeqNbr;
	    rev_entry.exptime = ttCurrentTime + AODVparams.ACTIVE_ROUTE_TIMEOUT;
	    rev_entry.nbor = [];
	    rev_entry.state = 'valid';
	    
	    l = length(routing_table{myID});
	    routing_table{myID}{l+1} = rev_entry;
	    
	    % Update expiry timer	    
	    updateExpiryTimer(myID);
	    	    
	  else
	    % Update existing entry
	    src_entry.exptime = ttCurrentTime + AODVparams.ACTIVE_ROUTE_TIMEOUT;
	    src_entry.destSeqNbr = max(src_entry.destSeqNbr, msg.srcSeqNbr);
	    src_entry.nextHop = msg.intermed;
	    src_entry.hops = msg.hopCnt + 1;
	    src_entry.state = 'valid';
	    updateEntry(src_entry, myID);
	  end
	  
	  % Check if we have a route to destination
	  dest_entry = findEntry(msg.dest, myID);
	  
	  haveroute = 0;
	  if (~isempty(dest_entry))
	    haveroute = strcmp(dest_entry.state, 'valid');
	  end
	  
	  if (msg.dest==myID | haveroute)
	    % We are the destination or we have a route to it
	    
	    if verbose
	      disp(['Node#' num2str(myID) ' has a route to destination#' num2str(msg.dest)]);
	    end
	    
	    if (~isempty(dest_entry))
	      % I am not the destination, but have a route to it
	      
	      if verbose
		disp('Sending first RREP from node with route');
	      end
	      
	      dest = dest_entry.dest;
	      seqNbr = dest_entry.destSeqNbr;
	      hopCnt = dest_entry.hops;
	      lifetime = dest_entry.exptime - ttCurrentTime;
	      dest_entry.nbor = [dest_entry.nbor msg.intermed];
	      updateEntry(dest_entry, myID);
	    else
	      % I am the destination itself
	      
	      if verbose
		disp('Sending first RREP from destination itself');
	      end
	      
	      dest = myID;
	      mySeqNbr = seqNbrs(myID, myID);
	      if (mySeqNbr + 1 == msg.srcSeqNbr)
		seqNbrs(myID, myID) = seqNbrs(myID, myID) + 1;
	      end
	      seqNbr = max(seqNbrs(myID, myID), msg.destSeqNbr);
	      hopCnt = 0;
	      lifetime = AODVparams.ACTIVE_ROUTE_TIMEOUT; 
	    end
	    
	    RREP.type = 2;
	    RREP.hopCnt = hopCnt;
	    RREP.dest = dest;
	    RREP.destSeqNbr = seqNbr;
	    RREP.src = msg.src;
	    RREP.lifetime = lifetime;
	    RREP.intermed = myID;
	    
	    % Send RREP to previous hop
	    ttSendMsg(msg.intermed, RREP, 20);
	    
	    etime = 0.0001;
	    
	  else
	    % We do not have a route to the destination
	    
	    % Rebroadcast RREQ
	    if verbose
	      disp(['Time: ' num2str(noww) ' Node#' num2str(myID) ' sending new broadcast']);
	    end
	    
	    msg.hopCnt = msg.hopCnt + 1;
	    msg.destSeqNbr = max(seqNbrs(myID, msg.dest), msg.destSeqNbr);
	    msg.intermed = myID;
	    
	    ttSendMsg(0, msg, 24);
	    
	    etime = 0.0001;
	    
	  end
	  
	else
	  etime = 0.00001; % used if RREQ is dropped
	end
	
      end
      
     case 2, % RREP
      
      if verbose
	disp(['Node#' num2str(myID) ' got an RREP from Node#' num2str(msg.intermed) ' for destination#' num2str(msg.dest)]);
      end
	
      % Create forward route entry, one could already exist
      dest_entry = findEntry(msg.dest, myID);
      
      if (isempty(dest_entry))
	% No entry exists, create new
	if verbose
	  disp(['Creating new forward entry from Node#' num2str(myID) ' to Node#' num2str(msg.dest)]);
	end
	  
	fwd_entry.dest = msg.dest;
	fwd_entry.nextHop = msg.intermed;
	fwd_entry.hops = msg.hopCnt + 1; % New Hop Count
	fwd_entry.destSeqNbr = msg.destSeqNbr;
	fwd_entry.exptime = ttCurrentTime + msg.lifetime;
	fwd_entry.nbor = [];
	fwd_entry.state = 'valid';
	
	l = length(routing_table{myID});
	routing_table{myID}{l+1} = fwd_entry;
	
	% Update expiry timer	    
	updateExpiryTimer(myID);
	
	propagate = 1;
	
	etime2 = 0.0001;
      else
	% Forward entry already exists in table
	% Should it be updated?
	cond1 = strcmp(dest_entry.state, 'invalid'); 
	cond2 = (strcmp(dest_entry.state, 'valid')) & (msg.destSeqNbr > dest_entry.destSeqNbr);
	cond3 = (msg.destSeqNbr > dest_entry.destSeqNbr) & (msg.hopCnt+1 < dest_entry.hops);
	
	if (cond1 | cond2 | cond3)
	  % Update existing entry
	  if verbose
	    disp(['Updating existing forward entry from Node#' num2str(myID) ' to Node#' num2str(msg.dest)]);
	  end
	  
	  dest_entry.nextHop = msg.intermed;
	  dest_entry.hops = msg.hopCnt + 1; % New Hop Count
	  dest_entry.destSeqNbr = msg.destSeqNbr;
	  dest_entry.exptime = ttCurrentTime + msg.lifetime;
	  if (strcmp(dest_entry.state,'invalid'))
	    dest_entry.nbor = [];
	  end
	  dest_entry.state = 'valid';
	  fwd_entry = dest_entry;
	  
	  updateEntry(dest_entry, myID);

	  propagate = 1;

	  etime2 = 0.0001;
	else
	  % Existing entry should be kept
	  % Do not propagate RREP 
	  if verbose
	    disp(['No entry updated in Node#' num2str(myID)]);
	  end
	    
	  propagate = 0;
		
	  etime2 = 0.0001;
	end
      end
      
      etime1 = 0;
      if (msg.src == myID)
	% Original source, no reverse entry exists
	if verbose
	  disp(['Node#' num2str(myID) ' got final RREP for route to Node#' num2str(msg.dest)]);
	end
	  
	% Start AODVSend to send buffered data messages
	sendMsg.dest = msg.dest;
	ttTryPost('AODVSendBox', sendMsg);
	ttCreateJob('AODVSendTask');
	
	nextHop = [];
	
	etime1 = 0.0001;
	
      elseif (propagate)
		
	% Update reverse entry from info in RREP
	% and get next hop towards source
	nextHop = updateReverse(msg, myID);
	
	% Update precursor list for forward entry
	ind = [];
	if ~isempty(fwd_entry.nbor)
	  ind = find(fwd_entry.nbor==nextHop);
	end
	if (isempty(ind))
	  fwd_entry.nbor = [fwd_entry.nbor nextHop];
	end
	updateEntry(fwd_entry, myID);    
	
	% Update RREP to continue back propagation
	RREP = msg;
	RREP.hopCnt = msg.hopCnt + 1;
	RREP.intermed = myID;
	
	ttSendMsg(nextHop, RREP, 20); 
	
	etime1 = 0.0001;
      
      end
      
      etime = etime1 + etime2;
      
     case 3, % RERR

      if verbose
	disp(['Node#' num2str(myID) ' got an RERR from Node#' num2str(msg.intermed) ' for destination#' num2str(msg.dest)]);
      end
      
      % Propagate RERR?
      
      table = routing_table{myID};
  
      for m = 1:length(table)
	if (strcmp(table{m}.state, 'valid') & table{m}.dest == msg.dest & table{m}.nextHop == msg.intermed)
	  
	  % Should send RERR to all nodes in precursor list (neighbors)
	  for n = 1:length(table{m}.nbor)
	    RERRentry.dest = table{m}.dest;
	    RERRentry.destSeqNbr = table{m}.destSeqNbr;
	    RERRentry.nbor = table{m}.nbor(n);
	    
	    l = length(data.RERRlist);
	    data.RERRlist{l+1} = RERRentry;
	  end
	  
	  % Invalidate route
	  if verbose
	    disp(['Node#' num2str(myID) ' invalidating route to Node#' num2str(table{m}.dest) ' through Node#' num2str(msg.intermed)]);
	  end
	    
	  table{m}.state = 'invalid';
	  
	end
      end
      
      % Update table
      routing_table{myID} = table;
      
      etime = 0.0001;
     case 4, % HELLO
      
      if verbose
	disp(['Time: ' num2str(noww) ' Node#' num2str(myID) ' got a hello message from Node#' num2str(msg.dest)]);
      end
	
      dataHello = ttGetData('HelloTask');
            
      % Update time stamp for last HELLO msg
      found = 0;
      for k = 1:length(dataHello.nbors)
	if (dataHello.nbors{k}.nborID==msg.dest)
	  dataHello.nbors{k}.lastHello = noww;
	  found = 1;
	end
      end
      if (~found)
	nbor.nborID = msg.dest;
	nbor.lastHello = noww;
	l = length(dataHello.nbors);
	dataHello.nbors{l+1} = nbor;
      end
      
      ttSetData('HelloTask', dataHello);
	
      etime = 0.0001;
    end
    
  end
  
  exectime = etime;
  
 case 2,
  % Send all RERRs
  if (~isempty(data.RERRlist))
    data.cnt = data.cnt + 1;
    
    if (data.cnt > length(data.RERRlist))
      
      data.RERRlist = {}; 
      data.cnt = 0;
      exectime = -1;
      
    else
      
      RERRentry = data.RERRlist{data.cnt};
      RERR.type = 3;
      RERR.dest = RERRentry.dest;
      RERR.destSeqNbr = RERRentry.destSeqNbr;
      RERR.intermed = data.nodeID;

      if verbose
	disp(['Node#' num2str(myID) ' sending RERR to Node#' num2str(RERRentry.nbor)]);
      end
	
      ttSendMsg(RERRentry.nbor, RERR, 12); 
    
      ttSleep(0.001);
      exectime = 0;
    
    end
    
  else
    exectime = -1;
  end 
  
 case 3,  
  ttSetNextSegment(2);
  exectime = 0;
  
end