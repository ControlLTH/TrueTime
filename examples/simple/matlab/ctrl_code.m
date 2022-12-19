function [exectime, data] = ctrl_code(segment, data)

switch segment
 case 1
  y = ttAnalogIn(1);
  data.u = -data.K * y;
  exectime = rand*data.exectime;
 case 2
  ttAnalogOut(1, data.u)
  exectime = -1;
end
