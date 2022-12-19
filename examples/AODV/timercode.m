function [exectime, data] = timercode(seg, data)

switch seg,
  
 case 1,
  ttCreateJob('TimerTask');
  exectime = -1;
  
end