function [exectime, data] = samplercode(seg, data)
switch seg
 case 1
  y = ttAnalogIn(data.yChan); % Read process output
  ttTryPost('Samples', y);    % Put sample in mailbox
  exectime = 0.0002;
 case 2
  ttCreateJob('pid_task')     % Trigger task job
  exectime = -1;
end
