make
cat test_proj.p  | ./server > test.c
make clean
gcc test.c
./a.out
