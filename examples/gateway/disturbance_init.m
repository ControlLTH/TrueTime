function disturbance_init

% Initialize TrueTime kernel
ttInitKernel('prioFP');  % scheduling policy - fixed priority

% Periodic disturbance task
starttime = 0.0;
period = 0.040;      % 40 ms

ttCreateTask('disturbance_task', starttime, period, 'disturbance_code');
ttCreateJob('disturbance_task',0)

