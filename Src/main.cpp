#include "MainFunction.h"
#include <random>
#include <ilcplex/ilocplex.h>
#include "Util.h"
#include <sys/types.h>
#include <sys/stat.h>
IloEnv cEnv;
std::default_random_engine cGloalRandomEngine;

void Init()
{
	srand((unsigned int)time(NULL));
	cGloalRandomEngine.seed(time(NULL));
}

int main(int argc, char * argv[])
{
#if 0
	mkdir("/home/zyecheng/VirginiaTech/Research/RTUnschedCore/Exec/ExpSpace/MinWeightedAvgRT_Nonpreemptive/hello", S_IRUSR | S_IWUSR);
	ofstream of("/home/zyecheng/VirginiaTech/Research/RTUnschedCore/Exec/ExpSpace/MinWeightedAvgRT_Nonpreemptive/hello/hello.txt", ios::out);
	if(of.is_open() == false)
		cout << strerror(errno) << endl;
	else
		of.close();
#endif
	Init();
	//f(); system("pause"); return 0;
	int iStatus = DispatchFunction(argc, argv);	
	return 0;
}
