function servo_init_new()

ttInitKernel('prioEDF');                            % Use EDF for Offline scheduling and Slot Shifting

period = 0.012;                                     % Hyper period
tasksNames = {'A' 'B' 'C' 'D'};                     % Names of Periodic Tasks
phases = [0.003 0.003 0.008 0.000];                   % Earliest start times of Periodic tasks
deadlines = [0.009 0.005 0.003 0.002];              % Deadlines of Periodic Tasks
wcets = [0.003 0.001 0.001 0.001];                   % WCETs of Periodic Tasks
tasksFunction = {'pid_code4', 'waste', 'waste', 'waste'};       % Functions used for each task

aperiodicPhases = [0.001, 0.002, 0.001, 0.026];                                                  % Earliest Starts times of Aperiodic Tasks
 
% Create task data (local memory)
data.K = 0.96;
data.Ti = 0.12;
data.Td = 0.049;
data.beta = 0.5;
data.N = 10;
data.h = period;
data.u = 0;
data.Iold = 0;
data.Dold = 0;
data.yold = 0;
data.rChan = 1;
data.yChan = 2;
data.uChan = 1;
data.t = 0;
data.exec = 0;

for task = 1 : 4                                    % For each Periodic Task
 ttCreatePeriodicTask(tasksNames{task}, phases(task), period, tasksFunction{task}, data);         % Create Periodic task with specified perameters
 ttSetDeadline(deadlines(task), tasksNames{task});                                              % Set deadline
 ttSetWCET(wcets(task),tasksNames{task});                                               % Define execution time of task
end

ttCreateTask('E', 0.047, 'waste2', data);               % Create Aperiodic task with specified perameters
ttSetTaskCriticality(1, 'E');
ttSetWCET(0.008,'E');                                               % Define execution time of task
for task = 1 : 3
 ttCreateJob('E', aperiodicPhases(task));                                     % Create a job at specified time
end

ttEnableSlotShifting('SlotShift', 0.001);                   % Use Slot Shifting
%                    name, slotlength
