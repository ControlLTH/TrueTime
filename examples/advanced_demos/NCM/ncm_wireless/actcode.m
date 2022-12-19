function [exectime, data] = actcode(seg, data)

switch seg,
 case 1, 
  % Read all buffered packets
  temp = ttTryFetch('control_signal');
  while ~isempty(temp),
    data.u = temp;
    temp = ttTryFetch('control_signal');
  end
  
  exectime = 0.0005;
 case 2,
  ttAnalogOut(1, data.u)
  exectime = -1; % finished
end
