#include "stdafx.h"
#include "TaskSet.h"
#include <cmath>
#include <time.h>
#include <algorithm>
#include <random>
#include <assert.h>
#include "Util.h"
#include <assert.h>
extern std::default_random_engine cGloalRandomEngine;
extern void my_assert(bool bCondition, char axInfo[]);
TaskSet::TaskSet()
{
	m_pcTaskArray = NULL;
	m_ppiPriorityTable = NULL;
	m_iTaskNum = 0;	
	m_dTotalUtilization = 0;	
	m_iHITaskNum = 0;
	m_ppcTasksArrayByPeriod = NULL;
#ifndef NOFSM
	m_ppcStateFlows = NULL;
#endif
}

TaskSet::TaskSet(const TaskSet & rcTaskSet)
	:TaskSet()
{	
	ClearAll();
	int iTaskNum = rcTaskSet.getTaskNum();
	Task * pcTaskArray = rcTaskSet.getTaskArrayPtr();
	for (int i = 0; i < iTaskNum; i++)
	{
		AddTask(pcTaskArray[i]);
	}
	ConstructTaskArray();
}


TaskSet::~TaskSet()
{
	ClearTaskArray();
#ifndef NOFSM
	if (m_ppcStateFlows)
	{
		delete[] m_ppcStateFlows;
	}
#endif
}

void TaskSet::GenRandomTaskset(int iTaskNum, double fTotalUtil, double fHighCritiPercent, double fCriticalityFactor, int iPeriodOpt, bool bConstrainedDeadline)
{		
	if (fHighCritiPercent > 1)
	{
		cout << "HI Task Percentage Can Not Exceed 1 !!!"<< endl;
		while (1);
	}
	int iHITaskNum = iTaskNum * fHighCritiPercent;
	/////////////////////
	double * pfUtilTable = new double[iTaskNum];
	double * pfPeriodTable = new double[iTaskNum];
	bool * pbCritiTable = new bool[iTaskNum];
	GenerateUtilTable(pfUtilTable, fTotalUtil, iTaskNum);
	GeneratePeriodTable(pfPeriodTable, iTaskNum, iPeriodOpt);
	GenerateCritiTable(pbCritiTable, iHITaskNum, iTaskNum);
	
	for (int i = 0; i < iTaskNum; i++)
	{
		if (pfUtilTable[i] == 0)
		{
			pfUtilTable[i] = fTotalUtil / 1000.0 / iTaskNum;
		}
	}

	for (int i = 0; i < iTaskNum; i++)
	{		
		pfUtilTable[i] = max(pfUtilTable[i], 0.001);
		if (bConstrainedDeadline)
		{
			double dWCET = (pbCritiTable ? fCriticalityFactor : 1) * pfUtilTable[i] * pfPeriodTable[i];
			double dDeadline = GenRandDeadline(pfPeriodTable[i], dWCET);
			m_setTasks.insert(Task(pfPeriodTable[i], dDeadline, pfUtilTable[i], fCriticalityFactor, pbCritiTable[i], i));
		}
		else
		{
			m_setTasks.insert(Task(pfPeriodTable[i], pfPeriodTable[i], pfUtilTable[i], fCriticalityFactor, pbCritiTable[i], i));
		}		
	}
	ConstructTaskArray();	
	delete pfUtilTable;
	delete pfPeriodTable;
	delete pbCritiTable;

}

void TaskSet::GenRandomTaskSet_SumHILOUtil(int iTaskNum, double dTotalUtil,
	double dHighCritiPercent, double dCriticalityFactor, int iPeriodOpt /* = 0 */, bool bConstrainedDeadline /* = false */)
{
	if (dHighCritiPercent > 1)
	{
		cout << "HI Task Percentage Can Not Exceed 1 !!!" << endl;
		while (1);
	}
	int iHITaskNum = iTaskNum * dHighCritiPercent;
	/////////////////////
	double * pfUtilTable = new double[iTaskNum];
	double * pfPeriodTable = new double[iTaskNum];
	bool * pbCritiTable = new bool[iTaskNum];
	GenerateUtilTable(pfUtilTable, dTotalUtil, iTaskNum);
	GeneratePeriodTable(pfPeriodTable, iTaskNum, iPeriodOpt);
	GenerateCritiTable(pbCritiTable, iHITaskNum, iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		pfUtilTable[i] = pfUtilTable[i] / (pbCritiTable[i] ? dCriticalityFactor : 1.0);
		if (bConstrainedDeadline)
		{			
			double dWCET = pfUtilTable[i] * pfPeriodTable[i];
			double dDeadline = GenRandDeadline(pfPeriodTable[i], dWCET);
			m_setTasks.insert(Task(pfPeriodTable[i], dDeadline, pfUtilTable[i], dCriticalityFactor, pbCritiTable[i], i));
		}
		else
		{			
			m_setTasks.insert(Task(pfPeriodTable[i], pfPeriodTable[i], pfUtilTable[i], dCriticalityFactor, pbCritiTable[i], i));
		}
	}
	ConstructTaskArray();
	delete pfUtilTable;
	delete pfPeriodTable;
	delete pbCritiTable;

}

