function [exectime, data] = powctrlcode(seg, data)

switch seg,
 case 1, 
  % Read all buffered packets
  msg = ttTryFetch('power_response'); % Obtain power ping response
  temp = msg;
  while ~isempty(temp),
    y = temp;
    temp = ttTryFetch('power_response'); % Obtain power ping response
  end
  
  if isempty(msg) & data.haverun ~= 0
    % No echo reply, the other node did probably not hear us
    data.transmitPower = data.transmitPower + 10;
    % Limit the maximum transmit power to 30 dbm
    data.transmitPower = min(30, data.transmitPower);
    ttSetNetworkParameter('transmitpower', data.transmitPower);
  else
    % An echo reply, the other did hear us - try to lower the transmission power
    data.transmitPower = data.transmitPower - 0.5;
    ttSetNetworkParameter('transmitpower', data.transmitPower);
  end 
  exectime = 0.00002;
 case 2,
  data.haverun = 1;
  msg.msg.sender = data.name;
  msg.type = 'power_ping';
  time = ttCurrentTime;
  %disp(['setting transmitpower to: ' num2str(data.transmitPower) ' in node: ' num2str(msg.msg.sender) ' at time ' num2str(time)]);
  ttSendMsg(data.receiver, msg, 80);    % Send 80 bits
  exectime = -1; % finished
end
