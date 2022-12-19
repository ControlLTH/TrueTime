function entry = findEntry(dest, tableID)

global routing_table

table = routing_table{tableID};

entry = [];

l = length(table);

for k = 1:l
  
  if (table{k}.dest==dest)
    entry = table{k};
    break;
  end

end