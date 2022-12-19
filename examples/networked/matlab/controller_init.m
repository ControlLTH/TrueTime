function controller_init(arg)

% Distributed control system: controller node
%
% Receives messages from the sensor node, computes control signal
% and sends it to the actuator node. Also contains a high-priority
% disturbing task.

% Initialize TrueTime kernel
ttInitKernel('prioDM') % deadline-monotonic scheduling

% Create task data (local memory)
data.h = 0.010;
data.K = 1.0;
data.Td = 0.04;
data.N = 100.0;
data.ad = data.Td/(data.N*data.h+data.Td);
data.bd = data.N*data.K*data.ad;
data.yold = 0;
data.Dold = 0;
data.u = 0;

% Sporadic controller task, activated by arriving network message
deadline = data.h;
ttCreateTask('controller_task', deadline, 'controller_code', data);
ttAttachNetworkHandler('controller_task')

% Periodic dummy task with higher priority
starttime = 0.0;
period = 0.007;
data = period*arg;
ttCreatePeriodicTask('dummy_task', starttime, period, 'dummy_code', data);

