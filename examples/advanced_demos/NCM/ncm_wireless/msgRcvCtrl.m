function [exectime, data] = msgRcvCtrl(seg, data)

temp = ttGetMsg;
ttTryPost(temp.type, temp.msg);

if strcmp('sensor_signal', temp.type)
  ttCreateJob('pid_task')
elseif strcmp('power_ping', temp.type)
  ttCreateJob('power_response_task');
end

exectime = -1;
