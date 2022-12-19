function mote_callback(cb,nodeID)

set_param([cb,'/Goto_x'],'GotoTag',['x',num2str(nodeID)]);
set_param([cb,'/Goto_y'],'GotoTag',['y',num2str(nodeID)]);
