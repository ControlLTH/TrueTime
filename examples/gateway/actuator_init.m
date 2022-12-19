function actuator_init

% Initialize TrueTime kernel
ttInitKernel('prioFP');  % scheduling policy - fixed priority

deadline = 10.0;
ttCreateTask('actuator_task', deadline, 'actuator_code');

ttAttachNetworkHandler('actuator_task')


