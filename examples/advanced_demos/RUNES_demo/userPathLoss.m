function [power]=userPathLoss(transmitPower,node1, x1, y1, node2, x2, y2, time)

distance = sqrt((x1 - x2)^2 + (y1 - y2)^2);
power = transmitPower/(distance+1)^3.5;
power = power * dipPathLoss(max(x2-x1,x1-x2),max(y2-y1,y1-y2), 30, 0.1);

