
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
	mem[FP] = 38;
	mem[SP] = 38; //start of sp after global vars are placed
  bb0:
	mem[HP] = mem[HP] - 20;
	mem[13] = mem[HP];
	mem[18] = 0;
	goto bb1;
  bb1:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[18] <20;
	if(mem[mem[FP] + 1] == 1) goto bb2;
	mem[SP] = mem[SP] - 1;
	goto bb3;
  bb2:
	mem[SP] = mem[SP] - 1;
	mem[HP] = mem[HP] - 1;
	mem[mem[13] + mem[18]] = mem[HP];
	mem[18] = mem[18] +1;
	goto bb1;
  bb3:
	mem[17] = 0;
	goto bb4;
  bb4:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[17] <=40;
	if(mem[mem[FP] + 1] == 1) goto bb5;
	mem[SP] = mem[SP] - 1;
	goto bb7;
  bb5:
	mem[SP] = mem[SP] - 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 6;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 17; //param xx
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 3;
	mem[FP] = mem[SP];
	/* end of stack setup for call to fib*/
	/* has label bb13*/
  bb13:
	/*function var section for 3with fib */
/* var  fb2 */
	mem[SP]++;
	mem[mem[SP]] = 0;
/* var  fb1 */
	mem[SP]++;
	mem[mem[SP]] = 0;
/* var  return */
	mem[SP]++;
	mem[mem[SP]] = 0;
	/* end of func var init */
  bb14:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 4] = mem[mem[mem[FP] - 1]] <0;
	if(mem[mem[FP] + 4] == 1) goto bb15;
	mem[SP] = mem[SP] - 1;
	goto bb16;
  bb15:
	mem[SP] = mem[SP] - 1;
	mem[mem[FP] + 3] = -1;
	goto bb25;
  bb25:
	mem[mem[mem[FP] - 1]] = mem[mem[mem[FP] - 1]] +1;
	mem[R1] = mem[mem[FP] + 3];
	/******* ra if stats *******/
	if (mem[mem[FP] - 2] == 0) goto bb0;
	if (mem[mem[FP] - 2] == 1) goto bb1;
	if (mem[mem[FP] - 2] == 2) goto bb2;
	if (mem[mem[FP] - 2] == 3) goto bb3;
	if (mem[mem[FP] - 2] == 4) goto bb4;
	if (mem[mem[FP] - 2] == 5) goto bb5;
	if (mem[mem[FP] - 2] == 6) goto bb6;
	if (mem[mem[FP] - 2] == 7) goto bb7;
	if (mem[mem[FP] - 2] == 13) goto bb13;
	if (mem[mem[FP] - 2] == 14) goto bb14;
	if (mem[mem[FP] - 2] == 15) goto bb15;
	if (mem[mem[FP] - 2] == 16) goto bb16;
	if (mem[mem[FP] - 2] == 17) goto bb17;
	if (mem[mem[FP] - 2] == 18) goto bb18;
	if (mem[mem[FP] - 2] == 19) goto bb19;
	if (mem[mem[FP] - 2] == 20) goto bb20;
	if (mem[mem[FP] - 2] == 21) goto bb21;
	if (mem[mem[FP] - 2] == 22) goto bb22;
	if (mem[mem[FP] - 2] == 23) goto bb23;
	if (mem[mem[FP] - 2] == 24) goto bb24;
	if (mem[mem[FP] - 2] == 25) goto bb25;
	/****************************/
  bb16:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 4] = mem[mem[mem[FP] - 1]] ==0;
	if(mem[mem[FP] + 4] == 1) goto bb17;
	mem[SP] = mem[SP] - 1;
	goto bb18;
  bb17:
	mem[SP] = mem[SP] - 1;
	mem[mem[FP] + 3] = 0;
	goto bb24;
  bb24:
	goto bb25;
  bb18:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 4] = mem[mem[mem[FP] - 1]] ==1;
	if(mem[mem[FP] + 4] == 1) goto bb19;
	mem[SP] = mem[SP] - 1;
	goto bb20;
  bb19:
	mem[SP] = mem[SP] - 1;
	mem[mem[FP] + 3] = 1;
	goto bb23;
  bb23:
	goto bb24;
  bb20:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 4] = mem[mem[mem[FP] - 1]] -1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 21;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP] + 4; //param xx
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 3;
	mem[FP] = mem[SP];
	/* end of stack setup for call to fib*/
	/* has label bb13*/
	goto bb13;
  bb21:
	mem[R2] = mem[FP]; //store size val for SP
 	mem[FP] = mem[mem[FP] - mem[mem[FP]]];
	mem[SP] = mem[R2] - mem[mem[R2]] - 1;
	mem[SP] = mem[SP] - 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 4] = mem[R1]; /*get return val*/
	mem[mem[FP] + 2] = mem[mem[FP] + 4];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 5] = mem[mem[mem[FP] - 1]] -2;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 22;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP] + 5; //param xx
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 3;
	mem[FP] = mem[SP];
	/* end of stack setup for call to fib*/
	/* has label bb13*/
	goto bb13;
  bb22:
	mem[R2] = mem[FP]; //store size val for SP
 	mem[FP] = mem[mem[FP] - mem[mem[FP]]];
	mem[SP] = mem[R2] - mem[mem[R2]] - 1;
	mem[SP] = mem[SP] - 2;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 4] = mem[R1]; /*get return val*/
	mem[mem[FP] + 1] = mem[mem[FP] + 4];
	mem[mem[FP] + 3] = mem[mem[FP] + 2] +mem[mem[FP] + 1];
	mem[SP] = mem[SP] - 1;
	goto bb23;
	mem[SP] = mem[FP]; //reset stack for func fib
  bb6:
	mem[R2] = mem[FP]; //store size val for SP
 	mem[FP] = mem[mem[FP] - mem[mem[FP]]];
	mem[SP] = mem[R2] - mem[mem[R2]] - 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[R1]; /*get return val*/
	mem[18] = mem[mem[FP] + 1];
	printf("%d\n", mem[18]);
	mem[SP] = mem[SP] - 1;
	goto bb4;
  bb7:
	goto _ra_1;
  _ra_1: return 0;
}
