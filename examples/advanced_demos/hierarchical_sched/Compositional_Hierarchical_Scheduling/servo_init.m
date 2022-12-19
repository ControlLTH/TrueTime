function servo_init()

% An example of compositional hierarchical scheduling using CBS servers.
% Since CBS gives away the CPU if there is nothing to execute, this
% implementation is limited to capacity reclaiming version of hierarchical
% scheduling. In order to remove capacity reclaiming, CBS code needs to be
% modified.

ttInitKernel('prioEDF');

ttCreatePeriodicTask('waste1', 0.006, 'waste');
ttCreatePeriodicTask('waste2', 0.012, 'waste');
ttCreatePeriodicTask('waste3', 0.006, 'waste');
ttCreatePeriodicTask('waste4', 0.012, 'waste');
ttCreatePeriodicTask('waste5', 0.024, 'waste');

ttCreateCBS('part1', 0.002, 0.006);
ttCreateCBS('part2', 0.002, 0.006);
ttCreateCBS('part3', 0.002, 0.006);

ttAttachCBS('waste1', 'part1');
ttAttachCBS('waste2', 'part1');
ttAttachCBS('waste3', 'part2');
ttAttachCBS('waste4', 'part2');
ttAttachCBS('waste5', 'part3');