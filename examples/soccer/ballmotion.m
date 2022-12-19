function [sys,x0,str,ts]=ballmotion(t,x,u,flag,params);

%ANIMATION S-function for animating the motion of the ball.

% Ball Dynamics, sample time 0.02
a = [1  0.01922; 0 0.9231];
b = [0.007791; 0.7688];
c = [1 0];
d = 0;

A = [a zeros(2,2);zeros(2,2) a];
B = [b zeros(2,1); zeros(2,1) b];
C = [c zeros(1,2); zeros(1,2) c];
D = [0 0];

switch flag,
 case 0,
  [sys,x0,str,ts] = mdlInitializeSizes(params);
 case 2,
  sys = mdlUpdate(t,x,u,A,B);
 case 3,
  sys = mdlOutputs(t,x,u,C);
 case 9,
  sys = mdlTerminate(t,x,u);
 case {1 ,4} % Unused flags
  sys = [];
 otherwise
  error(['Unhandled flag = ',num2str(flag)]);
end

%=============================================================================
% mdlInitializeSizes
%=============================================================================

function [sys,x0,str,ts]=mdlInitializeSizes(params)

global goalWait goalTime

goalWait = 0;
goalTime = 0;

% Read parameters
numMotes = params(1);
xInit = params(2);
yInit = params(3);

sizes = simsizes;
sizes.NumContStates  = 0; 
sizes.NumDiscStates  = 4;          
sizes.NumOutputs     = 0;
sizes.NumInputs      = 2*numMotes;
sizes.DirFeedthrough = 0;
sizes.NumSampleTimes = 1;   

sys = simsizes(sizes);
x0  = [xInit 0 yInit 0];
str = [];
ts  = [0.02 0]; 

%=============================================================================
% mdlUpdate
%=============================================================================

function [sys]=mdlUpdate(t,x,u,A,B)

global xBall yBall redScore blueScore redForce blueForce goalWait goalTime

numMotes = length(u)/2;

% check collision with motes
collision = 0;
ballForce = 1;
for k=1:numMotes
  
  dist = sqrt( (xBall-u(k))^2 + (yBall-u(k+numMotes))^2 );
  
  if dist <= 1 % mote radius + ball radius
    collision = 1; % collision with mote k
    if k <= 5
      ballForce = redForce;
    else
      ballForce = blueForce;
    end
    break;
  end
end

if collision
  xVec = xBall-u(k);
  yVec = yBall-u(k+numMotes);
  hyp = sqrt(xVec^2 + yVec^2);
  U = ballForce*2*[xVec; yVec] / (hyp*0.01); % impulse
else
  U = zeros(2,1);
end

if (abs(xBall) < 4 & abs(yBall) > 27)
  if goalWait == 0
    goalWait = 1;
    disp('GOAL!!!!!!!');
    if yBall > 0
      currentScore = str2num(get(blueScore, 'String'));
      set(blueScore, 'String', num2str(currentScore+1));
    else
      currentScore = str2num(get(redScore, 'String'));
      set(redScore, 'String', num2str(currentScore+1));
    end
    x(2)=0;
    x(4)=0;
    goalTime=t;
  end
  if t-goalTime > 1
    x = zeros(4,1);
    goalWait = 0;
  end
end

% Bounce
if (abs(xBall) > 19)
  x(2) = -x(2);
end
if (abs(yBall) > 26 & abs(xBall) > 3)
  x(4) = -x(4);
end

sys = A*x + B*U;

%=============================================================================
% mdlOutputs
%=============================================================================

function [sys]=mdlOutputs(t,x,u,C)

global xBall yBall

out = C*x;
xBall = out(1);
yBall = out(2);
sys = [];

%=============================================================================
% mdlTerminate
% Perform any end of simulation tasks.
%=============================================================================

function sys=mdlTerminate(t,x,u)

sys = [];




  


  
  
