function [exectime, data] = blueMsgRcvMaster(seg, data)

global xBall yBall xPos yPos takePass
msg = ttGetMsg;
node = msg.node;
takePass(node)=0;

if (msg.coll~=1)
  ballZone = getBallZone(xBall,yBall);
  
  %Räknar ut vilken nod som är närmast bollen och hur lång det är till målet
  
  [node, dist] = closestNode;
  
  returnPos(node);
  
  %Noden är över bollen. Passa, skjuta eller dribbla?
  if(yPos(node)>yBall)
    %Passa
    %Funktionen returnerar närmaste nod under bollen. Returnerar -1 om det
    %inte finns någon nod under
    
    [toPass, distToPass] = whichNodeToPass(node,1);
    %Om det finns någon ovanför att passa.
    if(toPass~=-1)
      %Passa bollen till toPass. Alltså skicka nodens nya referenspunkt och medela
      %toPass att bollen är på våg.
      pass(node, toPass);
      exectime = -1;
      return
      
      %Om det inte finns någon ovanför att passa.
    else
      %Dribbla runt bollen och passa eller skjut.
      dribble(node);
      exectime = -1;
      return      
    end	
    %Nu är noden ovanför bollen och ska antingen passa, dribbla eller skjuta.
  else
    if yBall > 10
      ballReach = 10;
    elseif yBall < -10
      ballReach = 15;
    else
      ballReach = 7;
    end
    
    %Om noden är för långt ifrån målet ska den passa någon som når målet.
    if(sqrt((xBall)^2 + (yBall-27)^2)> 1.5*ballReach)
      [toPass, distToPass] = whichNodeToPass(node,2);
      if( (toPass==-1) | (distToPass > ballReach))
	dribble(node);
	exectime = -1;
	return
      else
	pass(node, toPass);
	exectime = -1;
	return
      end
      
    else
      goal(node);
      exectime = -1;
      return 
      
    end
  end
  
  
else
  % Collision
  ref = getRandRef(node);
  newRef.xRef = ref(1);
  newRef.yRef = ref(2);
  ttSendMsg(node, newRef, 10); 
  %Då en nod har krockat kommer den att vara satt ur spel ett litet tag.
  %Därför skickas nästnärmaste nod till bollens position.
  [nearestNode, distToNearestNode] = whichNodeToPass(node, 3);
  
  newRef = kickBall(0,27,nearestNode,3);
  ttSendMsg(nearestNode, newRef, 10);
end				

exectime = -1;


function returnPos(node)
global yBall xPos yPos takePass inPlace state
%if (data.nodeID == 1) % red team's master
if (yBall>=10)
  newRef.xRef=3000;
elseif (yBall<-10)
  newRef.xRef=1000;
else
  newRef.xRef=2000;
end
for i=7:10
  %Skickar tillbaka noderna till deras ursprungspositioner
  if(i~=node & takePass(i)==0)
    ttSendMsg(i, newRef, 10);
    state{i}='Idle';
  end
  %	end
end

function [newRef, newBall] = kickBall(xRef,yRef, node, func)
%func = 1 skjut mot mål
%func = 2 Passa med spelare
%func = 3 Dribbla

global xBall yBall xPos yPos blueForce
if func==1
  blueForce=1;
  distance=2;
end

if func==2
  %Skjuter upp eller ner?
  distBallPass = sqrt((xRef-xBall)^2+(yRef-yBall)^2);
  
  if(yBall<yRef)
    v=asin(abs(xBall-xRef)/distBallPass);
    %ballForce = sqrt((xRef-xBall)^2+(yRef-yBall)^2)/15;
  else
    v=pi/2+acos(abs(xBall-xRef)/distBallPass);
    %ballForce = sqrt((xRef-xBall)^2+(yRef-yBall)^2)/20;
  end
  ballForce=distBallPass/(15+3*sin(v/2));
  
  if ballForce>=1
    blueForce = 1;
  else
    blueForce = ballForce;
  end  
  distance=2;
  if blueForce < 0.3
    blueForce = 0.3;
  end
  
end

if (func==3)
  blueForce=0.2;
  distance=1.7;
end


ballSize=1;
xAimVec = xRef - xBall;
yAimVec = yRef - yBall;
lengthAimVec = sqrt(xAimVec^2 + yAimVec^2);

