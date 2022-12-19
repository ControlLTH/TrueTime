function [exectime, data] = waste(seg, data)

switch seg
 case 1
  exectime = 0.003;
 case 2
  exectime = -1;
end
