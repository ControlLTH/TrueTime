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

#include "slotshifting.h"

#define NOASSOCIATION -1			// Identifies task is not associated to any interval
#define STARTVALUE 1000.0			// Initialization value for intervalStart in seconds (maximum limit to simulation time)
#define PRECISION 0.000000001
#define HP_PRECISION 1000000000.0f

enum {REGULAR, GUARANTEE_ALGO};			// Enums for updateSpareCapacity

using namespace std;

void slotShiftingOnline(SlotShifting *data);
void updateInterval(SlotShifting *slotShift);
void searchFirmTasks(vector<UserTask*> *firmTasks, SlotShifting *slotShift);
void findMinDeadlines(SlotShifting *slotShift, UserTask **minSoft, UserTask **minGuaranteed);
int scheduleTask(SlotShifting *slotShift, UserTask** executingTask);
int acceptanceTest(SlotShifting *slotShift, UserTask* firmTask);
void updateSpareCapacity(int taskInterval, SlotShifting *slotShift, int type, UserTask* aperiodic);
int getInterval(SlotShifting * slotShift, double absDeadline, int startIndex);
void guaranteeAlgorithm(SlotShifting * slotShift, UserTask* firmTask, int taskInterval);
double getDelta(double newSC, double oldSC);
void createNewIntervals(SlotShifting *slotShift);

// Slot-shifting schedule handler -- called at the Time-Triggered slot boundaries
double slotShiftCodeFcn(int segment, void *data) {

	Task *task, *next;
	UserTask *userTask;
	SlotShifting *slotShift = (SlotShifting*) data;

	// Remove all tasks from readyQ and move to tmpQ
	task = (Task*) rtsys->readyQs[slotShift->affinity]->getFirst();
	while (task != NULL)
	{
		next = (Task*) task->getNext();
		if (task->isUserTask() && task->affinity == slotShift->affinity)
		{
			userTask = (UserTask*) task;
			userTask->moveToList(slotShift->tmpQ);
			//OUT("Task %s moved to tmpQ", userTask->name);
		}
		task = next;
	}

	slotShiftingOnline(slotShift);							// Execute online procedure for Slot Shifting

	return FINISHED;
}

