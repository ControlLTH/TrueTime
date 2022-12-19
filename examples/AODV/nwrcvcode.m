function [exectime, data] = nwrcvcode(seg, data)

switch seg,
  
 case 1,
  % Let AODV layer deal with network message
  ttCreateJob('AODVRcvTask');
  exectime = -1;
  
end