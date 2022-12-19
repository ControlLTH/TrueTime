function [exectime, data] = dl_miss_code(seg, data)

task = sscanf(ttGetInvoker,'DLtimer:%s');
if ~isempty(task)
  ttKillJob(task)
end

exectime = -1;