void TaskSet::GenRandomTaskSet_SumHILOUtil_Split(int iTaskNum, double dTotalUtil, double dHIUtilPercentage,
	double dHighCritiPercent, double dCriticalityFactor, int iPeriodOpt /* = 0 */, bool bConstrainedDeadline /* = false */)
{
	if (dHighCritiPercent > 1)
	{
		cout << "HI Task Percentage Can Not Exceed 1 !!!" << endl;
		while (1);
	}
	int iHITaskNum = iTaskNum * dHighCritiPercent;
	/////////////////////
	double * pfUtilTable = new double[iTaskNum];
	double * pfPeriodTable = new double[iTaskNum];
	bool * pbCritiTable = new bool[iTaskNum];
	double dHIUtil = dTotalUtil * dHIUtilPercentage;
	double dLOUtil = dTotalUtil * (1 - dHIUtilPercentage);
	GenerateUtilTable(pfUtilTable, dHIUtil, iHITaskNum);
	GenerateUtilTable(&pfUtilTable[iHITaskNum], dLOUtil, iTaskNum - iHITaskNum);
	GeneratePeriodTable(pfPeriodTable, iTaskNum, iPeriodOpt);
	GenerateCritiTable(pbCritiTable, iHITaskNum, iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		pfUtilTable[i] = pfUtilTable[i] / (pbCritiTable[i] ? dCriticalityFactor : 1.0);
		if (bConstrainedDeadline)
		{
			double dWCET = pfUtilTable[i] * pfPeriodTable[i];
			double dDeadline = GenRandDeadline(pfPeriodTable[i], dWCET);
			m_setTasks.insert(Task(pfPeriodTable[i], dDeadline, pfUtilTable[i], dCriticalityFactor, pbCritiTable[i], i));
		}
		else
		{
			m_setTasks.insert(Task(pfPeriodTable[i], pfPeriodTable[i], pfUtilTable[i], dCriticalityFactor, pbCritiTable[i], i));
		}
	}
	ConstructTaskArray();
	delete pfUtilTable;
	delete pfPeriodTable;
	delete pbCritiTable;

}


void TaskSet::GenRandomTaskSetInteger(int iTaskNum, double fTotalUtil, double fHighCritiPercent, double fCriticalityFactor, int iPeriodOpt, bool bConstrainedDeadline)
{
	if (fHighCritiPercent > 1)
	{
		cout << "HI Task Percentage Can Not Exceed 1 !!!" << endl;
		while (1);
	}
	int iHITaskNum = iTaskNum * fHighCritiPercent;
	/////////////////////
	double * pfUtilTable = new double[iTaskNum];
	double * pfPeriodTable = new double[iTaskNum];
	bool * pbCritiTable = new bool[iTaskNum];
	GenerateUtilTable(pfUtilTable, fTotalUtil, iTaskNum);
	GeneratePeriodTable(pfPeriodTable, iTaskNum, iPeriodOpt);
	GenerateCritiTable(pbCritiTable, iHITaskNum, iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		pfPeriodTable[i] = round(pfPeriodTable[i]);
		pfPeriodTable[i] *= 1000;
		//double dWCET = floor((pbCritiTable ? fCriticalityFactor : 1) * pfUtilTable[i] * pfPeriodTable[i]);
		double dWCET = floor(pfUtilTable[i] * pfPeriodTable[i]);
		dWCET = max(dWCET, 1.0);
		pfUtilTable[i] = dWCET / pfPeriodTable[i];

		if (bConstrainedDeadline)
		{			
			double dDeadline = min(round(GenRandDeadline(pfPeriodTable[i], dWCET)), pfPeriodTable[i]);
			m_setTasks.insert(Task(pfPeriodTable[i], dDeadline, pfUtilTable[i], fCriticalityFactor, pbCritiTable[i], i));
		}
		else
		{
			m_setTasks.insert(Task(pfPeriodTable[i], pfPeriodTable[i], pfUtilTable[i], fCriticalityFactor, pbCritiTable[i], i));
		}


	}
	ConstructTaskArray();
	delete pfUtilTable;
	delete pfPeriodTable;
	delete pbCritiTable;

}

void TaskSet::GenRandomTaskSet_AUTOSTART_ProtectionMechanism(int iTaskNum, double dTotalUtil)
{

}

