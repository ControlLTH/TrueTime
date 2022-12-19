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

#ifndef CREATE_LOG
#define CREATE_LOG

#include "getnode.cpp"

// Create user-defined log

void ttCreateLog(const char *logname, const char *variable, int size) {

  DataNode* dn;
  Log *log;
  char buf[MAXCHARS];

  if (strcmp(logname,"") == 0) {
    TT_MEX_ERROR("ttCreateLog: Logname should be a non-empty string!"); 
    return;
  }
  // can only be called during initialization phase
  if (!rtsys->init_phase) {
    TT_MEX_ERROR("ttCreateLog: Can only be called from the init-function!");
    return;
  }

  dn = getNode(logname, rtsys->logList);
  if (dn != NULL) {
    sprintf(buf, "ttCreateLog: Name of log '%s' not unique!", logname);
    TT_MEX_ERROR(buf);
    return;
  }

  log = new Log(logname, variable, size);
  rtsys->logList->appendNode(new DataNode(log, log->name));
}

// Create task-execution-related log

void ttCreateLog(const char* taskname, int logtype, const char* variable, int size) {

  DataNode* dn;
  UserTask* task;
  char buf[MAXCHARS];
  int logno = logtype - 1;  // convert logtype to value between 0 and 3
  
  // can only be called during initialization phase
  if (!rtsys->init_phase) {
    TT_MEX_ERROR("ttCreateLog: Can only be called from the init-function!");
    return;
  }
  if (logno > EXECTIMELOG || logno < RESPONSETIMELOG) {
    sprintf(buf,"ttCreateLog: unknown log type %d", logtype);
    TT_MEX_ERROR(buf);
    return;
  }

  dn = getNode(taskname, rtsys->taskList);
  if (dn == NULL) {
    sprintf(buf, "ttCreateLog: Non-existent task '%s'", taskname);
    TT_MEX_ERROR(buf);
    return;
  }
  task = (UserTask*) dn->data;

  if (task->logs[logno] != NULL) {
    TT_MEX_ERROR("ttCreateLog: log already assigned");
    return;
  }
  task->logs[logno] = new Log(NULL, variable, size);
}

#endif
