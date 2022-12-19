function [exectime, data] = APPLsendcode(seg, data)

global sent

switch seg,

 case 1,
  % Data message to node 7 containing random number
  data.msg.dest = 7;
  data.msg.src = data.nodeID;
  data.msg.data = rand(1);
  data.msg.size = 4;
  exectime = 0.0002;
 
 case 2,
  % Pass message to AODV layer
  ttTryPost('AODVSendBox', data.msg);
  ttCreateJob('AODVSendTask');

  % Store sent data
  sent = [sent data.msg.data];
  
  exectime = -1;

end