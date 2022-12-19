function [exectime, data] = expcode(seg, data)

global routing_table
global verbose

switch seg,
  
 case 1,
  table = routing_table{data.nodeID};
  mintime = inf;
  noww = ttCurrentTime;

  for k = 1:length(table)
    % Invalidate timed out entry
    if (abs(table{k}.exptime - noww) < 0.0000001)
      table{k}.state = 'invalid';
    end
    if (table{k}.exptime > noww+0.0000001)
      mintime = min(mintime, table{k}.exptime);
    end
  end

  if verbose
    disp(['Time: ' num2str(noww) ' timer expiry in Node#' num2str(data.nodeID)]);
  end
    
  % Create new timer
  if (mintime < inf)
    ttCreateTimer('exptimer', mintime, 'timer_handler');
    data.expTimer = 1;
    
    if verbose
      disp(['Time: ' num2str(noww) ' Node#' num2str(data.nodeID) ' updating expiry timer to time: ' num2str(mintime)]);
    end
      
  else
    data.expTimer = 0;
  end
    
  % Update routing table
  routing_table{data.nodeID} = table;
  
  exectime = -1;
  
end