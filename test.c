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
	mem[18] = 1;
	mem[HP] = mem[HP] - 2;
	mem[14] = mem[HP];
	mem[mem[14]+1] = mem[18];
	mem[15] = mem[14];
	mem[17] = 200;
	mem[18] = mem[18] +1;
	goto bb1;
  bb1:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[19] <10;
	if(mem[mem[FP] + 1] == 1) goto bb2;
	mem[SP] = mem[SP] - 1;
	goto bb6;
  bb2:
	mem[SP] = mem[SP] - 1;
	mem[HP] = mem[HP] - 2;
	mem[mem[15]] = mem[HP];
	mem[mem[mem[15]]+1] = mem[18];
	mem[15] = mem[mem[15]];
	mem[19] = mem[19] +1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[18] %2;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 2] = mem[mem[FP] + 1] ==0;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 3] = mem[mem[FP] + 2];
	if(mem[mem[FP] + 3] == 1) goto bb3;
	mem[SP] = mem[SP] - 3;
	goto bb4;
  bb3:
	mem[SP] = mem[SP] - 3;
	mem[18] = mem[18] +1;
	goto bb5;
  bb5:
	goto bb1;
  bb4:
	mem[18] = mem[18] *2;
	goto bb5;
  bb6:
	mem[13] = mem[14];
	goto bb7;
  bb7:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[13] !=0;
	if(mem[mem[FP] + 1] == 1) goto bb8;
	mem[SP] = mem[SP] - 1;
	goto bb15;
  bb8:
	mem[SP] = mem[SP] - 1;
	printf("%d\n", mem[mem[13]+1]);
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[mem[13]+1] ==63;
	if(mem[mem[FP] + 1] == 1) goto bb9;
	mem[SP] = mem[SP] - 1;
	goto bb13;
  bb9:
	mem[SP] = mem[SP] - 1;
	printf("%s\n", "True");
	goto bb10;
  bb10:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[14] !=0;
	if(mem[mem[FP] + 1] == 1) goto bb11;
	mem[SP] = mem[SP] - 1;
	goto bb12;
  bb11:
	mem[SP] = mem[SP] - 1;
	printf("%d\n", mem[mem[14]+1]);
	mem[14] = mem[mem[14]];
	goto bb10;
  bb12:
	goto bb14;
  bb14:
	mem[13] = mem[mem[13]];
	goto bb7;
  bb13:
	mem[18] = 1;
	goto bb14;
  bb15:
	goto _ra_1;
  _ra_1: return 0;
}
