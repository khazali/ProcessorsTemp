#ifndef TEMPS_H
#define TEMPS_H
#ifndef _WIN32
#include "./ADL_include/adl_sdk.h"
#include <dlfcn.h>	//dyopen, dlsym, dlclose
#include <unistd.h>
#else
#include <windows.h>
#include "ADL_include\adl_sdk.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long int nprocs;
#ifndef _WIN32
int *CPUTempArray;
#endif





// Definitions of the used function pointers. Add more if you use other ADL APIs. Note that that sample will use mixture of legacy ADL and ADL2 APIs.
typedef int (*ADL_MAIN_CONTROL_CREATE )(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int (*ADL_MAIN_CONTROL_DESTROY )();
typedef int (*ADL_OVERDRIVE5_TEMPERATURE_GET) (int, int, ADLTemperature*);

#ifndef _WIN32
typedef int (*ADL_CONSOLEMODE_FILEDESCRIPTOR_SET) (int);
#endif	

#ifdef _WIN32
typedef int (*IPGInitialize) ();
typedef int (*IPGReadSample) ();
typedef int (*IPGGetPowerData) (int iNode, int iMSR, double *result, int *nResult);
typedef int (*IPGGetNumMsrs) (int *nMsr);
typedef int (*IPGGetMsrFunc) (int iMsr, int *funcID);

IPGInitialize pInitialize;
IPGGetPowerData pGetPowerData;
IPGReadSample pReadSample;
IPGGetNumMsrs pGetNumMsrs;
IPGGetMsrFunc pGetMsrFunc;
#endif

#ifndef _WIN32
void *hDLL;					// Handle to .so library
#else
HINSTANCE hDLL, hDLL2;		// Handle to DLL
#endif


ADL_MAIN_CONTROL_CREATE				ADL_Main_Control_Create;
ADL_MAIN_CONTROL_DESTROY			ADL_Main_Control_Destroy;
ADL_OVERDRIVE5_TEMPERATURE_GET		ADL_Overdrive5_Temperature_Get;
#ifndef _WIN32
ADL_CONSOLEMODE_FILEDESCRIPTOR_SET	ADL_ConsoleMode_FileDescriptor_Set;
#endif	

// Memory allocation function
void* __stdcall ADL_Main_Memory_Alloc ( int iSize ){
	void* lpBuffer = malloc ( iSize );
	return lpBuffer;
}

// Optional Memory de-allocation function
void __stdcall ADL_Main_Memory_Free ( void* lpBuffer ){
	if ( NULL != lpBuffer ){
		free ( lpBuffer );
		lpBuffer = NULL;
	}
}

#ifndef _WIN32
#define GetProcAddress dlsym
#endif

int InitADL (void){


#ifndef _WIN32
	hDLL = dlopen( "libatiadlxx.so", RTLD_LAZY|RTLD_GLOBAL);
#else
	hDLL = LoadLibrary("atiadlxx.dll");
	if (hDLL == NULL)
		// A 32 bit calling application on 64 bit OS will fail to LoadLIbrary.
		// Try to load the 32 bit library (atiadlxy.dll) instead
		hDLL = LoadLibrary("atiadlxy.dll");
#endif

	if (NULL == hDLL) {
		printf("ADL library not found!\n");
		return ADL_ERR;
	}

	ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE) GetProcAddress(hDLL,"ADL_Main_Control_Create");
	ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY) GetProcAddress(hDLL,"ADL_Main_Control_Destroy");	
	ADL_Overdrive5_Temperature_Get = (ADL_OVERDRIVE5_TEMPERATURE_GET) GetProcAddress(hDLL, "ADL_Overdrive5_Temperature_Get");

#ifndef _WIN32
	ADL_ConsoleMode_FileDescriptor_Set= (ADL_CONSOLEMODE_FILEDESCRIPTOR_SET) GetProcAddress(hDLL, "ADL_ConsoleMode_FileDescriptor_Set");
#endif

	if (NULL == ADL_Main_Control_Create ||
		NULL == ADL_Main_Control_Destroy ||
		NULL == ADL_Overdrive5_Temperature_Get) {
			printf("ADL's API is missing!\n");
			return ADL_ERR; 
	}

#ifndef _WIN32
	if (NULL == ADL_ConsoleMode_FileDescriptor_Set) {
			printf("ADL's API is missing!\n");
			return ADL_ERR; 
	}
#endif

	

	return ADL_OK;
}


double GetGPUTemp(void) {
	ADLTemperature myTemp;

	//InitADL ();
#ifndef _WIN32
	ADL_ConsoleMode_FileDescriptor_Set(-1);
#endif	
	ADL_Main_Control_Create (ADL_Main_Memory_Alloc, 1);
	ADL_Overdrive5_Temperature_Get(0, 0, &myTemp);	
	ADL_Main_Control_Destroy();
	return myTemp.iTemperature/1000.0;
	//if ((myTemp.iTemperature/1000)>GPU_TEMP_THRESHOLD) TerM("GPU Temperature too high. Shutting Down.");
}





