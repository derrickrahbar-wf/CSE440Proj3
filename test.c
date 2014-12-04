
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
void *label[3];

int main() {
	mem[HP] = MEM_MAX-1;
	label[0] = &&bb0;
	label[1] = &&bb1;
	label[2] = &&bb2;
	/* end of static initial setup */
	mem[12] = 0;
	mem[13] = 0;
	mem[14] = 0;
	mem[15] = 0;
	mem[16] = 0;
	mem[17] = 0;
	mem[FP] = 36;
	mem[SP] = 36; //start of sp after global vars are placed
  bb0:
	mem[HP] = mem[HP] - 1;
	mem[12] = mem[HP];
	mem[HP] = mem[HP] - 1;
	mem[18] = mem[HP];
	mem[16] = 17;
	mem[23] = 34;
	mem[13] = 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[FP];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[12];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 5;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[23];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 2;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = mem[13];
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[SP]] = 7;
	mem[FP] = mem[SP];
	/* end of stack setup for call to happy*/
	goto bb2;
  bb2:
	/* FUNCTION: happy */
	/*function var section for 5 */
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[SP] = mem[SP] + 20;
	/* end of func var init */
  bb3:
	mem[mem[FP] + 4] = mem[mem[FP] - 3];
	printf("%d\n", mem[mem[FP] - 4]);
	printf("%d\n", mem[mem[FP] - 3]);
	printf("%d\n", mem[mem[FP] - 2]);
	if(mem[mem[FP] - 1] == 1) goto t0;
	printf("False\n");
	goto t1;
  t0:
	printf("True\n");
  t1:
	mem[SP] = mem[FP]; //reset stack
	goto *label[mem[mem[FP] - 6]];
  bb1:
	mem[FP] = mem[mem[SP] - mem[mem[SP]]];
	mem[SP] = mem[SP] - mem[mem[SP]] - 1;
	mem[SP]++;
	mem[mem[SP]] = 0;
	mem[mem[FP] + 1] = mem[R1]; /*get return val*/
	mem[16] = mem[mem[FP] + 1];
	mem[SP] = mem[SP] - 1;
	goto _ra_1;
  _ra_1: return 0;
}
