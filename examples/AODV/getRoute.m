function route = getRoute(src, dest)

route = src;

found = 0;

while ~found
  entry = findEntry(dest, src);
  
  if (isempty(entry))
    found = 1;
  else
    
    route = [route entry.nextHop];
    
    if (entry.nextHop == dest)
      found = 1;
    else
      src = entry.nextHop;
    end
    
  end

end