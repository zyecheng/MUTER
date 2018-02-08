#include "stdafx.h"
#include <fstream>
#include "Util.h"
#include <sys/stat.h>
//#include <filesystem>
#include <deque>
#include <time.h>
#include <sys/time.h>
//#include <io.h>
//#include <direct.h>

#define _X86_
//#include <processthreadsapi.h>


void my_assert(bool bCondition, char axInfo[])
{
	if (!bCondition)
	{
		cout << "Asserted: " << axInfo << endl;
		while (1);
	}
}

bool IsFileExist(char axFileName[])
{
	ifstream ifstreamTestExist(axFileName, ios::in);
	bool bStatus = ifstreamTestExist.is_open();
	ifstreamTestExist.close();
	return bStatus;
}

int ExecuteSelf(char axCmd[])
{
	my_assert(IsFileExist("MCMILPPact.exe"), "Cannot Find Exec MCMILPPact.exe");
//	cout << "Hello " << endl;
//	cout << axCmd << endl;
	system(axCmd);
	return 0;
}

#if 0
void GetTaskSetFileName(deque<string> & rdequeFileNames, char axFolderName[])
{
	_finddata_t find_data;
	intptr_t file;
	char axSuffix[256] = { 0 };
	char axDirAndName[1024] = { 0 };
	sprintf(axSuffix, "%s\\*.tskst", axFolderName);
	file = _findfirst(axSuffix, &find_data);	
	int iStatus = file;
	while (iStatus != -1)
	{
		//cout << find_data.name << endl;
		sprintf(axDirAndName, "%s\\%s", axFolderName, find_data.name);			
		string stringWholeName(axDirAndName);
		rdequeFileNames.push_back(stringWholeName);
		iStatus = _findnext(file, &find_data);
	}
	_findclose(file);
}
#endif

void MakePath(char axPath[])
{
	string stringFilePath(axPath);
    string stringCommand = "mkdir -p " + stringFilePath;
    system(stringCommand.data());
    return;
	int iBeginPos = 0;
	int iEndPos = stringFilePath.find("\\");	
	while(iEndPos != -1)
	{
		//cout << stringFilePath.substr(0, iEndPos).data() << endl; system("pause"); 
		mkdir(stringFilePath.substr(0, iEndPos).data(),S_IRUSR | S_IWUSR);
		iEndPos = stringFilePath.find("\\", iEndPos + 1);
	}
	mkdir(stringFilePath.data(), S_IRUSR | S_IWUSR);
}

bool IsPathExist(char axPath[])
{	
	string stringFilePath(axPath);
	int iBeginPos = 0;
	int iEndPos = stringFilePath.find("\\");
	while (iEndPos != -1)
	{
		//cout << stringFilePath.substr(0, iEndPos).data() << endl; system("pause"); 
		int iStatus = mkdir(stringFilePath.substr(0, iEndPos).data(),S_IRUSR | S_IWUSR);
		if (iStatus == 0)
		{
			rmdir(stringFilePath.substr(0, iEndPos).data());
			return false;
		}
		iEndPos = stringFilePath.find("\\", iEndPos + 1);
	}
	int iStatus = mkdir(stringFilePath.data(),S_IRUSR | S_IWUSR);
	if (iStatus == 0)
	{
		rmdir(stringFilePath.substr(0, iEndPos).data());
		return false;
	}
	else
		return true;
}

signed long long getCPUTimeStamp()
{
	return clock();
}

bool StartWith(char axString[], char axComparison[])
{
	int iLengthIn = strlen(axString);
	int iLengthCmp = strlen(axComparison);
	if (iLengthIn < iLengthCmp)
		return false;
	for (int i = 0; i < iLengthCmp; i++)
	{
		if (axString[i] != axComparison[i])
			return false;
	}
	return true;
}

double ceil_tol(double dValue, double dTol)
{
	double dFloorInt = floor(dValue);
	if (dValue - dFloorInt > dTol)
	{
		return dFloorInt + 1;
	}
	else
	{
		return dFloorInt;
	}
}


int gcd(int a, int b)
{
	for (;;)
	{
		if (a == 0) return b;
		b %= a;
		if (b == 0) return a;
		a %= b;
	}
}

int lcm(int a, int b)
{
	int temp = gcd(a, b);

	return temp ? (a / temp * b) : 0;
}

double getWallTimeSecond()
{
  struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;

}


void EraseString(string & rcString, const char axTarget[])
{
	int iLen = strlen(axTarget);
	int iPosition = rcString.find(axTarget);
	while (iPosition != -1)
	{
		rcString.erase(iPosition, iLen);
		iPosition = rcString.find(axTarget);
	}
}

int _BitScanReverse(unsigned long * _Index, unsigned int _Mask)
{
	int iIndex = __builtin_clz(_Mask);
	* _Index = sizeof(unsigned int) * 8 - 1 - iIndex;
	return iIndex;
}

int  _BitScanForward(unsigned long * _Index, unsigned int _Mask)
{
	int iIndex = __builtin_ffs(_Mask);
        * _Index = iIndex - 1;
	return iIndex;
}

const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	localtime_r(&now, &tstruct);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}

unsigned long gcd_long(unsigned long a, unsigned long b)
{
	for (;;)
	{
		if (a == 0) return b;
		b %= a;
		if (b == 0) return a;
		a %= b;
	}
}

void Sleep(double dMS)
{
	usleep(dMS * 1000.0);
}
