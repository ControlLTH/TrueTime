function servo_init()

ttInitKernel('prioEDF');

tseq = [1 2 0 1 2 0];

ttCreatePeriodicTask('waste1', 0.006, 'waste');
ttCreatePeriodicTask('waste2', 0.008, 'waste');
ttCreatePeriodicTask('waste3', 0.007, 'waste');
ttCreateTimeTriggeredDispatcher('Off_Sched',0.001,tseq);
