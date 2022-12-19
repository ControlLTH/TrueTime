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

/*
	This function enables the Slot Shifting flag in Offline Class and some other variables
	and allocates memory for different variables to store properties related to Slot Shifting.
	Then it calls a procedure from slotshifting.cpp that Calculates all the Intervals' Start,
	End and Spare Capacities and then copy the Intervals' properties in variables in Offline
	Class for the life of Simulation. It also associates the tasks with particular intervals
	by storing tasks' interval in UserTask's variable called associateToSchedule and
	associationIndex.
 */
#ifndef SETENABLESLOTSHIFTING
#define SETENABLESLOTSHIFTING

#include "getnode.cpp"
#include "slotshifting.cpp"
#include "slotshiftinghooks.cpp"

unsigned long gcd(unsigned long m, unsigned long n)
{
	unsigned long tmp;
	while(m) { tmp = m; m = n % m; n = tmp; }
	return n;
}

unsigned long lcmFunc(unsigned long m, unsigned long n)
{
	return m / gcd(m, n) * n;
}

void ttEnableSlotShifting(const char* name, double slotLength) {

	char buf[MAXERRBUF];
	// Create Slot-shifter
	UserTask *userTask;
	DataNode *dn;

	if (strcmp(name,"") == 0) {
		TT_MEX_ERROR("ttEnableSlotShifting: Name should be a non-empty string!");
		return;
	}
	if (rtsys->prioFcn != rtsys->prioEDF) {
		TT_MEX_ERROR("ttEnableSlotShifting: Kernel must be initialized with \'prioEDF\' before calling ttEnableSlotShifting!");
		return;
	}

	dn = getNode(name, rtsys->slotShiftList);
	if (dn!=NULL) {
		TT_MEX_ERROR("ttEnableSlotShifting: Name of slot shifting object not unique!");
		return;
	}

	SlotShifting* slotShift = new SlotShifting(name,slotLength);
	rtsys->slotShiftList->appendNode(new DataNode(slotShift, slotShift->name));

	//OUT("Name = %s, Slot-length = %f\n", name, slotLength);
	// Create time-triggered handler and timer to activate dispatcher at the slot boundaries
	InterruptHandler *hdl = new InterruptHandler("slotshiftoverrunhandler");
	hdl->codeFcn = slotShiftCodeFcn;
	hdl->priority = -1000.0;
	hdl->display = false;
	hdl->nonpreemptible = true;
	hdl->data = (void *)slotShift;

	Timer *timer = new Timer("slotshiftoverruntimer");
	timer->period = slotLength;
	timer->isPeriodic = true;
	timer->isOverrunTimer = true;   // = do not delete after expiry
	timer->task = hdl;
	slotShift->slotTimer = timer;

	// Schedule slotShift timer at start
	slotShift->slotTimer->time = 0.0;
	slotShift->slotTimer->moveToList(rtsys->timeQ);

	// Attach all created tasks to the slotShift dispatcher and find out the hyperperiod, minimum phase and maximum phase of tasks
	unsigned long lcm = 1;								// lcm in microseconds
	double minPhase = HP_PRECISION, maxPhase = 0.0f;				// Phases in seconds
	dn = (DataNode*) rtsys->taskList->getFirst();
	while (dn != NULL)
	{
		userTask = (UserTask*) dn->data;
		// Make sure that the wcets, phases, deadlines and periods are integeral multiples of slot-length
		userTask->wcExecTime = ceil (userTask->wcExecTime / slotShift->slotLength - PRECISION) * slotShift->slotLength;
		userTask->deadline = ceil (userTask->deadline / slotShift->slotLength - PRECISION) * slotShift->slotLength;
		if (userTask->activationTimer != NULL)
		{
			userTask->activationTimer->time = ceil (userTask->activationTimer->time / slotShift->slotLength - PRECISION) * slotShift->slotLength;
			userTask->activationTimer->period = ceil (userTask->activationTimer->period / slotShift->slotLength - PRECISION) * slotShift->slotLength;
			//OUT ("Task %s, dl=%f, phi=%f, p=%f, wcet=%f", userTask->name, userTask->deadline, userTask->activationTimer->time, userTask->activationTimer->period, userTask->wcExecTime);
			if (userTask->deadline > userTask->activationTimer->period)
			{
				TT_MEX_ERROR("ttEnableSlotShifting: Current version of slot shifting only supports constrained deadlines.");
				return;
			}
		}
		/*else
		{
			OUT ("Task %s, dl=%f, wcet=%f", userTask->name, userTask->deadline, userTask->wcExecTime);
		}*/
		// Perform sanity checks
		if (userTask->wcExecTime > userTask->deadline)
		{
			sprintf(buf, "ttEnableSlotShifting: WCET of task %s can not exceed its deadline.\n", userTask->name);
			TT_MEX_ERROR(buf);
		}
		if (userTask->wcExecTime < slotLength)
		{
			sprintf(buf, "ttEnableSlotShifting: WCET of task %s can not be 0.\n", userTask->name);
			TT_MEX_ERROR(buf);
		}			
		// Attach task to the above created slot-shifter
		if (userTask->finish_hook != rtsys->default_finish)		// Check if free
		{
			char buf[MAXERRBUF];
			sprintf(buf, "ttEnableSlotShifting: Current version of slot-shifting can not be used with other servers and schedulers!");
			TT_MEX_ERROR(buf);
			return;
		}

		if (slotShift->affinity == -1 || slotShift->affinity == userTask->affinity) { // Assign affinity to core
			slotShift->affinity = userTask->affinity;
			slotShift->slotTimer->task->affinity = userTask->affinity;
		} else {
			sprintf(buf, "ttEnableSlotShifting: Slot-shifter '%s' already has affinity to core %d (i.e. tasks running on multiple core can not run under same slot-shifter) and this version only supports single core slot-shifting.\n", name, slotShift->affinity);
			TT_MEX_ERROR(buf);
			return;
		}
		userTask->slotShift = slotShift;

		if (userTask->activationTimer != NULL) {									// Task is a periodic task
			//OUT("period = %f, %d", (*userTask->activationTimer).period, (unsigned long)((*userTask->activationTimer).period * 1000000.0f));
			lcm = lcmFunc((unsigned long)((*userTask->activationTimer).period * HP_PRECISION), lcm);
			if (minPhase > userTask->activationTimer->time)
				minPhase = userTask->activationTimer->time;
			if (maxPhase < userTask->activationTimer->time)
				maxPhase = userTask->activationTimer->time;
		}

		// Assign task's hooks
		userTask->finish_hook = SlotShifting_finish;

		dn = (DataNode*) dn->getNext();
	}
	if (fabs(maxPhase) < 0.00000001f)
	{
		slotShift->hyperPeriod = (float) lcm / HP_PRECISION;
	}
	else
	{
		slotShift->hyperPeriod = minPhase + maxPhase + 2.0f * (float) lcm / HP_PRECISION;	// Leung, Merrill, Joseph, "A note on preemptive scheduling of periodic real-time tasks"
	}
	//OUT("Hyperperiod = %fs, lcm = %f, minPhase = %f, maxPhase = %f", slotShift->hyperPeriod, (float)lcm / HP_PRECISION, minPhase, maxPhase);
	if (minPhase > 0.000000001f)
	{
		TT_MEX_ERROR ("ttEnableSlotShifting: Current version of slot-shifting assumes that the minimum phase of taskset is zero.");
		return;
	}

	DataNode *dataNode;

	dataNode = (DataNode*) getNode(name, rtsys->slotShiftList);
	if (dataNode == NULL) {
		char buf[MAXERRBUF];
		sprintf(buf, "ttEnableSlotShifting: Non-existent slot-shifter '%s'. Contact TUKL with the example code.", name);
		TT_MEX_ERROR(buf);
		return;
	}

	slotShiftingOffline(dataNode->data);

	if (fabs(maxPhase) > 0.00000001f)
	{
		// Calculate index of interval to start repetition from
		for (slotShift->startIndexForSchedulingCycle = 0; slotShift->startIndexForSchedulingCycle < slotShift->savedIntervalStart.size(); slotShift->startIndexForSchedulingCycle++)
		{
			if ( (slotShift->savedIntervalEnd[slotShift->startIndexForSchedulingCycle] > (slotShift->hyperPeriod - (double)lcm / HP_PRECISION)) )	// interval with instant s + P [Leung, Merrill, Joseph, "A note on preemptive scheduling of periodic real-time tasks"]
			{
				break;
			}
		}
	}
	//OUT ("Start index for loading new scheduling table = %d", slotShift->startIndexForSchedulingCycle);
}

#endif
