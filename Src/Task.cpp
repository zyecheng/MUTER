#include "stdafx.h"
#include "Task.h"
extern void my_assert(bool bCondition, char axInfo[]);

Task::Task()
{
	m_dPeriod = 0;
	m_dUtilization = 0;
	m_dCriticalityFactor = 0;
	m_bCriticalityLevel = false;
	m_iIndex = -1;
}
#if 0
Task::Task(double fPeriod, double fUtilization, double fCriticalityFactor, bool bCriticalityLevel,int iTaskIndex)
{
	m_dPeriod = fPeriod;
	m_dUtilization = fUtilization;
	m_dCriticalityFactor = fCriticalityFactor;
	m_bCriticalityLevel = bCriticalityLevel;
	m_dDeadline = m_dPeriod;
	m_iIndex = iTaskIndex;
	Construct();
}
#endif
Task::Task(double dPeriod, double dDeadline, double dUtil, double dCritiFactor, bool bCriticality, int iTaskIndex)
{
	m_dPeriod = dPeriod;
	m_dUtilization = dUtil;
	m_dCriticalityFactor = dCritiFactor;
	m_bCriticalityLevel = bCriticality;
	m_iIndex = iTaskIndex;
	m_dDeadline = dDeadline;
	Construct();
}


Task::~Task()
{
}

void Task::Construct()
{
	m_dCLO = m_dPeriod * m_dUtilization;
	m_dCHI = m_dCLO * m_dCriticalityFactor;
}

void Task::AddPredecessor(int iTargetTaskIndex, double fMemoryCost, double fDelayCost)
{
	std::pair<set<Task::CommunicationLink>::iterator,bool> cInsertStatus = 
		m_setPredecessor.insert(CommunicationLink(iTargetTaskIndex, fMemoryCost, fDelayCost));
	if (!cInsertStatus.second)
	{
		m_setPredecessor.erase(cInsertStatus.first);
		m_setPredecessor.insert(CommunicationLink(iTargetTaskIndex, fMemoryCost, fDelayCost));
	}
}

void Task::AddSuccessor(int iTargetTaskIndex, double fMemoryCost, double fDelayCost)
{
	std::pair<set<Task::CommunicationLink>::iterator, bool> cInsertStatus =
		m_setSuccessor.insert(CommunicationLink(iTargetTaskIndex, fMemoryCost, fDelayCost));
	if (!cInsertStatus.second)
	{
		m_setSuccessor.erase(cInsertStatus.first);
		m_setSuccessor.insert(CommunicationLink(iTargetTaskIndex, fMemoryCost, fDelayCost));
	}
}

std::ostream & operator << (std::ostream & ostreamOutput, const Task & rcTask)
{
	char axTempBuffer[128] = { 0 };
	ostreamOutput << "****************************************************" << endl;
	ostreamOutput << "Task: " << rcTask.m_iIndex << endl;
	//if (m_bCriticalityLevel)
	//ostreamOutput <<
	ostreamOutput << "Criticality: " << (rcTask.m_bCriticalityLevel ? "HI" : "LO") << endl;
	ostreamOutput << "Period: " << rcTask.m_dPeriod << endl;
	ostreamOutput << "Deadline: " << rcTask.m_dDeadline << endl;
	ostreamOutput << "Utilization: " << rcTask.m_dUtilization << " / CLO: " << rcTask.m_dCLO;
	if (rcTask.m_bCriticalityLevel)	ostreamOutput << " / CHI: " << rcTask.m_dCHI;
	ostreamOutput << endl;
	ostreamOutput << "Predecessor(Memory, Delay): ";
	for (set<Task::CommunicationLink>::iterator iter = rcTask.m_setPredecessor.begin();
			iter != rcTask.m_setPredecessor.end(); iter++)
	{
		sprintf_s(axTempBuffer, "%d(%.2f,%.2f),", iter->m_iTargetTask, iter->m_dMemoryCost, iter->m_dDelayCost);
		ostreamOutput << axTempBuffer;
	}
	ostreamOutput << endl;
	ostreamOutput << "Successor(Memory, Delay): ";
	for (set<Task::CommunicationLink>::iterator iter = rcTask.m_setSuccessor.begin();
			iter != rcTask.m_setSuccessor.end(); iter++)
	{
		sprintf_s(axTempBuffer, "%d(%.2f,%.2f),", iter->m_iTargetTask, iter->m_dMemoryCost, iter->m_dDelayCost);
		ostreamOutput << axTempBuffer;
	}
	ostreamOutput << endl;

	return ostreamOutput;
}

