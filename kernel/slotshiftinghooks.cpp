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

#ifndef SLOT_SHIFTING_HOOKS
#define SLOT_SHIFTING_HOOKS

extern int getInterval(SlotShifting * slotShift, double absDeadline, int startIndex);

void SlotShifting_finish(UserTask *task) {

  rtsys->default_finish(task);
  
  if (task->budget > 0.000000001f && task->isGuaranteed)				// Code completed but wcet is still remaining
  {
	SlotShifting *slotShift = task->slotShift;
	int deadlineInterval = getInterval(slotShift, task->absDeadline, slotShift->intervalIndex);
	double prVal;
	while (task->budget > 0.000000001f)
	{
		task->budget -= slotShift->slotLength;
		for(int intervalIter = deadlineInterval; intervalIter >= slotShift->intervalIndex; intervalIter--)	{	// From deadline's interval till current interval in reverse order
			prVal = slotShift->intervalSpareCapacity[intervalIter];
			slotShift->intervalSpareCapacity[intervalIter] += slotShift->slotLength;		// Increase Spare capacity by 1 unit of Slot Length
			if (fabs(slotShift->intervalSpareCapacity[intervalIter]) < PRECISION)				// Spare Capacity is -0.00000
			{
				slotShift->intervalSpareCapacity[intervalIter] = 0.0f;
			}
			//OUT("    hook->2. Interval %d's spare capacity is changed from %f to %f, budget=%f", intervalIter, prVal, slotShift->intervalSpareCapacity[intervalIter], task->budget);
			if(slotShift->intervalSpareCapacity[intervalIter] > 0.0f)	{			// If not lent
				break;
			}
		}
	}
  }

  if (task->activationTimer == NULL)				// If aperiodic
	task->isGuaranteed = false;

  //mexPrintf("Offline_finish %s @%.4f: nbrJobs = %d\n", task->name, rtsys->time, cbs->nbrJobs);
}

#endif
