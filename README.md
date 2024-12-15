# jit compiled regex-parser (like grep)


The program creates a dfa from the regex and then jit compiles it to a64 machine code
no real optimisations are done on the state machine after converting to dfa so the generated code is in no way optimal.

# Result
test_input.c is 5.6G made by duplicating src/aarch64/aarch64_ins.h a bunch of times
```bash
$ time ./bin/Darwin/release/target -c '= ((-[0-9]+)|([1-9][0-9]*)|(0((x[0-9a-fA-F]+)|([0-7]))))' test_input.c
62000000

________________________________________________________
Executed in    4.75 secs    fish           external
   usr time    8.82 secs   76.00 micros    8.82 secs
   sys time    6.06 secs  981.00 micros    6.06 secs

$ time grep -c '= ((-[0-9]+)|([1-9][0-9]*)|(0((x[0-9a-fA-F]+)|([0-7]))))' test_input.c -E
62000000

________________________________________________________
Executed in   95.53 secs    fish           external
   usr time   91.85 secs    0.13 millis   91.85 secs
   sys time    2.03 secs    1.88 millis    2.03 secs

$ time rg -c '= ((-[0-9]+)|([1-9][0-9]*)|(0((x[0-9a-fA-F]+)|([0-7]))))' test_input.c
62000000

________________________________________________________
Executed in    4.81 secs    fish           external
   usr time    3.39 secs   73.00 micros    3.39 secs
   sys time    0.75 secs  886.00 micros    0.75 secs
```
these test are ran on my M1 macbook pro with 16 gigs of ram




