function [exectime, data] = msgRcvActuator(seg, data)

temp = ttGetMsg;
ttTryPost(temp.type, temp.msg);

if strcmp('control_signal', temp.type)
  ttCreateJob('act_task');
elseif strcmp('power_ping', temp.type)
  ttCreateJob('power_response_task');
end

exectime = -1;
