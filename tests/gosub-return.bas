010 let x = 321
020 gosub 100
030 if x = 123 then print "GOSUB-RETURN test: passed"
040 if x <> 123 then print "GOSUB-RETURN test: failed"
050 end

100 let x = 123
110 return

