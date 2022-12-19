function servo_init()

% An example of non-compositional hierarchical scheduling using only
% TTDispatchers

ttInitKernel('prioEDF');

part1 = [1 0 2 0 0 0];
part2 = [0 3 0 4 0 0];
part3 = [0 0 0 0 5 0];

ttCreatePeriodicTask('waste1', 0.006, 'waste');
ttCreatePeriodicTask('waste2', 0.012, 'waste');
ttCreatePeriodicTask('waste3', 0.006, 'waste');
ttCreatePeriodicTask('waste4', 0.012, 'waste');
ttCreatePeriodicTask('waste5', 0.024, 'waste');

ttCreateTimeTriggeredDispatcher('part1',0.001,part1);
ttCreateTimeTriggeredDispatcher('part2',0.001,part2);
ttCreateTimeTriggeredDispatcher('part3',0.001,part3);