void slotShiftingOffline(void *data)
{
	DataNode *dataNode;
	UserTask *userTask;

	SlotShifting *slotShift = (SlotShifting*) data;

	// Calculate start of intervals
	// Search for deadlines
	dataNode = (DataNode*) rtsys->taskList->getFirst();
	while (dataNode != NULL){							// Task list has not reached its end
		userTask = (UserTask*) dataNode->data;
		if (userTask->affinity == slotShift->affinity)				// Task belong to the same core as slot shifter
		{
			if (userTask->activationTimer != NULL)				// Task is a periodic task
			{
				int numberOfJobs = ceil(slotShift->hyperPeriod / userTask->activationTimer->period);
				//OUT ("Number of jobs of %s are %d, hp=%f, p=%f, phi=%f, dl=%f", userTask->name, numberOfJobs, slotShift->hyperPeriod, userTask->activationTimer->period, userTask->activationTimer->time, userTask->deadline);
				for (int jobIter = 0; jobIter < numberOfJobs; jobIter++)
				{
					bool deadlineFound = false;
					// Check if deadline is already present in the list created
					//OUT ("Looking for deadline %f", (userTask->activationTimer->time + jobIter * userTask->activationTimer->period + userTask->deadline));
					for(int intervalIndex = 0; intervalIndex < slotShift->intervalEnd.size(); intervalIndex++)		// For each interval
					{
						if (fabs(slotShift->intervalEnd[intervalIndex] - (userTask->activationTimer->time + jobIter * userTask->activationTimer->period + userTask->deadline)) < PRECISION){	// One of the interval's end is equal to this task's deadline
							deadlineFound = true;
						}
					}
					if (!deadlineFound){					// Interval end needs to be created
						if (!((userTask->activationTimer->time + jobIter * userTask->activationTimer->period + userTask->deadline) > slotShift->hyperPeriod))	// job not out of HP
						{
							//OUT ("Adding the deadline");
							slotShift->intervalEnd.push_back(userTask->activationTimer->time + userTask->activationTimer->period * jobIter + userTask->deadline);
						}
					}
				}
			}
		}
		dataNode = (DataNode*) dataNode->getNext();
	}

	// Sort the list of deadlines
	sort(slotShift->intervalEnd.begin(), slotShift->intervalEnd.end());
	//OUT("intervalEnd.size = %d", slotShift->intervalEnd.size());

	// Find EST of intervals (i.e. minimum EST of tasks inside the interval) and WCET of intervals (i.e. sum of WCETs of all tasks belonging to the interval)
	vector <double> intervalEST(slotShift->intervalEnd.size(), STARTVALUE);
	vector <double> intervalWCET(slotShift->intervalEnd.size(), 0.0f);
	dataNode = (DataNode*) rtsys->taskList->getFirst();
	while (dataNode != NULL){ 							// Task list has not reached its end
		userTask = (UserTask*) dataNode->data;
		if (userTask->affinity == slotShift->affinity)				// Task belong to the same core as slot shifter
		{
			if (userTask->activationTimer != NULL)				// Task is a periodic task
			{
				int numberOfJobs = ceil(slotShift->hyperPeriod / userTask->activationTimer->period);
				if (userTask->activationTimer->time + userTask->activationTimer->period * (numberOfJobs - 1) + userTask->deadline > slotShift->hyperPeriod)
					numberOfJobs--;
				int intervalNumber;
				//OUT("task %s found with est %f, wcet %f, deadline %f, period %f, jobs %d", userTask->name, userTask->activationTimer->time, userTask->wcExecTime, userTask->deadline, userTask->activationTimer->period, numberOfJobs);
				for (int jobIter = 0; jobIter < numberOfJobs; jobIter++)
				{
					//OUT ("Looking for deadline %f", (userTask->activationTimer->time + jobIter * userTask->activationTimer->period + userTask->deadline));
					intervalNumber = getInterval(slotShift, userTask->activationTimer->time + userTask->activationTimer->period * jobIter + userTask->deadline, 0);
					if (intervalNumber != NOASSOCIATION)	// Task belongs to the interval
					{
						//OUT ("found @ %d", intervalNumber);
						if (intervalEST[intervalNumber] > userTask->activationTimer->time + userTask->activationTimer->period * jobIter){
							//OUT ("EST set");
							intervalEST[intervalNumber] = userTask->activationTimer->time + userTask->activationTimer->period * jobIter;		// Set new minimum EST
						}
						intervalWCET[intervalNumber] += userTask->wcExecTime;
					}
				}
				userTask->isGuaranteed = true;
			}
		}
		dataNode = (DataNode*) dataNode->getNext();
	}
	//OUT("intervalEST.size = %d, intervalWCET.size = %d", intervalEST.size(), intervalWCET.size());

	// Calculate start of interval using the formula ---- >
	// intervalStart = max(end of previous interval, EST of interval)
	slotShift->intervalStart.push_back(intervalEST[0]);
	for (int intervalIter = 1; intervalIter < intervalEST.size(); intervalIter++)
	{
		slotShift->intervalStart.push_back( max(slotShift->intervalEnd[intervalIter-1], intervalEST[intervalIter]) );
	}
	//OUT("intervalStart.size = %d", slotShift->intervalStart.size());

	// Create empty intervals if necessary
	for(int intervalIter=0; intervalIter < slotShift->intervalEnd.size() - 1; intervalIter++)	{		// For each interval	(indexStart = slotShift->intervalEnd.size())
		if(fabs(slotShift->intervalStart[intervalIter + 1] - slotShift->intervalEnd[intervalIter]) > PRECISION)	{	// Interval (i+1)'s start is not equal to interval i's end
			slotShift->intervalEnd.insert(slotShift->intervalEnd.begin() + intervalIter + 1, slotShift->intervalStart[intervalIter + 1]);	// Fill new value of end with start
			slotShift->intervalStart.insert(slotShift->intervalStart.begin() + intervalIter + 1, slotShift->intervalEnd[intervalIter]);	// Insert empty interval's start
			intervalWCET.insert(intervalWCET.begin() + intervalIter + 1, 0.0f);
		}
	}

	// Make sure that first interval starts at 0
	if (slotShift->intervalStart[0] > PRECISION)	{
		// Shift arrays right
		slotShift->intervalStart.insert(slotShift->intervalStart.begin(), 0.0f);
		slotShift->intervalEnd.insert(slotShift->intervalEnd.begin(), slotShift->intervalStart[1]);
		intervalWCET.insert(intervalWCET.begin(), 0.0f);
	}

	// Make sure that in the case when maxPhase = 0, the last interval ends at the hyperperiod
	if (slotShift->startIndexForSchedulingCycle == 0)										// maxPhase == 0
	{
		if (fabs (slotShift->hyperPeriod - slotShift->intervalEnd[slotShift->intervalEnd.size() - 1]) > PRECISION)		// last interval ends before hyperPeriod
		{
			slotShift->intervalStart.push_back(slotShift->intervalEnd[slotShift->intervalEnd.size()-1]);
			slotShift->intervalEnd.push_back(slotShift->hyperPeriod);
			intervalWCET.push_back(0.0f);
		}
	}

	// Find out spare capacities
	slotShift->intervalSpareCapacity.assign( slotShift->intervalEnd.size(), 0.0f);
	for(int intervalIter = slotShift->intervalEnd.size() - 1; intervalIter >= 0 ; intervalIter--)					// For all intervals in reverse order
	{
		// Calculate Spare Capacity according to formula:
		// SC = |Ii| - intervalWCET + min (0, SC(Ii-1))
		slotShift->intervalSpareCapacity[intervalIter] = slotShift->intervalEnd[intervalIter] - slotShift->intervalStart[intervalIter] - intervalWCET[intervalIter];
		if (intervalIter != (slotShift->intervalEnd.size() - 1) && slotShift->intervalSpareCapacity[intervalIter + 1] < 0.0f){	// Previous interval has negative spare capacity and current interval is not the last one
			slotShift->intervalSpareCapacity[intervalIter] += slotShift->intervalSpareCapacity[intervalIter + 1];		// Add next spare capacity in current one
		}
	}

	// Print intervals
	//OUT("IntervalStart.size = %d, IntervalEnd.size = %d", slotShift->intervalStart.size(), slotShift->intervalEnd.size());
	for(int intervalIter = 0; intervalIter < slotShift->intervalStart.size(); intervalIter++){
		if (slotShift->intervalStart[intervalIter] > slotShift->intervalEnd[intervalIter])
		{
			TT_MEX_ERROR("Error (intervalStart is greater than intervalEnd) in Offline phase of slot shifting. Please contact TUKL with the example code");
			return;
		}
		if (fabs (slotShift->intervalSpareCapacity[intervalIter]) < PRECISION)
			slotShift->intervalSpareCapacity[intervalIter] = 0.0f;
		//OUT("IntervalStart[%d] = %f, IntervalEnd[%d] = %f, IntervalSC[%d] = %f", intervalIter, slotShift->intervalStart[intervalIter], intervalIter, slotShift->intervalEnd[intervalIter], intervalIter, slotShift->intervalSpareCapacity[intervalIter]);
		//OUT("IntervalStart[%d] = %f, IntervalEnd[%d] = %f", intervalIndex, slotShift->intervalStart[intervalIndex], intervalIndex, slotShift->intervalEnd[intervalIndex]);
	}

	// Save interval data
	for (int intervalIter = 0; intervalIter < slotShift->intervalEnd.size(); intervalIter++)
	{
		slotShift->savedIntervalStart.push_back(slotShift->intervalStart[intervalIter]);
		slotShift->savedIntervalEnd.push_back(slotShift->intervalEnd[intervalIter]);
		slotShift->savedIntervalSpareCapacity.push_back(slotShift->intervalSpareCapacity[intervalIter]);
	}
}

