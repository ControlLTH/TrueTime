function [exectime, data] = pid_code_skips(seg, data)

switch seg
 case 1
  if data.late == 1           % Skip this invocation if late
    data.late = 0;
    exectime = -1;
    return
  end
  r = ttAnalogIn(data.rChan); % Read reference
  y = ttAnalogIn(data.yChan); % Read process output
  data = pidcalc(data, r, y); % Calculate PID action
  exectime = 0.002;
 case 2
  ttAnalogOut(data.uChan, data.u); % Output control signal
  if ttCurrentTime > ttGetAbsDeadline  % Check whether finished late
    data.late = 1;
  end
  exectime = -1;
end
