function [exectime, data] = AODVsendcode(seg, data)

global routing_table
global seqNbrs
global AODVparams
global verbose

myID = data.nodeID;
mySeqNbrs = seqNbrs(myID, :); % vector of node sequence numbers

switch seg,
  
 case 1,
  msg = ttTryFetch('AODVSendBox');
  noww = ttCurrentTime;
  
  if (isfield(msg, 'data'))
    % Data message from application
    disp(['Time: ' num2str(noww) ' Application in Node#' num2str(myID) ' wants to send to Node#' num2str(msg.dest) ' Data: ' num2str(msg.data) ' Size: ' num2str(msg.size)]);
    
    % Check routing table
    dest_entry = findEntry(msg.dest, myID);
    
    % Is the entry valid?
    invalid = 0;
    if (~isempty(dest_entry))
      invalid = strcmp(dest_entry.state, 'invalid');
    end
    
    if (isempty(dest_entry) | invalid)
      % Not found in routing table
      disp('No (valid) route exists');
      
      % Increment broadcast ID and sequence number
      data.RREQID = data.RREQID + 1;
      mySeqNbrs(myID) = mySeqNbrs(myID) + 1;
      seqNbrs(myID, :) = mySeqNbrs;
      
      % Create RREQ
      RREQ.type = 1;
      RREQ.hopCnt = 0;
      RREQ.RREQID = data.RREQID;
      RREQ.dest = msg.dest;
      RREQ.destSeqNbr = mySeqNbrs(msg.dest);
      RREQ.src = myID;
      RREQ.srcSeqNbr = mySeqNbrs(myID);
      RREQ.intermed = myID;
     
      data.sendTo = 0; % broadcast
      data.msg = RREQ;
      data.size = 24;
      
      % buffer data until route has been established
      l = data.buflen(msg.dest);
      l = l + 1;
      disp(['Buffering message ' num2str(l)]);
      data.buffer{msg.dest}{l} = msg;
      data.buflen(msg.dest) = l;
      
      % exectime
      etime = 0.0001;
    else
      % Route to destination exists in table
      disp(['Route exists in table --- ' num2str(getRoute(myID, msg.dest))]);
      
      % Send data to first node on route to destination
      data.sendTo = dest_entry.nextHop;
      data.msg = msg;
      data.size = msg.size + 4; % extra overhead of 4 bytes due to destination ID

      % Update expiry timer
      dest_entry.exptime = ttCurrentTime + AODVparams.ACTIVE_ROUTE_TIMEOUT;
      updateEntry(dest_entry, myID);
      
      etime = 0.0001;			
    end
  
  else
    % Message from AODVRcv that a route has been established
    
    disp(['Time: ' num2str(noww) ' A new route has been established between Node#' num2str(myID) ' and Node#' num2str(msg.dest)]);
    disp([' --- ' num2str(getRoute(myID, msg.dest))]);
    
    disp([num2str(data.buflen(msg.dest)) ' data messages in buffer']);
    data.dest = msg.dest;
    data.emptyBuffer = 1; % Buffer should be emptied

    % Update expiry timer
    dest_entry = findEntry(msg.dest, myID);
    dest_entry.exptime = ttCurrentTime + AODVparams.ACTIVE_ROUTE_TIMEOUT;
    updateEntry(dest_entry, myID);
      
    etime = 0.0001;
  end

  exectime = etime;
 case 2,
  % Send buffered data messages?
  if (data.emptyBuffer)
 
    if (data.bufInd < data.buflen(data.dest))

      % Retrieve route entry
      dest_entry = findEntry(data.dest, myID);
      
      sendTo = dest_entry.nextHop;    

      % Retrieve next message in buffer
      data.bufInd = data.bufInd + 1;  
      disp(['Sending buffered message ' num2str(data.bufInd) ' to Node#' num2str(data.dest)]);

      msg = data.buffer{data.dest}{data.bufInd};
      size = msg.size + 4; % extra overhead of 4 bytes due to destination ID
      
      ttSendMsg(sendTo, msg, size); 
      ttSleep(0.001);

      exectime = 0;
    else
      % Finished sending data messages
      disp('Buffer emptied');
      data.emptyBuffer = 0;
      data.bufInd = 0;
      data.buflen(data.dest) = 0;
      exectime = -1;
    end
  
  else
    ttSendMsg(data.sendTo, data.msg, data.size); 
    
    exectime = -1;
  end
 
 case 3,
   ttSetNextSegment(2);
   exectime = 0;
end