void TaskSet::GenRandomTasksetMC(int iTaskNum, double fTotalUtil, 
	double fHighCritiPercent, double fCriticalityFactorUB, double fCriticalityFactorLB, int iPeriodOpt, bool bConstrainedDeadline)
{
	if (fHighCritiPercent > 1)
	{
		cout << "HI Task Percentage Can Not Exceed 1 !!!" << endl;
		while (1);
	}
	int iHITaskNum = iTaskNum * fHighCritiPercent;
	/////////////////////
	double * pfUtilTable = new double[iTaskNum];
	double * pfPeriodTable = new double[iTaskNum];
	double * pdCritiFactorTable = new double[iTaskNum];
	bool * pbCritiTable = new bool[iTaskNum];
	GeneratePeriodTable(pfPeriodTable, iTaskNum, iPeriodOpt);
	GenerateCritiTable(pbCritiTable, iHITaskNum, iTaskNum);
	GenerateCritiFactorTable(pdCritiFactorTable, fCriticalityFactorUB, fCriticalityFactorLB, iTaskNum);
	bool bInvalidUtil = true;
	int iInvalidTimes = 0;
	while (bInvalidUtil)
	{		
		GenerateUtilTable(pfUtilTable, fTotalUtil, iTaskNum);		
		bInvalidUtil = false;

#if 1
		for (int i = 0; i < iTaskNum; i++)
		{
			//if (pbCritiTable[i])
			{
				pfUtilTable[i] = pfUtilTable[i] / pdCritiFactorTable[i];
			}
		}
#endif

		for (int i = 0; i < iTaskNum; i++)
		{
			if (pfUtilTable[i] > 1.0)
			{
				bInvalidUtil = true;
				iInvalidTimes++;
				break;
			}
		}
		//cout << "\rInvalid Times: " << iInvalidTimes << "             ";
	}
	//cout << "Succeed" << endl;
	//cout << endl;


	for (int i = 0; i < iTaskNum; i++)
	{
		if (bConstrainedDeadline)
		{
			double dWCET = (pbCritiTable ? pdCritiFactorTable[i] : 1) * pfUtilTable[i] * pfPeriodTable[i];
			double dDeadline = GenRandDeadline(pfPeriodTable[i], dWCET);
			m_setTasks.insert(Task(pfPeriodTable[i], dDeadline, pfUtilTable[i], pdCritiFactorTable[i], pbCritiTable[i], i));
		}
		else
		{
			m_setTasks.insert(Task(pfPeriodTable[i], pfPeriodTable[i], pfUtilTable[i], pdCritiFactorTable[i], pbCritiTable[i], i));
		}
	}
	ConstructTaskArray();
	delete pfUtilTable;
	delete pfPeriodTable;
	delete pbCritiTable;

}

void TaskSet::ClearTaskArray()
{
	if (m_pcTaskArray)
	{
		delete[] m_pcTaskArray;
	}	
	m_pcTaskArray = NULL;
	if (m_ppcTasksArrayByPeriod)
	{
		delete[] m_ppcTasksArrayByPeriod;
	}
	m_ppcTasksArrayByPeriod = NULL;
	if (m_ppiPriorityTable)
	{
		for (int i = 0; i < m_iTaskNum; i++)
		{
			delete m_ppiPriorityTable[i];
		}
		delete m_ppiPriorityTable;
	}	
	m_ppiPriorityTable = NULL;
}

#define PREDEFINE_PERIOD_TABLE 0
#if 0
#endif

void TaskSet::GenerateUtilTable(double * pfUtilTable, double fTotalUtil, int iSize)
{
	double fSumU = fTotalUtil;	
	for (int i = 0; i < iSize - 1; i++)
	{
		double fRand = (double)rand() / RAND_MAX;
		double fNextSumU = fSumU * pow(fRand, (double)(1.0 / (iSize - i - 1)));
		pfUtilTable[i] = fSumU - fNextSumU;
		fSumU = fNextSumU;
	}
	pfUtilTable[iSize - 1] = (fSumU);
}

void TaskSet::GenerateCritiTable(bool * pbCritiTable,int iHITaskNum, int iSize)
{
//	srand((unsigned int)time(NULL));
	for (int i = 0; i < iSize; i++)
		pbCritiTable[i] = false;

	for (int i = 0; i < iHITaskNum; i++)
	{
		pbCritiTable[i] = true;
	}
}

void TaskSet::GenerateCritiFactorTable(double * pdCritiFactorTable, double dUB, double dLB, int iSize)
{
	assert(dUB >= dLB);
	for (int i = 0; i < iSize; i++)
		pdCritiFactorTable[i] = 1.0;

	for (int i = 0; i < iSize; i++)
	{
		double dPercent = (double)rand() / (double)RAND_MAX;
		pdCritiFactorTable[i] = dLB + (dUB - dLB) * dPercent;		
	}
}

//std::default_random_engine cRandomEngine;

double dLogUniformStart = log(10);
double dLogUniformEnd = log(1000);
std::uniform_real_distribution<double> cDistribution(dLogUniformStart, dLogUniformEnd);
void TaskSet::GeneratePeriodTable(double * pfPeriodTable, int iSize,int iOpt)
{
	switch (iOpt)
	{
	case PERIODOPT_TABLE:		
		for (int i = 0; i < iSize; i++)
		{			
			pfPeriodTable[i] = GeneratePeriod_Predef();
		}
		break;
	case PERIODOPT_TABLE_V2:
		for (int i = 0; i < iSize; i++)
		{
			pfPeriodTable[i] = GeneratePeriod_Predef_v2();
		}
		break;
	case PERIODOPT_TUPLE:
		for (int i = 0; i < iSize; i++)
		{
			pfPeriodTable[i] = GeneratePeriod_Tuple();
		}
		break;
	case PERIODOPT_LOG:
		for (int i = 0; i < iSize; i++)
		{
			pfPeriodTable[i] = round(exp(cDistribution(cGloalRandomEngine)));
		}
		break;
	}
}

void TaskSet::DisplayTaskInfo()
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		cout << "Task Index: " << i << " / ";
		cout << "T: " << m_pcTaskArray[i].getPeriod() << " / ";
		cout << "CLO: " << m_pcTaskArray[i].getCLO();
		if (m_pcTaskArray[i].getCriticality())
		{
			cout << " / CHI: " << m_pcTaskArray[i].getCHI();
		}
		cout << endl;
	}
}

