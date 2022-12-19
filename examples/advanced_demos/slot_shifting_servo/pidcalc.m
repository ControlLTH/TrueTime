function data = pidcalc(data, r, y)

P = data.K*(data.beta*r-y);
I = data.Iold;
D = data.Td/(data.N*data.h+data.Td)*data.Dold+data.N*data.K*data.Td/(data.N*data.h+data.Td)*(data.yold-y); 

data.u = P + I + D;
data.Iold = data.Iold + data.K*data.h/data.Ti*(r-y);
data.Dold = D;
data.yold = y;
