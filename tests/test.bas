010 system "valgrind ./dbi 'tests/expr.bas'"
020 system "valgrind ./dbi 'tests/relop.bas'"
030 system "echo '1 + 2, 3 * 4, 5 - 6' | valgrind ./dbi 'tests/input.bas'"
040 system "valgrind ./dbi 'tests/let.bas'"
050 system "valgrind ./dbi 'tests/gosub-return.bas'"
999 end