void slotShiftingOnline(SlotShifting *slotShift)
{
	Task *task, *next;
	UserTask *userTask;
	//OUT("@time %f", rtsys->time);

	// Load schedule and intervals at the start of the hyperperiod
	updateInterval(slotShift);

	// Search for hard tasks and sort the list if a hard task exists
	vector<UserTask*> firmTasks;
	searchFirmTasks(&firmTasks, slotShift);

	if (firmTasks.size() > 1)
	{
		// Sort the list of firm tasks according to deadline
		sort (firmTasks.begin(), firmTasks.end(), sortAccordingToAbsDeadline);
	}
	// Check if firm tasks can be accepted
	for (unsigned firmIter = 0; firmIter < firmTasks.size(); firmIter++)
	{
		int taskInterval;
		// Run Acceptance Test
		//OUT("FirmTask %s detected at time %f having absDeadline %f", firmTasks[firmIter]->name, rtsys->time, firmTasks[firmIter]->absDeadline);
		taskInterval = acceptanceTest(slotShift, firmTasks[firmIter]);
		if (taskInterval == NOASSOCIATION)		// task can not be accepted
		{
			// Remove the task
			firmTasks[firmIter]->execTime = 0.0;
			firmTasks[firmIter]->segment = 0;
			firmTasks[firmIter]->nextSegment = 1;
			firmTasks[firmIter]->finish_hook(firmTasks[firmIter]);
			firmTasks[firmIter]->remove();
			firmTasks.erase(firmTasks.begin() + firmIter);
			firmIter--;
			continue;
		}
		// Run guarantee algorithm for each of the hard tasks
		guaranteeAlgorithm(slotShift, firmTasks[firmIter], taskInterval);
	}
	firmTasks.clear();

	// Find out all tasks available to execute and find out the minimum deadline
	UserTask *minGuaranteed, *minSoft;
	minGuaranteed = NULL;
	minSoft = NULL;
	findMinDeadlines(slotShift, &minSoft, &minGuaranteed);

	// Determine task to execute (Scheduling Decision)
	UserTask *executingTask;
	if (slotShift->intervalSpareCapacity[slotShift->intervalIndex] > 0 && minSoft != NULL){		// Spare capacity is greater then 0 and there is a soft task
		executingTask = minSoft;
	}else {
		executingTask = minGuaranteed;
	}

	// Pick the task having minimum Deadline from tmpQ and execute it
	int taskInterval=0;
	taskInterval = scheduleTask(slotShift, &executingTask);

	// Update Spare Capacities
	updateSpareCapacity(taskInterval, slotShift, REGULAR, NULL);
}

