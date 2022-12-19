function regulator_init

% Distributed control system: regulator node
%
% Receives messages from the sensor node, computes control signal
% and sends it back to the actuator node.

% Initialize TrueTime kernel
ttInitKernel('prioFP'); % fixed priority
ttSetKernelParameter('energyconsumption', 0.010); % 10 mW

% Create mailboxes
ttCreateMailbox('sensor_signal', 10)
ttCreateMailbox('power_ping', 10)
ttCreateMailbox('power_response', 10)

% Controller parameters
h = 0.010;
N = 100000;
Td = 0.035;
K = 1.5;

% Create task data (local memory)
data.u = 0.0;
data.K = K; 
data.ad = Td/(N*h+Td);
data.bd = N*K*Td/(N*h+Td);
data.Dold = 0.0;
data.yold = 0.0;

% Create controller task
deadline = h;
prio = 1;
ttCreateTask('pid_task', deadline, 'ctrlcode', data);
ttSetPriority(prio, 'pid_task');

% Create power controller task
offset = 2;
period = 0.025;
prio = 2;
power_data.transmitPower = 20;
power_data.name = 2;     % We are node number 2 in the network
power_data.receiver = 1; % We are communicating with node 1
power_data.haverun = 0;  % We have not run yet
ttCreatePeriodicTask('power_controller_task', offset, period, 'powctrlcode', power_data);
ttSetPriority(prio, 'power_controller_task');

% Create power response task
deadline = 100;
prio = 3;
ttCreateTask('power_response_task', deadline, 'powrespcode');
ttSetPriority(prio, 'power_response_task');

% Initialize network
ttCreateHandler('nw_handler', 1, 'msgRcvCtrl');
ttAttachNetworkHandler('nw_handler');



