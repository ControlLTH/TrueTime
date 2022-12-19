function [exectime, data] = pidcode2(seg, data)
switch seg
 case 1 
  inp(1) = ttAnalogIn(1);
  inp(2) = ttAnalogIn(2);
  outp = ttCallBlockSystem(2, inp, 'controller');
  data.u = outp(1);
  exectime = outp(2);
 case 2
  ttAnalogOut(1, data.u);
  exectime = -1;
end