void guaranteeAlgorithm(SlotShifting * slotShift, UserTask* firmTask, int taskInterval)
{
	firmTask->isGuaranteed = true;
	if(fabs(slotShift->intervalEnd[taskInterval] - firmTask->absDeadline) > PRECISION)				// Deadline != End of interval
	{
		//OUT("Dividing Interval %d", taskInterval);
		// Create new intervals
		slotShift->intervalEnd.insert(slotShift->intervalEnd.begin() + taskInterval, firmTask->absDeadline);
		slotShift->intervalStart.insert(slotShift->intervalStart.begin() + taskInterval + 1, firmTask->absDeadline);
		slotShift->intervalSpareCapacity.insert(slotShift->intervalSpareCapacity.begin() + taskInterval, 0.0f);
		double effectiveLengthOfInterval;
		if (fabs(slotShift->intervalSpareCapacity[taskInterval] - rtsys->time) < PRECISION)
		{
			effectiveLengthOfInterval = slotShift->intervalEnd[taskInterval] - rtsys->time;
		}
		else
		{
			effectiveLengthOfInterval = slotShift->intervalEnd[taskInterval] - slotShift->intervalStart[taskInterval];
		}
		double oldSC = slotShift->intervalSpareCapacity[taskInterval + 1];
		slotShift->intervalSpareCapacity[taskInterval + 1] -= effectiveLengthOfInterval;
		slotShift->intervalSpareCapacity[taskInterval] = effectiveLengthOfInterval + getDelta(slotShift->intervalSpareCapacity[taskInterval + 1], oldSC);

		//OUT("Interval %d: start=%f, end=%f, sc=%f", taskInterval, slotShift->intervalStart[taskInterval], slotShift->intervalEnd[taskInterval], slotShift->intervalSpareCapacity[taskInterval]);
		//OUT("Interval %d: start=%f, end=%f, sc=%f", taskInterval + 1, slotShift->intervalStart[taskInterval + 1], slotShift->intervalEnd[taskInterval + 1], slotShift->intervalSpareCapacity[taskInterval + 1]);
	}
	// Update Spare Capacities
	updateSpareCapacity(taskInterval, slotShift, GUARANTEE_ALGO, firmTask);
}

