clear functions

rand('state',0);
rand;

global xPos yPos xPos_init yPos_init xBall yBall tellArrive xPos_attac yPos_attac
global takePass noticeBall inPlace Kx Ky xPos_defense yPos_defense konstitfel
global xPos_middle yPos_middle state


for i=1:10
  tellArrive(i) = 0;
  takePass(i) = 0;
  noticeBall(i) = 0;
  inPlace(i)=1;
  konstitfel(i)=1;
  state{i}='Idle';
end

init_red_mote;
init_blue_mote;

xBall = 0.2*rand;
yBall = 0.2*rand;

% Controller gains
Kx = 0.8*ones(1,10);
Ky = 0.8*ones(1,10);
