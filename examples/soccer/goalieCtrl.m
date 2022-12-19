function [exectime, data] = goalieCtrl(seg, data)

global xPos yPos

switch seg,
 
  % Beräkna ny position
 case 1, 
  x = ttAnalogIn(1);
  y = ttAnalogIn(2);
  

  % Set my position in global data structure
  xPos(data.nodeID) = x; 
  yPos(data.nodeID) = y;
  
  % Check sensor
  [ballSeen, ballPos] = proximitySensorGoalie(data.nodeID);
  
  if ballSeen	
    if abs(ballPos.xBall)>4
      data.xref = 4 * sign(ballPos.xBall);
    else
      data.xref = ballPos.xBall;
    end
    
    
  else
    data.xref = 0;
  end
  
  % P-controller
  data.u1 = data.K1*1.1*(data.xref-xPos(data.nodeID));
  
  if data.u1 > 10
    data.u1 =10;
  end
  
  ttSleep(0.1);
  
  exectime = 0.0005;
  
 case 2,
  ttAnalogOut(1, data.u1);
  
  
  exectime = 0; 
  
 case 3,
  ttSetNextSegment(1);
  exectime = 0.05; % finished 
  
end

