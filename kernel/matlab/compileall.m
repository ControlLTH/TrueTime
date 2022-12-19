% Copyright (c) 2016 Lund University
%
% Written by Anton Cervin, Dan Henriksson and Martin Ohlin,
% Department of Automatic Control LTH, Lund University, Sweden.
%   
% This file is part of TrueTime 2.0.
%
% TrueTime 2.0 is free software: you can redistribute it and/or
% modify it under the terms of the GNU General Public License as
% published by the Free Software Foundation, either version 3 of the
% License, or (at your option) any later version.
%
% TrueTime 2.0 is distributed in the hope that it will be useful, but
% without any warranty; without even the implied warranty of
% merchantability or fitness for a particular purpose. See the GNU
% General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with TrueTime 2.0. If not, see <http://www.gnu.org/licenses/>

mex -g ttAbortSimulation.cpp		       
disp('ttAbortSimulation.cpp')
disp('');
mex -g ttAnalogIn.cpp		       
disp('ttAnalogIn.cpp')
disp('');
mex -g ttAnalogOut.cpp
disp('ttAnalogOut.cpp')
disp('');
mex -g ttAttachCBS.cpp
disp('ttAttachCBS.cpp');
disp('');
mex -g ttAttachDLHandler.cpp
disp('ttAttachDLHandler.cpp');
disp('');
mex -g ttAttachNetworkHandler.cpp
disp('ttAttachNetworkHandler.cpp');
disp('');
mex -g ttAttachTBS.cpp
disp('ttAttachTBS.cpp');
disp('');
mex -g ttAttachTimeTriggeredDispatcher.cpp
disp('ttAttachTimeTriggeredDispatcher.cpp');
disp('');
mex -g ttAttachTriggerHandler.cpp
disp('ttAttachTriggerHandler.cpp');
disp('');
mex -g ttAttachWCETHandler.cpp  
disp('ttAttachWCETHandler.cpp');
disp('');
mex -g ttCallBlockSystem.cpp
disp('ttCallBlockSystem.cpp');
disp('');
mex -g ttCreateCBS.cpp
disp('ttCreateCBS.cpp');
disp('');
mex -g ttCreateEvent.cpp
disp('ttCreateEvent.cpp');
disp('');
mex -g ttCreateHandler.cpp 
disp('ttCreateHandler.cpp');
disp('');
mex -g ttCreateJob.cpp
disp('ttCreateJob.cpp');
disp('');
mex -g ttCreateLog.cpp
disp('ttCreateLog.cpp');
disp('');
mex -g ttCreateMailbox.cpp	
disp('ttCreateMailbox.cpp');
disp('');
mex -g ttCreateMonitor.cpp	
disp('ttCreateMonitor.cpp');
disp('');
mex -g ttCreatePeriodicTask.cpp  
disp('ttCreatePeriodicTask.cpp');
disp('');
mex -g ttCreatePeriodicTimer.cpp
disp('ttCreatePeriodicTimer.cpp');
disp('');
mex -g ttCreateSemaphore.cpp
disp('ttCreateSemaphore.cpp');
disp('');
mex -g ttCreateTask.cpp
disp('ttCreateTask.cpp');
disp('');
mex -g ttCreateTBS.cpp
disp('ttCreateTBS.cpp');
disp('');
mex -g ttCreateTimer.cpp 
disp('ttCreateTimer.cpp');
disp('');
mex -g ttCreateTimeTriggeredDispatcher.cpp
disp('ttCreateTimeTriggeredDispatcher.cpp');
disp('');
mex -g ttCurrentTask.cpp 
disp('ttCurrentTask.cpp');
disp('');
mex -g ttCurrentTime.cpp 
disp('ttCurrentTime.cpp');
disp('');
mex -g ttDiscardUnsentMessages.cpp 
disp('ttDiscardUnsentMessages.cpp');
disp('');
mex -g ttEnableSlotShifting.cpp
disp('ttEnableSlotShifting.cpp');
disp('');
mex -g ttEnterMonitor.cpp 
disp('ttEnterMonitor.cpp');
disp('');
mex -g ttExitMonitor.cpp
disp('ttExitMonitor.cpp');
disp('');
mex -g ttFetch.cpp
disp('ttFetch.cpp');
disp('');
mex -g ttGetAbsDeadline.cpp 
disp('ttGetAbsDeadline.cpp');
disp('');
mex -g ttGetArrival.cpp 
disp('ttGetArrival.cpp');
disp('');
mex -g ttGetBudget.cpp
disp('ttGetBudget.cpp');
disp('');
mex -g ttGetCPUTime.cpp 
disp('ttGetCPUTime.cpp');
disp('');
mex -g ttGetData.cpp 
disp('ttGetData.cpp');
disp('');
mex -g ttGetDeadline.cpp 
disp('ttGetDeadline.cpp');
disp('');
mex -g ttGetInvoker.cpp 
disp('ttGetInvoker.cpp');
disp('');
mex -g ttGetMsg.cpp
disp('ttGetMsg.cpp');
disp('');
mex -g ttGetPeriod.cpp
disp('ttGetPeriod.cpp');
disp('');
mex -g ttGetPriority.cpp 
disp('ttGetPriority.cpp');
disp('');
mex -g ttGetRelease.cpp  
disp('ttGetRelease.cpp');
disp('');
mex -g ttGetWCET.cpp
disp('ttGetWCET.cpp');
disp('');
mex -g ttGive.cpp
disp('ttGive.cpp');
disp('');
mex -g ttInitKernel.cpp  
disp('ttInitKernel.cpp');
disp('');
mex -g ttKillJob.cpp
disp('ttKillJob.cpp');
disp('');
mex -g ttLogNow.cpp
disp('ttLogNow.cpp');
disp('');
mex -g ttLogStart.cpp
disp('ttLogStart.cpp');
disp('');
mex -g ttLogStop.cpp
disp('ttLogStop.cpp');
disp('');
mex -g ttLogValue.cpp
disp('ttLogValue.cpp');
disp('');
mex -g ttNonPreemptible.cpp
disp('ttNonPreemptible.cpp');
disp('');
mex -g ttNoSchedule.cpp
disp('ttNoSchedule.cpp');
disp('');
mex -g ttNotify.cpp
disp('ttNotify.cpp');
disp('');
mex -g ttNotifyAll.cpp
disp('ttNotifyAll.cpp');
disp('');
mex -g ttPost.cpp
disp('ttPost.cpp');
disp('');
mex -g ttRemoveTimer.cpp
disp('ttRemoveTimer.cpp');
disp('');
mex -g ttRetrieve.cpp
disp('ttRetrieve.cpp');
disp('');
mex -g ttSendMsg.cpp
disp('ttSendMsg.cpp');
disp('');
mex -g ttSetAbsDeadline.cpp
disp('ttSetAbsDeadline.cpp');
disp('');
mex -g ttSetBudget.cpp
disp('ttSetBudget.cpp');
disp('');
mex -g ttSetCBSParameters.cpp
disp('ttSetCBSParameters.cpp');
disp('');
mex -g ttSetCPUAffinity.cpp
disp('ttSetCPUAffinity.cpp');
disp('');
mex -g ttSetData.cpp
disp('ttSetData.cpp');
disp('');
mex -g ttSetDeadline.cpp
disp('ttSetDeadline.cpp');
disp('');
mex -g ttSetKernelParameter.cpp
disp('ttSetKernelParameter.cpp');
disp('');
mex -g ttSetNetworkParameter.cpp
disp('ttSetNetworkParameter.cpp');
disp('');
mex -g ttSetNextSegment.cpp
disp('ttSetNextSegment.cpp');
disp('');
mex -g ttSetNumberOfCPUs.cpp
disp('ttSetNumberOfCPUs.cpp');
disp('');
mex -g ttSetPeriod.cpp
disp('ttSetPeriod.cpp');
disp('');
mex -g ttSetPriority.cpp
disp('ttSetPriority.cpp');
disp('');
mex -g ttSetTaskCriticality.cpp
disp('ttSetTaskCriticality.cpp');
disp('');
mex -g ttSetWCET.cpp
disp('ttSetWCET.cpp');
disp('');
mex -g ttSleep.cpp
disp('ttSleep.cpp');
disp('');
mex -g ttSleepUntil.cpp
disp('ttSleepUntil.cpp');
disp('');
mex -g ttTake.cpp
disp('ttTake.cpp');
disp('');
mex -g ttTryFetch.cpp
disp('ttTryFetch.cpp');
disp('');
mex -g ttTryPost.cpp
disp('ttTryPost.cpp');
disp('');
mex -g ttUltrasoundPing.cpp
disp('ttUltrasoundPing.cpp');
disp('');
mex -g ttWait.cpp
disp('ttWait.cpp');
disp('');
