% Network parameters
global transmitPower receiverThreshold pathloss 
global errorCodingThreshold pingPeriod 
transmitPower = 36;     %dbm
receiverThreshold = -48; %dbm
errorCodingThreshold = 0.03;
pathloss = 3.5;
pingPeriod = 1;

% Misc parameters
global nbrOfMotes trackLength trackWidth
global nbrOfNodes nbrOfObst 
scale = 15;          % Defines size of objects in animation
nbrOfNodes = 6;     % Sets number of nodes
nbrOfObst = 1;      % Sets number of obstacles on track
nbrOfMotes = 3;     % Sets number of motes in simulation
trackLength = 800;  % Sets length of track in cm
trackWidth = 200;   % Sets width of track in cm

% Mote shape (Defines the shape of a mote in animation)
global MRAD MV
MRAD = scale*[1.2500 0.9014 0.5590 0.2795 1.0078 1.0078 0.2795 0.5590 0.9014 1.2500 1.2500 0.9014 0.5590 0.5590 0.9014 1.2500 1.2500];
MV = [2.2143 2.5536	2.0344	2.6779	3.0172	-3.0172	-2.6779	-2.0344 -2.5536	-2.2143	-0.9273	-0.588	-1.1071	1.1071	0.588	0.9273	2.2143];

% Node shape (Defines the shape of a node in animation)
global NodeShapeX NodeShapeY
NodeShapeX = scale*[-0.5  0.5  0.5 -0.5 -0.5];
NodeShapeY = scale*[ 0.5  0.5 -0.5 -0.5  0.5];

% Node positions (Defines where the nodes are positioned in the animation
% and simulation)
global nodeXpos nodeYpos nodeBelievedXpos nodeBelievedYpos
deltaX = trackLength/nbrOfNodes;
yPos = trackWidth/2;
%xPosAdd = 15;
xPosAdd = 0;
yPosAdd = 0;
for k = 1:nbrOfNodes
    nodeXpos(k) = deltaX*(k-0.5) + xPosAdd;
    nodeYpos(k) = yPos - yPosAdd;
    yPos = -yPos;
    xPosAdd = -xPosAdd;
    yPosAdd = -yPosAdd;
end
for k = 1: nbrOfNodes
        nodeBelievedXpos(k) = deltaX*(k-0.5);
        nodeBelievedYpos(k) = yPos;
        yPos = - yPos;
end
disp('Node positions');
disp(nodeXpos);
disp(nodeYpos);

% Other inits
R1 = 3;     % radius left wheel
R2 = 3;     % radius right wheel
D = 30;     % distance between wheels
T = 1;      % motor dynamics parameter