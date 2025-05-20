01 print "give me x and n to calculate x ^ n" : input x, n
02 let r = 1 : let m = n
03 if m = 0 then goto 99
04 let r = r * x
05 let m = m - 1
06 goto 03
99 print x, " ^ ", n, " = ", r : return
