function [exectime, data] = dummy_code(seg, data)
switch seg
 case 1 
  exectime = data;
 otherwise
  exectime = -1;
end
