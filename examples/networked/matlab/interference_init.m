function interference_init(arg)

% Distributed control system: interference node
%
% Generates disturbing network traffic with bandwidth arg (given in block mask)

% Initialize TrueTime kernel
ttInitKernel('prioFP');  % fixed priority scheduling

% Interference task
period = 0.001;
offset = 0.0005;
data = arg;
ttCreatePeriodicTask('interference_task', offset, period, 'interference_code', data);
