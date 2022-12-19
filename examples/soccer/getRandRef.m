function ref = getRandRef(node)

switch (mod(node,5)),
 case 2,
  ref(1) = 19*rand(1)-19;
  ref(2) = 26*rand(1);
 case 3,
  ref(1) = 19*rand(1);
  ref(2) = 26*rand(1);
 case 4,
  ref(1) = 19*rand(1)-19;
  ref(2) = 26*rand(1)-26;
 case 0,
  ref(1) = 19*rand(1);
  ref(2) = 26*rand(1)-26;
end