void TaskSet::DisplayTaskInfoDetailed()
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		cout << m_pcTaskArray[i];
	}
}

void TaskSet::RemoveLinkWeight()
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		Task & rcTask = m_pcTaskArray[i];
		{			
			set<Task::CommunicationLink> & rsetPredecessor = rcTask.getPredecessorSet();
			for (set<Task::CommunicationLink>::iterator iter = rsetPredecessor.begin();
				iter != rsetPredecessor.end();)
			{
				Task::CommunicationLink cCopy = *iter;
				iter++;
				rsetPredecessor.erase(cCopy);
				cCopy.m_dDelayCost = 1.0;
				rsetPredecessor.insert(cCopy);
			}
		}

		{
			set<Task::CommunicationLink> & rsetSucessor = rcTask.getSuccessorSet();
			for (set<Task::CommunicationLink>::iterator iter = rsetSucessor.begin();
				iter != rsetSucessor.end();)
			{
				Task::CommunicationLink cCopy = *iter;
				iter++;
				rsetSucessor.erase(cCopy);
				cCopy.m_dDelayCost = 1.0;
				rsetSucessor.insert(cCopy);
			}
		}
	}
}

double TaskSet::SmallestOffset(int iTaskA, int iTaskB)
{
	double dPeriodA = m_pcTaskArray[iTaskA].getPeriod();
	double dPeriodB = m_pcTaskArray[iTaskB].getPeriod();
	double dLCM = lcm(dPeriodA, dPeriodB);
	int iInstances = round( dLCM / dPeriodA);
	double dStart = 0;	
	double dOffset = dPeriodB;
	for (int i = 0; i < iInstances; i++)
	{		
		double dThisOffset = ceil(dStart / dPeriodB) * dPeriodB - dStart;
		if (dThisOffset == 0.0)
		{
			dThisOffset = dPeriodB;
		}		
		dOffset = min(dOffset, dThisOffset);
		dStart += dPeriodA;
	}
	return dOffset;
}

//std::default_random_engine cRandomEngineInt;

std::uniform_int_distribution<int> cDistributionInt(0, 1020);

double TaskSet::GenRandDeadline(double dPeriod, double dExecution)
{
	std::uniform_real_distribution<double> cDistribution(dExecution, dPeriod);
	return cDistribution(cGloalRandomEngine);
}

double TaskSet::GeneratePeriod_Tuple(double fHarmonic)
{
	int aaiFactors[][2] = {
		{ 2, 4 }, { 6, 12 }, {5,10}
	};
	double fPeriod = 1;
	int iMaxFactor = cDistributionInt(cGloalRandomEngine) % 3 + 1;
	for (int i = 0; i < iMaxFactor; i++)
	{
		fPeriod *= aaiFactors[cDistributionInt(cGloalRandomEngine) % 3][cDistributionInt(cGloalRandomEngine) % 2];
	}

	return fPeriod;
}

double TaskSet::GeneratePeriod_Predef(double fHarmonic /* = 0 */)
{
	double afPeriods[] = { 1, 2, 4, 5, 10, 20, 40, 50, 100, 200, 400, 500, 1000 };
	int iNPeriods = sizeof(afPeriods) / sizeof(double);
	if (fHarmonic == 0)
	{
		int iPeriodOpt = rand() % iNPeriods;
		return afPeriods[iPeriodOpt];
	}
	else
	{
		deque<double> dequeHarmonics;
		for (int i = 0; i < iNPeriods; i++)
		{
			double fRatio = (fHarmonic > afPeriods[i]) ? (fHarmonic / afPeriods[i]) : (afPeriods[i] / fHarmonic);
			if (ceil(fRatio) - fRatio == 0)
			{
				dequeHarmonics.push_back(afPeriods[i]);
			}
		}
		my_assert(dequeHarmonics.size() != 0, "In GeneratePeriod_Predef. No harmonic candidate exists!");
		int iOpt = rand() % dequeHarmonics.size();
		return dequeHarmonics[iOpt];
	}
	return 0;
}

double TaskSet::GeneratePeriod_Predef_v2(double fHarmonic /* = 0 */)
{
	double afPeriods[] = { 100, 150, 200, 250, 300, };
	int iNPeriods = sizeof(afPeriods) / sizeof(double);
	if (fHarmonic == 0)
	{
		int iPeriodOpt = rand() % iNPeriods;
		return afPeriods[iPeriodOpt];
	}
	else
	{
		deque<double> dequeHarmonics;
		for (int i = 0; i < iNPeriods; i++)
		{
			double fRatio = (fHarmonic > afPeriods[i]) ? (fHarmonic / afPeriods[i]) : (afPeriods[i] / fHarmonic);
			if (ceil(fRatio) - fRatio == 0)
			{
				dequeHarmonics.push_back(afPeriods[i]);
			}
		}
		my_assert(dequeHarmonics.size() != 0, "In GeneratePeriod_Predef. No harmonic candidate exists!");
		int iOpt = rand() % dequeHarmonics.size();
		return dequeHarmonics[iOpt];
	}
	return 0;
}

