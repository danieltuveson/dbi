010 rem Intro
010 if s <> 0 then goto s
020 system "clear" : big : print "<?>" : big : sleep 1 : system "clear"
030 print "If you enter an invalid value and get booted from the game, type 'run'"
040 print "and it should resume roughly where you left off." : sleep 5 : system "clear"
050 let s = 10 : goto s

010 system "clear" : if s = 0 then let s = 100 : if s <> 0 then gosub s
020 system "clear" 
030 if s = x then print "Invalid input"
040 let x = s : gosub s 
050 goto 020

100 rem Home
110 print "You are in a small rowhouse"
120 print "1: look around"
121 print "2: leave the house" : input i
130 if i = 1 then let s = 200
140 if i = 2 then let s = 300
150 return

200 rem Look
200 system "clear"
210 print "You walk around the house."
211 print "There's a phone on the nightstand in the bedroom."
212 print "There is a gun on the dining room table."
213 print "There is an orb in the bathtub - the orb is emitting a faint blue glow."
214 print "There is a somewhat small yellow dog following you around."
215 print "" : print "What do you do?"
220 print "1: take the phone"
221 print "2: take the gun"
222 print "3: take the orb"
225 print "4: ponder the mysteries of the orb"
226 print "5: pet the dog" : input i
231 if i < 1 then return : if i > 5 then return
232 if i >= 1 then if i <= 3 then let x = i
233 if i = 4 then goto 240 : if i = 5 then goto 250
234 let s = 300 : return
240 sleep 1 : print "hmm..." : sleep 2 : print "yes..." : sleep 1 : print "I see..." 
241 sleep 1 : print "" : print "What next?"
242 goto 220
250 print "The dog is pleased." : sleep 1 : print ""
251 print "What do you do now?"
252 goto 220

300 rem On your way out...
300 system "clear" : let s = 300
310 print "On your way out, you notice you are followed by a somewhat small yellow dog"
320 print "1: leash that boy up and take him for a walk" 
321 print "2: leave him in the house" 
322 print "3: let him run out" : input i 
330 if i = 1 then goto 360
340 if i = 2 then goto 370
350 if i = 3 then goto 380 : print "Invalid input" : goto 310
360 let d = 1 : print "You have a new friend" : goto 390
370 let d = 2 : print "'Goodbye dog!' .. he looks sad that you are leaving" : goto 390
380 let d = 3 : print "He sprints through the door and is out of sight before you can even think to chase after him" : goto 390
390 goto 400

400 rem Outside
400 system "clear" : let s = 40 : print "You're standing outside of the house you were just in"

9990 rem End
9991 system "clear" : print "The End. Type 'run' to start over" : let s = 0
9999 end

9900 rem Template Code
9900 rem let s = 9901
9901 rem let s = x : print "something"
9902 rem print "1 : xxxx", print "2: xxxx", print "3: xxxx" : input i : print ""
9903 rem if i = 1 then goto xxxx
9904 rem if i = 2 then goto xxxx
9905 rem if i = 3 then goto xxxx : goto
9906 rem Template Code
9907 rem Template Code
9908 rem Template Code
9909 rem Template Code

