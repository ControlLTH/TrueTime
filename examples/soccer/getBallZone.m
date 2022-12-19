function zone = getBallZone(x,y)

if (x < 0)
  if (y > 0)
    zone = 1;
  else
    zone = 3;
  end
else
  if (y > 0)
    zone = 2;
  else
    zone = 4;
  end
end