#pragma once
#include "Task.h"
#include <map>
#include <vector>
#include <assert.h>
#ifndef NOFSM
#include <Stateflow.h>
#include <FileReader.h>
#endif

#include "MySet.h"
class MCMILPModel;
class MCMILPModelW;
class CSpaceRecurEntry;
class ResponseTimeCalculator;
class SchedTest_AMCMax;
class TaskSetPriorityStruct;
class TaskSet
{
#define PRIORITY_HIGHER	1
#define PRIORITY_LOWER	0
#define PRIORITY_UNDEF	-1
#define PERIODOPT_TABLE	0
#define PERIODOPT_TUPLE	1
#define PERIODOPT_LOG	2
#define PERIODOPT_TABLE_V2	3

#define GET_TASKSET_NUM_PTR(pcTaskSet, iTaskNum, pcTaskArray)	\
	int iTaskNum = (pcTaskSet)->getTaskNum();	\
	Task * pcTaskArray = (pcTaskSet)->getTaskArrayPtr()
public:
	typedef Task::CommIter CommIter;
protected:
	//The Task Array
	Task * m_pcTaskArray;	
	Task ** m_ppcTasksArrayByPeriod;
	//Some Statistic About the Task Set
	int m_iTaskNum;
	 double m_dTotalUtilization;	
	int m_iHITaskNum;
	//Predefine Priority Table
	int ** m_ppiPriorityTable;
	//task order by period
	set<Task> m_setTasks;		
	//Some other parameter
	double m_dAvailableMemory;
	double m_dPeriodGCD;
public:
	TaskSet();
	TaskSet(const TaskSet & rcTaskSet);
	~TaskSet();
	void GenRandomTaskset(int iTaskNum, double fTotalUtil, double fHighCritiPercent, double fCriticalityFactor, int iPeriodOpt = 0,bool bConstrainedDeadline = false);
	void GenRandomTasksetMC(int iTaskNum, double fTotalUtil, double fHighCritiPercent, 
		double fCriticalityFactorUB, double fCriticalityFactorLB, int iPeriodOpt = 0, bool bConstrainedDeadline = false);
	void GenRandomTaskSetInteger(int iTaskNum, double fTotalUtil, double fHighCritiPercent, double fCriticalityFactor, int iPeriodOpt = 0, bool bConstrainedDeadline = false);
	void GenRandomTaskSet_AUTOSTART_ProtectionMechanism(int iTaskNum, double dTotalUtil);
	void GenRandomTaskSet_SumHILOUtil(int iTaskNum, double dTotalUtil, double dHighCritiPercent, double dCritialityFactor, int iPeriodOpt = 0, bool bConstrainedDeadline = false);
	void GenRandomTaskSet_SumHILOUtil_Split(int iTaskNum, double dTotalUtil, double dHIUtilPercentage,
		double dHighCritiPercent, double dCritialityFactor, int iPeriodOpt = 0, bool bConstrainedDeadline = false);
	//interface function
	Task * getTaskArrayPtr() const	{ return m_pcTaskArray; }
	Task ** getTaskArrayByPeriosPtr()	{ return m_ppcTasksArrayByPeriod; }
	int getTaskNum() const	{ return m_iTaskNum; }
	double getActualTotalUtil()	{ return m_dTotalUtilization; }
	int getActualHighCritiNum()	{ return m_iHITaskNum; }
	double getAvailableMemory()	{ return m_dAvailableMemory; }
	void setAvailableMemory(double fMemory)	{ m_dAvailableMemory = fMemory; }
	void DisplayTaskInfo();
	void DisplayTaskInfoDetailed();
	//interfaces for building the task set
	void AddTask(double fPeriod,double dDeadline, double fUtil, double CritiFactor, bool bHICriticality,int iIndex = -1);
	void AddTask(Task & rcTask);	
	void RemoveTask(Task & rcTask);
	void AddLink(int iSource, int iDestination, double dMemory, double dDelay);
	void ConstructTaskArray();//when task adding is finished, call this function to stablize the data structure;
	
