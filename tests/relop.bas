010 if 1 <  0 then goto 110
020 if 0 >  1 then goto 120
030 if 1 <= 0 then goto 130
040 if 0 >= 1 then goto 140
050 if 1 <> 1 then goto 150
060 if 1 >< 1 then goto 160
070 if 1 =  0 then goto 170
080 print "relop test: passed" : end

110 print "relop test failed on 1 < 0" : end 
120 print "relop test failed on 0 > 1"  : end 
130 print "relop test failed on 1 <= 0" : end 
140 print "relop test failed on 0 >= 1" : end 
150 print "relop test failed on 1 <> 1" : end 
160 print "relop test failed on 1 >< 1" : end 
170 print "relop test failed on 1 = 0" : end 

