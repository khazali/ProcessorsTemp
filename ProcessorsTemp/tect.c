#include "Temps.h"

int main (void) {
	InitADL ();
	InitIPG();

	
	printf("GPU Temperature = %f\n", GetGPUTemp());
	printf("CPU Temperature = %f\n", GetCPUTemp());
	
	FinalizeGPU();
	FinalizeCPU();

	return 0;
}