// Looks for interval of the deadline
int getInterval(SlotShifting *slotShift, double absDeadline, int startIndex)
{
	for (unsigned intervalIter = startIndex; intervalIter < slotShift->intervalEnd.size(); intervalIter++)
	{
		//OUT ("Inter%d, start=%f, end=%f, sc=%f", intervalIter, slotShift->intervalStart[intervalIter], slotShift->intervalEnd[intervalIter], slotShift->intervalSpareCapacity[intervalIter]);
		if (fabs(slotShift->intervalEnd[intervalIter] - absDeadline) < PRECISION)
		{
			//OUT("returning %d", intervalIter);
			return intervalIter;
		}
	}
	return NOASSOCIATION;
}

// Used to calculate delta for adjusting the spare capacities
double getDelta(double newSC, double oldSC) 
{
	if (oldSC > PRECISION)
		if (newSC < PRECISION)
			return newSC;
		else
			return 0.0f;
	else
		return newSC - oldSC;
}

// Update Spare Capacities till taskInterval according to task userTask
void updateSpareCapacity(int taskInterval, SlotShifting *slotShift, int type, UserTask* aperiodic){

	double prVal;		// Only used while printing
	if (type == REGULAR)
	{
		if (taskInterval==NOASSOCIATION)	{		// Either system is left idle or a Soft task is selected
			prVal = slotShift->intervalSpareCapacity[slotShift->intervalIndex];
			// Decrease Spare capacity by 1 unit of Slot Length
			slotShift->intervalSpareCapacity[slotShift->intervalIndex] -= slotShift->slotLength;
			if (fabs(slotShift->intervalSpareCapacity[slotShift->intervalIndex]) < PRECISION)				// Spare Capacity is -0.00000
			{
				slotShift->intervalSpareCapacity[slotShift->intervalIndex] = 0.0f;
			}
			//OUT("    1. Interval %d's spare capacity is changed from %f to %f", slotShift->intervalIndex, prVal, slotShift->intervalSpareCapacity[slotShift->intervalIndex]);
		}
		else											// Either Hard task or Static Task was executed and
		{
			if (taskInterval != slotShift->intervalIndex)	{				// Task does not belong to current interval
				// Increase spare capacity of the interval to which the task is associated
				for(int intervalIter= taskInterval ; intervalIter >= slotShift->intervalIndex; intervalIter--)	{	// From deadline's interval till current interval in reverse order
					prVal = slotShift->intervalSpareCapacity[intervalIter];
					slotShift->intervalSpareCapacity[intervalIter] += slotShift->slotLength;		// Increase Spare capacity by 1 unit of Slot Length
					if (fabs(slotShift->intervalSpareCapacity[intervalIter]) < PRECISION)				// Spare Capacity is -0.00000
					{
						slotShift->intervalSpareCapacity[intervalIter] = 0.0f;
					}
					//OUT("    2. Interval %d's spare capacity is changed from %f to %f", intervalIter, prVal, slotShift->intervalSpareCapacity[intervalIter]);
					if(slotShift->intervalSpareCapacity[intervalIter] > 0.0f)	{			// If not lent
						break;
					}
				}
				prVal = slotShift->intervalSpareCapacity[slotShift->intervalIndex];
				slotShift->intervalSpareCapacity[slotShift->intervalIndex] -= slotShift->slotLength;		// Decrease SC of current interval by 1 unit of Slot Length
				if (fabs(slotShift->intervalSpareCapacity[slotShift->intervalIndex]) < PRECISION)				// Spare Capacity is -0.00000
				{
					slotShift->intervalSpareCapacity[slotShift->intervalIndex] = 0.0f;
				}
				//OUT("    3. Interval %d's spare capacity is changed from %f to %f", slotShift->intervalIndex, prVal, slotShift->intervalSpareCapacity[slotShift->intervalIndex]);
			}
			else
			{
				//OUT("    4. No need to update spare capacity");
			}
		}
	}
	else					// type == GUARANTEE_ALGO
	{
		double delta = -1.0f * aperiodic->wcExecTime;
		double oldSC;
		while(delta < 0.0f) {
			//OUT ("Delta = %f", delta);
			oldSC = slotShift->intervalSpareCapacity[taskInterval];
			slotShift->intervalSpareCapacity[taskInterval] += delta;
			if (fabs(slotShift->intervalSpareCapacity[taskInterval]) < PRECISION)
				slotShift->intervalSpareCapacity[taskInterval] = 0.0f;
			//OUT ("Interval %d: SC_new=%f, SC_old=%f", taskInterval, slotShift->intervalSpareCapacity[taskInterval], oldSC);
			delta = getDelta(slotShift->intervalSpareCapacity[taskInterval], oldSC);
			if (fabs(delta) < PRECISION)
				delta = 0.0f;
		        taskInterval--;
		}
	}
}

