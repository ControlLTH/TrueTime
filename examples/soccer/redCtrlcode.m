function [exectime, data] = redCtrlcode(seg, data)

global xPos yPos xVel yVel xPos_middle yPos_middle xBall yBall oldBallZone tellArrive noticeBall
global xPos_defense yPos_defense xPos_attac yPos_attac konstitfel
switch seg,

  % Ny referens punkt.
 case 1,
  
  msg = ttTryFetch('packets');
  if ~isempty(msg)
    if(msg.xRef==1000)
      data.xref=xPos_defense(data.nodeID);
      data.yref=yPos_defense(data.nodeID);
    elseif(msg.xRef==2000)
      data.xref=xPos_middle(data.nodeID);
      data.yref=yPos_middle(data.nodeID);
    elseif(msg.xRef==3000)
      data.xref=xPos_attac(data.nodeID);
      data.yref=yPos_attac(data.nodeID);
    else
      data.xref = msg.xRef;
      data.yref = msg.yRef;
      tellArrive(data.nodeID)=0;
    end
  end
  data.waiting = 0;
  exectime = 0.0005;
  
  % Beräkna ny position
 case 2, 
  x = ttAnalogIn(1);
  y = ttAnalogIn(2);
  xv = ttAnalogIn(3);
  yv = ttAnalogIn(4);
  
  % Set my position in global data structure
  xPos(data.nodeID) = x; 
  yPos(data.nodeID) = y;
  xVel(data.nodeID) = xv;
  yVel(data.nodeID) = yv; 
  
  % Check sensor
  [obst, nodePos, ballSeen] = proximitySensor(data.nodeID);
  if (obst) % Obstacle sensed, reverse!
    data.u1 = -data.u1;
    data.u2 = -data.u2;
    msg.node = data.nodeID;
    msg.coll = 1;
    % Tell base that I want new ref because of collision
    ttSendMsg(data.master, msg, 10);
    exectime = 0.0005;
    return;
  end
  if(ballSeen & tellArrive(data.nodeID)==0 & noticeBall(data.nodeID) == 0)
    noticeBall(data.nodeID) = 1;
    msg.node = data.nodeID;
    msg.coll = 0;
    tellArrive(data.nodeID)=1;
    ttSendMsg(data.master, msg,10) 
    data.waiting = 1;
    ttSetNextSegment(5);
    exectime=0;
  end 
  
  if(nodePos.xPos~=100)
    lengthOther=sqrt((data.xref-nodePos.xPos)^2+(data.yref-nodePos.yPos)^2);
    lengthUs=sqrt((data.xref-x)^2+(data.yref-y)^2);
    
    if(lengthUs > lengthOther)
      v = acos((x*nodePos.xPos+y*nodePos.yPos)/(lengthUs*lengthOther));
      if(v<0.5)
	if rand(1)<0.5
	  slumpX = 1;
	else
	  slumpX = -1; 
	end
	if rand(1)<0.5
	  slumpY = 1;
	else
	  slumpY = -1;
	end
	data.xref = nodePos.xPos + slumpX*2;	
	data.yref = nodePos.yPos + slumpY*2;	
      end
    end
  end
  
  % Check if reference reached
  if konstitfel(data.nodeID) == 1
    data.xref = x+1;
    konstitfel(data.nodeID) = 0;
  end
  
  
  dist = sqrt( (data.xref-x)^2 + (data.yref-y)^2 );
  if (dist<0.8 & data.waiting~=1)
    data.xvel=0;
    data.yvel=0;
    msg.node = data.nodeID;
    msg.coll = 0;% Controller gains
    
    if tellArrive(data.nodeID) == 0
      ttSendMsg(data.master, msg, 10); % Tell master that I arrived  
      tellArrive(data.nodeID) = 1;
      noticeBall(data.nodeID)=0;
      data.waiting = 1;% Controller gains
      ttSetNextSegment(5);
      exectime=0;
    end
  else
    data.xvel=-(xPos(data.nodeID)-data.xref)/(dist*0.05);
    data.yvel=-(yPos(data.nodeID)-data.yref)/(dist*0.05);
  end
  
  % P-controller
  data.u1 = data.K1*1.1*(data.xvel-xVel(data.nodeID));
  data.u2 = data.K2*1.1*(data.yvel-yVel(data.nodeID));
 
  if data.u1 > 10
    data.u1 =10;
  end
  if data.u2 > 10
    data.u2 = 10; 
  end
  
  ttSetNextSegment(4); % No obstacle. go to 4
  exectime = 0.0005;
 case 3,
  % keep reverse motion for 0.5 seconds to move away from obstacle
  ttAnalogOut(1, data.u1);
  ttAnalogOut(2, data.u2);
  exectime = 0.9; % time to let motes back off and receive new ref
 case 4,
  ttAnalogOut(1, data.u1);
  ttAnalogOut(2, data.u2);
  ttSetNextSegment(1);
  ttSleep(0.05);
  exectime = 0; % finished 
  
 case 5,
  ttWait('nodePacket');
  exectime = 0;
  
 case 6,
  ttSetNextSegment(1);
  exectime = 0;
end