void CPUInit(void) {	
#ifdef _WIN32
#ifndef _SC_NPROCESSORS_ONLN
	SYSTEM_INFO info;
	GetSystemInfo(&info);
#define sysconf(a) info.dwNumberOfProcessors
#define _SC_NPROCESSORS_ONLN
#endif
#endif
#ifdef _SC_NPROCESSORS_ONLN
	nprocs = sysconf(_SC_NPROCESSORS_ONLN);	
#else
	nprocs=0;
#endif

#ifndef _WIN32
	if ((CPUTempArray=(int *) malloc(nprocs*sizeof(int)))==NULL) {
		puts("Can not allocate memory for CPUTempArray");
		exit(-1);
	}
#endif	
}

void InitIPG(void) {
	char *pszPath=NULL;
	char pathStr[256];
	int pathLen;	

	pszPath = getenv("IPG_Dir");	
	if (pszPath == NULL) puts("This needs Intel Power Gadget!\n");
	strcpy(pathStr, pszPath);
	pathLen=strlen(pathStr);
	pathStr[pathLen]='\\';
	pathStr[pathLen+1]='E';
	pathStr[pathLen+2]='n';
	pathStr[pathLen+3]='e';
	pathStr[pathLen+4]='r';
	pathStr[pathLen+5]='g';
	pathStr[pathLen+6]='y';
	pathStr[pathLen+7]='L';
	pathStr[pathLen+8]='i';
	pathStr[pathLen+9]='b';	
#if _M_X64
	pathStr[pathLen+10]='6';
	pathStr[pathLen+11]='4';		
#else
	pathStr[pathLen+10]='3';
	pathStr[pathLen+11]='2';
#endif	
	pathStr[pathLen+12]='.';
	pathStr[pathLen+13]='d';
	pathStr[pathLen+14]='l';
	pathStr[pathLen+15]='l';
	pathStr[pathLen+16]='\0';

	hDLL2 = LoadLibrary(pathStr);

	pInitialize = (IPGInitialize) GetProcAddress(hDLL2, "IntelEnergyLibInitialize");	
	pReadSample = (IPGReadSample) GetProcAddress(hDLL2, "ReadSample");	
	pGetPowerData = (IPGGetPowerData) GetProcAddress(hDLL2, "GetPowerData");
	pGetNumMsrs = (IPGGetNumMsrs) GetProcAddress(hDLL2, "GetNumMsrs");
	pGetMsrFunc = (IPGGetMsrFunc) GetProcAddress(hDLL2, "GetMsrFunc");	
}

double GetCPUTemp (void) {	
#ifndef _WIN32
	//needs lm-sensors package
	int realCoreNum=0;
	FILE *fp;
	char str[256], numdiv[3], str1[80];
	register int i, j, l;

	strcpy(str, "/sys/class/hwmon/hwmon0/device/temp");
	l=strlen(str);
	for (i=1; i<=nprocs; i++) {
		sprintf(numdiv,"%d",i);
		j=0;
		while (numdiv[j]) {
			str[l+j]=numdiv[j];
			j++;
		}
		str[l+j]='_';
		str[l+j+1]='i';
		str[l+j+2]='n';
		str[l+j+3]='p';
		str[l+j+4]='u';
		str[l+j+5]='t';
		str[l+j+6]='\0';

		if ((fp=fopen(str, "rt"))==NULL) CPUTempArray[i-1]=0;
		else {
			realCoreNum++;
			Read_Word(fp, str1);
			CPUTempArray[i-1]=atoi(str1)/1000;
			fclose(fp);
			printf("CPU Core %d Temperature = %d\n", realCoreNum, CPUTempArray[i-1]);
			if (CPUTempArray[i-1]>CPU_TEMP_THRESHOLD) TerM("CPU Temperature too high. Shutting Down.");
		}		 
	}
#else
	int i, iMSR;
	double data[3];
	int nData;
	int funcID;

	//InitIPG();
	pInitialize();
	pGetNumMsrs(&iMSR);
	pReadSample();

	for (i=0; i<iMSR; i++) {
		pGetPowerData(0, i, data, &nData);	
		pGetMsrFunc(i, &funcID);

		if (funcID == 2){
			return data[0];
		}	
	}	
#endif
}


void FinalizeCPU(void) {
#ifdef _WIN32
	FreeLibrary(hDLL2);
#else
	free(CPUTempArray);
#endif	
}

void FinalizeGPU(void) {
#ifdef _WIN32
	FreeLibrary(hDLL);
#else
	dlclose(hDLL);
#endif		
}
#endif