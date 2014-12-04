#include <stdio.h>
#define MEM_MAX 65536
#define NUM_REG 9
#define GLOBAL_REGION (NUM_REG+3)
#define FP NUM_REG
#define SP (NUM_REG + 1)
#define HP (NUM_REG + 2)
#define R1 0
#define R2 1
#define R3 2
#define R4 3
#define R5 4
#define R6 5
#define R7 6
#define R8 7
#define R9 8
int mem[MEM_MAX]; 

int main() {
	mem[HP] = MEM_MAX-1;
	/* end of static initial setup */

	mem[12] = 0;
	mem[13] = 0;
	mem[14] = 0;
	mem[15] = 0;
	mem[16] = 0;
	mem[17] = 0;
	mem[18] = 0;
	mem[19] = 0;
	mem[FP] = 19;
	mem[SP] = 19; //start of sp after global vars are placed
  bb0:
	mem[19] = 0;
	mem[HP] = mem[HP] - 8;
	mem[13] = mem[HP];
	mem[mem[13]+5] = 15;
	mem[19] = mem[mem[13]+5] +3;
	printf("%d\n", mem[mem[13]+5]);
	mem[HP] = mem[HP] - 10;
	mem[12] = mem[HP];
	mem[mem[12]+7] = 1;
	mem[mem[13]+5] = mem[mem[12]+7];
	mem[16] = 1 |0;
	printf("%d\n", mem[19]);
	if(mem[16] == 1) goto t0;
	printf("False\n");
	goto t1;
  t0:
	printf("True\n");
  t1:
	printf("%d\n", mem[mem[13]+5]);
	mem[HP] = mem[HP] - 2;
	mem[mem[13]] = mem[HP];
	mem[HP] = mem[HP] - 2;
	mem[mem[mem[13]]+1] = mem[HP];
	mem[HP] = mem[HP] - 1;
	mem[mem[mem[mem[13]]+1]+1] = mem[HP];
	mem[mem[mem[mem[mem[13]]+1]+1]] = 5;
	printf("%d\n", mem[mem[mem[mem[mem[13]]+1]+1]]);
	mem[19] = 0;
	goto bb1;
  bb1:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[mem[mem[mem[mem[13]]+1]+1]] >0;
	if(mem[mem[FP] + 1] == 1) goto bb2;
	mem[SP] = mem[SP] - 1;
	goto bb18;
  bb2:
	mem[SP] = mem[SP] - 1;
	mem[mem[mem[mem[mem[13]]+1]+1]] = mem[mem[mem[mem[mem[13]]+1]+1]] -1;
	printf("%d\n", mem[mem[mem[mem[mem[13]]+1]+1]]);
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[19] ==0;
	if(mem[mem[FP] + 1] == 1) goto bb3;
	mem[SP] = mem[SP] - 1;
	goto bb4;
  bb3:
	mem[SP] = mem[SP] - 1;
	printf("%d\n", mem[19]);
	mem[19] = 1;
	goto bb17;
  bb17:
	goto bb1;
  bb4:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[19] ==1;
	if(mem[mem[FP] + 1] == 1) goto bb5;
	mem[SP] = mem[SP] - 1;
	goto bb6;
  bb5:
	mem[SP] = mem[SP] - 1;
	printf("%d\n", mem[19]);
	mem[19] = 2;
	goto bb16;
  bb16:
	goto bb17;
  bb6:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[19] ==2;
	if(mem[mem[FP] + 1] == 1) goto bb7;
	mem[SP] = mem[SP] - 1;
	goto bb8;
  bb7:
	mem[SP] = mem[SP] - 1;
	printf("%d\n", mem[19]);
	mem[19] = 3;
	goto bb15;
  bb15:
	goto bb16;
  bb8:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[19] ==3;
	if(mem[mem[FP] + 1] == 1) goto bb9;
	mem[SP] = mem[SP] - 1;
	goto bb10;
  bb9:
	mem[SP] = mem[SP] - 1;
	printf("%d\n", mem[19]);
	mem[19] = 4;
	goto bb14;
  bb14:
	goto bb15;
  bb10:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[19] ==4;
	if(mem[mem[FP] + 1] == 1) goto bb11;
	mem[SP] = mem[SP] - 1;
	goto bb12;
  bb11:
	mem[SP] = mem[SP] - 1;
	printf("%d\n", mem[19]);
	mem[19] = 5;
	goto bb13;
  bb13:
	goto bb14;
  bb12:
	printf("%s\n", "True");
	goto bb13;
  bb18:
	mem[HP] = mem[HP] - 2;
	mem[mem[12]+9] = mem[HP];
	mem[mem[mem[12]+9]] = 7;
	printf("%s\n", "False");
	printf("%d\n", mem[mem[mem[12]+9]]);
	goto _ra_1;
  _ra_1: return 0;
}
