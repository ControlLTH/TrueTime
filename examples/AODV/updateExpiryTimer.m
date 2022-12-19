function updateExpiryTimer(tableID)

global routing_table
global verbose

dataTimer = ttGetData('TimerTask');

if (dataTimer.expTimer == 1)
  ttRemoveTimer('exptimer');
end

mintime = inf;
table = routing_table{tableID};

for k = 1:length(table)
  if ((table{k}.exptime < mintime) & strcmp(table{k}.state,'valid'))
    mintime = table{k}.exptime;
  end
end

if (mintime < inf)
  ttCreateTimer('exptimer', mintime, 'timer_handler');
  dataTimer.expTimer = 1;
  
  noww = ttCurrentTime;
  
  if verbose
    disp(['Time: ' num2str(noww) ' Node#' num2str(tableID) ' updating expiry timer to time: ' num2str(mintime)]);
  end
    
else
  dataTimer.expTimer = 0;
end
    
ttSetData('TimerTask', dataTimer); 
