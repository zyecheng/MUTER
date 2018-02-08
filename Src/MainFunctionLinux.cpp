#include "stdafx.h"
#include <fstream>
#include "MainFunction.h"
#include "Util.h"
#include "TaskSet.h"
#include "NoCFlowFocusRefinement.h"
#include "EventDrivenPACaseStudy.h"
typedef int(*MainFunc)(int, char *[]);
#define BUFFERLEN 1024
#define TPCCREDTHRESHOLD 20

struct FunctionEntry
{
	char axFuncName[BUFFERLEN];
	MainFunc pfuncFunction;
};

FunctionEntry ptagAllFunction[] = {
	//{ "DummyTemp", DummyTemp },
	{ "NoCCaseStudy_Enhanced-BnB", NoCFlowCaseStudy_ArtifactEvaluationWrapUp::NoCAutonomousVehicleCaseStudy_EnhancedBnB },
	{ "NoCCaseStudy_MUTER-guided", NoCFlowCaseStudy_ArtifactEvaluationWrapUp::NoCAutonomousVehicleCaseStudy_MUTERGuided_PerUtilization },
	{ "NoCCaseStudy_MILP", NoCFlowCaseStudy_ArtifactEvaluationWrapUp::NoCAutonomousVehicleCaseStudy_MILP_PerUtilization },
	{ "NoCUSweep", NoCFlowCaseStudy_ArtifactEvaluationWrapUp::NoCSyntheticUSweep_PerUtilization_MethodOption },
	{ "NoCNSweep", NoCFlowCaseStudy_ArtifactEvaluationWrapUp::NoCOptimizationNSweep_PerN_MethodOption },


	{ "VehicleSystem_All", EventDrivenPACaseStudy_ArtifactEvaluation::VehicleSystem_All },
	//{ "VechicleSystem_BreakDownUtil_MILPApprox", EventDrivenPACaseStudy_ArtifactEvaluation::VechicleSystem_BreakDownUtil_MILPApprox },
	//{ "VechicleSystem_BreakDownUtil_MUTERApprox", EventDrivenPACaseStudy_ArtifactEvaluation::VechicleSystem_BreakDownUtil_MUTERApprox },
	//{ "VechicleSystem_BreakDownUtil_MUTERAccurate", EventDrivenPACaseStudy_ArtifactEvaluation::VechicleSystem_BreakDownUtil_MUTERAccurate },	
	{ "VechicleSystem_BreakDownUtil", EventDrivenPACaseStudy_ArtifactEvaluation::VechicleSystem_BreakDownUtil_PerAllocation },

	
}; 

int iFunctionNum = sizeof(ptagAllFunction) / sizeof(FunctionEntry);

extern void my_assert(bool bCondition, char axInfo[]);

int ExtractArgument(int argc, char * argv[], ArgName2Arg & rmapName2Arg)
{
	char axBuffer[256] = { 0 };
	int iDefaultIndex = 0;
	for (int i = 0; i < argc; i++)
	{
		string stringCommmand = string(argv[i]);
		if (argv[i][0] == '-')
		{
			//name specified
			int iEqualityIndex = stringCommmand.find('=', 1);
			if (iEqualityIndex == -1)
			{
				cout << "I don't recognize this: " << stringCommmand.data() << endl;
				my_assert(0, "");
			}
			string stringArgName = stringCommmand.substr(1, iEqualityIndex - 2 + 1);
			string stringArg = stringCommmand.substr(iEqualityIndex + 1, stringCommmand.size() - iEqualityIndex - 1 + 1);
			rmapName2Arg[stringArgName] = stringArg;
		}
		else
		{
			sprintf_s(axBuffer, "arg%d", iDefaultIndex);
			iDefaultIndex++;
			string stringArgName(axBuffer);
			rmapName2Arg[stringArgName] = stringCommmand;
		}
	}	

	for (ArgName2Arg::iterator iter = rmapName2Arg.begin(); iter != rmapName2Arg.end(); iter++)
	{
	//	cout << iter->first << " " << iter->second << endl;		
	}
	return 0;
}

int DispatchFunction(int argc, char * argv[])
{
	if (argc < 2)
	{
		return 0;
	}

	for (int i = 0; i < iFunctionNum; i++)
	{
		if (strcmp(argv[1], ptagAllFunction[i].axFuncName) == 0)
		{
			return ptagAllFunction[i].pfuncFunction(argc, argv);			
		}
	}
	cout << "Unknown Function Option: " << argv[1] << endl;
	cout << "------------------------------------------------" << endl;
	cout << "Available function options: " << endl << endl;
	for (int i = 0; i < iFunctionNum; i++)
	{
		cout << ptagAllFunction[i].axFuncName << endl;
	}
	//my_assert(false, "Unknown Function Option");
	return 0;
}

int DummyTemp(int argc, char * argv[])
{

	return 0;
}
