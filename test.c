
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
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 4;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 0;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 3;
	mem[FP] = mem[SP];
	/* end of stack setup for call to fib*/
	/* has label bb10*/
	/*function var section for 1with fib */
	mem[SP]++;
	mem[mem[SP]] = 0;
	/* end of func var init */
  bb11:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 2] = mem[mem[FP] - 1] <0;
	if(mem[mem[FP] + 2] == 1) goto bb12;
	mem[SP] = mem[SP] - 1;
	goto bb13;
  bb12:
	mem[SP] = mem[SP] - 1;
	mem[mem[FP] + 1] = 1;
	goto bb22;
  bb22:
	mem[R1] = mem[mem[FP] + 1];
  bb13:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 2] = mem[mem[FP] - 1] ==0;
	if(mem[mem[FP] + 2] == 1) goto bb14;
	mem[SP] = mem[SP] - 1;
	goto bb15;
  bb14:
	mem[SP] = mem[SP] - 1;
	mem[mem[FP] + 1] = 0;
	goto bb21;
  bb21:
	goto bb22;
  bb15:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 2] = mem[mem[FP] - 1] ==1;
	if(mem[mem[FP] + 2] == 1) goto bb16;
	mem[SP] = mem[SP] - 1;
	goto bb17;
  bb16:
	mem[SP] = mem[SP] - 1;
	mem[mem[FP] + 1] = 1;
	goto bb20;
  bb20:
	goto bb21;
  bb17:
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 2] = mem[mem[FP] - 1] -2;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 18;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[mem[FP] + 2];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 3;
	mem[SP] = mem[SP] - 1;
	goto bb10;
  bb18:
	mem[FP] = mem[mem[SP] - mem[mem[SP]]];
	mem[SP] = mem[SP] - mem[mem[SP]] - 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 2] = mem[R1]; /*get return val*/
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 3] = mem[mem[FP] - 1] -1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 19;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[mem[FP] + 3];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 3;
	mem[SP] = mem[SP] - 2;
	goto bb10;
  bb19:
	mem[FP] = mem[mem[SP] - mem[mem[SP]]];
	mem[SP] = mem[SP] - mem[mem[SP]] - 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 2] = mem[R1]; /*get return val*/
	mem[SP]++;
	mem[mem[SP]] = 0;
get_offset_and_class_for_va_id couldnt find var $7
	mem[mem[FP] + 3] = mem[mem[FP] + 2] +mem[];
	mem[mem[FP] + 1] = mem[mem[FP] + 3];
	mem[SP] = mem[SP] - 2;
	goto bb20;
	mem[SP] = mem[FP]; //reset stack for func fib
	/******* ra if stats *******/
	if (mem[mem[FP] - 3] == 0) goto bb0;
	if (mem[mem[FP] - 3] == 1) goto bb1;
	if (mem[mem[FP] - 3] == 2) goto bb2;
	if (mem[mem[FP] - 3] == 3) goto bb3;
	if (mem[mem[FP] - 3] == 4) goto bb4;
	if (mem[mem[FP] - 3] == 11) goto bb11;
	if (mem[mem[FP] - 3] == 12) goto bb12;
	if (mem[mem[FP] - 3] == 13) goto bb13;
	if (mem[mem[FP] - 3] == 14) goto bb14;
	if (mem[mem[FP] - 3] == 15) goto bb15;
	if (mem[mem[FP] - 3] == 16) goto bb16;
	if (mem[mem[FP] - 3] == 17) goto bb17;
	if (mem[mem[FP] - 3] == 18) goto bb18;
	if (mem[mem[FP] - 3] == 19) goto bb19;
	if (mem[mem[FP] - 3] == 20) goto bb20;
	if (mem[mem[FP] - 3] == 21) goto bb21;
	if (mem[mem[FP] - 3] == 22) goto bb22;
	/****************************/
  bb4:
	mem[FP] = mem[mem[SP] - mem[mem[SP]]];
	mem[SP] = mem[SP] - mem[mem[SP]] - 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[R1]; /*get return val*/
	mem[18] = mem[mem[FP] + 1];
	printf("%d\n", mem[18]);
	mem[SP] = mem[SP] - 1;
	goto _ra_1;
  _ra_1: return 0;
}