// Search if a hard task arrived and store the sorted (according to absDeadline) hard tasks in vector hardTasks
void searchFirmTasks(vector<UserTask*> *firmTasks, SlotShifting *slotShift){
	// Check if there is an aperiodic task.
	UserTask *userTask;

	Task *task = (Task*) slotShift->tmpQ->getFirst(), *next;
	while (task != NULL)	{										// tempQ has not reached its end
		next = (Task*) task->getNext();
		userTask = (UserTask*) task;
		if (userTask->activationTimer == NULL)								// Task is an aperiodic
		{
			if (userTask->firmTask == true)	{								// Task is a hard task
				if (userTask->isGuaranteed == false)	{						// Task was not served before
					(*firmTasks).push_back(userTask);
				}
			}
		}
		task = next;
	}
}

// Updates intervalIndex upon correct slot boundary
void updateInterval(SlotShifting *slotShift)
{
	// Check if new scheduling table needs to be loaded
	if (fabs(slotShift->intervalEnd[slotShift->intervalEnd.size()-1] - rtsys->time) < PRECISION)		// Last interval in memory is reached to its end
	{
		createNewIntervals(slotShift);
	}
	if (fabs(slotShift->intervalEnd[slotShift->intervalIndex] - rtsys->time) < PRECISION)
	{
		// Check if spare capacity was correct for the previous interval
		if (fabs(slotShift->intervalSpareCapacity[slotShift->intervalIndex]) > PRECISION)
		{
			char buf[MAXERRBUF];
			sprintf(buf, "Error! Spare capacity of interval %d is less than 0. Contact TUKL with the example code.\n", slotShift->intervalIndex);
			TT_MEX_ERROR(buf);
			return;
		}
		//OUT("Previous SC=%f, Interval index changed to %d", slotShift->intervalSpareCapacity[slotShift->intervalIndex], slotShift->intervalIndex+1);
		slotShift->intervalIndex++;
	}
}

