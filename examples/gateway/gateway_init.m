function gateway_init

% Initialize TrueTime kernel
ttInitKernel('prioFP');  % scheduling policy - fixed priority

deadline = 10.0;
ttCreateTask('gateway_task', deadline, 'gateway_code');

% Network handler (common for networks 1 and 2)
ttAttachNetworkHandler(1,'gateway_task')
ttAttachNetworkHandler(2,'gateway_task')
