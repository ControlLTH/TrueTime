function entry = findInCache(src, RREQID, cache_vect)

entry = [];

for k = 1:length(cache_vect)

  if (cache_vect{k}.src==src & cache_vect{k}.RREQID==RREQID)
    entry = cache_vect(k);
    break;
  end

end

