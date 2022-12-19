function eventfun(k)

blk = [bdroot '/node' num2str(k) '/Events'];
val = str2num(get_param(blk,'Value'));
set_param(blk,'Value',num2str(-val));
