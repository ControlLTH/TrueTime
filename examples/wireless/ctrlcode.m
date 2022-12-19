function [exectime, data] = ctrlcode(seg, data)

switch seg,
 case 1, 
  % Read all buffered packets
  temp = ttTryFetch('sensor_signal');
  while ~isempty(temp),
    y = temp;
    temp = ttTryFetch('sensor_signal');
  end
    
  r = ttAnalogIn(1);    % Read reference value
  P = data.K*(r-y);
  D = data.ad*data.Dold + data.bd*(data.yold-y);
  data.u = P + D;
  data.Dold = D;
  data.yold = y;
  exectime = 0.0005;
 case 2,
  msg.msg = data.u;
  msg.type = 'control_signal';
  ttSendMsg(1, msg, 80);    % Send 80 bits to node 1 (actuator)
  exectime = -1; % finished
end
