% TTCREATEHANDLER   Create a TrueTime interrupt handler.
%
%  Usage: ttCreateHandler(name, priority, codeFcn) 
%         ttCreateHandler(name, priority, codeFcn, data)
%         ttCreateHandler(name, priority, codeFcn, data, queueLength)
%
%  Inputs:
%    name         Name of the handler, must be unique.
%    priority     The priority of the handler. Should be a positive number,
%                 where a small number represents a high priority.
%    codeFcn      The code function of the handler. Should be a string
%                 with the name of the corresponding m-file.
%    data         An arbitrary data structure used to store
%                 handler-specific data. Optional argument.
%    queueLength  The maximum number of pending interrupts that can be
%                 queued while an interrupt is being served.
%       
% See also TTATTACHDLHANDLER, TTATTACHNETWORKHANDLER, TTATTACHTRIGGERHANDLER,
%          TTATTACHWCETHANDLER, TTCREATETIMER, TTCREATEPERIODICTIMER

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