void TaskSet::AddTask(double fPeriod, double dDeadline, double fUtil, double CritiFactor, bool bHICriticality, int iIndex /* = -1 */)
{
	//if no index is specified, assign a index.	
	iIndex = m_setTasks.size();	
	std::pair<set<Task>::iterator,bool> cPair = m_setTasks.insert(Task(fPeriod, dDeadline, fUtil, CritiFactor, bHICriticality, iIndex));		
}

void TaskSet::AddTask(Task & rcTask)
{	
	if (!rcTask.getName().empty())
	{
		for (set<Task>::iterator iter = m_setTasks.begin();
			iter != m_setTasks.end(); iter++)
		{
			if (iter->getName().compare(rcTask.getName()) == 0)
			{
				return;
			}
		}
	}

	Task cTask = rcTask;
	cTask.setIndex(m_setTasks.size());
	m_setTasks.insert(cTask);
}

void TaskSet::RemoveTask(Task & rcTask)
{
	for (set<Task>::iterator iter = m_setTasks.begin();
		iter != m_setTasks.end(); )
	{
		
		if (iter->getName().compare(rcTask.getName()) == 0)
		{
			set<Task>::iterator iterTemp = iter;
			iter++;
			m_setTasks.erase(iterTemp);
		}
		else
			iter++;
	}
	
}

void TaskSet::AddLink(int iSource, int iDestination, double dMemory, double dDelay)
{
	m_pcTaskArray[iSource].AddSuccessor(iDestination, dMemory, dDelay);
	m_pcTaskArray[iDestination].AddPredecessor(iSource, dMemory, dDelay);
}

void TaskSet::ConstructTaskArray()
{
	ClearTaskArray();	

	m_iTaskNum = m_setTasks.size();
	if (m_iTaskNum == 0) return;
	m_dTotalUtilization = 0;
	m_iHITaskNum = 0;	
	m_pcTaskArray = new Task[m_iTaskNum];
	m_ppiPriorityTable = new int *[m_iTaskNum];
	m_ppcTasksArrayByPeriod = new Task *[m_iTaskNum];
	for (int i = 0; i < m_iTaskNum; i++)	m_ppcTasksArrayByPeriod[i] = NULL;
	int iIndex = 0;
	
	for (set<Task>::iterator iter = m_setTasks.begin();
		iter != m_setTasks.end(); iter++)
	{	
		m_pcTaskArray[iter->getTaskIndex()] = *iter;
		m_ppiPriorityTable[iter->getTaskIndex()] = new int[m_iTaskNum];
		for (int i = 0; i < m_iTaskNum; i++)
			m_ppiPriorityTable[iter->getTaskIndex()][i] = PRIORITY_UNDEF;
		m_dTotalUtilization += iter->getUtilization();
		m_iHITaskNum += iter->getCriticality() ? 1 : 0;
		//std::pair<set<Task *>::iterator, bool> cPair = m_setTasksIndexOrdered.insert(&m_pcTaskArray[iIndex]);
		if (m_ppcTasksArrayByPeriod[iIndex] != NULL)
		{
			cout << "More than one task has the same index" << endl;
			while (1);
		}
		m_ppcTasksArrayByPeriod[iIndex] = &m_pcTaskArray[iter->getTaskIndex()];
		iIndex++;
	}	

	my_assert(m_iTaskNum > 0, "Not a single task?");
	m_dPeriodGCD = m_pcTaskArray[0].getPeriod() * 1000.0;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_dPeriodGCD = gcd_long(m_dPeriodGCD, m_pcTaskArray[i].getPeriod() * 1000.0);
	}

	m_dPeriodGCD = m_dPeriodGCD / 1000.0;
	assert(m_dPeriodGCD > 0);
}

void TaskSet::ClearAll()
{
	ClearTaskArray();
	m_setTasks.clear();
	m_iTaskNum = 0;
	m_dTotalUtilization = 0;
	m_iHITaskNum = 0;
}

void TaskSet::WriteImageFile(ofstream & OutputFile)
{
	OutputFile.write((const char *)& m_iTaskNum, sizeof(int));
	OutputFile.write((const char *)& m_dAvailableMemory, sizeof(double));
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_pcTaskArray[i].WriteImage(OutputFile);
	}

	WriteExtendedParameterImage(OutputFile);
}

void TaskSet::WriteImageFile(char axFileName[])
{
	ofstream ofstreamOuputFile(axFileName, ios::out | ios::binary);	
	WriteImageFile(ofstreamOuputFile);
	ofstreamOuputFile.close();
}

void TaskSet::WriteBasicImageFile(char axFileName[])
{
	ofstream ofstreamOuputFile(axFileName, ios::out | ios::binary);
	ofstreamOuputFile.write((const char *)& m_iTaskNum, sizeof(int));
	ofstreamOuputFile.write((const char *)& m_dAvailableMemory, sizeof(double));
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_pcTaskArray[i].WriteImage(ofstreamOuputFile);
	}
	ofstreamOuputFile.close();
}


void TaskSet::ReadImageFile(ifstream & InputFile)
{
	int iTaskNum = 0;
	InputFile.read((char *)& iTaskNum, sizeof(int));	
	InputFile.read((char *)& m_dAvailableMemory, sizeof(double));
	for (int i = 0; i < iTaskNum; i++)
	{
		Task cReadTask;
		cReadTask.ReadImage(InputFile);
		cReadTask.Construct();
		AddTask(cReadTask);
	}
	ConstructTaskArray();

	ReadExtendedParameterImage(InputFile);
}

