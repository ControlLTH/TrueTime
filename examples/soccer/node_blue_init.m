function node_blue_init(argument)

% Slave, gets reference from Base station

global xPos yPos xVel yVel Kx Ky

nodeID = argument;

% Initialize TrueTime kernel
ttInitKernel('prioFP'); % fixed priority

% Position controller in timer handler
data.u1 = 0;
data.u2 = 0;
data.xvel=0;
data.yvel=0;
data.Ix = 0;
data.Iy = 0;
data.Tix = 0.7;
data.Tiy = 0.7;
data.K1 = Kx(nodeID);
data.K2 = Ky(nodeID);
data.xref = xPos(nodeID);
data.yref = yPos(nodeID);
data.nodeID = nodeID;
data.waiting = 0;
data.master = 6; % node 6 is my master

prio = 2;

ttCreateEvent('blueNodePacket');
ttCreateTask('Ctrlblue',0.05,'blueCtrlcode',data);
ttCreateJob('Ctrlblue');

% Initialize network
ttCreateHandler('nw_handler', 1, 'blueMsgRcvSlave');
ttAttachNetworkHandler('nw_handler');

ttCreateMailbox('packets', 10);
