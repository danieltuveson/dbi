01 print "*********************************************************************************************"
02 print "** NOTE: this script uses python's cgi library and requires python 3.12 or earlier to work **"
03 print "*********************************************************************************************"
10 system "sudo mkdir examples/cgi-bin" : print "making cgi-bin"
15 system "sudo cp dbi examples/cgi-bin/dbi" : print "copying script"
20 system "sudo cp examples/cgi.bas examples/cgi-bin/cgi.bas" : print "setting permissions"
30 system "sudo chmod -R 777 examples/cgi-bin" : print "running python http server"
40 system "sudo python3 -m http.server 8080 --cgi --directory examples"
50 system "sudo rm -rf examples/cgi-bin" : print "finished"
60 end

01 PRINT "This doesn't actually work for reasons that I am too stupid to figure out"
02 end
