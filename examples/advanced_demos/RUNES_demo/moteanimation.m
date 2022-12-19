function [sys,x0,str,ts]=moteanimation(t,x,u,flag,params)
%ANIMATION S-function for animating the motion of the motes.

switch flag,
 case 0,
  [sys,x0,str,ts] = mdlInitializeSizes(params);
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

function [sys,x0,str,ts]=mdlInitializeSizes(params)

% Global variables used for the rendering
global transmitPower receiverThreshold pathloss motehdl gmotehdl transrad 
global MRAD MV walls nbrOfNodes nbrOfObst node numMotes
global nodeXpos nodeYpos NodeShapeX NodeShapeY
global state_text botText
global changetime oldval

changetime = 0;
oldval = 0;

% Read parameters
numMotes = params(1);
ts = params(2);
xmax = params(3);
ymax = params(4);

% Initialize the figure for use with this simulation

figure(1),clf;
% tit=title('2D Animation');
% set(tit,'FontSize', 30);
set(gcf,'Renderer','OpenGL');
set(gcf,'MenuBar','none');
set(gcf,'Position',[10 10 1000 1000*ymax/xmax])
grid off
axis on
set(gca,'Xlim',[0 xmax],'Ylim',[-ymax/2 ymax/2],'nextplot','add');
% set(gca,'OuterPosition', [-0.14 -0.04 1.24 1.1]);

powlin = 0.01*10^(transmitPower/10);       %dbm->Watt
threslin = 0.01*10^(receiverThreshold/10); %dbm->Watt

reach = (powlin/threslin)^(1/pathloss) - 1;  %how far does the signal reach

grad = 0:10:360;
% x and y coordinates to paint a motes transrad
transRadX = reach*cos(grad*pi/180); 
transRadY = reach*sin(grad*pi/180); 

%Draw obstacles
%obs = [200, 0, 25; 400, 0, 15; 600, 0, 25];
obs = [200, 0, 20];
for i = 1:1
  testX = obs(i,3)*cos(grad*pi/180); 
  testY = obs(i,3)*sin(grad*pi/180);
  testColor = [1 0 0];
  testrad = patch(testX + obs(i,1),testY + obs(i,2), testColor);
end


k=6;
state_text(k) = text(nodeXpos(k)- 30,nodeYpos(k)*0.75, ['Val: ']);
set(state_text(k), 'FontSize',12,'Color',[0 0 1.0]);
set(state_text(k), 'FontWeight','Bold');


k=1;
state_text(k) = text(nodeXpos(k)- 30,nodeYpos(k)*0.70, ['Val: ']);
set(state_text(k), 'FontSize',12,'Color',[1.0 0 0]);
set(state_text(k), 'FontWeight','Bold');

% paint the bots
%for k = 1:numMotes
for k = 1:1
  X = MRAD.*cos(MV);
  Y = MRAD.*sin(MV);
  moteX = X;
  moteY = Y;
  moteColour = [k*0.1 k*0.1 k*0.1];
  
  %the mote
  motehdl(k) = patch(moteX, moteY, moteColour);
  
  botText = text(15, -85, ['0']);
  set(botText, 'FontSize',16,'Color',[0 0 1.0]);
  set(botText, 'FontWeight','Bold');
  
  %the transmission radius
  radiusColor = [0 0.6 0.7];
  transrad(k) = patch(transRadX,transRadY, radiusColor);
  set(transrad(k),'FaceAlpha', 0.1); % genomskinlig
  
  %the ghost mote
  gmotehdl(k) = patch(moteX, moteY, moteColour);
  set(gmotehdl(k),'FaceAlpha', 0.1); % genomskinlig
  
  %motetext(k) = text(xInit(k)-1.8*moteradius, yInit(k)+1.8*moteradius, num2str(k));
end

set(gmotehdl(1),'ButtonDownFcn','disp(7)');

%Paint the walls
walls(1) = patch([0 0 xmax xmax 0], [ymax/2 ymax/2-0.2 ymax/2-0.2 ymax/2 ymax/2], [0 0 0]);
walls(2) = patch([0 0 xmax xmax 0], -1*[ymax/2 ymax/2-0.2 ymax/2-0.2 ymax/2 ymax/2], [0 0 0]);


% first time draw nodes

nodeX = 1.67*NodeShapeX' + nodeXpos(1);
nodeY = 1.67*NodeShapeY' + nodeYpos(1);

node(1) = patch(nodeX,nodeY, [0.8 0.2 0.6]);
set(node(1),'ButtonDownFcn','eventfun(1)');

