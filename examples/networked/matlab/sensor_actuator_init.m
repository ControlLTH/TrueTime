function sensor_actuator_init

% Distributed control system: sensor node
%
% Samples the plant periodically and sends the samples to the 
% controller node. Actuates controls sent from controller.

% Initialize TrueTime kernel
ttInitKernel('prioDM');   % deadline-monotonic scheduling

% Periodic sensor task
starttime = 0.0;
period = 0.010;
ttCreatePeriodicTask('sensor_task', starttime, period, 'sensor_code');

% Sporadic actuator task, activated by arriving network message
deadline = 10.0;
ttCreateTask('actuator_task', deadline, 'actuator_code');
ttAttachNetworkHandler('actuator_task')

