make clean
make
cat $1  | ./server > test.c
make clean
gcc test.c
./a.out