void TaskSet::ReadImageFile(char axFileName[])
{
	ClearAll();
	ifstream ifstreamInputFile(axFileName, ios::in | ios::binary);
	if (!ifstreamInputFile.is_open())
	{
		cout << axFileName << endl;
		my_assert(false, "Cannot Find Taskset File");
	}
	
	ReadImageFile(ifstreamInputFile);
	ifstreamInputFile.close();
}

void TaskSet::ReadBasicImageFile(char axFileName[])
{
	ClearAll();
	ifstream ifstreamInputFile(axFileName, ios::in | ios::binary);
	if (!ifstreamInputFile.is_open())
	{
		cout << axFileName << endl;
		my_assert(false, "Cannot Find Taskset File");
	}

	int iTaskNum = 0;
	ifstreamInputFile.read((char *)& iTaskNum, sizeof(int));
	ifstreamInputFile.read((char *)& m_dAvailableMemory, sizeof(double));
	for (int i = 0; i < iTaskNum; i++)
	{
		Task cReadTask;
		cReadTask.ReadImage(ifstreamInputFile);
		cReadTask.Construct();
		AddTask(cReadTask);
	}
	ConstructTaskArray();
	ifstreamInputFile.close();
}

void TaskSet::WriteTextFile(ofstream & OutputFile)
{
	OutputFile << "Task Set Info:" << endl;
	OutputFile << "Total Number of Task: " << m_iTaskNum << endl;
	OutputFile << "Total Number of HI Task: " << m_iHITaskNum << endl;
	OutputFile << "Total Utilization: " << m_dTotalUtilization << endl;
	OutputFile << "Total Available Memory: " << m_dAvailableMemory << endl;
	OutputFile << endl << "Tasks Info:" << endl;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		OutputFile << m_pcTaskArray[i];
	}
	WriteExtendedParameterText(OutputFile);
}

void TaskSet::WriteTextFile(char axFileName[])
{
	ofstream ofstreamOutputFile(axFileName, ios::out);
	WriteTextFile(ofstreamOutputFile);
	ofstreamOutputFile.close();
}

void TaskSet::WriteBasicTextFile(char axFileName[])
{
	ofstream OutputFile(axFileName, ios::out);
	OutputFile << "Task Set Info:" << endl;
	OutputFile << "Total Number of Task: " << m_iTaskNum << endl;
	OutputFile << "Total Number of HI Task: " << m_iHITaskNum << endl;
	OutputFile << "Total Utilization: " << m_dTotalUtilization << endl;
	OutputFile << "Total Available Memory: " << m_dAvailableMemory << endl;
	OutputFile << endl << "Tasks Info:" << endl;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		OutputFile << m_pcTaskArray[i];
	}
	OutputFile.close();
}

double TaskSet::getWorstDelay()
{
	double fWorstDelay = 0;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		set<Task::CommunicationLink> & rsetSucessor = m_pcTaskArray[i].getSuccessorSet();
		for (set<Task::CommunicationLink>::iterator iter = rsetSucessor.begin();
			iter != rsetSucessor.end(); iter++)
		{
			int iSrcIndex = i;
			int iDstIndex = iter->m_iTargetTask;
			if (m_pcTaskArray[iSrcIndex].getPeriod() >= m_pcTaskArray[iDstIndex].getPeriod())
			{
				fWorstDelay += iter->m_dDelayCost;
			}
		}
	}
	return fWorstDelay;
}

double TaskSet::getSumDelayCost()
{
	double fWorstDelay = 0;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		set<Task::CommunicationLink> & rsetSucessor = m_pcTaskArray[i].getSuccessorSet();
		for (set<Task::CommunicationLink>::iterator iter = rsetSucessor.begin();
			iter != rsetSucessor.end(); iter++)
		{
			int iSrcIndex = i;
			int iDstIndex = iter->m_iTargetTask;
			//if (m_pcTaskArray[iSrcIndex].getPeriod() >= m_pcTaskArray[iDstIndex].getPeriod())
			{
				fWorstDelay += iter->m_dDelayCost;
			}
		}
	}
	return fWorstDelay;

}

void TaskSet::NameTasksByIndex()
{
	char axTemp[128] = { 0 };
	for (int i = 0; i < m_iTaskNum; i++)
	{
		sprintf_s(axTemp, "%d", i);
		m_pcTaskArray[i].setName(axTemp);
	}
}

Task * TaskSet::FindTask(string stringName)
{	
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (m_pcTaskArray[i].getName().compare(stringName) == 0)
		{
			return &m_pcTaskArray[i];
		}
	}
	return NULL;
}

Task * TaskSet::FindTask(char axName[])
{
	return FindTask(string(axName));
}

Task * TaskSet::FindTask(const char axName[])
{
	return FindTask(string(axName));
}

int TaskSet::getHyperPeriod()
{
	int iHyperPeriod = (int)m_pcTaskArray[0].getPeriod();
	for (int i = 0; i < m_iTaskNum; i++)
	{		
		iHyperPeriod = lcm(iHyperPeriod, (int)m_pcTaskArray[i].getPeriod());		
	}	
	return iHyperPeriod;
}

