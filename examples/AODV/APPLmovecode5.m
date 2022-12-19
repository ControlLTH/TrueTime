function [exectime, data] = APPLmovecode5(seg, data)

switch seg,

 case 1,
  noww = ttCurrentTime;
  if (noww >= 3 & noww <= 8) 
    ttAnalogOut(2, 2); % Move in pos. y-dir
  else
    ttAnalogOut(2, 0); % Stay still 
  end
  exectime = 0.0002;  
 
 case 2,
  exectime = -1;

end