%Vi försöker förutspå var bollen kommer att hamna.
newBall.xRef =  xBall + blueForce *2*8*xAimVec/(lengthAimVec);
newBall.yRef =  yBall + blueForce *2*8*yAimVec/(lengthAimVec) -3;


if(sqrt((xBall-xPos(node))^2 + (yBall-yPos(node))^2)>=distance)
  
  %Sätter referenspunken bakom bollen, i förhållande till målet
  newRef.xRef = xBall - (ballSize*xAimVec)/lengthAimVec;
  newRef.yRef = yBall - (ballSize*yAimVec)/lengthAimVec;
  
else
  newRef.xRef = xBall;
  newRef.yRef = yBall;
end

if func==3 & newRef.yRef < 0 & yPos(node)> yBall
  if (xPos(node) > newRef.xRef - 2) | (xPos(node) < newRef.xRef + 2)
    if xPos(node) > 0
      newRef.xRef = newRef.xRef + 1;
    else
      newRef.xRef = newRef.xRef - 1;
    end
  end
end



function [node, dist] = closestNode
global xBall yBall xPos yPos 
dist = 1000;
for i = 7:10
  distToBall = sqrt((xBall-xPos(i))^2 + (yBall-yPos(i))^2);
  %distFromMoteToGoal(i-6) = sqrt((xPos(i))^2 + (yPos(i)-27)^2);
  if(dist>distToBall)
    dist=distToBall;
    node = i;
  end   
end

function [toPass, distToPass] = whichNodeToPass(node, func)
%func=1 Noden ligger över bollen.
%func=2 Noden ligger under bollen.
%func=3 Närmasta noden
global xBall yBall xPos yPos 
distToPass=50;
toPass=-1;
for i = 7:10
  %Beräknar till vem noden ska passa. 	
  if(func==1)
    if(yBall>=yPos(i))
      toPassY = sqrt((xPos(i)-xBall)^2+(yPos(i)-yBall)^2);
      if(toPassY<distToPass & i ~= node)
	distToPass=toPassY;
	toPass=i;
      end
    end	 	
  end  
  if(func==2)
    if(yBall<=yPos(i))
      toPassY = sqrt((xPos(i)-xBall)^2+(yPos(i)-yBall)^2);
      if(toPassY<distToPass & i ~= node)
	distToPass=toPassY;
	toPass=i;
      end
    end
  end
  if(func==3)
    toPassY = sqrt((xPos(i)-xBall)^2+(yPos(i)-yBall)^2);
    if(toPassY<distToPass & i ~= node)
      distToPass=toPassY;
      toPass=i;
    end
  end	 	  
end		

function pass(node, toPass)
global xPos yPos takePass state
%sprintf('LINE 39: Mote: %d, Passa till mote: %d',node,toPass)
state{node}=['Pass to ' num2str(toPass)];
state{toPass}=['Receive pass from ' num2str(node)];

[newRef, newBall] = kickBall(xPos(toPass),yPos(toPass)+3,node,2);
takePass(toPass) = 1;
if xPos(toPass)>0
  newBall.xRef = newBall.xRef + 2;
else
  newBall.xRef = newBall.xRef - 2;
end
ttSendMsg(node,newRef,10);
ttSendMsg(toPass,newBall,10);




function dribble(node)
global xBall yBall xPos yPos state
state{node}='Dribble';
%sprintf('LINE 39: Mote: %d, Dribbla',node)
[newRef, newBall] = kickBall(0,27,node,3);
ttSendMsg(node, newRef,10);


function goal(node)
global state
%sprintf('Mote: %d, SKJUT',node)
state{node}='Shoot';
temp = rand(1);
slump = rand(1);
if temp < 0.5
  slump = -slump;
end

[newRef, newBall] = kickBall(slump,27,node,1);
ttSendMsg(node, newRef, 10);


%function freeKick
%global x takePass newRef
%	for i = 2:5
%		newRef.xRef= -3 +2*(i-2);
%		newRef.yRef = 10;
%		takePass(i) = 1;
%		ttSendMsg(i, newRef, 10);
%	end
%	ttSleep(500);
%	
%	%x = zeros(10,1);

