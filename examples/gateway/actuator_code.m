function [exectime, data] = actuator_code(seg,data)

switch seg
  case 1
    exectime = 0.005;
    
  case 2
    msg = ttGetMsg;
  
    if ~isempty(msg)
      output = msg.data;
      delay = ttCurrentTime - msg.timestamp; % calculate end-to-end delay
    else
      output = 0;  % indicates receipt of empty msg
      delay = 0;
    end
  
    ttAnalogOut(1,output)
    ttAnalogOut(2,delay)
  
    exectime = -1;
  
end
