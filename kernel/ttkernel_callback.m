function ttkernel_callback(cb,mask_values)

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

% Keep track of number of input and output ports 
nip = 0;
nop = 0;

% Check if number of analog inputs and outputs has changed
ninputsoutputs = mask_values{3};
if ~isequal(size(ninputsoutputs),[1 2]) || ~isequal(abs(round(ninputsoutputs)),ninputsoutputs)
  errordlg('Analog inputs and outputs: wrong format')
  return
end
% input port changed?
block = [cb '/A//D'];
oldblocktype = get_param(block, 'BlockType');
if ninputsoutputs(1) == 0 
  newblocktype = 'Ground';
else
  newblocktype = 'Inport';
end
if ~isequal(newblocktype,oldblocktype)
  position = get_param(block, 'Position');
  orientation = get_param(block, 'Orientation');
  delete_block(block)
  add_block(['built-in/' newblocktype],block,'Position',position,'Orientation',orientation);
end
% output port changed?
block = [cb '/D//A'];
oldblocktype = get_param(block, 'BlockType');
if ninputsoutputs(2) == 0
  newblocktype = 'Terminator';
else
  newblocktype = 'Outport';
end
if ~isequal(newblocktype,oldblocktype)
  position = get_param(block, 'Position');
  orientation = get_param(block, 'Orientation');
  delete_block(block)
  add_block(['built-in/' newblocktype],block,'Position',position,'Orientation',orientation);
end
% Set correct input and output port numbers
if ninputsoutputs(1) > 0 
  nip = nip + 1;
  set_param([cb '/A//D'],'Port',num2str(nip))
end
if ninputsoutputs(2) > 0 
  nop = nop + 1;
  set_param([cb '/D//A'],'Port',num2str(nop))
end

% Check if number of external triggers has changed
ntriggers = mask_values{4};
if ~isequal(size(ntriggers),[1 1]) || ~isequal(abs(round(ntriggers)),ntriggers)
  errordlg('Triggers: wrong format')
  return
end
% trigger port changed?
block = [cb '/Triggers'];
oldblocktype = get_param(block, 'BlockType');
mask_visibilities = get_param(cb, 'MaskVisibilities');
if ntriggers == 0
  newblocktype = 'Ground';
  mask_visibilities{5} = 'off';
else
  newblocktype = 'Inport';
  mask_visibilities{5} = 'on';
end
set_param(cb, 'MaskVisibilities', mask_visibilities)
if ~isequal(newblocktype,oldblocktype)
  position = get_param(block, 'Position');
  orientation = get_param(block, 'Orientation');
  delete_block(block)
  add_block(['built-in/' newblocktype],block,'Position',position,'Orientation',orientation);
end
% Set correct input port numbers
if ntriggers > 0 
  nip = nip + 1;
  set_param([cb '/Triggers'],'Port',num2str(nip))
end

% Check if network node number(s) has changed
nwnodenbr = mask_values{6};
if isempty(nwnodenbr)
  nwnodenbr = zeros(0,2);
elseif ~isempty(find(nwnodenbr==0, 1))
  errordlg('Network and node numbers: wrong format')
  return
end
if size(nwnodenbr) == [1 1]
  nwnodenbr = [1 nwnodenbr]; % default network is 1
end
if ~isequal(size(nwnodenbr,2), 2) || ~isequal(abs(round(nwnodenbr)),nwnodenbr)
  errordlg('Network and node numbers: wrong format')
  return
end  
% Remove all "From" blocks and associated lines
Nold = str2double(get_param([cb '/RcvMux'],'Inputs'));
for k = 1:Nold
  delete_line(cb, ['From' num2str(k) '/1'], ['RcvMux/' num2str(k)]);
  delete_block([cb '/From' num2str(k)]);
end
N = size(nwnodenbr,1);
% Set new Mux size
set_param([cb '/RcvMux'],'Inputs',num2str(max([1 N])));
% If no networks, add a ground input
if N==0 
  name = [cb '/From1'];
  position = [10 20 70 20+18];
  add_block('built-in/Ground',name,'Position',position,'ShowName','off');
  add_line(cb,'From1/1','RcvMux/1');
end
% Add new "From" blocks and associated lines
for k = 1:N
  network = num2str(nwnodenbr(k,1));
  node = num2str(nwnodenbr(k,2));
  name = [cb '/From' num2str(k)];
  gototag = ['rcv' network '_' node];
  position = [10 k*20 70 k*20+18];
  add_block('built-in/From',name,'Position',position,'GotoTag',gototag,'ShowName','off');
  add_line(cb,['From' num2str(k) '/1'],['RcvMux/' num2str(k)]);
end
% Remove all "Goto" blocks and associated lines
Nold = str2double(get_param([cb '/SndDemux'],'Outputs'));
for k = 1:Nold
  delete_line(cb, ['SndDemux/' num2str(k)], ['Goto' num2str(k) '/1']);
  delete_block([cb '/Goto' num2str(k)]);
end
% Set new Demux size
set_param([cb '/SndDemux'],'Outputs',num2str(max([1 N])));
% If no networks, add a terminator output
if N==0 
  name = [cb '/Goto1'];
  position = [710 20 770 20+18];
  add_block('built-in/Terminator',name,'Position',position,'ShowName','off');
  add_line(cb,'SndDemux/1','Goto1/1');
end
% Add new "Goto" blocks and associated lines
for k = 1:N
  network = num2str(nwnodenbr(k,1));
  node = num2str(nwnodenbr(k,2));
  name = [cb '/Goto' num2str(k)];
  gototag = ['snd' network '_' node];
  position = [710 k*20 770 k*20+18];
  add_block('built-in/Goto',name,'Position',position,'GotoTag',gototag,'ShowName','off','TagVisibility','global');
  add_line(cb,['SndDemux/' num2str(k)],['Goto' num2str(k) '/1']);
end
% Change block label
mylabel = [];
for k=1:size(nwnodenbr,1)
  if k>1, mylabel = [mylabel '\n']; end
  mylabel = [mylabel num2str(nwnodenbr(k,1)) ': ' num2str(nwnodenbr(k,2))];
end
if isempty(mylabel)
  mylabel = '';
end
set_param(cb,'label',mylabel)

% Indices, names and types of optional ports
iixx = [8 9 10];
xxnames = {'Schedule','Energy','Power'};
onblocks = {'Outport','Inport','Outport'};
offblocks = {'Terminator','Constant','Terminator'};
for ix = iixx
  i = find(ix==iixx);
  block = [cb '/' xxnames{i}];
  oldblocktype = get_param(block, 'BlockType');
  position = get_param(block, 'Position');
  orientation = get_param(block, 'Orientation');
  if isequal(mask_values{ix}, 1)
    newblocktype = onblocks{i};
  else 
    newblocktype = offblocks{i};
  end
  if ~isequal(newblocktype,oldblocktype)
    delete_block(block)
    add_block(['built-in/' newblocktype], block, 'Position', position, 'Orientation', orientation)
  end
  % Set correct port numbers
  if isequal(mask_values{ix}, 1)
    if strcmp(onblocks{i},'Inport')
      nip = nip + 1;
      set_param(block,'Port',num2str(nip))
    elseif strcmp(onblocks{i},'Outport')
      nop = nop + 1;
      set_param(block,'Port',num2str(nop))
    end
  end
end