for k = 2:nbrOfNodes
  color = [0 0 1.0];
  nodeX = NodeShapeX' + nodeXpos(k);
  nodeY = NodeShapeY' + nodeYpos(k);
  node(k) = patch(nodeX, nodeY, color);
  id_text(k) = text(nodeXpos(k)-25,nodeYpos(k)*0.9,[num2str(k)]);
  set(id_text(k), 'FontSize',10,'Color',[0 0 0]);
  set(id_text(k), 'FontWeight','Bold');
end

id_text(1) = text(nodeXpos(1)-25,nodeYpos(1)*0.9,['1']);
set(id_text(1), 'FontSize',10,'Color',[0 0 0]);
set(id_text(1), 'FontWeight','Bold');

set(node(2),'ButtonDownFcn','eventfun(2)');
set(node(3),'ButtonDownFcn','eventfun(3)');
set(node(4),'ButtonDownFcn','eventfun(4)');
set(node(5),'ButtonDownFcn','eventfun(5)');
set(node(6),'ButtonDownFcn','eventfun(6)');


sizes = simsizes;
sizes.NumContStates  = 0;
sizes.NumDiscStates  = 0;          
sizes.NumOutputs     = 0;
sizes.NumInputs      = nbrOfNodes*3 + nbrOfObst + (3+3)*numMotes + (nbrOfNodes - 1) + 1 + 1; % x and y plus cart velocity 
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



global transmitPower receiverThreshold pathloss motehdl 
global gmotehdl transrad MRAD MV nbrOfNodes node numMotes
persistent first_time oldval changetime
global state_text botText

%disp('real x');
%disp(u(1));
%disp('estimated x')
%disp(u(10));




% numMotes = length(motehdl);

powlin = 0.001*10^(transmitPower/10);
threslin = 0.001*10^(receiverThreshold/10);
reach = (powlin/threslin)^(1/pathloss) - 1;

grad = 0:10:360;
% x and y coordinates to paint a motes transrad
transRadX = reach*cos(grad*pi/180); 
transRadY = reach*sin(grad*pi/180); 

%update graphics
%for k=1:numMotes
for k=1:1
  realAng = u(k*3);
  
  X = MRAD.*cos(MV+realAng);
  Y = MRAD.*sin(MV+realAng);
  
  moteX = X' + u(k*3-2);
  moteY = Y' + u(k*3-1);
  
  gX = MRAD.*cos(MV+u(3*numMotes + k*3));
  gY = MRAD.*sin(MV+u(3*numMotes + k*3));
  
  gmoteX = gX' + u(3*numMotes + k*3 - 2);
  gmoteY = gY' + u(3*numMotes + k*3 - 1);
  
  %the mote
  set(motehdl(k),'XData', moteX, 'YData', moteY);
  
  %transmission radius
  set(transrad(k),'XData', u(k*3-2) + transRadX, 'YData', u(k*3-1) + transRadY);
  
  %the ghost mote
  set(gmotehdl(k),'XData', gmoteX, 'YData', gmoteY);
  
  %text
  %set(motetext(k), 'Position', [u(k)-1.8*moteradius u(k+numMotes)+1.8*moteradius]);
end



% Set node colors
for k=2:nbrOfNodes;
  if u(numMotes*6 + k*3) == 1,
    set(node(k),'FaceColor', [1 0 0]); 
  else
    set(node(k),'FaceColor', [0 0 1.0]); 
  end
end


for k = 6:6
  set(state_text(k), 'String','');
  set(state_text(k), 'String', ['Val: ' num2str(u(43))]);
end

if t== 0
  changetime = 0;
  oldval = 0;
end

if u(42) ~= oldval 
  oldval = u(42);
  changetime = t;
end
exp((changetime - t)/10);
set(state_text(1),'Color',[1 1-exp((changetime - t)/5) 1-exp((changetime - t)/5)]);

for k = 1:1
  set(state_text(k), 'String','');
  set(state_text(k), 'String', ['Val: ' num2str(u(42))]);
end

%set(botText,'Position',[u(1) u(2)]);
set(botText,'String',[num2str(u(44))]);

% Try to synchronize the animation with the computer time
if isempty(first_time),
  first_time = clock;
end
while etime(clock, first_time)*2<t,
  % disp('waiting')
end

drawnow;

sys=[];

%=============================================================================
% mdlTerminate
% Perform any end of simulation tasks.
%=============================================================================

function sys=mdlTerminate(t,x,u)

sys = [];


%=============================================================================

