function updateEntry(entry, tableID)

global routing_table

table = routing_table{tableID};

l = length(table);

for k = 1:l
  
  if (table{k}.dest==entry.dest)
    table{k} = entry;

    routing_table{tableID} = table;
    updateExpiryTimer(tableID);
    break;
  end
end