bool operator < (const Task & rcLhs, const Task & rcRhs)
{
	if (rcLhs.getPeriod() < rcRhs.getPeriod())
	{
		return true;
	}
	else if (rcLhs.getPeriod() == rcRhs.getPeriod())
	{
		if (rcRhs.getCriticality() && rcLhs.getCriticality())
			return rcLhs.getTaskIndex() < rcRhs.getTaskIndex();
		else if (rcLhs.getCriticality())
			return true;
		else if (rcRhs.getCriticality())
			return false;
		else
			return rcLhs.getTaskIndex() < rcRhs.getTaskIndex();
	}
	return false;
}

void Task::WriteImage(ofstream & OutputFile)
{
	OutputFile.write((const char *)& m_iIndex, sizeof(int));
	OutputFile.write((const char *)& m_dPeriod, sizeof(double));
	OutputFile.write((const char *)& m_dDeadline, sizeof(double));
	OutputFile.write((const char *)& m_dUtilization, sizeof(double));
	OutputFile.write((const char *)& m_dCriticalityFactor, sizeof(double));
	OutputFile.write((const char *)& m_bCriticalityLevel, sizeof(bool));

	//write successor
	int iSize = m_setSuccessor.size();
	OutputFile.write((const char *)& iSize, sizeof(int));
	for (set<CommunicationLink>::iterator iter = m_setSuccessor.begin();
		iter != m_setSuccessor.end(); iter++)
	{
		int iIndex = iter->m_iTargetTask;
		double fDelayCost = iter->m_dDelayCost;
		double fMemoryCost = iter->m_dMemoryCost;
		OutputFile.write((const char *)& iIndex, sizeof(int));
		OutputFile.write((const char *)& fDelayCost, sizeof(double));
		OutputFile.write((const char *)& fMemoryCost, sizeof(double));
	}
	//Write Predecessor
	iSize = m_setPredecessor.size();
	OutputFile.write((const char *)& iSize, sizeof(int));
	for (set<CommunicationLink>::iterator iter = m_setPredecessor.begin();
		iter != m_setPredecessor.end(); iter++)
	{
		int iIndex = iter->m_iTargetTask;
		double fDelayCost = iter->m_dDelayCost;
		double fMemoryCost = iter->m_dMemoryCost;
		OutputFile.write((const char *)& iIndex, sizeof(int));
		OutputFile.write((const char *)& fDelayCost, sizeof(double));
		OutputFile.write((const char *)& fMemoryCost, sizeof(double));
	}
}

void Task::ReadImage(ifstream & InputFile)
{	
	InputFile.read((char *)& m_iIndex, sizeof(int));
	InputFile.read((char *)& m_dPeriod, sizeof(double));
	InputFile.read((char *)& m_dDeadline, sizeof(double));
	InputFile.read((char *)& m_dUtilization, sizeof(double));
	InputFile.read((char *)& m_dCriticalityFactor, sizeof(double));
	InputFile.read((char *)& m_bCriticalityLevel, sizeof(bool));

	//write successor
	int iSize = 0;
	InputFile.read((char *)& iSize, sizeof(int));
	for (int i = 0; i < iSize;i++)
	{
		int iIndex = 0;
		double fDelayCost = 0;
		double fMemoryCost = 0;
		InputFile.read((char *)& iIndex, sizeof(int));
		InputFile.read((char *)& fDelayCost, sizeof(double));
		InputFile.read((char *)& fMemoryCost, sizeof(double));
		AddSuccessor(iIndex, fMemoryCost, fDelayCost);
	}
	//Write Predecessor
	iSize = 0;
	InputFile.read((char *)& iSize, sizeof(int));
	for (int i = 0; i < iSize; i++)
	{
		int iIndex = 0;
		double fDelayCost = 0;
		double fMemoryCost = 0;
		InputFile.read((char *)& iIndex, sizeof(int));
		InputFile.read((char *)& fDelayCost, sizeof(double));
		InputFile.read((char *)& fMemoryCost, sizeof(double));
		AddPredecessor(iIndex, fMemoryCost, fDelayCost);
	}
}

int Task::getOutputLink(int iTaskIndex, CommunicationLink & rcLink)
{
	set<CommunicationLink>::iterator iterFind = m_setSuccessor.find({ iTaskIndex, 0, 0 });
	if (iterFind != m_setSuccessor.end())
	{
		rcLink = *iterFind;
		return 1;
	}
	return 0;
}

int Task::getInputLink(int iTaskIndex, CommunicationLink & rcLink)
{
	set<CommunicationLink>::iterator iterFind = m_setPredecessor.find({ iTaskIndex, 0, 0 });
	if (iterFind != m_setPredecessor.end())
	{
		rcLink = *iterFind;
		return 1;
	}
	return 0;
}
