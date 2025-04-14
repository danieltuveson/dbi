010 REM fibonacci - set n before calling
020 let x = 0 : rem fib(n - 2)
030 let y = 1 : rem fib(n - 1)
040 let r = 1 : rem fib(n) (return value)
050 if n = 0 then let r = 0
055 if n = 0 then return
060 if n = 1 then return
070 let i = 1
080 let i = i + 1 : rem Top of loop
090 if n = i then print "fib(", n, ") = ", r
100 if n = i then return
110 let r = x + y
120 let x = y
130 let y = r
140 goto 80

print "***********************************************"
print "** Set n and run to calculate fibonacci of n **"
print "***********************************************"