TaskSetPriorityStruct TaskSet::GenDMPriroityAssignment()
{
	class Comp
	{		
	public:
		TaskSet * m_pcTaskSet;
		bool operator () (int iLHS, int iRHS)
		{
			GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
			
			if (pcTaskArray[iLHS].getDeadline() == pcTaskArray[iRHS].getDeadline())
			{
				return iLHS < iRHS;
			}
			return pcTaskArray[iLHS].getDeadline() < pcTaskArray[iRHS].getDeadline();
		}
	};
	Comp cComp;
	cComp.m_pcTaskSet = this;
	set<int, Comp> cSort(cComp);
	GET_TASKSET_NUM_PTR(this, iTaskNum, pcTaskArray);
	TaskSetPriorityStruct cRet(*this);
	int iPriority = 0;
	for (int i = 0; i < iTaskNum; i++) cSort.insert(i);
	for (set<int, Comp>::iterator iter = cSort.begin(); iter != cSort.end(); iter++)
	{
		cRet.setPriority(*iter, iPriority++);
	}
	return cRet;
}

TaskSetPriorityStruct TaskSet::GenRMPriorityAssignment()
{
	GET_TASKSET_NUM_PTR(this, iTaskNum, pcTaskArray);
	TaskSetPriorityStruct cRet(*this);
	for (int i = 0; i < iTaskNum; i++)
	{
		int iTask = m_ppcTasksArrayByPeriod[i]->getTaskIndex();
		cRet.setPriority(iTask, i);
	}
	return cRet;
}

#if OLDTASKPASTRUCT

TaskSetPriorityStruct::TaskSetPriorityStruct()
{	
	m_pcTaskSet = NULL;
}

TaskSetPriorityStruct::TaskSetPriorityStruct(TaskSet & rcTaskSet)
{
	m_pcTaskSet = &rcTaskSet;	
	int iTaskNum = rcTaskSet.getTaskNum();
	m_vectorPriority2Task.reserve(iTaskNum);
	m_vectorTask2Priority.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorPriority2Task.push_back(-1);
		m_vectorTask2Priority.push_back(-1);
	}	
}

int TaskSetPriorityStruct::unset(int iTaskIndex)
{
	int iTaskNum = m_pcTaskSet->getTaskNum();
	int iPriority = m_vectorTask2Priority[iTaskIndex];	
	assert(iTaskIndex < iTaskNum);
	m_vectorTask2Priority[iTaskIndex] = -1;
	if (iPriority != -1)
		m_vectorPriority2Task[iPriority] = -1;
	return 0;	
}

void TaskSetPriorityStruct::Clear()
{
	assert(m_pcTaskSet);
	int iTaskNum = m_pcTaskSet->getTaskNum();
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorPriority2Task[i] = -1;
		m_vectorTask2Priority[i] = -1;
	}
}

int TaskSetPriorityStruct::getSize()
{
	int iSize = 0;
	int iTaskNum = m_pcTaskSet->getTaskNum();
	
	for (int i = 0; i < iTaskNum; i++)
	{
		if (m_vectorTask2Priority[i] != -1)
			iSize++;
	}
	return iSize;
}

void TaskSetPriorityStruct::Print()
{
	int iTaskNum = m_pcTaskSet->getTaskNum();
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "Task " << i << ": " << m_vectorTask2Priority[i] << endl;
	}
}

void TaskSetPriorityStruct::PrintByPriority()
{
	int iTaskNum = m_pcTaskSet->getTaskNum();
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "Priority " << i << ": " << m_vectorPriority2Task[i] << endl;
	}
}

void TaskSetPriorityStruct::CopyFrom_Strict(TaskSetPriorityStruct & rcOther)
{
	assert(rcOther.m_pcTaskSet == m_pcTaskSet);
	int iTaskNum = m_pcTaskSet->getTaskNum();
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorPriority2Task[i] = rcOther.getTaskByPriority(i);
		m_vectorTask2Priority[i] = rcOther.getPriorityByTask(i);
	}
}

void TaskSetPriorityStruct::WriteTextFile(char axFileName[])
{
	assert(m_pcTaskSet);
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	ofstream ofstreamOutFile(axFileName, ios::out);
	ofstreamOutFile << iTaskNum << endl;
	for (int i = 0; i < iTaskNum; i++)
	{
		ofstreamOutFile << "Task " << i << " : " << m_vectorTask2Priority[i] << endl;
	}
	ofstreamOutFile.close();
}

void TaskSetPriorityStruct::ReadTextFile(char axFileName[])
{
	Clear();
	ifstream ifstreamInFile(axFileName, ios::in);
	int iTaskNum = 0;
	ifstreamInFile >> iTaskNum;
	m_vectorTask2Priority.clear();
	m_vectorPriority2Task.clear();
	m_vectorTask2Priority.reserve(iTaskNum);
	m_vectorPriority2Task.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorPriority2Task.push_back(-1);
		m_vectorTask2Priority.push_back(-1);
	}

	for (int i = 0; i < iTaskNum; i++)
	{
		int iTaskIndex = 0;
		int iPriority = 0;
		string stringDummy, stringDummy2;
		ifstreamInFile >> stringDummy >> iTaskIndex >> stringDummy2 >> iPriority;
		m_vectorPriority2Task[iPriority] = iTaskIndex;
		m_vectorTask2Priority[iTaskIndex] = iPriority;
	}
}

