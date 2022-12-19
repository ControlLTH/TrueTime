function [exectime, data] = pidcode4(seg, data)
switch seg
 case 1
  r = ttAnalogIn(data.rChan); % Read reference
  y = ttAnalogIn(data.yChan); % Read feedback
  data = pidcalc(data, r, y); % Calculate PID action
  exectime = 0.003;           % Set execution time of this segment
 case 2
  ttAnalogOut(data.uChan, data.u)  % Output control signal
  exectime = -1;                   % End of job
end
