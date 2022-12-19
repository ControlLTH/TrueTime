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

global motehdl transrad pow thres ID

% Read parameters
numMotes = params(1);
ts = params(2);
xmax = params(3);
ymax = params(4);

% Initialize the figure for use with this simulation

figure(1),clf;
tit=title('Node Topology -- AODV routing');
set(tit,'FontSize', 15);
set(gcf,'Renderer','OpenGL');
set(gcf,'Position',[20 350 700 700])  %[700 500 700 1050])
%grid on
set(gca,'Xlim',[-xmax xmax],'Ylim',[-ymax ymax],'nextplot','add');
set(gca,'XTickLabel',[]);
set(gca,'YTickLabel',[]);
set(gca, 'XTick', -xmax:1:xmax,'XColor', [0 0 0]);
set(gca, 'YTick', -ymax:1:ymax,'YColor', [0 0 0]);

moteradius = 0.5;

powlin = 0.001*10^(pow/10);
threslin = 0.001*10^(thres/10);

reach = (powlin/threslin)^(1/3.5) - 1;

deg = 0:10:360;

X = moteradius*cos(deg*pi/180); 
Y = moteradius*sin(deg*pi/180); 

radC = [0.2 0.8 0.2;
	0.8 0.2 0.2;
	0.2 0.2 0.8;
	0.8 0.8 0.2;
	0.8 0.2 0.8;
	0.2 0.8 0.8;
	0.8 0.5 0.2;
	0.2 0.8 0.5;
	0.5 0.2 0.8;
	0.5 0.5 0.5];

for k = 1:numMotes
  
  moteX = X'+xInit(k);
  moteY = Y'+yInit(k);

  motehdl(k) = patch(moteX, moteY, radC(k,:));
  transrad(k) = line(reach*(1/moteradius)*X'+xInit(k),reach*(1/moteradius)*Y'+yInit(k));
  set(transrad(k),'Color', radC(k,:))
  ID(k) = text(xInit(k)-1.8*moteradius, yInit(k)+1.8*moteradius, num2str(k));
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

global motehdl transrad pow thres ID

numMotes = length(motehdl);

moteradius = 0.5;
deg = 0:10:360;
X = moteradius*cos(deg*pi/180); 
Y = moteradius*sin(deg*pi/180); 

powlin = 0.001*10^(pow/10);
threslin = 0.001*10^(thres/10);

reach = (powlin/threslin)^(1/3.5) - 1;

%update graphics
for k=1:numMotes

  moteX = X'+u(k);
  moteY = Y'+u(k+numMotes);
  radX = (1/moteradius)*reach*X' + u(k);
  radY = (1/moteradius)*reach*Y' + u(k+numMotes);
  
  set(motehdl(k),'XData', moteX, 'YData', moteY);
  set(transrad(k),'XData', radX, 'YData', radY);
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




  


  
  