void createNewIntervals(SlotShifting *slotShift)
{
	// load schedule
	for (unsigned intervalIndex = slotShift->startIndexForSchedulingCycle; intervalIndex < slotShift->savedIntervalEnd.size(); intervalIndex++)
	{
		slotShift->intervalStart.push_back(slotShift->intervalEnd[slotShift->intervalEnd.size()-1]);
		slotShift->intervalEnd.push_back(slotShift->intervalStart[slotShift->intervalStart.size()-1] + slotShift->savedIntervalEnd[intervalIndex] - slotShift->savedIntervalStart[intervalIndex]);
		slotShift->intervalSpareCapacity.push_back(slotShift->savedIntervalSpareCapacity[intervalIndex]);
		//OUT("New interval %d created: start = %f, end = %f, sc = %f", slotShift->intervalStart.size() - 1, slotShift->intervalStart[slotShift->intervalStart.size() - 1], slotShift->intervalEnd[slotShift->intervalEnd.size() - 1], slotShift->intervalSpareCapacity[slotShift->intervalSpareCapacity.size() - 1]);
	}
}

// Algorithm for acceptance of an online task in slotshifting
int acceptanceTest(SlotShifting *slotShift, UserTask* firmTask)
{
	if ((firmTask->absDeadline - rtsys->time) >= firmTask->wcExecTime) {// Deadline is at least as far as computation time
		
		int createdIntervals = slotShift->intervalEnd.size();
		// Create new intervals till deadline if necessary
		while (slotShift->intervalEnd[slotShift->intervalEnd.size()-1] < firmTask->absDeadline)
		{
			createNewIntervals(slotShift);
		}
		createdIntervals = slotShift->intervalEnd.size() - createdIntervals;

		// Look for interval of deadline (Cannot use getInterval() since it returns the interval for which the deadline match. Here it may not be the case)
		int deadlineInterval;
		for (deadlineInterval = slotShift->intervalIndex; deadlineInterval < slotShift->intervalEnd.size(); deadlineInterval++)
		{
			if (slotShift->intervalEnd[deadlineInterval] > firmTask->absDeadline)
			{
				if (fabs(firmTask->absDeadline - slotShift->intervalEnd[deadlineInterval-1]) < PRECISION)		// deadlines are equal
					deadlineInterval--;
				break;
			}
		}
		//OUT("deadlineInterval = %d, start = %f, end = %f, sc = %f", deadlineInterval, slotShift->intervalStart[deadlineInterval], slotShift->intervalEnd[deadlineInterval], slotShift->intervalSpareCapacity[deadlineInterval]);

		// Calculate the available spare capacity according to the formula
		// availableSC = currentIntervalSC + fullIntervalsSC + lastIntervalSC (where lastIntervalSC = min(SC of the last interval, aperiodic->absDeadline - intervalEnd)
		// Find out spare capacities in between the deadline interval and current interval
		double fullIntervalsSC = 0.0f, lastIntervalSC = 0.0f;			// Spare capacities between deadline's interval and current interval
		for (int intervalIter = deadlineInterval - 1; intervalIter > slotShift->intervalIndex; intervalIter--){
			if(slotShift->intervalSpareCapacity[intervalIter] > 0.0f){ 		// Spare capacity of interval i is not negative
				fullIntervalsSC += slotShift->intervalSpareCapacity[intervalIter];
			}
		}
		lastIntervalSC = min (slotShift->intervalSpareCapacity[deadlineInterval], (firmTask->absDeadline - slotShift->intervalEnd[deadlineInterval - 1]));
		//OUT("CurrentIntSC = %f, fullIntSC = %f, lastIntSC = %f, TotalAvailableSC = %f, Required = %f", slotShift->intervalSpareCapacity[slotShift->intervalIndex], fullIntervalsSC, lastIntervalSC, slotShift->intervalSpareCapacity[slotShift->intervalIndex] + fullIntervalsSC + lastIntervalSC, firmTask->wcExecTime);
		if (slotShift->intervalSpareCapacity[slotShift->intervalIndex] + fullIntervalsSC + lastIntervalSC < firmTask->wcExecTime)	{	// Available spare capacities are not enough
			//OUT("Not enough spare capacity. Task %s terminated at %f", firmTask->name, rtsys->time);

			// Remove extra created intervals
			for (int intervalIter = 0; intervalIter < createdIntervals; intervalIter++)
			{
				//OUT("Deleting interval %d from memory.", slotShift->intervalStart.size() - 1);
				slotShift->intervalStart.pop_back();
				slotShift->intervalEnd.pop_back();
				slotShift->intervalSpareCapacity.pop_back();
			}
			return NOASSOCIATION;
		}
		else
		{
			//OUT("Task can be accepted.");
			return deadlineInterval;
		}
	}
	else						// Deadline is closer than required time to execute
	{
		// Can not be accommodated, remove task
		//OUT("Deadline - Release < WCET. Task %s terminated at %f", firmTask->name, rtsys->time);
		return NOASSOCIATION;
	}
}

