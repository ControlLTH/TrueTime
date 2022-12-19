function [exectime, data] = controller_code(seg,data)

switch seg
  case 1

    exectime = 0.005;
    
  case 2
    msg = ttGetMsg;
    if ~isempty(msg)
      
      msg.data = -2*msg.data;             % P controller
      msg.destination = [3 1];            % final destination = actuator
      
      ttSendMsg(2, msg, 250);          % send message to gateway (node 2)
    end
    exectime = -1;
    
end