	void ClearAll();
	void WriteImageFile(ofstream & OutputFile);
	void WriteImageFile(char axFileName[]);
	void ReadImageFile(ifstream & InputFile);
	void ReadImageFile(char axFileName[]);
	void WriteTextFile(ofstream & OutputFile);
	void WriteTextFile(char axFileName[]);
	
	void WriteBasicImageFile(char axFileName[]);	
	void ReadBasicImageFile(char axFileName[]);	
	void WriteBasicTextFile(char axFileName[]);

	double getWorstDelay();
	double getSumDelayCost();	
	int ** getPredefPriTable()	{ return m_ppiPriorityTable; }
	void NameTasksByIndex();
	Task * FindTask(char axName[]);
	Task * FindTask(const char axName[]);
	Task * FindTask(string stringName);
	int getHyperPeriod();
	void RemoveLinkWeight();
	double SmallestOffset(int iTaskA, int iTaskB);
	double getPeriodGCD()	{ return m_dPeriodGCD; }

	//For Randomly Generated TaskSet
	static void GenerateUtilTable(double * pfUtilTable, double fTotalUtil, int iSize);
	static void GeneratePeriodTable(double * pfPeriodTable, int iSize, int iOpt = 0);
	static void GenerateCritiTable(bool * pbCritiTable, int iHITaskNum, int iSize);
	static void GenerateCritiFactorTable(double * pdCritiFactorTable, double dUB, double dLB, int iSize);
	static double GeneratePeriod_Tuple(double fHarmonic = 0);
	static double GeneratePeriod_Predef(double fHarmonic = 0);
	static double GeneratePeriod_Predef_v2(double fHarmonic = 0);
	static double GeneratePeriod_Log();
	static double GenRandDeadline(double dPeriod, double dExecution);

	virtual TaskSetPriorityStruct GenDMPriroityAssignment();
	virtual TaskSetPriorityStruct GenRMPriorityAssignment();
protected:
	void ClearTaskArray();
	
	virtual void WriteExtendedParameterImage(ofstream & OutputFile)	{}
	virtual void ReadExtendedParameterImage(ifstream & InputFile)	{}
	virtual void WriteExtendedParameterText(ofstream & OuputFile)	{}

	friend class MCMILPModel;
	friend class MCMILPModelW;
	friend class ResponseTimeCalculator;	


	//FSM Implementation
#ifndef NOFSM
protected:
	Stateflow ** m_ppcStateFlows;
	vector<double> m_vectorStateFlowMaxWCET;
public:
	void ReadStateflow(char axFileName[]);
	void ReadRawStateflow(char axFileName[]);
	void BuildFSMData();
	Stateflow getFSM(int iTaskIndex);
#endif
};

class CSpaceRecurEntry
{
public:
	int m_iCurrentIndex;
	 double m_dTime;
public:
	CSpaceRecurEntry(int iCurrentIndex, double fTime)
	{
		m_iCurrentIndex = iCurrentIndex;
		m_dTime = fTime;
	}

	friend bool operator < (const CSpaceRecurEntry & rcLhs, const CSpaceRecurEntry & rcRhs)
	{
		if (rcLhs.m_iCurrentIndex < rcRhs.m_iCurrentIndex)
		{
			return true;
		}
		else if (rcLhs.m_iCurrentIndex == rcRhs.m_iCurrentIndex)
		{
			//return rcLhs.m_dTime < rcRhs.m_dTime;
			return (rcLhs.m_dTime - rcRhs.m_dTime) <= -1e-9;
		}
		return false;
	}
};

