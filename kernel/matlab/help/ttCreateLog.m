% TTCREATELOG   Create a user-defined or task-related log
%
%  Usage: ttCreateLog(logname, variable, size)
%         ttCreateLog(taskname, logtype, variable, size)
%
%  Inputs:
%    name      The name of a user-defined log. Values can later
%              be written using ttLogStart, ttLogStop and ttLogNow.              
%    variable  The name of the variable in MATLAB workspace to
%              which the log will be written after the simulation.
%    size      The maximum number of elements in the log.
%    taskname  Name of the task for which the log is created.
%    logtype   The task log type, should be any of:
%                 
%              1  - Response time log
%              2  - Release latency log, time between arrival
%                   and release of each task job
%              3  - Start latency log, time between arrival
%                   and start of execution for each task job
%              4  - Execution time log
%
% See also TTLOGSTART, TTLOGSTOP, TTLOGNOW, TTLOGVALUE

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
