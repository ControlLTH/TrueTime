function [sys,x0,str,ts]=mote_state(t,x,u,flag,init_state);
%ANIMATION S-function for animating the motion of the motes.
switch flag,
 case 0,
  [sys,x0,str,ts]= mdlInitializeSizes;
 case 2,
  sys = mdlUpdate(t,x,u);
 case 3,
  sys = mdlOutputs(t,x,u);
 case 65,
 case 9,
  sys = mdlTerminate(t,x,u);
 otherwise
  error(['Unhandled flag = ',num2str(flag)]);
end

%=============================================================================
% mdlInitializeSizes
%=============================================================================

function [sys,x0,str,ts]=mdlInitializeSizes

global state
global axes_handle
global state_text

ts = 0.1;
if length(state) >0
% Read parameters
numMotes = length(state);

% Initialize the figure for use with this simulation
xmax=30;
ymax=700;
figure_handle = figure(3);
clf(figure_handle);
set(figure_handle,'Renderer','OpenGL');
set(figure_handle,'Position',[550 400 500 400])
axes_handle = axes;
title('MOTE STATUS');
grid(axes_handle, 'on')
set(axes_handle,'Xlim',[-xmax xmax],'Ylim',[-ymax ymax],'nextplot','add');
set(axes_handle,'XTickLabel',[]);
set(axes_handle,'YTickLabel',[]);
set(axes_handle, 'XTick', [-xmax+0.5 0 xmax-0.5],'XColor', [1 1 1]);
set(axes_handle, 'YTick', [-ymax+3 0 ymax-3],'YColor', [1 1 1]);
set(axes_handle,'Color',[1 1 0.3]);

moteTitle = text(-xmax+1, ymax-60, 'Player Status:');
set(moteTitle, 'FontSize', 24);

for i=1:numMotes
  state_text(i) = text(-xmax+1, ymax-(100*i)-100, ['Mote ' num2str(i) ...
		    ' State '	state{i}]);
  set(state_text(i), 'FontSize', 16, 'Color', [0 0 0]);
end

end
set(axes_handle,'Color',[1 1 0.3]);
sizes = simsizes;
sizes.NumContStates  = 0;
sizes.NumDiscStates  = 0;          
sizes.NumOutputs     = 0;
sizes.NumInputs      = 0; % x and y 
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

global state
global axes_handle
global state_text

numMotes = length(state);
xmax=30;
ymax=700;
%update graphics

if length(state) > 0
  for i=1:numMotes
    set(state_text(i), 'String', '');
    set(state_text(i), 'String', ['Mote ' num2str(i) ' State	' state{i}]);
  end
end

drawnow;

sys=[];


%=============================================================================
% mdlTerminate
% Perform any end of simulation tasks.
%=============================================================================

function sys=mdlTerminate(t,x,u)
close
sys = [];




  


  
  