class TaskIndexPair
{
public:
	int m_iIndexA;
	int m_iIndexB;
public:
	TaskIndexPair(int iIndexA, int iIndexB)
	{
		m_iIndexA = iIndexA;
		m_iIndexB = iIndexB;
	}
	~TaskIndexPair(){}
	friend bool operator < (const TaskIndexPair & rcA, const TaskIndexPair & rcB)
	{
		if (rcA.m_iIndexA == rcB.m_iIndexA)
		{
			return rcA.m_iIndexB < rcB.m_iIndexB;
		}

		return rcA.m_iIndexA < rcB.m_iIndexB;
	}

	
};

#define OLDTASKPASTRUCT 1

#if OLDTASKPASTRUCT 
class TaskSetPriorityStruct
{
protected:
	TaskSet * m_pcTaskSet;
	vector<int> m_vectorTask2Priority;
	vector<int> m_vectorPriority2Task;
public:
	TaskSetPriorityStruct();
	TaskSetPriorityStruct(TaskSet & rcTaskSet);
	~TaskSetPriorityStruct()	{}	
	int setPriority(int iTaskIndex, int iPriority);		
	int unset(int iTaskIndex);	
	int getTaskByPriority(int iPriority);
	int getPriorityByTask(int iTask);
	void Clear();
	int getSize();
	void Print();
	void PrintByPriority();
	void CopyFrom_Strict(TaskSetPriorityStruct & rcOther);
	void WriteTextFile(char axFileName[]);
	void ReadTextFile(char axFileName[]);
	void Swap(int iTaskA, int iTaskB);
	void PutImmediatelyAfter(int iTaskA, int iTaskB);
	void PutImmediatelyBefore(int iTaskA, int iTaskB);
	bool VerifyConsistency();
	void getHPSet(int iTaskIndex, MySet<int> & rsetHPSet);
	void getHPESet(int iTaskIndex, MySet<int> & rsetHPESet);
	void getLPSet(int iTaskIndex, MySet<int> & rsetHPSet);
	void getLPESet(int iTaskIndex, MySet<int> & rsetHPSet);
};

inline int TaskSetPriorityStruct::getPriorityByTask(int iTask)
{
	assert(m_pcTaskSet);
	assert(iTask < m_pcTaskSet->getTaskNum());
	return m_vectorTask2Priority[iTask];
}

inline int TaskSetPriorityStruct::getTaskByPriority(int iPriority)
{
	assert(m_pcTaskSet);
	assert(iPriority < m_pcTaskSet->getTaskNum());
	return m_vectorPriority2Task[iPriority];
}

inline int TaskSetPriorityStruct::setPriority(int iTaskIndex, int iPriority)
{
	assert(m_pcTaskSet);
	int iTaskNum = m_pcTaskSet->getTaskNum();
	assert(iPriority < iTaskNum);
	assert(iTaskIndex < iTaskNum);
	int iTaskAtThisPri = m_vectorPriority2Task[iPriority];
	assert(iTaskAtThisPri == -1);

	m_vectorTask2Priority[iTaskIndex] = iPriority;
	m_vectorPriority2Task[iPriority] = iTaskIndex;
	return iTaskIndex;
}

inline void TaskSetPriorityStruct::Swap(int iTaskA, int iTaskB)
{
	int iPriA = m_vectorTask2Priority[iTaskA];
	int iPriB = m_vectorTask2Priority[iTaskB];
	unset(iTaskA);
	unset(iTaskB);
	if (iPriA != -1)
	{
		setPriority(iTaskB, iPriA);
	}

	if (iPriB != -1)
	{
		setPriority(iTaskA, iPriB);
	}

}

