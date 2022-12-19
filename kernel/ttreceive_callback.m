function ttreceive_callback(cb,nwnbr,receiver,triggeroutput,senderoutput,lengthoutput,priorityoutput,timestampoutput,signalpoweroutput,msgIDoutput)

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

% Fix the From block
gototag = ['rcv' num2str(nwnbr) '_' num2str(receiver)];
set_param([cb '/From'], 'GotoTag', gototag);

% Keep track of number of output ports 
nop = 1; % data output ports always exist

% Check if any Show X output port have changed
names = {'Trigger','Sender','Length','Prio','Timestamp','Signal power','Message ID'};
values = {triggeroutput,senderoutput,lengthoutput,priorityoutput,timestampoutput,signalpoweroutput,msgIDoutput};
for k=1:length(names)
  block = [cb '/' names{k}];
  oldblocktype = get_param(block, 'BlockType');
  position = get_param(block, 'Position');
  orientation = get_param(block, 'Orientation');
  if isequal(values{k}, 1)
    newblocktype = 'Outport';
  else 
    newblocktype = 'Terminator';
  end
  if ~isequal(newblocktype,oldblocktype)
    delete_block(block)
    add_block(['built-in/' newblocktype], block, 'Position', position, 'Orientation', orientation)
  end
  % Set correct port numbers
  if isequal(values{k}, 1)
    nop = nop + 1;
    set_param(block,'Port',num2str(nop))
  end
end
