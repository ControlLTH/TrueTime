function ttsend_callback(cb,trigger,nwnbr,sender,recsource,lengthsource,priosource,msgIDsource)

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

%%% Mask change callback

if nargin == 0
  mask_visibilities = get_param(gcb, 'MaskVisibilities');
  mask_values = get_param(gcb, 'MaskValues');
  for k = [5 7 9 11]
    if strcmp(mask_values{k}, 'internal')
      mask_visibilities{k+1} = 'on';
    else
      mask_visibilities{k+1} = 'off';
    end
  end
  set_param(gcb, 'MaskVisibilities', mask_visibilities)
  return
end
  
%%% Mask initialization callback

% Fix the trigger type
set_param([cb '/Subsystem/Trigger'], 'TriggerType', trigger);

% Fix the Goto block
gototag = ['snd' num2str(nwnbr) '_' num2str(sender)];
set_param([cb '/Goto'], 'GotoTag', gototag);

% Keep track of number of input ports 
nip = 2; % data and trigger input ports always exist
 
% Check if internal/external sources have changed
names = {'Receiver','Length','Prio','MsgID'};
values = {recsource,lengthsource,priosource,msgIDsource};

for k = 1:length(names)
  block = [cb '/' names{k}];
  oldblocktype = get_param(block, 'BlockType');
  position = get_param(block, 'Position');
  orientation = get_param(block, 'Orientation');
  if isequal(values{k}, 2)
    newblocktype = 'Inport';
  else 
    newblocktype = 'Constant';
  end
  if ~isequal(newblocktype,oldblocktype)
    delete_block(block)
    add_block(['built-in/' newblocktype], block, 'Position', position, 'Orientation', orientation)
  end
  % Set correct port numbers
  if isequal(values(k), 2)
    nip = nip + 1;
    set_param(block,'Port',num2str(nip))
  end
end
