function [exectime, data] = APPLrcvcode(seg, data)

global received

switch seg,

 case 1,
  exectime = 0.0002;
 case 2,
  msg = ttTryFetch('AODVRcvBox');
  noww = ttCurrentTime;
  
  disp(['Application in Node#' num2str(data.nodeID) ' receiving data: ' num2str(msg)]);
  
  % Store received data
  received = [received msg];
  
  exectime = -1;
end