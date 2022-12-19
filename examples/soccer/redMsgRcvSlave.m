function [exectime, data] = redMsgRcvSlave(seg, data)

msg = ttGetMsg;
ttTryPost('packets', msg);
ttNotify('nodePacket');
exectime = -1;
