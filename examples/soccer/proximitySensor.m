function [obst, nodePos, ballSeen, ballPos] = proximitySensor(nodeID)

global xPos yPos xBall yBall noticeBall

moteradius = 0.8;
tol = 2*moteradius;
nodePos.xPos=100;
nodePos.yPos=100;
ballPos.xBall=100;
ballPos.yBall=100;
ballSeen = 0;
myX = xPos(nodeID);
myY = yPos(nodeID);
distToBall=sqrt((xBall-myX)^2+(yBall-myY)^2);
%Noden kan se bollen om den är innom dess synfält vilket är 8*toll.
if(distToBall<8*tol)
  ballSeen = 1;
  ballPos.xBall=xBall;
  ballPos.yBall=yBall;
end

for k=1:length(xPos)
  if (k~=nodeID)
    dist = sqrt( (myX-xPos(k))^2 + (myY-yPos(k))^2 );
    
    if dist < tol
      obst = 1;
      return;
    end
    if (dist < 6*tol)
      nodePos.xPos = xPos(k);
      nodePos.yPos = yPos(k);
    end
    
  end
end

obst = 0;
