clear functions

rand('state',0);

nbrNodes = 7;

% Lots of printouts or just a few?
global verbose
verbose = 0; 

% Initialize routing table (7 nodes)
global routing_table
for k=1:nbrNodes
  routing_table{k} = [];
end
  
% Initialize AODV sequence numbers
global seqNbrs
seqNbrs = zeros(nbrNodes, nbrNodes);

% Initialize AODV parameters
global AODVparams
AODVparams.ACTIVE_ROUTE_TIMEOUT = 3; % 3000 msec
AODVparams.MY_ROUTE_TIMEOUT = 2 * AODVparams.ACTIVE_ROUTE_TIMEOUT;
AODVparams.HELLO_INTERVAL = 1; % 1000 msec
AODVparams.ALLOWED_HELLO_LOSS = 2;
AODVparams.DELETE_PERIOD = AODVparams.ALLOWED_HELLO_LOSS * AODVparams.HELLO_INTERVAL;

% Initialize node positions, transmission power, and thresholds
global xPos yPos pow thres

% X positions
xPos(1) = -15;
xPos(2) = -10;
xPos(3) = -5;
xPos(4) =  0;
xPos(5) =  5;
xPos(6) =  5;
xPos(7) =  15;

% Y positions
yPos(1) =  5;
yPos(2) = -5;
yPos(3) =  10;
yPos(4) = -5;
yPos(5) =  5;
yPos(6) = -10;
yPos(7) =  5;

pow = -8; % dBm
thres = -48;
codethres = 0.03;

% To track sent (from node 1) and 
% received (in node 7) data
global sent
sent = [];
global received
received = [];
