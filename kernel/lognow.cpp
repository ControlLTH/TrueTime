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

#ifndef LOG_NOW_VALUE
#define LOG_NOW_VALUE

#define simtime2time(time) ((time - rtsys->clockOffset) / rtsys->clockDrift)

// utility function also used in defaulthooks
void logwrite(Log *log, double value) {
  if (log) {
    if (log->entries < log->size) {
      log->vals[log->entries] = value;
      log->entries++;
    }
  }
}

void ttLogNow(const char *logname) {

  DataNode* dn;
  Log* log;

  dn = getNode(logname, rtsys->logList);
  if (dn == NULL) {
    // Log does not exist 
    char buf[MAXCHARS];
    sprintf(buf, "ttLogNow: Non-existent log '%s'!", logname);
    TT_MEX_ERROR(buf);
    return;
  }

  log = (Log*)dn->data;
  logwrite(log, simtime2time(rtsys->time));
}


void ttLogValue(const char *logname, double value) {

  DataNode* dn;
  Log* log;

  dn = getNode(logname, rtsys->logList);
  if (dn == NULL) {
    // Log does not exist 
    char buf[MAXCHARS];
    sprintf(buf, "ttLogValue: Non-existent log '%s'!", logname);
    TT_MEX_ERROR(buf);
    return;
  }

  log = (Log*)dn->data;
  logwrite(log, value);
}

#endif
