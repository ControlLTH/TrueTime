function f=dipPathLoss(x,y,d,alpha)

f=max([mod(x,d)*(d-mod(x,d)) mod(y,d)*(d-mod(y,d))])/(d/2)^2;

