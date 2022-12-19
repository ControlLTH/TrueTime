function controller_init

% Initialize TrueTime kernel
ttInitKernel('prioFP');  % scheduling policy - fixed priority

deadline = 10.0;
ttCreateTask('controller_task', deadline, 'controller_code');

% Network handler
ttAttachNetworkHandler('controller_task')
