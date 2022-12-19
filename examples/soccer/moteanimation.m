function [sys,x0,str,ts]=moteanimation(t,x,u,flag,params,xInit,yInit);

%ANIMATION S-function for animating the motion of the motes.

switch flag,
 case 0,
  [sys,x0,str,ts] = mdlInitializeSizes(params,xInit,yInit);
 case 2,
  sys = mdlUpdate(t,x,u);
 case 3,
  sys = mdlOutputs(t,x,u);
 case 9,
  sys = mdlTerminate(t,x,u);
 otherwise
  error(['Unhandled flag = ',num2str(flag)]);
end

%=============================================================================
% mdlInitializeSizes
%=============================================================================

function [sys,x0,str,ts]=mdlInitializeSizes(params,xInit,yInit)

global motehdl ID ball redScore blueScore

% Read parameters
numMotes = params(1);
ts = params(2);
xmax = params(3);
ymax = params(4);

% Initialize the figure for use with this simulation

figure(1),clf;
title('MOTE SOCCER');
set(gcf,'Renderer','OpenGL');
set(gcf,'Position',[0 20 550 800])  %[700 500 700 1050])
grid on
set(gca,'Xlim',[-xmax xmax],'Ylim',[-ymax ymax],'nextplot','add');
set(gca,'XTickLabel',[]);
set(gca,'YTickLabel',[]);
set(gca, 'XTick', [-xmax+0.5 0 xmax-0.5]);
set(gca, 'YTick', [-ymax+3 0 ymax-3]);
set(gca,'Color',[0 0.8 0]);

moteradius = 0.8;

C1 = [0.9 0 0];  % red team
C2 = [0 0 0.9]; % blue team
C3 = [0.2 0.2 0.2];
C4 = [0.9 0.7 0]; % ball color

for k=1:3
  Cteam1(1,1,k)=C1(k);
  Cteam2(1,1,k)=C2(k);
  Cteam1(1,2,k)=C3(k);
  Cteam2(1,2,k)=C3(k);
end
  
grad = 0:10:360;

X = moteradius*cos(grad*pi/180); 
Y = moteradius*sin(grad*pi/180); 

ball = patch(0.8*X, 0.8*Y, C4);  

goalX = [-5 -5 5 5 4 4 -4 -4];
goalY = [-3 -0.1 -0.1 -3 -3 -1 -1 -3] + ymax;
goal1 = patch(goalX, goalY, [0.8 0.8 0.8]);
goal1 = patch(goalX, -goalY, [0.8 0.8 0.8]);
score = text(-xmax+1, ymax-2, 'SCORE:');
set(score, 'FontSize', 18);
redScore = text(-xmax+9, ymax-2, '0');
set(redScore, 'FontSize', 18, 'Color', [1 0 0]);
blueScore = text(-xmax+11, ymax-2, '0');
set(blueScore, 'FontSize', 18, 'Color', [0 0 1]);

for k = 1:numMotes
  
  moteX=[X'+xInit(k) 0.5*X'+xInit(k)];
  moteY=[Y'+yInit(k) 0.5*Y'+yInit(k)];
  
  if (k <= numMotes/2)
    C = Cteam1;
  else
    C = Cteam2;
  end
  
  motehdl(k) = patch(moteX, moteY, C);
  ID(k) = text(xInit(k)-1.5*moteradius, yInit(k)+1.5*moteradius, num2str(k));
end

sizes = simsizes;
sizes.NumContStates  = 0;
sizes.NumDiscStates  = 0;          
sizes.NumOutputs     = 0;
sizes.NumInputs      = 2*numMotes; % x and y 
sizes.DirFeedthrough = 0;
sizes.NumSampleTimes = 1;   

sys = simsizes(sizes);

x0  = [];
str = [];
ts  = [ts 0]; % Hybrid block


%=============================================================================
% mdlOutputs
%=============================================================================

function [sys]=mdlOutputs(t,x,u)

sys = [];

%=============================================================================
% mdlUpdate
%=============================================================================

function [sys]=mdlUpdate(t,x,u)

global motehdl ID ball xBall yBall

numMotes = length(motehdl);

moteradius = 0.8;
grad = 0:10:360;
X = moteradius*cos(grad*pi/180); 
Y = moteradius*sin(grad*pi/180); 

set(ball, 'XData', 0.8*X+xBall, 'YData', 0.8*Y+yBall);  

%update graphics
for k=1:numMotes
  moteX=[X'+u(k) 0.5*X'+u(k)];
  moteY=[Y'+u(k+numMotes) 0.5*Y'+u(k+numMotes)];
  
  set(motehdl(k),'XData', moteX, 'YData', moteY);
  set(ID(k), 'Position', [u(k)-1.5*moteradius u(k+numMotes)+1.5*moteradius]); 
end

drawnow;

sys=[];


%=============================================================================
% mdlTerminate
% Perform any end of simulation tasks.
%=============================================================================

function sys=mdlTerminate(t,x,u)

sys = [];




  


  
  

