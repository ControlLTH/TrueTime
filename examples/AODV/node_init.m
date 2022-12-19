function node_init(arg)

% Wireless Ad-hoc Routing Using AODV.
%
% This example shows an implementation of Ad-hoc On-Demand Distance
% Vector (AODV) routing in TrueTime. A scenario involving seven 
% nodes is used, all initialized by this initialization script.
%
% Node 1 sends data periodically to node 7 with period 0.5.
% The initial route that is established is 1 -> 3 -> 5 -> 7.
% At time t=3, node 5 breaks the route by moving away. The route 
% is repaired by node 6, creating the route 1 -> 2 -> 5 -> 6 -> 7.

nodeID = arg;

% Initialize TrueTime kernel
ttInitKernel('prioFP'); % nbrOfInputs, nbrOfOutputs, fixed priority

ttCreateMailbox('AODVSendBox', 20); % Data messages to AODV layer
ttCreateMailbox('AODVRcvBox', 20);  % Data messages from AODV layer

% AODV task to send application messages
% initiates route discovery if necessary
dataAODVsnd.nodeID = nodeID;
dataAODVsnd.RREQID = 0;   
dataAODVsnd.buffer = {};  % message buffer
dataAODVsnd.buflen = zeros(1,7); 
dataAODVsnd.emptyBuffer = 0;
dataAODVsnd.bufInd = 0;
prio = 5;
ttCreateTask('AODVSendTask', 1, 'AODVsendcode', dataAODVsnd);
ttSetPriority(prio, 'AODVSendTask');

% Task to process incoming messages
% higher prio than AODVSend to allow faster routes to be
% obtained while sending buffered data
dataAODVrcv.nodeID = nodeID;
dataAODVrcv.cache = {};    % cache for processed RREQs
dataAODVrcv.cnt = 0;
dataAODVrcv.RERRlist = {}; % list of RERRs to propagate
prio = 1;
ttCreateTask('AODVRcvTask', 1, 'AODVrcvcode', dataAODVrcv);
ttSetPriority(prio, 'AODVRcvTask');

% Periodic task to send HELLO messages and
% discover/handle lost connections
datahello.nodeID = nodeID;
datahello.lastRREQ = -100;
datahello.nbors = {};
datahello.cnt = 0;
datahello.RERRlist = {}; % list of RERRs to send
offset = 0.1*rand(1); % to avoid collisions between HELLO msgs
prio = 3;
HELLO_INTERVAL = 1;
ttCreatePeriodicTask('HelloTask', offset, HELLO_INTERVAL, 'hellocode', datahello);
ttSetPriority(prio, 'HelloTask');

% Timer handler
ttCreateHandler('timer_handler', 2, 'timercode');
% Task to handle expiry of routing table entries
data_exp.nodeID = nodeID;
data_exp.expTimer = 0; % flag to indicate timer set
prio = 4;
ttCreateTask('TimerTask', 1, 'expcode', data_exp);
ttSetPriority(prio, 'TimerTask');


% Application tasks
% -----------------
% Node 1 periodically sends data to Node 7
% Jobs of RcvTask in node 7 created from AODV layer
% Node 5 starts moving at time 3  to break the route
% Node 6 starts moving at time 10 to re-establish the route
prio = 10;
starttime = 0;
period = 0.5;
if (nodeID==1)
  datasend.nodeID = nodeID;
  ttCreatePeriodicTask('SendTask', starttime, period, 'APPLsendcode', datasend);
  ttSetPriority(prio, 'SendTask');
end
if (nodeID==5)
  ttCreatePeriodicTask('MoveTask', starttime, period, 'APPLmovecode5');
  ttSetPriority(prio, 'MoveTask');
end
if (nodeID==6)
  ttCreatePeriodicTask('MoveTask', starttime, period, 'APPLmovecode6');
  ttSetPriority(prio, 'MoveTask');
end
if (nodeID==7)
  datarcv.nodeID = nodeID;
  ttCreateTask('RcvTask', 1, 'APPLrcvcode', datarcv);
  ttSetPriority(prio, 'RcvTask');
end
  
% Create and attach network handler
data_nwhdl.nodeID = nodeID;
ttCreateHandler('nw_handler', 2, 'nwrcvcode', data_nwhdl);
ttAttachNetworkHandler('nw_handler');
