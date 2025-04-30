010 system "valgrind ./dbi 'tests/expr.bas'"
020 system "valgrind ./dbi 'tests/relop.bas'"
030 system "echo '1 + 2, 3 * 4, 5 - 6' | valgrind ./dbi 'tests/input.bas'"
040 system "valgrind ./dbi 'tests/let.bas'"
050 system "valgrind ./dbi 'tests/gosub-return.bas'"

110 system "valgrind ./dbi -c 'tests/expr.bas' && echo 'passed'"
120 system "valgrind ./dbi -c 'tests/relop.bas' && echo 'passed'"
130 rem system "echo '1 + 2, 3 * 4, 5 - 6' | valgrind ./dbi 'tests/input.bas'"
140 system "valgrind ./dbi -c 'tests/let.bas' && echo 'passed'"
150 system "valgrind ./dbi -c 'tests/gosub-return.bas' && echo 'passed'"

999 end
