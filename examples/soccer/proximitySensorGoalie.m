function [ballSeen, ballPos] = proximitySensorGoalie(nodeID)

global xPos yPos xBall yBall

moteradius = 0.8;
tol = 2*moteradius;

ballPos.xBall=0;


ballSeen = 0;

myX = xPos(nodeID);
myY = yPos(nodeID);

distToBall=sqrt((xBall-myX)^2+(yBall-myY)^2);
	%Noden kan se bollen om den är innom dess synfält vilket är 8*tol.
if(distToBall<12*tol)
	ballSeen = 1;
	ballPos.xBall=xBall;
	ballPos.yBall=yBall;
end


