function [exectime, data] = APPLmovecode6(seg, data)

switch seg,

 case 1,
  noww = ttCurrentTime;
  if (noww >= 10 & noww <= 14) 
    ttAnalogOut(2, 2); % Move to create bridge
  else
    ttAnalogOut(2, 0); % Stay still 
  end
  exectime = 0.0002;  

 case 2,  
  exectime = -1;

end
