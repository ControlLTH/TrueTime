/*
 * Copyright (c) 2016 Lund University
 *
 * Written by Anton Cervin, Dan Henriksson and Martin Ohlin,
 * Department of Automatic Control LTH, Lund University, Sweden.
 *   
 * This file is part of TrueTime 2.0.
 *
 * TrueTime 2.0 is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * TrueTime 2.0 is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TrueTime 2.0. If not, see <http://www.gnu.org/licenses/>
 */

// This file contains the dispatcher for time-triggered scheduling and calls the function for
// slot shifting when Slot Shifting is enabled.

#ifndef CREATE_TIME_TRIGGERED_DISPATCHER
#define CREATE_TIME_TRIGGERED_DISPATCHER

#include "getnode.cpp"
#include "attachtimetriggereddispatcher.cpp"

// TT dispatcher handler -- called at the Time-Triggered slot boundaries
double ttDispatcherCodeFcn(int segment, void *data) {

	char buf[200];
	Task *task, *next;
	UserTask *userTask;
	TimeTriggeredDispatcher *dispatcher = (TimeTriggeredDispatcher*) data;
	int slotIndex=((int)((float)(rtsys->time/dispatcher->slotLength)) % dispatcher->schedulingTable.size());

	// Remove all tasks from readyQ belonging to TTDispatcher and move to tmpQ
	task = (Task*) rtsys->readyQs[dispatcher->affinity]->getFirst();
	while (task != NULL)
	{
		next = (Task*) task->getNext();
		if (task->isUserTask())
		{
			userTask = (UserTask*) task;
			if (userTask->ttdisp == dispatcher)
			{
				task->moveToList(dispatcher->tmpQ);
			}
		}
		task = next;
	}

	if (dispatcher->schedulingTable[slotIndex] > 0)
	{
		// Select the task to execute
		bool taskMoved = false;
		task = (Task*) dispatcher->tmpQ->getFirst();
		while (task != NULL)
		{
			next = (Task*) task->getNext();
			userTask = (UserTask*) task;
			// Select one of the tasks to execute
			if (userTask->taskIdentifier == dispatcher->schedulingTable[slotIndex])		// Current slot is for selected task
			{
				userTask->moveToList(rtsys->readyQs[dispatcher->affinity]);
				taskMoved = true;
				//OUT("Task %s moved to readyQ\n", userTask->name);
				break;
			}
			task = next;
		}
		// If job is not yet released, notify user
		if (!taskMoved)
		{
			sprintf(buf, "disp(\'@time %f: TTDispatcher Info: Task with identifier %d is not ready to execute\')", rtsys->time, dispatcher->schedulingTable[slotIndex]);
			mexEvalString(buf);
		}
	}

	// Schedule dispatcher timer for next slot
	dispatcher->slotTimer->time = rtsys->time + dispatcher->slotLength;
	dispatcher->slotTimer->moveToList(rtsys->timeQ);

	return FINISHED;
}

void ttCreateTimeTriggeredDispatcher(const char *name, double Ts, double *tSeq, int size) {

	char errbuf[MAXERRBUF];
	UserTask *userTask;
	DataNode *dn;
	TimeTriggeredDispatcher *dispatcher, *ndispatcher;

	if (strcmp(name,"") == 0) {
		TT_MEX_ERROR("ttCreateTimeTriggeredDispatcher: Name should be a non-empty string!");
		return;
	}
	if (rtsys->prioFcn == NULL) {
		TT_MEX_ERROR("ttCreateTimeTriggeredDispatcher: Kernel must be initialized before creation of TT Dispatcher!");
		return;
	}

	dn = getNode(name, rtsys->ttdispList);
	if (dn!=NULL) {
		TT_MEX_ERROR("ttCreateTimeTriggeredDispatcher: Name of TimeTriggeredDispatcher Object not unique!");
		return;
	}

	// Check for invalid task identifiers
	for(int i = 0;i<size;i++)
	{
		if((int)*(tSeq+i)>0)
		{
			bool taskFound = false;
			dn = (DataNode*) rtsys->taskList->getFirst();
			while (dn != NULL)
			{
				userTask = (UserTask*) dn->data;
				if (userTask->taskIdentifier == (int)*(tSeq+i))		// task idenifiers match
				{
					taskFound = true;
					break;
				}
				dn = (DataNode*) dn->getNext();
			}
			if (!taskFound)
			{
				snprintf(errbuf, MAXERRBUF, "Task identifier %d not found!", (int)*(tSeq+i));
				TT_MEX_ERROR(errbuf);
				return;
			}
		}
		else if ((int)*(tSeq+i)<0)
		{
			TT_MEX_ERROR("Task identifiers must be greater than 0. No negative integers allowed.");
			return;
		}
	}

	dispatcher = new TimeTriggeredDispatcher(name,Ts,tSeq,size);
	rtsys->ttdispList->appendNode(new DataNode(dispatcher, dispatcher->name));

	// Create time-triggered handler and timer to activate dispatcher at the slot boundaries
	InterruptHandler *hdl = new InterruptHandler("ttdispatcher");
	hdl->codeFcn = ttDispatcherCodeFcn;
	hdl->priority = -1000.0;
	hdl->display = false;
	hdl->nonpreemptible = true;
	hdl->data = (void *)dispatcher;

	Timer *timer = new Timer("ttdispatcher");
	timer->period = Ts;
	timer->isPeriodic = true;
	timer->isOverrunTimer = true;   // = do not delete after expiry
	timer->task = hdl;
	dispatcher->slotTimer = timer;

	// Schedule dispatcher timer at start
	dispatcher->slotTimer->time = 0.0;
	dispatcher->slotTimer->moveToList(rtsys->timeQ);

	// Attach tasks defined in the table to the dispatcher
	dn = (DataNode*) rtsys->taskList->getFirst();
	while (dn != NULL)
	{
		userTask = (UserTask*) dn->data;
		for(int i = 0;i<size;i++)
		{
			if((int)*(tSeq+i) == userTask->taskIdentifier)
			{
				ttAttachTimeTriggeredDispatcher(userTask->name, name);
				//sprintf(errbuf, "disp(\'Task %s attached to TTdispatcher\')", userTask->name);
				//mexEvalString(errbuf);
				break;
			}
		}
		dn = (DataNode*) dn->getNext();
	}

	// Make sure that existing TTDispatchers have no overlapping with current dispatcher
	dn = (DataNode*) rtsys->ttdispList->getFirst();
	while (dn != NULL)
	{
		ndispatcher = (TimeTriggeredDispatcher*) dn->data;
		if (ndispatcher->affinity == dispatcher->affinity && dispatcher != ndispatcher)
		{
			if (ndispatcher->schedulingTable.size() != size)
			{
				TT_MEX_ERROR("All TTDispatchers must have same scheduling table lengths! Check for hyperperiod of the dispatchers will be implemented in the next version.");
				return;
			}
			if (fabs(ndispatcher->slotLength - Ts) > 0.0000000001f)
			{
				TT_MEX_ERROR("All TTDispatchers must have same slot lengths.");
				return;
			}
			for (unsigned slotIter = 0; slotIter < size; slotIter++)
			{
				if ((int)*(tSeq+slotIter) > 0 && ndispatcher->schedulingTable[slotIter] > 0)
				{
					TT_MEX_ERROR("Two tasks can not execute on a single processor at the same time.");
					return;
				}
			}
		}
		dn = (DataNode*) dn->getNext();
	}
}

#endif
