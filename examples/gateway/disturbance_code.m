function [exectime, data] = disturbance_code(seg,data)

networkNbr = floor(rand*3) + 1;  % select a random network (1-3)
msg = [];                        % empty message
priority = 0;                    % highest priority

ttSendMsg([networkNbr 3], msg, 1250, 0);  % send high prio msg to itself

exectime = 0.01+0.03*rand;
ttSetNextSegment(1);
