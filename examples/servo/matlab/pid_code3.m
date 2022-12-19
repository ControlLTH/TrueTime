function [exectime, data] = pidcode3(seg, data)
switch seg
 case 1
  r = ttAnalogIn(data.rChan); % Read reference
  y = ttAnalogIn(data.yChan); % Read process output
  data = pidcalc(data, r, y); % Calculate PID action
  exectime = 0.002;
 case 2
  ttAnalogOut(data.uChan, data.u); % Output control signal
  data.t = data.t + data.h;        % Increase time base
  ttSetNextSegment(1);             % Loop back to segment 1
  ttSleepUntil(data.t);            % Sleep until next period
  exectime = 0;
end
