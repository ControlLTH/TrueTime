%

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

vstr = version;
v = sscanf(vstr,'%f',1);
if v < 7.0
  error('TrueTime requires Matlab 7.0 (R14) or later to run')
end

kerneldir = getenv('TTKERNEL');
if isempty(kerneldir)
  error('Environment variable TTKERNEL not defined - please set this variable first.')
end

ccd = pwd;

try 

  cd(kerneldir)
  pwd

  disp('Compiling TrueTime Kernel block...');
  mex -g ttkernelMATLAB.cpp             % Compile kernel block
  disp('...done.')
  disp('Compiling TrueTime Network block...');
  mex -g ttnetwork.cpp                  % Compile network block
  disp('...done.')
  disp('Compiling TrueTime Wireless Network block...');
  mex -g ttwnetwork.cpp                 % Compile Wireless Network block
  disp('...done.')
  disp('Compiling TrueTime Ultrasound Network block...');
  mex -g ttusnetwork.cpp    % Compile Ultrasound Network block
  disp('...done.')
  disp('Compiling TrueTime Send block...');
  mex -g ttsend.cpp         % Compile Send block
  disp('...done.')
  disp('Compiling TrueTime Receive block...');
  mex -g ttreceive.cpp      % Compile Receive block
  disp('...done.')
  
  disp('Compiling TrueTime NCM network block...');
  mex -g ttNCM.cpp
  disp('...done.')
  
  cd matlab
  
  disp('Compiling TrueTime MEX-functions...');
  disp('');
  
  compileall % Will compile all TrueTime MEX-functions
  
  disp('Compiling C++ examples...');
  disp('');

  cd(kerneldir)
  cd ../examples/simple/c++
  disp('simple/c++/simple_init.cpp');
  ttmex simple_init.cpp

  cd(kerneldir)
  cd ../examples/servo/c++
  disp('servo/c++/servo_init.cpp');
  ttmex servo_init.cpp

  cd(kerneldir)
  cd ../examples/threeservos/c++
  disp('threeservos/threeservos_init.cpp');
  ttmex threeservos_init.cpp
  
  cd(kerneldir)
  cd ../examples/networked/c++
  disp('networked/c++/interference_init.cpp');
  ttmex interference_init.cpp
  disp('networked/c++/sensor_actuator_init.cpp');
  ttmex sensor_actuator_init.cpp
  disp('networked/c++/controller_init.cpp');
  ttmex controller_init.cpp

  disp('');
  disp('TrueTime compiled successfully!')
  
catch
  
  disp('Compilation using MEX failed! (Run ''mex -setup C++''')
  disp('to configure your C++ compiler)')

end
  
cd(ccd)
