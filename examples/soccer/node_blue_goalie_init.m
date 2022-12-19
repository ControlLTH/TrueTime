function node_blue_goalie_init(argument)

% Master, provides reference trajectories

global xPos yPos Kx Ky

nodeID = argument;

% Initialize TrueTime kernel
ttInitKernel('prioFP'); % fixed priority

data.nodeID = nodeID;
data.u1 = 0;
data.u2 = 0;
data.K1 = Kx(nodeID);
data.K2 = Ky(nodeID);
data.xref = xPos(nodeID);
data.yref = yPos(nodeID);

ttCreateEvent('blueGoaliePacket');
ttCreateTask('CtrlblueGoalie', 0.05, 'goalieCtrl', data);
ttCreateJob('CtrlblueGoalie');

% Initialize network
ttCreateHandler('nw_handler', 1, 'blueMsgRcvMaster', data);
ttAttachNetworkHandler('nw_handler'); 
