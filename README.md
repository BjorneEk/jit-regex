# jit compiled regex-parser (like grep)
#
The program creates a dfa from the regex and then jit compiles it to a64 machine code


# Result1
test_input.c is 5.6G made by duplicating src/aarch64/aarch64_ins.h a bunch of times
no real optimisations are done on the state machine after converting to dfa so the generated code is in no way optimal.
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

# Result2
this is after refactoring and optimizing the jit compilation, however no longer ran on multiple threads.
input is a large file containing lines with 'hej, hej på dej, hejdå', with 2 lines having a digit at the end
```bash
$ time ./bin/Darwin/target 'hej' sample_inputs/hej.in -oc
3628800

________________________________________________________
Executed in   45.19 millis    fish           external
   usr time   32.85 millis   62.00 micros   32.79 millis
   sys time    9.80 millis  979.00 micros    8.82 millis

❯ time grep 'hej' sample_inputs/hej.in -oc
1209600

________________________________________________________
Executed in  813.69 millis    fish           external
   usr time  802.88 millis    0.13 millis  802.75 millis
   sys time    9.10 millis    1.16 millis    7.94 millis

$ time rg 'hej' sample_inputs/hej.in -oc
3628800

________________________________________________________
Executed in  170.25 millis    fish           external
   usr time  141.31 millis   63.00 micros  141.25 millis
   sys time   14.99 millis  949.00 micros   14.04 millis

$ time ./bin/Darwin/target 'hej, hej på dej, hejdå[0-9]' sample_inputs/hej.in -oc
2

________________________________________________________
Executed in   16.45 millis    fish           external
   usr time    6.95 millis   58.00 micros    6.89 millis
   sys time    6.50 millis  721.00 micros    5.78 millis

$ time grep 'hej, hej på dej, hejdå[0-9]' sample_inputs/hej.in -oc
2

________________________________________________________
Executed in  686.90 millis    fish           external
   usr time  670.23 millis    0.15 millis  670.08 millis
   sys time    7.66 millis    1.21 millis    6.45 millis

$ time rg 'hej, hej på dej, hejdå[0-9]' sample_inputs/hej.in -oc
2

________________________________________________________
Executed in  102.03 millis    fish           external
   usr time   89.10 millis   48.00 micros   89.06 millis
   sys time    8.46 millis  767.00 micros    7.69 millis
```



