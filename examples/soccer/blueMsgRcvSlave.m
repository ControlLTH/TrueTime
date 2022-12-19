function [exectime, data] = blueMsgRcvSlave(seg, data)

msg = ttGetMsg;
ttTryPost('packets', msg);
ttNotify('blueNodePacket');
exectime = -1;