void findMinDeadlines(SlotShifting *slotShift, UserTask **minSoft, UserTask **minGuaranteed)
{
	Task *task, *next;
	UserTask* userTask;
	//OUT ("\ttempQ length = %d", slotShift->tmpQ->length());
	task = (Task*) slotShift->tmpQ->getFirst();
	while (task != NULL){
		next = (Task*) task->getNext();
		userTask = (UserTask*) task;
		//OUT ("    Checking task %s", userTask->name);
		if (userTask->activationTimer == NULL) 						// Task is an aperiodic task
		{
			if (userTask->firmTask == false)					// Task is a soft aperiodic
			{
				if ((*minSoft) != NULL)
				{
					if ((*minSoft)->absDeadline > userTask->absDeadline)		// Selected task's deadline is even earlier
						(*minSoft) = userTask;
				}
				else
					(*minSoft) = userTask;
			}
			else									// Task is a hard aperiodic
			{
				if ((*minGuaranteed) != NULL)
				{
					if ((*minGuaranteed)->absDeadline > userTask->absDeadline)	// Selected task's deadline is even earlier
						(*minGuaranteed) = userTask;
				}
				else
					(*minGuaranteed) = userTask;
			}
		}
		else 										// Task is a periodic task
		{
			if ((*minGuaranteed) != NULL)
			{
				if ((*minGuaranteed)->absDeadline > userTask->absDeadline)		// task has even earlier deadline
					(*minGuaranteed) = userTask;				// Store new deadline
			}
			else
				(*minGuaranteed) = userTask;
		}
		task = next;
	}
	/*if ((*minGuaranteed) != NULL)
	{
		OUT ("    minGuaranteed = %s(%f)", (*minGuaranteed)->name, (*minGuaranteed)->absDeadline);
	}
	if ((*minSoft) != NULL)
	{
		OUT ("    minSoft = %s(%f)", (*minSoft)->name, (*minSoft)->absDeadline);
	}*/
}


// Schedules the task defined by structure taskForSorting and returns the index of its interval
int scheduleTask(SlotShifting *slotShift, UserTask** executingTask)
{
	// If no task is specified, remain Idle
	if ((*executingTask) == NULL)		{			// No task available to execute
		return NOASSOCIATION;
	}

	int taskInterval = 0;
	(*executingTask)->moveToList(rtsys->readyQs[slotShift->affinity]);				// Move to readyQ
	if ((*executingTask)->isGuaranteed == true)		{								// Selected task is associated to some interval
		taskInterval = getInterval(slotShift, (*executingTask)->absDeadline, slotShift->intervalIndex);			// Store interval
		if (taskInterval == NOASSOCIATION)							// task is a guaranteed task and the interval that it belongs to doesn't exist yet
		{
			// Create new intervals
			createNewIntervals(slotShift);
			taskInterval = getInterval(slotShift, (*executingTask)->absDeadline, slotShift->intervalIndex);			// Store interval
		}
	}
	else			// Selected task is not associated to any interval (Soft Task)
	{
		taskInterval = NOASSOCIATION;
	}
	//OUT("    Task Picked = %s, isGuaranteed = %d, taskInterval = %d", (*executingTask)->name, (*executingTask)->isGuaranteed, taskInterval);
	return taskInterval;
}