bool TaskSetPriorityStruct::VerifyConsistency()
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		int iTask = i;
		int iPriority = m_vectorTask2Priority[i];
		if (m_vectorPriority2Task[iPriority] != iTask)
		{			
			PrintByPriority();
			return false;
		}
	}
	return true;
}

void TaskSetPriorityStruct::getHPSet(int iTaskIndex, MySet<int> & rsetHPSet)
{
	assert(m_vectorPriority2Task.size() == m_vectorTask2Priority.size());
	assert(m_vectorPriority2Task.size() > iTaskIndex);
	int iTaskPriority = m_vectorTask2Priority[iTaskIndex];
	for (int iHP = iTaskPriority - 1; iHP >= 0; iHP--)
	{
		rsetHPSet.insert(m_vectorPriority2Task[iHP]);
	}
}

void TaskSetPriorityStruct::getHPESet(int iTaskIndex, MySet<int> & rsetHPESet)
{
	getHPSet(iTaskIndex, rsetHPESet);
	rsetHPESet.insert(iTaskIndex);
}

void TaskSetPriorityStruct::getLPSet(int iTaskIndex, MySet<int> & rsetLPSet)
{
	assert(m_vectorPriority2Task.size() == m_vectorTask2Priority.size());
	assert(m_vectorPriority2Task.size() > iTaskIndex);
	int iTaskPriority = m_vectorTask2Priority[iTaskIndex];
	for (int iHP = iTaskPriority - 1; iHP >= 0; iHP--)
	{
		rsetLPSet.insert(m_vectorPriority2Task[iHP]);
	}
}

void TaskSetPriorityStruct::getLPESet(int iTaskIndex, MySet<int> & rsetLPESet)
{
	getLPSet(iTaskIndex, rsetLPESet);
	rsetLPESet.insert(iTaskIndex);
}
#else
#endif

//TaskSetExt_LinkCriticalSection

TaskSetExt_LinkCritialSection::TaskSetExt_LinkCritialSection()
	:TaskSet()
{

}

TaskSetExt_LinkCritialSection::~TaskSetExt_LinkCritialSection()
{

}

void TaskSetExt_LinkCritialSection::WriteExtendedParameterImage(ofstream & OutputFile)
{
	OutputFile.write("LinkCriticalSectionInfo", sizeof("LinkCriticalSectionInfo"));
	int iValue = m_dequeLinkCriticalSectionInfo.size();
	OutputFile.write((const char *)&iValue, sizeof(int));
	for (deque<LinkCriticalSectionInfo>::iterator iter = m_dequeLinkCriticalSectionInfo.begin();
		iter != m_dequeLinkCriticalSectionInfo.end(); iter++)
	{
		OutputFile.write((const char *)&(*iter), sizeof(LinkCriticalSectionInfo));
	}
}

void TaskSetExt_LinkCritialSection::WriteExtendedParameterText(ofstream & OuputFile)
{
	char axBuffer[512] = { 0 };
	OuputFile << "Communication Link Critical Section Length" << endl;	
	for (deque<LinkCriticalSectionInfo>::iterator iter = m_dequeLinkCriticalSectionInfo.begin();
		iter != m_dequeLinkCriticalSectionInfo.end(); iter++)
	{
		sprintf_s(axBuffer, "Link(%d, %d): Read = %.2f, Write = %.2f", iter->iSource, iter->iDestination, iter->dReadCSLen, iter->dWriteCSLen);
		OuputFile << axBuffer << endl;
	}
}

void TaskSetExt_LinkCritialSection::ReadExtendedParameterImage(ifstream & InputFile)
{
	char axBuffer[512] = { 0 };
	InputFile.read(axBuffer, sizeof("LinkCriticalSectionInfo"));	
	if (strcmp(axBuffer, "LinkCriticalSectionInfo") != 0)
	{
		return;
		cout << "Missing LinkCriticalSectionInfo Segment" << endl;
		while (1);
	}

	int iSize = 0;
	InputFile.read((char *)&iSize, sizeof(int));
	for (int i = 0; i < iSize; i++)
	{
		LinkCriticalSectionInfo cCSInfo;
		InputFile.read((char *)&cCSInfo, sizeof(LinkCriticalSectionInfo));
		m_dequeLinkCriticalSectionInfo.push_back(cCSInfo);
	}
}

TaskSetExt_LinkCritialSection::LinkCriticalSectionInfo TaskSetExt_LinkCritialSection::getLinkCriticalSectionInfo(int iSource, int iDesination)
{
	for (deque<LinkCriticalSectionInfo>::iterator iter = m_dequeLinkCriticalSectionInfo.begin(); iter != m_dequeLinkCriticalSectionInfo.end(); iter++)
	{
		if ((iter->iSource == iSource) && (iter->iDestination == iDesination))
		{
			return *iter;
		}
	}

	cout << "Cannot find the link cs info" << endl;
	cout << iSource << " " << iDesination << endl;
	while (1);

	return LinkCriticalSectionInfo();
}
