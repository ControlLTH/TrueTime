function soccer_callback(cb,N)

muxblocks = {'xMux', 'yMux'};
fromblocks={'x', 'y'};

for j=1:length(muxblocks),
   Nold=str2num(get_param([cb '/' muxblocks{j}], 'Inputs'));
    for k=1:Nold
      to_name=[muxblocks{j} '/' num2str(k)];
      from_name=[fromblocks{j} num2str(k) '/1'];
      delete_line(cb, from_name, to_name);
      delete_block([cb '/' fromblocks{j} num2str(k)]);
    end
  set_param([cb '/' muxblocks{j}], 'Inputs', num2str(N));
end

for k=1:N
    name=[cb '/x' num2str(k)];
    add_block('built-in/From',name,'Position',[60 30+(k-1)*20 90 30+k*20],'GotoTag',['x' num2str(k)]);
    add_line(cb,['x' num2str(k) '/1'],['xMux/' num2str(k)]);
    name=[cb '/y' num2str(k)];
    add_block('built-in/From',name,'Position',[125 130+(k-1)*20 155 130+k*20],'GotoTag',['y' num2str(k)]);
    add_line(cb,['y' num2str(k) '/1'],['yMux/' num2str(k)]);
end

