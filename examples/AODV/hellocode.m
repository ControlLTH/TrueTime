function [exectime, data] = hellocode(seg, data)

global routing_table
global seqNbrs
global AODVparams
global testvar
global verbose

switch seg,

 case 1,
  noww = ttCurrentTime;
  
  if verbose
    disp(['Time: ' num2str(noww) ' Node#' num2str(data.nodeID) ' running periodic HELLO task']);
  end
  
  table = routing_table{data.nodeID};
  
  l = length(table);
  
  % Determine active routes
  activeroute = 0;
  hellonodes = [];
  for k = 1:l  
    if (strcmp(table{k}.state, 'valid'))
      activeroute = 1;
      hellonodes = [hellonodes table{k}.nextHop];
    end
  end
  
  period = ttGetPeriod;
  if (activeroute & (data.lastRREQ < noww - period))
    if verbose
      disp('Broadcasting HELLO msg');
    end
      
    RREP.type = 4;
    RREP.hopCnt = 0;
    RREP.dest = data.nodeID;
    RREP.destSeqNbr = seqNbrs(data.nodeID, data.nodeID);
    RREP.src = 0;
    RREP.lifetime = AODVparams.DELETE_PERIOD; % ALLOWED_HELLO_LOSS * HELLO_INTERVAL;
    RREP.intermed = data.nodeID;
  
    ttSendMsg(0, RREP, 24);
    
  end
    
  % Determine local connectivity
  keepInd = [];
  for k = 1:length(data.nbors)
    if (noww - data.nbors{k}.lastHello > AODVparams.DELETE_PERIOD)
      disp(['Node#' num2str(data.nodeID) ' lost connection to Node#' num2str(data.nbors{k}.nborID)]);
      
      % Send RERRs
      for m = 1:length(table)
	if (strcmp(table{m}.state, 'valid') & table{m}.nextHop == data.nbors{k}.nborID)
	  % Should send RERR to all nodes in precursor list
	  for n = 1:length(table{m}.nbor)
	    RERRentry.dest = table{m}.dest;
	    RERRentry.destSeqNbr = table{m}.destSeqNbr;
	    RERRentry.nbor = table{m}.nbor(n);
	    
	    l = length(data.RERRlist);
	    data.RERRlist{l+1} = RERRentry;
	  end
	  
	  % Invalidate route
	  if verbose
	    disp(['Node#' num2str(data.nodeID) ' invalidating route to Node#' num2str(table{m}.dest) ' through unreachable Node#' num2str(data.nbors{k}.nborID)]);
	  end
	    
	  table{m}.state = 'invalid';
	end
      end
      
      % Update table
      routing_table{data.nodeID} = table;
      
    else
      % keep this entry
      keepInd = [keepInd k];
    end
  end
  if (~isempty(keepInd))
    data.nbors = data.nbors(keepInd);
  else
    data.nbors = {};
  end
  
  exectime = 0.0001;
  
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