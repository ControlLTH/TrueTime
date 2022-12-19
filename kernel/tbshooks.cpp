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

#ifndef TBS_HOOKS
#define TBS_HOOKS

/**
 * TBS hooks to implement total bandwidth servers.
 * See also the additional handler code in createtbs.cpp
 */


void TBS_arrival(UserTask *task) {

  rtsys->default_arrival(task);

  TBS* tbs = task->tbs;
  
    // Activate the tbsTimer to assign the deadline and resort the jobs in the readyQ (doing it over here wouldn't result in correct readyQ since the new task arriving is not yet pushed to the Q)
    tbs->tbsTimer->time = rtsys->time;
    tbs->tbsTimer->moveToList(rtsys->timeQ);

}

#endif
