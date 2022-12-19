clear all; close all
ccd = pwd;
try
  disp('testing examples/simple/matlab...')
  cd simple/matlab
  open_system('simple')
  sim('simple',0.1)
  close_system('simple')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/simple/c++...')
  cd simple/c++
  open_system('simple')
  sim('simple',0.1)
  close_system('simple')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/servo/matlab...')
  cd servo/matlab
  open_system('servo')
  sim('servo',0.1)
  close_system('servo')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/servo/c++...')
  cd servo/c++
  open_system('servo')
  sim('servo',0.1)
  close_system('servo')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/threeservos/matlab...')
  cd threeservos/matlab
  open_system('threeservos')
  sim('threeservos',0.1)
  close_system('threeservos')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/threeservos/c++...')
  cd threeservos/c++
  open_system('threeservos')
  sim('threeservos',0.1)
  close_system('threeservos')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/networked/matlab...')
  cd networked/matlab
  open_system('networked')
  sim('networked',0.1)
  close_system('networked')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/networked/c++...')
  cd networked/c++
  open_system('networked')
  sim('networked',0.1)
  close_system('networked')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/networked/standalone...')
  cd networked/standalone
  open_system('networked')
  sim('networked',0.1)
  close_system('networked')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)


clear all; close all
ccd = pwd;
try
  disp('testing examples/gateway...')
  cd gateway
  open_system('gateway')
  sim('gateway',0.1)
  close_system('gateway')
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/AODV...')
  cd AODV
  open_system('AODV')
  sim('AODV',0.1)
  close_system('AODV')
  disp('OK')
  close all
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/wireless...')
  cd wireless
  open_system('wireless')
  sim('wireless',0.1)
  close_system('wireless')
  close all
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all
ccd = pwd;
try
  disp('testing examples/soccer...')
  cd soccer
  open_system('soccer')
  sim('soccer',0.1)
  close_system('soccer')
  close all
  disp('OK')
catch
  disp('Failed!')
end
cd(ccd)

clear all; close all