inline void TaskSetPriorityStruct::PutImmediatelyAfter(int iTaskA, int iTaskB)
{
	int iOriginalTaskAPriority = m_vectorTask2Priority[iTaskA];
	int iOriginalTaskBPriority = m_vectorTask2Priority[iTaskB];
	assert(iOriginalTaskAPriority != -1);
	assert(iOriginalTaskBPriority != -1);
	assert(iOriginalTaskBPriority != iOriginalTaskAPriority);
	if (iOriginalTaskAPriority < iOriginalTaskBPriority)
	{			
		for (int iPLevel = iOriginalTaskAPriority + 1; iPLevel <= iOriginalTaskBPriority; iPLevel++)
		{
			int iThisTask = m_vectorPriority2Task[iPLevel];
			m_vectorTask2Priority[iThisTask]--;
			m_vectorPriority2Task[m_vectorTask2Priority[iThisTask]] = iThisTask;
		}
		m_vectorTask2Priority[iTaskA] = iOriginalTaskBPriority;
		m_vectorPriority2Task[iOriginalTaskBPriority] = iTaskA;
	}
	else
	{		
		//for (int iPLevel = iOriginalTaskBPriority + 1; iPLevel < iOriginalTaskAPriority; iPLevel++)
		for (int iPLevel = iOriginalTaskAPriority - 1; iPLevel > iOriginalTaskBPriority; iPLevel--)
		{
			int iThisTask = m_vectorPriority2Task[iPLevel];
			m_vectorTask2Priority[iThisTask]++;
			m_vectorPriority2Task[m_vectorTask2Priority[iThisTask]] = iThisTask;
		}
		m_vectorTask2Priority[iTaskA] = iOriginalTaskBPriority + 1;
		m_vectorPriority2Task[m_vectorTask2Priority[iTaskA]] = iTaskA;
	}
	//assert(VerifyConsistency());
}

inline void TaskSetPriorityStruct::PutImmediatelyBefore(int iTaskA, int iTaskB)
{
	int iOriginalTaskAPriority = m_vectorTask2Priority[iTaskA];
	int iOriginalTaskBPriority = m_vectorTask2Priority[iTaskB];
	assert(iOriginalTaskAPriority != -1);
	assert(iOriginalTaskBPriority != -1);
	assert(iOriginalTaskBPriority != iOriginalTaskAPriority);
	if (iOriginalTaskAPriority < iOriginalTaskBPriority)
	{
		for (int iPLevel = iOriginalTaskAPriority + 1; iPLevel < iOriginalTaskBPriority; iPLevel++)
		{
			int iThisTask = m_vectorPriority2Task[iPLevel];
			m_vectorTask2Priority[iThisTask]--;
			m_vectorPriority2Task[m_vectorTask2Priority[iThisTask]] = iThisTask;
		}
		m_vectorTask2Priority[iTaskA] = iOriginalTaskBPriority - 1;
		m_vectorPriority2Task[iOriginalTaskBPriority - 1] = iTaskA;
	}
	else
	{				
		for (int iPLevel = iOriginalTaskAPriority - 1; iPLevel > iOriginalTaskBPriority - 1; iPLevel--)
		{			
			int iThisTask = m_vectorPriority2Task[iPLevel];
			m_vectorTask2Priority[iThisTask]++;
			m_vectorPriority2Task[m_vectorTask2Priority[iThisTask]] = iThisTask;			
		}
		m_vectorTask2Priority[iTaskA] = iOriginalTaskBPriority;
		m_vectorPriority2Task[m_vectorTask2Priority[iTaskA]] = iTaskA;
	}
	//assert(VerifyConsistency());
}

#else
#endif

class TaskSetExt_LinkCritialSection : public TaskSet
{
public:
	struct LinkCriticalSectionInfo
	{
		int iSource;
		int iDestination;
		double dWriteCSLen;
		double dReadCSLen;
	};
	deque<LinkCriticalSectionInfo> m_dequeLinkCriticalSectionInfo;

	TaskSetExt_LinkCritialSection();
	~TaskSetExt_LinkCritialSection();
	void WriteExtendedParameterImage(ofstream & OutputFile);
	void ReadExtendedParameterImage(ifstream & InputFile);
	void WriteExtendedParameterText(ofstream & OuputFile);
	LinkCriticalSectionInfo getLinkCriticalSectionInfo(int iSource, int iDesination);
};