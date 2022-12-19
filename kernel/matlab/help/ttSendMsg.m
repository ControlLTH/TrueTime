% TTSENDMSG   Send a message over a network.
%             This function is used both for wired and wireless networks.
%
%  Usage: ttSendMsg(receiver, data, length)
%         ttSendMsg(receiver, data, length, priority)
%         ttSendMsg([network receiver], data, length)
%         ttSendMsg([network receiver], data, length, priority)
%
%  Inputs:
%    receiver  The number of the receiving node (a number between
%              1 and the number of nodes). It is allowed to send 
%              messages to oneself. Specify receiver number 0 to 
%              broadcast a message to all nodes in the network. 
%    data      The contents of the message (arbitrary data).
%    length    The length of the message, in bits. Determines 
%              the time it will take to transmit the message.
%              NOTE: Changed from bytes to bits in TrueTime 1.4
%    priority  The priority of the message (relevant only for
%              CSMA/AMP networks). If not specified, the priority 
%              will be given by the number of the sending node,
%              i.e., messages sent from node 1 will have the
%              highest priority by default.
%    network   The network interface on which the message should 
%              be sent. The default network number is 1. 
%       
% See also TTINITNETWORK, TTGETMSG

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
