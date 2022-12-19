function [exectime, data] = powrespcode(seg, data)

switch seg,
 case 1, 
  data.msg.msg = ttTryFetch('power_ping');
  data.msg.type = 'power_response';
  exectime = 0.00002;
 case 2,
%  disp(['power ping received from node: ' num2str(data.msg.msg.sender) ', sending response'])
  ttSendMsg(data.msg.msg.sender, data.msg, 80); % Reply to the sender
  exectime = -1; % finished
end
