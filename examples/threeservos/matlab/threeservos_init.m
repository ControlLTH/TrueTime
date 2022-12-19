function threeservos_init(arg)

% Task scheduling and control.
%
% This example extends the simple PID control example (located in
% $DIR/examples/servo) to the case of three PID-tasks running
% concurrently on the same CPU controlling three different servo
% systems. The effect of the scheduling policy on the global control
% performance is demonstrated.

% Initialize TrueTime kernel

switch arg
 case 1   % DM scheduling
  ttInitKernel('prioDM')
  codefcn = 'pid_code';
 case {2, % plain EDF scheduling
       3, % EDF scheduling, kill jobs that overrun
       5, % EDF scheduling with pid_task3 inside CBS
       6} % EDF scheduling with pid_task3 inside TBS
  ttInitKernel('prioEDF')
  codefcn = 'pid_code';
 case 4   % EDF scheduling, skip next job if current one late
  ttInitKernel('prioEDF')
  codefcn = 'pid_code_skips';
 otherwise
  error('Illegal init argument')
end

% Task parameters
starttimes = [0 0 0];
periods = [0.006 0.005 0.004];
tasknames = {'pid_task1', 'pid_task2', 'pid_task3'};

% Create the three tasks
for i = 1:3
  data.K = 0.96;
  data.Ti = 0.12;
  data.Td = 0.049;
  data.beta = 0.5;
  data.N = 10;
  data.h = periods(i);
  data.u = 0;
  data.Iold = 0;
  data.Dold = 0;
  data.yold = 0;
  data.rChan = 1;
  data.yChan = i+1;
  data.uChan = i;
  data.late = 0;
  
  ttCreatePeriodicTask(tasknames{i}, starttimes(i), periods(i), codefcn, data);
  ttCreateLog(tasknames{i},1,['response' num2str(i)],1000)
end

switch arg
  case 3 % EDF scheduling, kill jobs that overrun
    ttCreateHandler('dl_miss_handler', 1, 'dl_miss_code')
    for i = 1:3
      ttAttachDLHandler(tasknames{i}, 'dl_miss_handler')
    end
  case 5 % EDF scheduling with pid_task3 inside CBS
    ttCreateCBS('cbs', 0.001, 0.004)   % Allow max 25% utilization for pid_task3
    ttAttachCBS(tasknames{3}, 'cbs')
  case 6 % EDF scheduling with pid_task3 inside TBS
    ttCreateTBS('tbs', 0.25)           % Allow max 25% utilization for pid_task3
    ttSetWCET(0.002, tasknames{3})     % Set WCET of the task
    ttAttachTBS(tasknames{3}, 'tbs')
end
