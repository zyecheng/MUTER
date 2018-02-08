#pragma once
#include "ResponseTimeCalculator.h"
#include "MCRbfUBCalculator.h"
#include "TPCCSet.h"
#include "Util.h"
#include <map>



class SchedTest_AMCMax
{
protected:
	ResponseTimeCalculator m_cRtCalculator;
public:
	SchedTest_AMCMax()	{}
	~SchedTest_AMCMax() {};
	bool operator () (int iTaskindex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
protected:
	void Initialize(TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize);
};

class SchedTest_AMCRTB :
	public SchedTest_AMCMax
{
public:
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
};

class SchedTest_LO :
	public SchedTest_AMCMax
{
public:
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
};

class SchedTest_RbfLUB
{
protected:
	MCRbfUBCalculator m_cCalculator;
public:
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
protected:
	void Initialize(TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);

};

class SchedTest_RbfPLUB
	:public SchedTest_RbfLUB
{
public:
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
};

class SchedTest_AMCSepRbf
{
protected:
	ResponseTimeCalculator m_cRTCalculator;
	MCRbfUBCalculator m_cRbfCalculator;	
public:
	SchedTest_AMCSepRbf();
	~SchedTest_AMCSepRbf();
	void Initialize(TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
};

class SchedTest_AMCSepRbf_Switched
	:public SchedTest_AMCSepRbf
{
public:
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
};

class SchedTest_MILPRbf
	:public SchedTest_AMCSepRbf
{
public:
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
};

class SchedTest_MILPRbf_RTLOBound
	:public SchedTest_AMCSepRbf
{
public:
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
};

class SchedTest_MILPRbf_WCBound
	:public SchedTest_AMCSepRbf
{
public:
	bool operator () (int iTaskIndex, TaskSet * pcTaskSet, int * piPrioirtyTable, int iSize, void * pvExtraArg);
private:
	double WorstCaseBound(int iTaskIndex, TaskSet * pcTaskSet, int * piPriorityTable, double dTotalTime, double dLOResponseTime);

};

template<class T = SchedTest_AMCMax>
class AudsleyEngine	
{
#define AUDSLEY_SCHEDTYPE_LO	0
#define AUDSLEY_SCHEDTYPE_MIXCRITI	1
private:		
	bool m_bIsSchedulable;
	TaskSet * m_pcTaskSet;
	void * m_pvExtraArg;
	int * m_piPriorityTable;
public:
	AudsleyEngine()
	{		
		m_bIsSchedulable = false;
		m_piPriorityTable = NULL;
		m_pvExtraArg = NULL;
	}

	AudsleyEngine(TaskSet & rcTaskSet, void * pvExtraArg)
	{
		Initialize(rcTaskSet, pvExtraArg);
		m_piPriorityTable = NULL;
		m_bIsSchedulable = false;
		m_pvExtraArg = pvExtraArg;
	}

	~AudsleyEngine()	
	{
		ClearPriorityTable();
	}

	int getPriority(int iTaskIndex)
	{
		return m_piPriorityTable[iTaskIndex];
	}

	int * getPriorityTable()	{ return m_piPriorityTable; }

	void Initialize(TaskSet & rcTaskSet, void * pvExtraArg)
	{
		m_pcTaskSet = &rcTaskSet;
		m_pvExtraArg = pvExtraArg;
		CreatePriorityTable();
	}

	bool Run()
	{		
		my_assert(m_pcTaskSet != NULL, "In Run. AudsleyEngine Isn't Initialized Yet");
		m_bIsSchedulable = AudsleyAlgorithm();
		return m_bIsSchedulable;
	}

	void DisplayResult()
	{
		int iTaskNum = m_pcTaskSet->getTaskNum();
		if (m_bIsSchedulable)
		{
			for (int i = 0; i < iTaskNum; i++)
			{
				cout << "Task Index: " << i << " / Priority: " << m_piPriorityTable[i];
				cout << endl;
			}
		}
		else
		{
			cout << "Unschedulable" << endl;
		}
	}
private:
	bool CanSetLowestPriority(int iTaskIndex)
	{
		int iTaskNum = m_pcTaskSet->getTaskNum();
		my_assert(iTaskIndex < iTaskNum, "In CanSetLowestPriority. Task Index Out of Bound");
		my_assert(m_piPriorityTable[iTaskIndex] == 0, "In CanSetLowestPriority. Target Task Already Assigned Priority. Algorithmic Error");
		for (int i = 0; i < iTaskNum; i++)
		{
			if ((m_pcTaskSet->getPredefPriTable()[iTaskIndex][i] == PRIORITY_HIGHER) && (m_piPriorityTable[i] == 0))
			{
				//if there exists a task which has not been assigned a priority but which should have lower priority than this task. 
				//Then this task can not be set to the lowest priority.
				return false;
			}
		}
		return true;
	}

	void CreatePriorityTable()
	{
		my_assert(m_pcTaskSet != NULL, "TaskSet not correlated yet");
		ClearPriorityTable();
		m_piPriorityTable = new int[m_pcTaskSet->getTaskNum()];
		memset(m_piPriorityTable, 0, sizeof(int)*m_pcTaskSet->getTaskNum());		
	}

	void ClearPriorityTable()
	{
		if (m_piPriorityTable)
		{
			delete m_piPriorityTable;
		}
		m_piPriorityTable = NULL;
	}

	bool IsSchedulable(int iTaskIndex)
	{
		return T()(iTaskIndex, m_pcTaskSet, m_piPriorityTable, m_pcTaskSet->getTaskNum(),m_pvExtraArg);
	}

	int getNextCandidateTask(int iStartIndex)
	{
		int iTaskNum = m_pcTaskSet->getTaskNum();
		for (int i = iStartIndex; i < iTaskNum; i++)
		{
			if (m_piPriorityTable[i] == 0)
			{
				if (CanSetLowestPriority(i))
				{
					return i;
				}
			}
		}
		return -1;
	}

	bool AudsleyAlgorithm()
	{
		int iTaskNum = m_pcTaskSet->getTaskNum();
		int iCurrentPriority = iTaskNum;
		int iTargetTask = 0;

		//initialize all priority to 0
		for (int i = 0; i < iTaskNum; i++)
		{
			m_piPriorityTable[i] = 0;
		}

		while (iCurrentPriority != 0)
		{
			iTargetTask = getNextCandidateTask(iTargetTask);
			if (iTargetTask == -1)
			{
				//no more candidate to select
				return false;
			}

			m_piPriorityTable[iTargetTask] = iCurrentPriority;
			if (IsSchedulable(iTargetTask))
			{
				iTargetTask = 0;
				iCurrentPriority--;
			}
			else
			{
				m_piPriorityTable[iTargetTask] = 0;
				iTargetTask++;
			}
		}
		return true;
	}
};


