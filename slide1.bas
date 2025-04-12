7777 REM use this to open browser from BASIC
7778 REM /mnt/c/Program Files (x86)/Google/Chrome/Application/chrome.exe 

01 REM Start here
01 let q = 2 : REM quick sleep number
02 let s = 4 : REM regular sleep number
03 let l = 9 : REM long sleep number
05 if f = 0 then goto 09
06 if f = 1 then goto 100
09 goto 9997

09 REM SLIDE 0 - Academia
09 system "sudo echo 'test'"
10 system "sudo clear" : big : print "A History", "of", "BASIC" : big : sleep l : system "sudo clear"
11 print "- Beginners' All-purpose Symbolic Instruction Code" : sleep s
20 print "- Created by some guys at Dartmouth in the 60s" : sleep s
30 print "- Hungarian math guy John G Kemeny" : sleep q
35 print "- Mustache guy computer dork Thomas E Kurtz" : sleep q
36 print "https://en.wikipedia.org/wiki/Thomas_E._Kurtz#/media/File:Thomas_E._Kurtz.gif" 
37 system "sudo /mnt/c/Program Files (x86)/Google/Chrome/Application/chrome.exe 'https://en.wikipedia.org/wiki/Thomas_E._Kurtz#/media/File:Thomas_E._Kurtz.gif'" 
38 sleep l 
40 print "- Students + timesharing = BASIC" : sleep s
41 print "- Became popular in industry: adopted by DEC, Data General, IBM" : sleep s
50 print "- A programming language for stupid people" : sleep q
60 print "  (see quotation)"
70 let f = 1 : goto 9999

100 REM SLIDE 1 - Bill Muthafuckin Gates
110 system "sudo clear" : big : print "Bill","Muthafuckin","Gates" : big : sleep l : system "sudo clear"
120 print "- Bill Gates and his bros create their own version of BASIC for the Altair 880" : sleep s
130 print "- 'Micro-soft'"
199 let f = 2 : goto 9999

200 REM SLIDE 2 - Hobbyists vs Bill Muthafuckin Gates
210 system "sudo clear" : big : print "A History", "of" : big : print "tiny" : big : print "BASIC" : big : system "sudo clear"
211 print "- Homebrew Computer Club (some famous guys there)" : sleep s
212 print "- Everyone loves using Altair BASIC" : sleep s
218 print "- Fuck this Bill Gates asshole" : sleep s
219 print "- We'll make our own BASIC" : sleep s : print "- with blackjack" : sleep s : print "- and hookers" : sleep s
220 print "- Spec created by Standford lecturer / weird hippie Dennis Allison" : sleep s
230 print "- 'People's computing company'" : sleep s
240 print "- " : sleep s
250 print "- These guys love newsletters" : sleep s
260 print "https://upload.wikimedia.org/wikipedia/en/3/39/DrDobbs_first.png"
270 system "/mnt/c/Program Files (x86)/Google/Chrome/Application/chrome.exe 'https://upload.wikimedia.org/wikipedia/en/3/39/DrDobbs_first.png'"
199 let f = 3 : goto 9999

9990 REM LAST SLIDE
9991 print "/mnt/c/Program Files (x86)/Google/Chrome/Application/chrome.exe 'https://www.youtube.com/watch?v=LNAwUxZ5nfw'"
9992 system "sudo open 'https://www.youtube.com/watch?v=LNAwUxZ5nfw'" : sleep 110
9997 big : print "FUCK" : print "BILL" : print "GATES" : big
9998 print "He still sucks"

9999 REM Jump here to return to console without exiting

