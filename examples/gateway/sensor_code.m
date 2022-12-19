function [exectime, data] = sensor_code(seg,data)

switch seg
  
  case 1 
    exectime = 0.005;
  
  case 2
    msg.timestamp = ttCurrentTime;
    msg.data = ttAnalogIn(1);
    msg.destination = [2 1];    % final destination = controller
    
    ttSendMsg(2, msg, 250);     % send msg to gateway (node 2)
    
    exectime = -1;
end
