function [exectime, data] = pidcode4(seg, data)
switch seg
 case 1
  r = ttAnalogIn(data.rChan); % Read reference
  y = ttTryFetch('Samples');  % Read sample from mailbox
  data = pidcalc(data, r, y); % Calculate PID action
  exectime = 0.0018;
 case 2
  ttAnalogOut(data.uChan, data.u)  % Output control signal
  exectime = -1;
end
