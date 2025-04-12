10   REM Program entrypoint
20   gosub 110
100  goto 9999

110  REM This routine prints a countdown
120  big
130  let x = 10
140  REM Top of loop
150  print x
160  let x = x - 1
170  sleep 2:beep
180  if x > 0 then goto 140
190  print "BASIC - a programming language"
200  big
210  return

9999 REM Jump here to return to console without exiting
