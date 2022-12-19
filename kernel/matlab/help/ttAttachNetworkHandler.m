% TTATTACHNETWORKHANDLER  Attach an interrupt handler to a network
%                         interface. The handler will be invoked
%                         every time a message arrives over the
%                         network. (Note: If you want to use
%                         polling, no network handler is needed.)
%
%  Usage: ttAttachNetworkHandler(handlername)
%         ttAttachNetworkHandler(network, handlername)
%
%  Inputs:
%    handlername  The name of the interrupt handler that should be
%                 invoked when a message arrives over the network.
%    network      The number of the TrueTime network block.
%                 The default network number is 1.
%
% See also TTCREATEINTERRUPTHANDLER, TTSENDMSG, TTGETMSG

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
