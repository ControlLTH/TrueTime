#ifndef INVOKE_TASK
#define INVOKE_TASK

void invoke_task(Task *task, const char *invoker) {

  debugPrintf("'%s': task_invoke('%s','%s') at %f\n", rtsys->blockName, task->name, invoker, rtsys->time);

  if (task->isUserTask()) {
    UserTask *usertask = (UserTask*)task;

    usertask->arrival_hook(usertask);
    if (usertask->nbrInvocations == 0) {
      usertask->arrival = rtsys->time;
      usertask->release = rtsys->time;
      usertask->release_hook(usertask);
      usertask->moveToList(rtsys->readyQs[usertask->affinity]);
      usertask->state = READY;
    } else {
      TaskInvocation *ti = new TaskInvocation();
      ti->timestamp = rtsys->time;
      strncpy(ti->invoker, invoker, MAXCHARS); // not used
      usertask->pending->appendNode(new DataNode(ti, NULL));
    }
    usertask->nbrInvocations++;

  } else {

    InterruptHandler *handler = (InterruptHandler*)task;

    if (handler->nbrInvocations == 0) {
      handler->timestamp = rtsys->time;
      strncpy(handler->invoker, invoker, MAXCHARS);
      handler->moveToList(rtsys->readyQs[handler->affinity]);
      handler->state = READY;
    } else {
      TaskInvocation *ti = new TaskInvocation();
      ti->timestamp = rtsys->time;
      strncpy(ti->invoker, invoker, MAXCHARS);
      handler->pending->appendNode(new DataNode(ti, NULL));
    }
    handler->nbrInvocations++;

  }

}

#endif
