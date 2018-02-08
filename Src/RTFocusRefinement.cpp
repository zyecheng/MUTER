#include "stdafx.h"
#include "RTFocusRefinement.h"
#include "ResponseTimeCalculator.h"
#include "StatisticSet.h"
#include <assert.h>
#include "Timer.h"
#ifdef LINUX
#include <unistd.h>
#else
#include <direct.h>
#endif

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

WeightedTaskSet::WeightedTaskSet()
{

}

WeightedTaskSet::~WeightedTaskSet()
{

}

void WeightedTaskSet::setWeightTable(vector<double> & rvectorWeightTable)
{
	assert(m_pcTaskArray);
	assert(rvectorWeightTable.size() == m_iTaskNum);
	m_vectorWeightTable = rvectorWeightTable;
}

void WeightedTaskSet::GenRandomWeight(int iRange /* = 10000 */)
{
	m_vectorWeightTable.clear();
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_vectorWeightTable.push_back((rand() % iRange) + 1);
	}
}

void WeightedTaskSet::WriteExtendedParameterImage(ofstream & OutputFile)
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		OutputFile.write((const char *)&m_vectorWeightTable[i], sizeof(double));
	}
}

void WeightedTaskSet::ReadExtendedParameterImage(ifstream & InputFile)
{
	m_vectorWeightTable.clear();
	assert(m_iTaskNum > 0);
	m_vectorWeightTable.reserve(m_iTaskNum);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		double dWeight = 0;
		InputFile.read((char *)&dWeight, sizeof(double));
		m_vectorWeightTable.push_back(dWeight);
	}
}

void WeightedTaskSet::WriteExtendedParameterText(ofstream & OuputFile)
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		OuputFile << "Task " << i << " weight: " << m_vectorWeightTable[i] << endl;
	}
}

double WeightedTaskSet::getWeight(int iTaskIndex)
{
	assert(iTaskIndex < m_iTaskNum);
	assert(iTaskIndex < m_vectorWeightTable.size());
	return m_vectorWeightTable[iTaskIndex];
}

RTRange::RTRange()
{

}

RTRange::RTRange(int iTaskIndex, double dUpperBound)
{
	m_iTaskIndex = iTaskIndex;
	m_dUpperBound = dUpperBound;
}

bool operator < (const RTRange & rcLHS, const RTRange & rcRHS)
{
	return rcLHS.m_iTaskIndex < rcRHS.m_iTaskIndex;
}

ostream & operator << (ostream & os, const RTRange & rcRHS)
{
	//os << "(" << rcRHS.m_iTaskIndex << ", " << rcRHS.m_dUpperBound << ")";
	char axBuffer[512] = { 0 };
	sprintf_s(axBuffer, "(%d, %.2f)", rcRHS.m_iTaskIndex, rcRHS.m_dUpperBound);
	os << axBuffer;
	return os;
}

bool RTRangeUniqueComparator::operator () (const RTRange & rcLHS, const RTRange & rcRHS)
{
	if (rcLHS.m_iTaskIndex == rcRHS.m_iTaskIndex)
	{
		return rcLHS.m_dUpperBound < rcRHS.m_dUpperBound;
	}
	else
	{
		return rcLHS.m_iTaskIndex < rcRHS.m_iTaskIndex;
	}
	return false;
}

RTUnschedCoreComputer::RTUnschedCoreComputer()
{

}

RTUnschedCoreComputer::RTUnschedCoreComputer(System & rcSystem)
	:GeneralUnschedCoreComputer(rcSystem)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorRTLimits.push_back(-1);		
	}
}

bool RTUnschedCoreComputer::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
#if 0
	//AMC-rtb Temp
	_AMCRtbRTCalculator cRTCalc(*m_pcTaskSet);
	double dRT = cRTCalc.CalculateRTTask(iTaskIndex, rcPriorityAssignment);
	bool bStatus = DOUBLE_GE(m_vectorRTLimits[iTaskIndex], dRT, 1e-8);
	return bStatus;
#endif

	{
		_LORTCalculator cRTCalc(*m_pcTaskSet);
		double dRT = cRTCalc.CalculateRTTask(iTaskIndex, rcPriorityAssignment);
		m_dLastRT = dRT;
		bool bStatus = DOUBLE_GE(m_vectorRTLimits[iTaskIndex], dRT, 1e-8);
		return bStatus;
	}
}

void RTUnschedCoreComputer::IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		//m_vectorRTLimits[i] = pcTaskArray[i].getDeadline();
		assert(m_cTasksRTSamples[i].empty() == false);
		m_vectorRTLimits[i] = min(pcTaskArray[i].getDeadline(), *m_cTasksRTSamples[i].crbegin());
	}

	for (SchedConds::iterator iter = rcSchedCondsFlex.begin(); iter != rcSchedCondsFlex.end(); iter++)
	{
		m_vectorRTLimits[iter->m_iTaskIndex] = min(iter->m_dUpperBound, m_vectorRTLimits[iter->m_iTaskIndex]);
	}

	for (SchedConds::iterator iter = rcSchedCondsFixed.begin(); iter != rcSchedCondsFixed.end(); iter++)
	{
		m_vectorRTLimits[iter->m_iTaskIndex] = min(iter->m_dUpperBound, m_vectorRTLimits[iter->m_iTaskIndex]);
	}
}

void RTUnschedCoreComputer::ConvertToMUS_Schedulable(RTRange & rcErased, SchedConds & rcSchedCondsFlex, SchedConds & rcShcedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcErased.m_iTaskIndex;
	double dLB = rcErased.m_dUpperBound;
	double dUB = pcTaskArray[iTaskIndex].getDeadline();
	//cout << rcErased << endl;
	//cout << dLB << " " << dUB << endl;
//	PrintSetContent(rcSchedCondsFlex.getSetContent());
	TaskSetPriorityStruct cPriorityAssignmentThis(*m_pcTaskSet);
	TaskSetPriorityStruct & cPriorityAssignmentLastUnsched = rcPriorityAssignment;
	cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
	MySet<double> & rsetRTSamples = m_cTasksRTSamples[iTaskIndex];
	assert(rsetRTSamples.empty() == false);
	while (rsetRTSamples.lower_bound(dLB) != rsetRTSamples.lower_bound(dUB))
	{
		double dCurrent = (dUB + dLB) / 2;	
		//cout << dCurrent << " " << dUB << " " << dLB << endl;
		rcSchedCondsFlex.insert({ iTaskIndex, dCurrent });
		bool bStatus = IsSchedulable(rcSchedCondsFlex, rcShcedCondsFixed, cPriorityAssignmentThis);
		rcSchedCondsFlex.erase({ iTaskIndex, dCurrent });
		if (bStatus)
		{
			dUB = dCurrent;
			cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
		}
		else
		{
			dLB = dCurrent;
			cPriorityAssignmentLastUnsched.CopyFrom_Strict(cPriorityAssignmentThis);
		}
	}
//	cout << dUB << endl;
	MySet<double>::iterator iterLB = rsetRTSamples.lower_bound(dUB);
	assert(iterLB != rsetRTSamples.end());
	iterLB--;
	dLB = *iterLB;
	rcSchedCondsFlex.insert({ iTaskIndex, dLB });
}

void RTUnschedCoreComputer::NextCore_PerturbSchedCondEle(SchedCondElement rcFlexEle, SchedCondElement rcRefCoreEle, SchedConds & rcSchedConds)
{
	rcSchedConds.erase(rcFlexEle);
	rcFlexEle.m_dUpperBound = max(rcRefCoreEle.m_dUpperBound + 1, rcFlexEle.m_dUpperBound);
	rcSchedConds.insert(rcFlexEle);
}

void RTUnschedCoreComputer::NextCore_RestoreSchedCondEle(SchedCondElement rcFlexEle, SchedCondElement rcRefCoreELe, SchedConds & rcSchedConds)
{
	rcSchedConds.erase(rcFlexEle);
	rcSchedConds.insert(rcFlexEle);
}

bool RTUnschedCoreComputer::IsCoreContained(SchedConds & rcSchedConds, UnschedCore & rcCore)
{
	for (SchedConds::iterator iter = rcCore.begin(); iter != rcCore.end(); iter++)
	{
		SchedConds::iterator iterFind = rcSchedConds.find(*iter);
		if (iterFind == rcSchedConds.end())
			return false;

		if (iterFind->m_dUpperBound > iter->m_dUpperBound)
		{
			return false;
		}
	}
	return true;
}

void RTUnschedCoreComputer::GenerateRTSamplesByResolution(double dResolution)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	m_cTasksRTSamples.clear();
	for (int i = 0; i < iTaskNum; i++)
	{		
		GenerateRTSamplesByResolution(i, 0, pcTaskArray[i].getDeadline(), dResolution);		
	}
}

void RTUnschedCoreComputer::GenerateRTSamplesByResolution(int iTaskIndex, double dLB, double dUB, double dResolution)
{	
	m_cTasksRTSamples[iTaskIndex].clear();
	for (double dSample = dLB; dSample <= dUB; dSample += dResolution)
	{
		m_cTasksRTSamples[iTaskIndex].insert(dSample);
	}	
	m_cTasksRTSamples[iTaskIndex].insert(dUB);
	m_cTasksRTSamples[iTaskIndex].insert(dLB);
}

void RTUnschedCoreComputer::AddRTSamples(int iTaskIndex, double dSample)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	assert(iTaskIndex < iTaskNum);
	assert(dSample <= pcTaskArray[iTaskIndex].getDeadline());
	m_cTasksRTSamples[iTaskIndex].insert(dSample);
}

double RTUnschedCoreComputer::getRTSampleLB(int iTaskIndex, double dQuery)
{
	assert(m_cTasksRTSamples[iTaskIndex].empty() == false);
	MySet<double>::iterator iterLB = m_cTasksRTSamples[iTaskIndex].lower_bound(dQuery);
	iterLB--;
	if (iterLB == m_cTasksRTSamples[iTaskIndex].end())
	{
		return 0;
	}
	double dLB = *iterLB;
	return dLB;
}

double RTUnschedCoreComputer::getRTSampleUB(int iTaskIndex, double dQuery)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	assert(m_cTasksRTSamples[iTaskIndex].empty() == false);	
	MySet<double>::iterator iterUB = m_cTasksRTSamples[iTaskIndex].lower_bound(dQuery);
	if (iterUB == m_cTasksRTSamples[iTaskIndex].end())
	{
		cout << "There is no sample strictly greater than the query" << endl;
		cout << dQuery << " " << *m_cTasksRTSamples[iTaskIndex].crbegin() << endl;
		while (1);
	}
	if (*iterUB == dQuery)
	{
		iterUB++;
	}
	if (iterUB == m_cTasksRTSamples[iTaskIndex].end())
	{
		cout << "There is no sample strictly greater than the query" << endl;
		cout << dQuery << " " << *m_cTasksRTSamples[iTaskIndex].crbegin() << endl;
		while (1);
	}

	double dUB = *iterUB;
	return dUB;
}

void RTUnschedCoreComputer::CombineSchedConds(SchedConds & rcShedCondsA, SchedConds & rcSchedCondsB, SchedConds & rcSchedCondsCombined)
{
	rcSchedCondsCombined = rcShedCondsA;
	for (SchedConds::iterator iterEle = rcSchedCondsB.begin(); iterEle != rcSchedCondsB.end(); iterEle++)
	{
		SchedConds::iterator iterFind = rcSchedCondsCombined.find(*iterEle);
		if (iterFind != rcSchedCondsCombined.end())
		{
			if (iterFind->m_dUpperBound > iterEle->m_dUpperBound)
			{
				rcSchedCondsCombined.erase(iterFind);
				rcSchedCondsCombined.insert(*iterEle);
			}
		}
		else
		{
			rcSchedCondsCombined.insert(*iterEle);
		}
	}	
}

void RTUnschedCoreComputer::CombineSchedConds(SchedConds & rcSchedCondsCombined, SchedConds & rcSchedCondsB)
{	
	for (SchedConds::iterator iterEle = rcSchedCondsB.begin(); iterEle != rcSchedCondsB.end(); iterEle++)
	{
		SchedConds::iterator iterFind = rcSchedCondsCombined.find(*iterEle);
		if (iterFind != rcSchedCondsCombined.end())
		{
			if (iterFind->m_dUpperBound > iterEle->m_dUpperBound)
			{
				rcSchedCondsCombined.erase(iterFind);
				rcSchedCondsCombined.insert(*iterEle);
			}
		}
		else
		{
			rcSchedCondsCombined.insert(*iterEle);
		}		
	}
}


MinRTSum::MinRTSum()
{

}

MinRTSum::MinRTSum(System & rcTaskSet)
	:RTUnschedCoreComputer(rcTaskSet)
{
	m_iDisplay = 0;
}

double MinRTSum::ComputeMinRTSum(MySet<int> & rsetConcernedTasks,
	SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	MySet<int> setLowestSchedulable;
	double dRTSum = 0;
	int iConcernedFullfilled = 0;
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	for (int i = 0; i < iTaskNum; i++)
	{
		int iPriorityLevel = iTaskNum - i - 1;
		int iCand = -1;
		for (int j = 0; j < iTaskNum; j++)
		{
			if (rcPriorityAssignment.getPriorityByTask(j) != -1)	continue;
			if (setLowestSchedulable.count(j))	continue;
			rcPriorityAssignment.setPriority(j, iPriorityLevel);
			bool bStatus = SchedTest(j, rcPriorityAssignment, rcSchedCondsFlex, rcSchedCondsFixed);
			if (bStatus)
			{
				setLowestSchedulable.insert(j);
			}
			rcPriorityAssignment.unset(j);
		}
		if (setLowestSchedulable.empty())	return -1;
		//Find out a candidate
		for (MySet<int>::iterator iter = setLowestSchedulable.begin(); iter != setLowestSchedulable.end(); iter++)
		{
			if (rsetConcernedTasks.count(*iter) == 0)
			{
				iCand = *iter;				
				break;
			}

			if (iCand == -1)
			{
				iCand = *iter;
			}

			if (pcTaskArray[*iter].getCLO() > pcTaskArray[iCand].getCLO())
			{
				iCand = *iter;
			}
		}
		rcPriorityAssignment.setPriority(iCand, iPriorityLevel);
		setLowestSchedulable.erase(iCand);
		if (rsetConcernedTasks.count(iCand))
		{		
			dRTSum += cRTCalc.CalculateRTTask(iCand, rcPriorityAssignment);
			iConcernedFullfilled++;
			if (iConcernedFullfilled == rsetConcernedTasks.size())
				break;
		}
	}
	return dRTSum;
}

void Paper_SmallExampleDemo()
{
	struct ExampleTaskSetConfig
	{
		double dPeriod;
		double dUtil;
		bool bCriticality;
	}atagConfig[] = {
		{ 2000, 0.0, false },
		{ 10, 2.0 / 10.0, false },
		{ 20, 3.0 / 20.0, false },
		{ 40, 16.0 / 40.0, false },
		{ 100, 3.0 / 100.0, false },
		{ 200, 17.0 / 200.0, false },
		{ 400, 16.0 / 400.0, false },
		{ 500, 15.0 / 500.0, false },
	};
	TaskSet cTaskSet;
	int iNTask = sizeof(atagConfig) / sizeof(struct ExampleTaskSetConfig);
	for (int i = 0; i < iNTask; i++)
	{
		cTaskSet.AddTask(atagConfig[i].dPeriod, atagConfig[i].dPeriod, atagConfig[i].dUtil, 2.0, atagConfig[i].bCriticality, i + 1);
	}
	cTaskSet.ConstructTaskArray();
	cTaskSet.DisplayTaskInfo();
	typedef RTUnschedCoreComputer::SchedConds SchedConds;
	typedef RTUnschedCoreComputer::UnschedCores UnschedCores;
	SchedConds cSchedConds;
	cSchedConds.insert({ 1, 2.0 });
	cSchedConds.insert({ 2, 3.0 });
	cSchedConds.insert({ 3, 16.0 });
	cSchedConds.insert({ 4, 3.0 });
	cSchedConds.insert({ 5, 17.0 });
	cSchedConds.insert({ 6, 40.0 });	
	cSchedConds.insert({ 7, 32.0 });
	RTUnschedCoreComputer cUnSchedCoreComp(cTaskSet);
	cUnSchedCoreComp.GenerateRTSamplesByResolution(1.0);

	SchedConds cDummyFixed;
	UnschedCores cCores;

	cUnSchedCoreComp.ComputeUnschedCores(cSchedConds, cDummyFixed, cCores, -1);
	for (UnschedCores::iterator iter = cCores.begin(); iter != cCores.end(); iter++)
	{
		cout << "Core " << iter - cCores.begin() << ": ";
		PrintSetContent(iter->getSetContent());
	}
	system("pause");
	MinRTSum cMinRTSum(cTaskSet);
	cMinRTSum.GenerateRTSamplesByResolution(1.0);
	cMinRTSum.m_iDisplay = 1;
//	cMinRTSum.Run();
//	cMinRTSum.PrintResult(cout);		
	system("pause");	
}	

RTSumRange_Type::RTSumRange_Type()
{

}

RTSumRange_Type::RTSumRange_Type(int iTaskIndex)
{
	m_setTasks.insert(iTaskIndex);	 
}

RTSumRange_Type::RTSumRange_Type(MySet<int> & rsetTasks)
{	
	m_setTasks = rsetTasks;	
}

bool operator < (const RTSumRange_Type & rcLHS, const RTSumRange_Type & rcRHS)
{
	if (rcLHS.m_setTasks.size() == rcRHS.m_setTasks.size())
	{
		MySet<int>::iterator iterLHS = rcLHS.m_setTasks.begin();
		MySet<int>::iterator iterRHS = rcRHS.m_setTasks.begin();
		for (; iterLHS != rcLHS.m_setTasks.end(); iterLHS++, iterRHS++)
		{
			if (*iterLHS < *iterRHS)
			{
				return true;
			}
			else if (*iterLHS > *iterRHS)
			{
				return false;
			}
		}
	}
	else
	{
		return rcLHS.m_setTasks.size() < rcRHS.m_setTasks.size();
	}
	return false;
}

ostream & operator << (ostream & os, const RTSumRange_Type & rcRHS)
{
	char axBuffer[512] = { 0 };	
	os << "{";
	MySet<int> & rsetTasks = (MySet<int> &)rcRHS.m_setTasks;
#if 1
	for (MySet<int>::iterator iter = rcRHS.m_setTasks.begin(); iter != rcRHS.m_setTasks.end(); iter++)
	{
		if (iter != rcRHS.m_setTasks.begin())
		{
			os << ", ";
		}
		os << *iter;
	}
#endif	
	os << "}";
	return os;
}


RTSumRangeUnschedCoreComputer::RTSumRangeUnschedCoreComputer()
{

}
	
RTSumRangeUnschedCoreComputer::RTSumRangeUnschedCoreComputer(SystemType & rcSystem)
	:GeneralUnschedCoreComputer_MK(rcSystem)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorRTLimits.push_back(-1);
	}

	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorVirtualDeadline.push_back(-1);
	}

	SortTaskByWCET(m_vectorWCETSort);
}

double RTSumRangeUnschedCoreComputer::ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{	
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	double dRT = cRTCalc.CalculateRTTask(iTaskIndex, rcPriorityAssignment);
	return dRT;
}

bool RTSumRangeUnschedCoreComputer::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{ 
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dVirtualDeadline = pcTaskArray[iTaskIndex].getDeadline();
	RTSumRange_Type cQuery;
	cQuery.m_setTasks.insert(iTaskIndex);
	SchedConds::iterator iterFlex = rcSchedCondsFlex.find(cQuery);
	SchedConds::iterator iterFixed = rcSchedCondsFixed.find(cQuery);
	if (iterFlex != rcSchedCondsFlex.end())
	{
		dVirtualDeadline = min(dVirtualDeadline, iterFlex->second);
	}

	if (iterFixed != rcSchedCondsFixed.end())
	{
		dVirtualDeadline = min(dVirtualDeadline, iterFixed->second);
	}

	double dRT = ComputeRT(iTaskIndex, rcPriorityAssignment);
	bool bStatus = DOUBLE_GE(dVirtualDeadline, dRT, 1e-3);
	return bStatus;
}

bool RTSumRangeUnschedCoreComputer::IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{	
	MySet<int> setConcernedTasks;
	rcPriorityAssignment.Clear();
	double dBound = 0;
	int iSummationNum = 0;
	for (SchedConds::iterator iter = rcSchedCondsFlex.begin(); iter != rcSchedCondsFlex.end(); iter++)
	{
		if (iter->first.m_setTasks.size() > 1)
		{
			setConcernedTasks = iter->first.m_setTasks;
			dBound = iter->second;
			iSummationNum++;
		}
	}

	for (SchedConds::iterator iter = rcSchedCondsFixed.begin(); iter != rcSchedCondsFixed.end(); iter++)
	{
		if (iter->first.m_setTasks.size() > 1)
		{
			setConcernedTasks = iter->first.m_setTasks;
			dBound = iter->second;
			iSummationNum++;
		}
	}
	assert(iSummationNum <= 1);
	double dActualRTSum = ComputeMinRTSum_v2(setConcernedTasks, rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	bool bStatus = (dActualRTSum <= dBound) && (dActualRTSum >= 0);
	return bStatus;
}

bool RTSumRangeUnschedCoreComputer::IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	TaskSetPriorityStruct cDummyPA(*m_pcTaskSet);
	return IsSchedulable(rcSchedCondsFlex, rcSchedCondsFixed, cDummyPA);
}

int RTSumRangeUnschedCoreComputer::ComputeAllCores(UnschedCores & rcUnschedCores, int iLimit, double dTimeout /* = 1e74 */)
{
	SchedConds rcSchedConds;
	SchedConds cDummyFixed;
	rcSchedConds.clear();
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		SchedCondType cType;
		cType.m_setTasks.insert(i);
		rcSchedConds.insert({ cType, max(pcTaskArray[i].getCLO() - 1.0, 0.0) });
	}
	int iNum = ComputeUnschedCores(rcSchedConds, cDummyFixed, rcUnschedCores, -1);
	return iNum;
}

double RTSumRangeUnschedCoreComputer::ComputeMinRTSum(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex,
SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	MySet<int> setLowestSchedulable;
	double dRTSum = 0;	
	for (int i = 0; i < iTaskNum; i++)
	{
		int iPriorityLevel = iTaskNum - i - 1;
		int iCand = -1;
		for (int j = 0; j < iTaskNum; j++)
		{
			if (rcPriorityAssignment.getPriorityByTask(j) != -1)	continue;
			if (setLowestSchedulable.count(j))	continue;
			rcPriorityAssignment.setPriority(j, iPriorityLevel);
			bool bStatus = SchedTest(j, rcPriorityAssignment, rcSchedCondsFlex, rcSchedCondsFixed);
			if (bStatus)
			{
				setLowestSchedulable.insert(j);
			}
			rcPriorityAssignment.unset(j);
		}
		if (setLowestSchedulable.empty())	return -1;
		//Find out a candidate
		for (MySet<int>::iterator iter = setLowestSchedulable.begin(); iter != setLowestSchedulable.end(); iter++)
		{
			if (rsetConcernedTasks.count(*iter) == 0)
			{
				iCand = *iter;
				break;
			}

			if (iCand == -1)
			{
				iCand = *iter;
			}

			if (pcTaskArray[*iter].getCLO() > pcTaskArray[iCand].getCLO())
			{
				iCand = *iter;
			}
		}
		rcPriorityAssignment.setPriority(iCand, iPriorityLevel);
		setLowestSchedulable.erase(iCand);
		if (rsetConcernedTasks.count(iCand))
		{
			dRTSum += ComputeRT(iCand, rcPriorityAssignment);			
		}
	}
	return dRTSum;
}

double RTSumRangeUnschedCoreComputer::ComputeMinRTSum_v2(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	vector<int> vectorConcerned, vectorUnconcerned;
	vector<bool> vectorConcernedFlag;
	vectorConcernedFlag.reserve(iTaskNum);
	vectorConcerned.reserve(iTaskNum);
	vectorUnconcerned.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)	vectorConcernedFlag.push_back(false);
	for (MySet<int>::iterator iter = rsetConcernedTasks.begin(); iter != rsetConcernedTasks.end(); iter++)
	{
		vectorConcernedFlag[*iter] = true;
	}

	for (vector<int>::iterator iter = m_vectorWCETSort.begin(); iter != m_vectorWCETSort.end(); iter++)
	{
		int i = *iter;
		if (vectorConcernedFlag[i])
		{
			vectorConcerned.push_back(i);
		}
		else
		{
			vectorUnconcerned.push_back(i);
		}
	}
	rcPriorityAssignment.Clear();
	double dRTSum = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		int iPriorityLevel = iTaskNum - i - 1;
		int iCand = -1;
		if (rcPriorityAssignment.getTaskByPriority(iPriorityLevel) != -1)	continue;
		bool bIsAssigned = false;
		for (vector<int>::iterator iter = vectorUnconcerned.begin(); iter != vectorUnconcerned.end(); iter++)
		{
			int j = *iter;
			if (vectorConcernedFlag[j])	continue;
			if (rcPriorityAssignment.getPriorityByTask(j) != -1)	continue;			

			rcPriorityAssignment.setPriority(j, iPriorityLevel);
			bool bStatus = SchedTest(j, rcPriorityAssignment, rcSchedCondsFlex, rcSchedCondsFixed);
			if (bStatus)
			{
				bIsAssigned = true;
				break;
			}
			rcPriorityAssignment.unset(j);
		}
		if (bIsAssigned) continue;
		for (vector<int>::iterator iter = vectorConcerned.begin(); iter != vectorConcerned.end(); iter++)
		{
			int j = *iter;
			if (!vectorConcernedFlag[j])	continue;
			if (rcPriorityAssignment.getPriorityByTask(j) != -1)	continue;			

			rcPriorityAssignment.setPriority(j, iPriorityLevel);
			double dVirtualDeadline = m_vectorVirtualDeadline[j];
			double dRT = ComputeRT(j, rcPriorityAssignment);
			bool bStatus = DOUBLE_GE(dVirtualDeadline, dRT, 1e-3);
			if (bStatus)
			{
				dRTSum += dRT;
				bIsAssigned = true;
				break;
			}
			rcPriorityAssignment.unset(j);
		}

		if (!bIsAssigned)
		{
			return -1.0;
		}

	}
	return dRTSum;
}

void RTSumRangeUnschedCoreComputer::ConvertToMUS_Schedulable(SchedCond & rcErased, SchedConds & rcSchedCondsFlex, SchedConds & rcShcedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	RTSumRange_Type cType = rcErased.first;
	int iLB = rcErased.second;
	int iUB = 0;
	for (MySet<int>::iterator iter = cType.m_setTasks.begin(); iter != cType.m_setTasks.end(); iter++)
	{
		iUB += pcTaskArray[*iter].getDeadline();
	}
	//cout << rcErased << endl;
	//cout << dLB << " " << dUB << endl;
	//	PrintSetContent(rcSchedCondsFlex.getSetContent());
	TaskSetPriorityStruct cPriorityAssignmentThis(*m_pcTaskSet);
	TaskSetPriorityStruct & cPriorityAssignmentLastUnsched = rcPriorityAssignment;
	cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);		
	while (iUB - iLB > 1)
	{
		int iCurrent = (iUB + iLB) / 2;
		//cout << dCurrent << " " << dUB << " " << dLB << endl;
		rcSchedCondsFlex.insert({ cType, iCurrent });
		bool bStatus = IsSchedulable(rcSchedCondsFlex, rcShcedCondsFixed, cPriorityAssignmentThis);
		rcSchedCondsFlex.erase(cType);
		if (bStatus)
		{
			iUB = iCurrent;
			cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
		}
		else
		{
			iLB = iCurrent;
			cPriorityAssignmentLastUnsched.CopyFrom_Strict(cPriorityAssignmentThis);
		}
	}
	//	cout << dUB << endl;	
	rcSchedCondsFlex.insert({ cType, iLB });

}

class TaskWCETDComparator
{
public:
	TaskSet * m_pcTaskSet;
	vector<double> * m_pvectorVirtualDeadline;
	bool operator () (int iLHS, int iRHS)
	{
		if (iLHS == iRHS)	return false;
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		if (pcTaskArray[iLHS].getCLO() == pcTaskArray[iRHS].getCLO())
		{
			if ((*m_pvectorVirtualDeadline)[iLHS] == (*m_pvectorVirtualDeadline)[iRHS])
			{
				return iLHS < iRHS;
			}
			else
			{
				return (*m_pvectorVirtualDeadline)[iLHS] > (*m_pvectorVirtualDeadline)[iRHS];
			}
		}
		else
		{
			return pcTaskArray[iLHS].getCLO() > pcTaskArray[iRHS].getCLO();
		}
		return false;
	}
};

void RTSumRangeUnschedCoreComputer::SortTaskByWCET(vector<int> & rvectorSort)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	rvectorSort.clear();
	rvectorSort.reserve(iTaskNum);
	TaskWCETDComparator cComp;
	cComp.m_pcTaskSet = m_pcTaskSet;
	cComp.m_pvectorVirtualDeadline = &m_vectorVirtualDeadline;
	set<int, TaskWCETDComparator> setSort(cComp);
	for (int i = 0; i < iTaskNum; i++)
	{
		setSort.insert(i);
	}
	for (set<int>::iterator iter = setSort.begin(); iter != setSort.end(); iter++)
	{
		rvectorSort.push_back(*iter);
	}
}

void RTSumRangeUnschedCoreComputer::IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorVirtualDeadline[i] = pcTaskArray[i].getDeadline();
	}

	for (SchedConds::iterator iter = rcSchedCondsFlex.begin(); iter != rcSchedCondsFlex.end(); iter++)
	{
		if (iter->first.m_setTasks.size() == 1)
		{
			int iTaskIndex = *iter->first.m_setTasks.begin();
			m_vectorVirtualDeadline[iTaskIndex] = min(m_vectorVirtualDeadline[iTaskIndex], (double)iter->second);
		}
	}

	for (SchedConds::iterator iter = rcSchedCondsFixed.begin(); iter != rcSchedCondsFixed.end(); iter++)
	{
		if (iter->first.m_setTasks.size() == 1)
		{
			int iTaskIndex = *iter->first.m_setTasks.begin();
			m_vectorVirtualDeadline[iTaskIndex] = min(m_vectorVirtualDeadline[iTaskIndex], (double)iter->second);
		}
	}
	SortTaskByWCET(m_vectorWCETSort);
}

//Exp Non preemptive min sum rt
NonpreemptiveMinAvgRT_Exp::NonpreemptiveMinAvgRT_Exp()
{

}

NonpreemptiveMinAvgRT_Exp::NonpreemptiveMinAvgRT_Exp(SystemType & rcSystem)
	:RTSumRangeUnschedCoreComputer(rcSystem)
{

}

double NonpreemptiveMinAvgRT_Exp::NonpreemptiveRTRHS(double dTime, int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dRHS = pcTaskArray[iTaskIndex].getCLO();
	double dBlocking = ComputeBlocking(iTaskIndex, rcPriorityAssignment);
	dRHS += dBlocking;
	int iPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		if (rcPriorityAssignment.getPriorityByTask(i) < iPriority)
		{
			dRHS += ceil((dTime - pcTaskArray[iTaskIndex].getCLO()) / pcTaskArray[i].getPeriod()) * pcTaskArray[i].getCLO();
		}
	}
	return dRHS;
}

double NonpreemptiveMinAvgRT_Exp::ComputeBlocking(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	double dBlocking = pcTaskArray[iTaskIndex].getCLO();	
	int iPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		if (rcPriorityAssignment.getPriorityByTask(i) > iPriority)
		{
			dBlocking = max(dBlocking, pcTaskArray[i].getCLO());
		}
	}
	return dBlocking;
}

double NonpreemptiveMinAvgRT_Exp::ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dRT_Prev = 0, dRT = pcTaskArray[iTaskIndex].getCLO();
	while (DOUBLE_EQUAL(dRT, dRT_Prev, 1e-6) == false)
	{
		dRT_Prev = dRT;
		dRT = NonpreemptiveRTRHS(dRT_Prev, iTaskIndex, rcPriorityAssignment);
		if (dRT > pcTaskArray[iTaskIndex].getDeadline())
		{
			break;
		}
	}
	return dRT;
}

void RTSumRangeUnschedCoreComputer::Recur_MinimalRelaxation(SchedConds & rcSchedCondsFlex, SchedCond & rcSchedCondInFlex, SchedCond & rcSchedCondRef)
{
	rcSchedCondsFlex[rcSchedCondInFlex.first] = max(rcSchedCondInFlex.second, rcSchedCondRef.second + 1);
}

//Intuitive Min weighted RT sum


MinWeightedRTSumHeuristic::MinWeightedRTSumHeuristic()
{

}

MinWeightedRTSumHeuristic::MinWeightedRTSumHeuristic(SystemType & rcSystem)
	:GeneralUnschedCoreComputer_MK(rcSystem)
{
	m_vectorWeight = rcSystem.getWeightTable();
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorMinWeightRTSumRTTable.push_back(-1);
	}

	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorVirtualDeadline.push_back(pcTaskArray[i].getDeadline());
	}
	SortTaskByNonminalWCET(m_vectorNominalWCETSort);
}

bool MinWeightedRTSumHeuristic::IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	MySet<int> setConcernedTasks;
	rcPriorityAssignment.Clear();
	double dBound = 0;
	int iSummationNum = 0;
	for (SchedConds::iterator iter = rcSchedCondsFlex.begin(); iter != rcSchedCondsFlex.end(); iter++)
	{
		if (iter->first.m_setTasks.size() > 1)
		{
			setConcernedTasks = iter->first.m_setTasks;
			dBound = iter->second;
			iSummationNum++;
		}
	}

	for (SchedConds::iterator iter = rcSchedCondsFixed.begin(); iter != rcSchedCondsFixed.end(); iter++)
	{
		if (iter->first.m_setTasks.size() > 1)
		{
			setConcernedTasks = iter->first.m_setTasks;
			dBound = iter->second;
			iSummationNum++;
		}
	}
	assert(iSummationNum <= 1);
	double dActualRTSum = ComputeMinWeightedRTSum(setConcernedTasks, rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment, m_vectorMinWeightRTSumRTTable);
	bool bStatus = (dActualRTSum <= dBound) && (dActualRTSum >= 0);
	return bStatus;
}

bool MinWeightedRTSumHeuristic::IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	TaskSetPriorityStruct cDummyPA(*m_pcTaskSet);
	return IsSchedulable(rcSchedCondsFlex, rcSchedCondsFixed, cDummyPA);
}

void MinWeightedRTSumHeuristic::SortTaskByNonminalWCET(vector<int> & rvectorSort)
{	
	class TaskWeightedWCETDComparator
	{
	public:
		TaskSet * m_pcTaskSet;
		vector<double> * m_pvectorVirtualDeadline;
		vector<double> * m_pvectorWeight;
		bool operator () (int iLHS, int iRHS)
		{
			if (iLHS == iRHS)	return false;
			GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
			double dWeightLHS = (*m_pvectorWeight)[iLHS];			
			double dWeightRHS = (*m_pvectorWeight)[iRHS];
			assert(dWeightRHS > 0);
			assert(dWeightLHS > 0);
			double dNominalWCET_LHS = 1e74;
			double dNominalWCET_RHS = 1e74;			
			dNominalWCET_LHS = (pcTaskArray[iLHS].getCLO() / (*m_pvectorWeight)[iLHS]);
			dNominalWCET_RHS = (pcTaskArray[iRHS].getCLO() / (*m_pvectorWeight)[iRHS]);
			if (dNominalWCET_LHS == dNominalWCET_RHS)
			{
				if ((*m_pvectorVirtualDeadline)[iLHS] == (*m_pvectorVirtualDeadline)[iRHS])
				{
					return iLHS < iRHS;
				}
				else
				{
					return (*m_pvectorVirtualDeadline)[iLHS] > (*m_pvectorVirtualDeadline)[iRHS];
				}
			}
			else
			{
				return dNominalWCET_LHS > dNominalWCET_RHS;
			}
			return false;
		}
	};

	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	if (m_vectorWeight.size() != iTaskNum)
	{
		cout << "Weight vector not initialized" << endl;
		assert(0);
	}
	rvectorSort.clear();
	rvectorSort.reserve(iTaskNum);
	TaskWeightedWCETDComparator cComp;
	cComp.m_pcTaskSet = m_pcTaskSet;
	cComp.m_pvectorVirtualDeadline = &m_vectorVirtualDeadline;
	cComp.m_pvectorWeight = &m_vectorWeight;
	set<int, TaskWeightedWCETDComparator> setSort(cComp);
	for (int i = 0; i < iTaskNum; i++)
	{
		setSort.insert(i);
	}
	for (set<int>::iterator iter = setSort.begin(); iter != setSort.end(); iter++)
	{
		rvectorSort.push_back(*iter);
	}
	//cComp(1, 2);
}

void MinWeightedRTSumHeuristic::IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorVirtualDeadline[i] = pcTaskArray[i].getDeadline();
	}
	const vector<double> & rvectorWeightTable = m_pcTaskSet->getWeightTable();
	for (SchedConds::iterator iter = rcSchedCondsFlex.begin(); iter != rcSchedCondsFlex.end(); iter++)
	{
		if (iter->first.m_setTasks.size() == 1)
		{
			int iTaskIndex = *iter->first.m_setTasks.begin();
			assert(rvectorWeightTable[iTaskIndex] > 0);
			m_vectorVirtualDeadline[iTaskIndex] = min(m_vectorVirtualDeadline[iTaskIndex], (double)iter->second / rvectorWeightTable[iTaskIndex]);
		}
	}

	for (SchedConds::iterator iter = rcSchedCondsFixed.begin(); iter != rcSchedCondsFixed.end(); iter++)
	{
		if (iter->first.m_setTasks.size() == 1)
		{
			int iTaskIndex = *iter->first.m_setTasks.begin();
			assert(rvectorWeightTable[iTaskIndex] > 0);
			m_vectorVirtualDeadline[iTaskIndex] = min(m_vectorVirtualDeadline[iTaskIndex], (double)iter->second / rvectorWeightTable[iTaskIndex]);
		}
	}	
}

double MinWeightedRTSumHeuristic::ComputeMinWeightedRTSum(vector<int> & rvectorExamOrder, MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorRTTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	if (rvectorRTTable.size() != iTaskNum)
	{
		rvectorRTTable.clear();
		rvectorRTTable.reserve(iTaskNum);
		for (int i = 0; i < iTaskNum; i++)
		{
			rvectorRTTable.push_back(0);
		}
	}

	assert(rvectorExamOrder.size() == iTaskNum);	
	IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	vector<int> vectorConcerned, vectorUnconcerned;
	vector<bool> vectorConcernedFlag;
	vectorConcernedFlag.reserve(iTaskNum);
	vectorConcerned.reserve(iTaskNum);
	vectorUnconcerned.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)	vectorConcernedFlag.push_back(false);
	for (MySet<int>::iterator iter = rsetConcernedTasks.begin(); iter != rsetConcernedTasks.end(); iter++)
	{
		vectorConcernedFlag[*iter] = true;
	}

	for (vector<int>::iterator iter = rvectorExamOrder.begin(); iter != rvectorExamOrder.end(); iter++)
	{
		int i = *iter;
		if (vectorConcernedFlag[i])
		{
			vectorConcerned.push_back(i);
		}
		else
		{
			vectorUnconcerned.push_back(i);
		}
	}
	rcPriorityAssignment.Clear();
	double dRTSum = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		int iPriorityLevel = iTaskNum - i - 1;
		int iCand = -1;
		if (rcPriorityAssignment.getTaskByPriority(iPriorityLevel) != -1)	continue;
		bool bIsAssigned = false;
		for (vector<int>::iterator iter = vectorUnconcerned.begin(); iter != vectorUnconcerned.end(); iter++)
		{
			int j = *iter;
			if (vectorConcernedFlag[j])	continue;
			if (rcPriorityAssignment.getPriorityByTask(j) != -1)	continue;

			rcPriorityAssignment.setPriority(j, iPriorityLevel);
			double dVirtualDeadline = m_vectorVirtualDeadline[j];
			double dRT = ComputeRT(j, rcPriorityAssignment);
			//bool bStatus = DOUBLE_GE(dVirtualDeadline, dRT, 1e-7);
			bool bStatus = dVirtualDeadline >= dRT;
			if (bStatus)
			{
				bIsAssigned = true;
				break;
			}
			rcPriorityAssignment.unset(j);
		}
		if (bIsAssigned) continue;
		for (vector<int>::iterator iter = vectorConcerned.begin(); iter != vectorConcerned.end(); iter++)
		{
			int j = *iter;
			if (!vectorConcernedFlag[j])	continue;
			if (rcPriorityAssignment.getPriorityByTask(j) != -1)	continue;

			rcPriorityAssignment.setPriority(j, iPriorityLevel);
			double dVirtualDeadline = m_vectorVirtualDeadline[j];
			double dRT = ComputeRT(j, rcPriorityAssignment);
			//bool bStatus = DOUBLE_GE(dVirtualDeadline, dRT, 1e-7);
			bool bStatus = dVirtualDeadline >= dRT;
			if (bStatus)
			{
				rvectorRTTable[j] = dRT;
				dRTSum += m_vectorWeight[j] * dRT;
				bIsAssigned = true;
				break;
			}
			rcPriorityAssignment.unset(j);
		}

		if (!bIsAssigned)
		{
			return -1.0;
		}

	}
	return dRTSum;
}

double MinWeightedRTSumHeuristic::ComputeMinWeightedRTSum(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorRTTable)
{
	double dRTSum = 0;
	if (m_vectorMasterOrder.empty())
	{
		assert(m_vectorNominalWCETSort.empty() == false);
		dRTSum = ComputeMinWeightedRTSum(m_vectorNominalWCETSort, rsetConcernedTasks, rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment, m_vectorMinWeightRTSumRTTable);
	}
	else
		dRTSum = ComputeMinWeightedRTSum(m_vectorMasterOrder, rsetConcernedTasks, rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment, m_vectorMinWeightRTSumRTTable);
	return dRTSum;
}

double MinWeightedRTSumHeuristic::ComputeMinWeightedRTSum(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	vector<double> vectorRTTable;	
	double dMinRTSum = ComputeMinWeightedRTSum(rsetConcernedTasks, rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment, vectorRTTable);
	return dMinRTSum;
}

int MinWeightedRTSumHeuristic::RollingDownAdjustment(int iTaskIndex, vector<bool> & rvectorConcernedTaskFlag, TaskSetPriorityStruct & rcPriorityAssignment,
	double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	vector<double> vectorRollingRTTable = rvectorRTTable;
	TaskSetPriorityStruct cRollingPriorityAssignment = rcPriorityAssignment;	
	double dBestRollingCost = rdCurrentCost;
	double dRollingCost = rdCurrentCost;
	int iTaskPriorityLevel = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	int iLastRecordPriorityLevel = iTaskPriorityLevel;	
	int iStatus = 0;
	while (iTaskPriorityLevel < (iTaskNum - 1))
	{		
		assert(iTaskPriorityLevel != -1);

		int iLPTask = cRollingPriorityAssignment.getTaskByPriority(iTaskPriorityLevel + 1);
		double dThisTaskWeight = rvectorWeightTable[iTaskIndex];
		double dLPTaskWeight = rvectorConcernedTaskFlag[iLPTask] ? rvectorWeightTable[iLPTask] : 0.0;

		double dOriginalThisTaskRT = vectorRollingRTTable[iTaskIndex];
		double dOriginalLPTaskRT = vectorRollingRTTable[iLPTask];
		cRollingPriorityAssignment.Swap(iTaskIndex, iLPTask);

		double dNewThisTaskRT = ComputeRT(iTaskIndex, cRollingPriorityAssignment);//Property of the LL task system;
		if (dNewThisTaskRT > pcTaskArray[iTaskIndex].getDeadline())	break;			
		double dNewLPTaskRT = ComputeRT(iLPTask, cRollingPriorityAssignment);
		vectorRollingRTTable[iLPTask] = dNewLPTaskRT;
		vectorRollingRTTable[iTaskIndex] = dNewThisTaskRT;


		double dCostDiff =
			(dThisTaskWeight * dOriginalThisTaskRT + dLPTaskWeight * dOriginalLPTaskRT) -
			(dThisTaskWeight * dNewThisTaskRT + dLPTaskWeight * dNewLPTaskRT);
		dRollingCost = dRollingCost - dCostDiff;
		if (dRollingCost < dBestRollingCost)
		{
			// a better solution found
			// update best solution
			for (int iPLevel = iLastRecordPriorityLevel; iPLevel <= iTaskPriorityLevel + 1; iPLevel++)
			{
				int iChangedTask = cRollingPriorityAssignment.getTaskByPriority(iPLevel);
				rvectorRTTable[iChangedTask] = vectorRollingRTTable[iChangedTask];
				if (iPLevel <= iTaskPriorityLevel)
				{
					int iSwapA = rcPriorityAssignment.getTaskByPriority(iPLevel);
					int iSwapB = rcPriorityAssignment.getTaskByPriority(iPLevel + 1);
					rcPriorityAssignment.Swap(iSwapA, iSwapB);
				}				
			}
			iLastRecordPriorityLevel = iTaskPriorityLevel + 1;
			dBestRollingCost = dRollingCost;
			iStatus++;
			//rcPriorityAssignment.PrintByPriority();
			//cout << "Best Cost: " << dBestRollingCost << endl;
			//system("pause");
		}
		iTaskPriorityLevel = cRollingPriorityAssignment.getPriorityByTask(iTaskIndex);
	}	
	rdCurrentCost = dBestRollingCost;	
	return iStatus;
}

int MinWeightedRTSumHeuristic::RollingDownAdjustment_v2(int iTaskIndex, vector<bool> & rvectorConcernedTaskFlag, TaskSetPriorityStruct & rcPriorityAssignment,
	double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	vector<double> vectorRollingRTTable = rvectorRTTable;	
	vector<double> vectorRollinRTTable_ForSchedTest = rvectorRTTable;
	TaskSetPriorityStruct cRollingPriorityAssignment = rcPriorityAssignment;
	double dBestRollingCost = rdCurrentCost;
	double dRollingCost = rdCurrentCost;
	int iTaskPriorityLevel = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	int iLastRecordPriorityLevel = iTaskPriorityLevel;	
	int iStatus = 0;
	int iLastCandPLevel = iTaskPriorityLevel;
	while (iTaskPriorityLevel < (iTaskNum - 1))
	{		
		//find the highest priority task in lower priority task set that can be put at an immediate higher priority level while maintaining schedulability		
		bool bChanged = false;
		for (int iCandPLevel = iLastCandPLevel + 1; iCandPLevel < iTaskNum; iCandPLevel++)
		{
			int iCandTask = cRollingPriorityAssignment.getTaskByPriority(iCandPLevel);
			cRollingPriorityAssignment.PutImmediatelyBefore(iCandTask, iTaskIndex);
			assert(cRollingPriorityAssignment.getTaskByPriority(iTaskPriorityLevel) == iCandTask);
			//test schedulability			
			vectorRollinRTTable_ForSchedTest = vectorRollingRTTable;
			bool bSchedTestStatus = true;
			for (int iTestPLevel = cRollingPriorityAssignment.getPriorityByTask(iCandTask); iTestPLevel <= iCandPLevel; iTestPLevel++)
			{
				int iSchedTestTask = cRollingPriorityAssignment.getTaskByPriority(iTestPLevel);				
				//double dNewRT = cRTCalc.CalculateRTTask(iSchedTestTask, cRollingPriorityAssignment, dRTLB);
				double dNewRT = ComputeRT(iSchedTestTask, cRollingPriorityAssignment);
				vectorRollinRTTable_ForSchedTest[iSchedTestTask] = dNewRT;
				if (dNewRT > pcTaskArray[iSchedTestTask].getDeadline())
				{
					//unschedulable. restore the priority assignment and continue to search for new candidate.
					cRollingPriorityAssignment.PutImmediatelyAfter(iCandTask, cRollingPriorityAssignment.getTaskByPriority(iCandPLevel));
					bSchedTestStatus = false;
					break;
				}
			}

			if (bSchedTestStatus)
			{
				//schedulable. update RT table and cost
				for (int iTestPLevel = iTaskPriorityLevel; iTestPLevel <= iCandPLevel; iTestPLevel++)
				{
					int iTestTask = cRollingPriorityAssignment.getTaskByPriority(iTestPLevel);
					double dWeight = rvectorConcernedTaskFlag[iTestTask] ? rvectorWeightTable[iTestTask] : 0.0;
					dRollingCost += (vectorRollinRTTable_ForSchedTest[iTestTask] - vectorRollingRTTable[iTestTask]) * dWeight;
					vectorRollingRTTable[iTestTask] = vectorRollinRTTable_ForSchedTest[iTestTask];
					{
						//double dTestRT = cRTCalc.CalculateRTTask(iTestTask, cRollingPriorityAssignment);
						double dTestRT = ComputeRT(iTestTask, cRollingPriorityAssignment);
						assert(dTestRT == vectorRollingRTTable[iTestTask]);
					}					
				}
				bChanged = true;
				if (dRollingCost < dBestRollingCost)
				{
					//new best cost found update result.
					dBestRollingCost = dRollingCost;
					rcPriorityAssignment = cRollingPriorityAssignment;
					rvectorRTTable = vectorRollingRTTable;
					iStatus++;
				}
				iLastCandPLevel = iCandPLevel;//Next time continue the search after this candidate priority level.
				break;
			}
		}
		if (!bChanged)
			break;
		iTaskPriorityLevel = cRollingPriorityAssignment.getPriorityByTask(iTaskIndex);
	}
	rdCurrentCost = dBestRollingCost;	
	return iStatus;
}

double MinWeightedRTSumHeuristic::RollingDownToFixedPointAdjustment(MySet<int> & rsetConcernedTask, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iStatus = 0;	
	vector<double> vectorRTTable;
	vector<bool> vectorConcernedTasksFlags;
	GenConcernedTaskFlag(rsetConcernedTask, vectorConcernedTasksFlags);	
	double dCost = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		//double dRT = cCalc.CalculateRTTask(i, rcPriorityAssignment);
		double dRT = ComputeRT(i, rcPriorityAssignment);
		dCost += rvectorWeightTable[i] * dRT;		
		vectorRTTable.push_back(dRT);
	}
	dCost = RollingDownToFixedPointAdjustment(vectorConcernedTasksFlags, rcPriorityAssignment, dCost, vectorRTTable, rvectorWeightTable);
	return dCost;
}

double MinWeightedRTSumHeuristic::RollingDownToFixedPointAdjustment(vector<bool> & rvectorConcernedTaskFlag,
	TaskSetPriorityStruct & rcPriorityAssignment, double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	int iStatus = 0;
	bool bProceed = true;
	int iLastChangeTask = -1;
	while (bProceed)
	{		
		for (int i = 0; i < iTaskNum; i++)
		{
			if (iLastChangeTask == i)
			{
				bProceed = false;
				break;
			}
			int iStatus = RollingDownAdjustment_v2(i, rvectorConcernedTaskFlag, rcPriorityAssignment, rdCurrentCost, rvectorRTTable, rvectorWeightTable);
			if (iStatus > 0)
			{
				iLastChangeTask = i;
			}
		}

		if (iLastChangeTask == -1) break;
	}
	return rdCurrentCost;
}

int MinWeightedRTSumHeuristic::RollingUpAdjustment(int iTaskIndex, vector<bool> & rvectorConcernedTaskFlag, TaskSetPriorityStruct & rcPriorityAssignment,
	double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iStatus = 0;	
	vector<double> vectorTempRTTable = rvectorRTTable;
	TaskSetPriorityStruct cTempPA = rcPriorityAssignment;	
	bool bProgress = true;
	double dRollingCost = rdCurrentCost;
	while (true)
	{		
		int iRollTaskPriorityLevel = cTempPA.getPriorityByTask(iTaskIndex);
		assert(iRollTaskPriorityLevel != -1);	
		//Find the first task that can be put immediately after the task		
		int iCandTask = -1;
		int iHPLevel = iRollTaskPriorityLevel - 1;
		for (; iHPLevel >= 0; iHPLevel--)
		{			
			int iHPTask = cTempPA.getTaskByPriority(iHPLevel);
			cTempPA.PutImmediatelyAfter(iHPTask, iTaskIndex);
			double dRT = ComputeRT(iHPTask, cTempPA);
			if (pcTaskArray[iHPTask].getDeadline() >= dRT)
			{							
				iCandTask = iHPTask;
				break;
			}
			cTempPA.PutImmediatelyBefore(iHPTask, cTempPA.getTaskByPriority(iHPLevel));
		}
		
		if (iCandTask == -1)
			break;

		//can be put immediately after.
//		cTempPA.PutImmediatelyAfter(iCandTask, iTaskIndex);
		//Calculate Cost
		double dCostChange = 0;
		for (int iIntermediatePLevel = iHPLevel; iIntermediatePLevel <= iRollTaskPriorityLevel; iIntermediatePLevel++)
		{
			int iSchedTestTask = cTempPA.getTaskByPriority(iIntermediatePLevel);
			
			//double dNewRT = cCalc.CalculateRTTask(iSchedTestTask, cTempPA, dRTLB);
			double dNewRT = ComputeRT(iSchedTestTask, cTempPA);
			double dWeight = (rvectorConcernedTaskFlag[iSchedTestTask]) ? rvectorWeightTable[iSchedTestTask] : 0.0;
			dCostChange += (dNewRT - vectorTempRTTable[iSchedTestTask]) * dWeight;
			vectorTempRTTable[iSchedTestTask] = dNewRT;
		}
		dRollingCost += dCostChange;
		if (dRollingCost < rdCurrentCost)
		{
			iStatus++;
			rcPriorityAssignment = cTempPA;
			rvectorRTTable = vectorTempRTTable;
			rdCurrentCost = dRollingCost;
			//cout << "Target Task: " << iTaskIndex << endl;
			//AnalyzeSuboptimalityFromSmallExample_PrintInfo(*m_pcTaskSet, cTempPA, rvectorWeightTable);
			//system("pause");
		}

	}
	return iStatus;
}

double MinWeightedRTSumHeuristic::RollingUpToFixedPointAdjustment(MySet<int> & rsetConcernedTask, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iStatus = 0;	
	vector<double> vectorRTTable;
	vector<bool> vectorConcernedTasksFlags;
	GenConcernedTaskFlag(rsetConcernedTask, vectorConcernedTasksFlags);
	//_LORTCalculator cCalc(*m_pcTaskSet);
	double dCost = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		//double dRT = cCalc.CalculateRTTask(i, rcPriorityAssignment);
		double dRT = ComputeRT(i, rcPriorityAssignment);
		dCost += rvectorWeightTable[i] * dRT;		
		vectorRTTable.push_back(dRT);
	}
	dCost = RollingUpToFixedPointAdjustment(vectorConcernedTasksFlags, rcPriorityAssignment, dCost, vectorRTTable, rvectorWeightTable);
	return dCost;
}

double MinWeightedRTSumHeuristic::RollingUpToFixedPointAdjustment(vector<bool> & rvectorConcernedTaskFlag, 
	TaskSetPriorityStruct & rcPriorityAssignment, double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iStatus = 0;	
	bool bProceed = true;
	int iLastChangeTask = -1;
	while (bProceed)
	{				
		for (int i = 0; i < iTaskNum; i++)
		{
			if (iLastChangeTask == i)
			{
				bProceed = false;
				break;
			}
			int iStatus = RollingUpAdjustment(i, rvectorConcernedTaskFlag, rcPriorityAssignment, rdCurrentCost, rvectorRTTable, rvectorWeightTable);
			if (iStatus > 0)
			{
				iLastChangeTask = i;
			}
		}
		if (iLastChangeTask == -1) break;
	}
	return rdCurrentCost;
}

double MinWeightedRTSumHeuristic::RollingToFixedPointAdjustment(MySet<int> & rsetConcernedTask, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iStatus = 0;	
	vector<double> vectorRTTable;
	vector<bool> vectorConcernedTasksFlags;
	GenConcernedTaskFlag(rsetConcernedTask, vectorConcernedTasksFlags);	
	double dCost = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		//double dRT = cCalc.CalculateRTTask(i, rcPriorityAssignment);
		double dRT = ComputeRT(i, rcPriorityAssignment);
		dCost += rvectorWeightTable[i] * dRT;		
		vectorRTTable.push_back(dRT);
	}	
	bool bProceed = true;
	int iLastChangeOperation = -1;
	
	while (true)
	{		
		double dPrevCost = dCost;
		if (iLastChangeOperation == 0)		break;
		RollingDownToFixedPointAdjustment(vectorConcernedTasksFlags, rcPriorityAssignment, dCost, vectorRTTable, rvectorWeightTable);
		if (dPrevCost > dCost)
		{
			iLastChangeOperation = 0;
		}

		dPrevCost = dCost;
		if (iLastChangeOperation == 1) break;
		RollingUpToFixedPointAdjustment(vectorConcernedTasksFlags, rcPriorityAssignment, dCost, vectorRTTable, rvectorWeightTable);
		if (dPrevCost > dCost)
		{
			iLastChangeOperation = 1;
		}
		
		if (iLastChangeOperation == -1)	break;
	}
	return dCost;
}

void MinWeightedRTSumHeuristic::GenConcernedTaskFlag(MySet<int> & rsetConcernedTasks, vector<bool> & rvectorConcernedTaskFlag)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	if (rvectorConcernedTaskFlag.size() != iTaskNum)
	{
		rvectorConcernedTaskFlag.reserve(iTaskNum);
		for (int i = 0; i < iTaskNum; i++)
		{
			rvectorConcernedTaskFlag.push_back(false);
		}
	}
	else
	{
		for (int i = 0; i < iTaskNum; i++)
		{
			rvectorConcernedTaskFlag[i] = false;
		}
	}

	for (MySet<int>::iterator iter = rsetConcernedTasks.begin(); iter != rsetConcernedTasks.end(); iter++)
	{
		rvectorConcernedTaskFlag[*iter] = true;
	}
}

double MinWeightedRTSumHeuristic::ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	double dRT = cRTCalc.CalculateRTTask(iTaskIndex, rcPriorityAssignment);
	return dRT;
}

void MinWeightedRTSumHeuristic::setWeight(vector<double> & rvectorWeight)
{
	m_vectorWeight = rvectorWeight;
}

bool MinWeightedRTSumHeuristic::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dVirtualDeadline = pcTaskArray[iTaskIndex].getDeadline();
	RTSumRange_Type cQuery;
	cQuery.m_setTasks.insert(iTaskIndex);
	SchedConds::iterator iterFlex = rcSchedCondsFlex.find(cQuery);
	SchedConds::iterator iterFixed = rcSchedCondsFixed.find(cQuery);
	if (iterFlex != rcSchedCondsFlex.end())
	{
		dVirtualDeadline = min(dVirtualDeadline, iterFlex->second);
	}

	if (iterFixed != rcSchedCondsFixed.end())
	{
		dVirtualDeadline = min(dVirtualDeadline, iterFixed->second);
	}

	double dRT = ComputeRT(iTaskIndex, rcPriorityAssignment);
	bool bStatus = DOUBLE_GE(dVirtualDeadline, dRT, 1e-3);
	return bStatus;
}

void MinWeightedRTSumHeuristic::ConvertToMUS_Schedulable(SchedCond & rcErased, SchedConds & rcSchedCondsFlex, SchedConds & rcShcedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	RTSumRange_Type cType = rcErased.first;
	SchedCondContent iLB = rcErased.second;
	SchedCondContent iUB = 0;
	const vector<double> & rvectorWeightTable = m_pcTaskSet->getWeightTable();
	for (MySet<int>::iterator iter = cType.m_setTasks.begin(); iter != cType.m_setTasks.end(); iter++)
	{
		iUB += pcTaskArray[*iter].getDeadline() * rvectorWeightTable[*iter];
	}
	//cout << rcErased << endl;
	//cout << dLB << " " << dUB << endl;
	//	PrintSetContent(rcSchedCondsFlex.getSetContent());
	TaskSetPriorityStruct cPriorityAssignmentThis(*m_pcTaskSet);
	TaskSetPriorityStruct & cPriorityAssignmentLastUnsched = rcPriorityAssignment;
	cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
	while (iUB - iLB > 1)
	{
		SchedCondContent iCurrent = (iUB + iLB) / 2;
		//cout << dCurrent << " " << dUB << " " << dLB << endl;
		rcSchedCondsFlex.insert({ cType, iCurrent });
		bool bStatus = IsSchedulable(rcSchedCondsFlex, rcShcedCondsFixed, cPriorityAssignmentThis);
		rcSchedCondsFlex.erase(cType);
		if (bStatus)
		{
			iUB = iCurrent;
			cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
		}
		else
		{
			iLB = iCurrent;
			cPriorityAssignmentLastUnsched.CopyFrom_Strict(cPriorityAssignmentThis);
		}
	}
	//	cout << dUB << endl;	
	rcSchedCondsFlex.insert({ cType, iLB });

}

void MinWeightedRTSumHeuristic::Recur_MinimalRelaxation(SchedConds & rcSchedCondsFlex, SchedCond & rcSchedCondInFlex, SchedCond & rcSchedCondRef)
{
	rcSchedCondsFlex[rcSchedCondInFlex.first] = max(rcSchedCondInFlex.second, rcSchedCondRef.second + 1);
}

//MinWeightedRTSumHeuristic_NonPreemptive
MinWeightedRTSumHeuristic_NonPreemptive::MinWeightedRTSumHeuristic_NonPreemptive()
{
}

MinWeightedRTSumHeuristic_NonPreemptive::MinWeightedRTSumHeuristic_NonPreemptive(SystemType & rcSystem)
	:MinWeightedRTSumHeuristic(rcSystem)
{

}

double MinWeightedRTSumHeuristic_NonPreemptive::ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	_NonPreemptiveLORTCalculator cRTCalc(*m_pcTaskSet);
	double dRT = cRTCalc.CalculateRT(iTaskIndex, rcPriorityAssignment);
	return dRT;
}

void RTSumUnschedCoreOptExample()
{
	struct ExampleTaskSetConfig
	{
		double dPeriod;
		double dUtil;
		bool bCriticality;
	}atagConfig[] = {
		{ 2000, 0.0, false },
		{ 10, 2.0 / 10.0, false },//20%
		{ 20, 3.0 / 20.0, false },//15%
		{ 40, 10.0 / 40.0, false },//40%
		{ 100, 3.0 / 100.0, false },
	//	{ 200, 17.0 / 200.0, false },
	//	{ 400, 16.0 / 400.0, false },
	//	{ 500, 15.0 / 500.0, false },
	};
	TaskSet cTaskSet;
	int iNTask = sizeof(atagConfig) / sizeof(struct ExampleTaskSetConfig);
	for (int i = 0; i < iNTask; i++)
	{
		cTaskSet.AddTask(atagConfig[i].dPeriod, atagConfig[i].dPeriod, atagConfig[i].dUtil, 2.0, atagConfig[i].bCriticality, i + 1);
	}
	cTaskSet.ConstructTaskArray();
	cTaskSet.DisplayTaskInfo();
	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
	typedef RTSumRangeUnschedCoreComputer::SchedConds SchedConds;
	typedef RTSumRangeUnschedCoreComputer::SchedCond SchedCond;
	typedef RTSumRangeUnschedCoreComputer::SchedCondType SchedCondType;
	RTSumRangeUnschedCoreComputer cUCCalc(cTaskSet);
	_LORTCalculator cRTCalc(cTaskSet);
	SchedConds cDummyFixed;

	SchedConds cRTSRSet1;

	set<int> cRTSR1_set = { 1, 2, 3, 4 };
	MySet<int> MySetcRTSR1_set(cRTSR1_set);
	cRTSRSet1[SchedCondType(MySetcRTSR1_set)] = 24;

	set<int> cRTSR2_set = { 3 };
	MySet<int> MySetcRTSR2_set(cRTSR2_set);
	cRTSRSet1[SchedCondType(MySetcRTSR2_set)] = 3;

	cUCCalc.PrintSchedConds(cRTSRSet1, cout); cout << endl;

	TaskSetPriorityStruct cPA(cTaskSet);
	cout << cUCCalc.ComputeMinRTSum_v2(MySetcRTSR1_set, cDummyFixed, cDummyFixed, cPA) << endl;
	cPA.PrintByPriority();
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "RT: " << cRTCalc.CalculateRTTask(i, cPA) << endl;
	}

	{
		set<int> cRTSR1_set = { 1, 2, 3, 4 };
		MySet<int> MySetcRTSR1_set(cRTSR1_set);
		cRTSRSet1[SchedCondType(MySetcRTSR1_set)] = 35;

		set<int> cRTSR2_set = { 3 };
		MySet<int> MySetcRTSR2_set(cRTSR2_set);
		cRTSRSet1[SchedCondType(MySetcRTSR2_set)] = 20;
		typedef RTSumRangeUnschedCoreComputer::UnschedCores UnschedCores;
		RTSumRangeUnschedCoreComputer::UnschedCores cUnschedCores;
		cUCCalc.PrintSchedConds(cRTSRSet1, cout); cout << endl;
		cUCCalc.ComputeUnschedCores(cRTSRSet1, cDummyFixed, cUnschedCores, -1);
		for (UnschedCores::iterator iter = cUnschedCores.begin(); iter != cUnschedCores.end(); iter++)
		{
			cUCCalc.PrintSchedConds(*iter, cout); cout << endl;
		}
		system("pause");
	}

	{
		set<int> cRTSR1_set = { 1, 2, 3, 4 };
		MySet<int> MySetcRTSR1_set(cRTSR1_set);
		//cRTSRSet1[SchedCondType(MySet<int>(cRTSR1_set))] = 35;

		set<int> cRTSR2_set = { 2 };
		MySet<int> MySetcRTSR2_set(cRTSR2_set);
		cRTSRSet1[SchedCondType(MySetcRTSR2_set)] = 3;
		typedef RTSumRangeUnschedCoreComputer::UnschedCores UnschedCores;
		RTSumRangeUnschedCoreComputer::UnschedCores cUnschedCores;
		TaskSetPriorityStruct rcPA(cTaskSet);
		cout << cUCCalc.ComputeMinRTSum_v2(MySetcRTSR1_set, cRTSRSet1, cDummyFixed, rcPA) << endl;
		for (int i = 0; i < iTaskNum; i++)
		{
			cout << "RT: " << cRTCalc.CalculateRTTask(i, rcPA) << endl;
		}
		system("pause");
	}

	cPA.Clear();
	MySet<int> cRTSR1MySet(cRTSR1_set);
	double dMinRTSum = cUCCalc.ComputeMinRTSum_v2(cRTSR1MySet, cRTSRSet1, cDummyFixed, cPA);
	_LORTCalculator cCalc(cTaskSet);
	cout << cCalc.CalculateRTTask(1, cPA) << endl;
	cout << cCalc.CalculateRTTask(2, cPA) << endl;
	cout << cCalc.CalculateRTTask(3, cPA) << endl;
	cout << dMinRTSum << endl;
	bool bStatus = cUCCalc.IsSchedulable(cRTSRSet1, cDummyFixed);
	cout << bStatus << endl;
	system("pause");

#ifdef MCMILPPACT
	cout << "FR Demo" << endl;
	RTSumFocusRefinement cRTSumFR(cTaskSet);
	RTSumFocusRefinement::LinearRTConst cLinearConst = {
		{ { 3, 1.0 }, { 4, 1.0 }, {-1, 20} }
	};

	cLinearConst = {
		{ { 2, 1.0 }, { 3, 1.0 }, { -1, 20 } }
	};

	cRTSumFR.AddLinearRTConst(cLinearConst);
	cRTSumFR.setCorePerIter(2);
	cRTSumFR.setLogFile("RTSumFRPaperExample_Log.txt");
	cRTSumFR.FocusRefinement(1);
	TaskSetPriorityStruct cPAFR(cTaskSet);
	cPAFR = cRTSumFR.getPriorityAssignmentSol();
	_LORTCalculator cCalcFR(cTaskSet);
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "Task " << i << ": " << cCalcFR.CalculateRTTask(i, cPAFR) << endl;
	}
	cPAFR.Print();
#endif
	system("pause");
}

void TestNonpreemptveRTSum()
{
	TaskSet cTaskSet;
	int iOpt = 0;
	cin >> iOpt;
	if (iOpt == 1)
	{
		cTaskSet.GenRandomTaskSetInteger(30, 0.1, 0.0, 1.0, PERIODOPT_TABLE_V2);
	}
	else if (iOpt ==2 )
	{
		cTaskSet.ReadImageFile("NPRTSum.tskst");
	}
	else if (iOpt == 3)
	{
		struct ExampleTaskSetConfig
		{
			double dPeriod;
			double dUtil;
			bool bCriticality;
		}atagConfig[] = {
		//	{ 2000, 0.0, false },
			{ 6, 0.5 / 6.0, false },//15%
			{ 20, 3.0 / 20.0, false },//15%
			{ 40, 5.0 / 40.0, false },//40%
			{ 100, 1.0 / 100.0, false },
			//	{ 200, 17.0 / 200.0, false },
			//	{ 400, 16.0 / 400.0, false },
			//	{ 500, 15.0 / 500.0, false },
		};		
		int iNTask = sizeof(atagConfig) / sizeof(struct ExampleTaskSetConfig);
		for (int i = 0; i < iNTask; i++)
		{
			cTaskSet.AddTask(atagConfig[i].dPeriod, atagConfig[i].dPeriod, atagConfig[i].dUtil, 2.0, atagConfig[i].bCriticality, i + 1);
		}
		cTaskSet.ConstructTaskArray();
	}
	else
	{
		cout << "Invalid Option " << endl;
		return;
	}
	cTaskSet.WriteTextFile("NPRTSum.tskst.txt");

	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);

	cout << "By WCET Monotonic" << endl;
	NonpreemptiveMinAvgRT_Exp cCalc(cTaskSet);
	MySet<int> setConcernedTasks;
	for (int i = 0; i < iTaskNum; i++)	setConcernedTasks.insert(i);
	NonpreemptiveMinAvgRT_Exp::SchedConds cDummyFixed;
	TaskSetPriorityStruct cPriorityAssignment(cTaskSet);
	int iMinRTSum = cCalc.ComputeMinRTSum_v2(setConcernedTasks, cDummyFixed, cDummyFixed, cPriorityAssignment);
	cout << iMinRTSum << endl;
	cPriorityAssignment.Print();
#ifdef MCMILPPACT
	cout << "By MILP" << endl;
	NonpreemptiveRTMILP cMILPModel(cTaskSet);
	cMILPModel.Solve(2);
	cout << "Objective: " << cMILPModel.getObj() << endl;
	cMILPModel.PrintResult(cout);	
#endif
	return;
}

void TestNPMinAvgRTAlg()
{
	StatisticSet cStat;
	int iTestN = 0;
	int iDiff = 0;
	char axBuffer[512] = { 0 };
	while (true)
	{
		TaskSet cTaskSet;
		cTaskSet.GenRandomTaskSetInteger(30, 0.5, 0.0, 1.0, PERIODOPT_TABLE_V2);
		GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
		NonpreemptiveMinAvgRT_Exp cCalc(cTaskSet);
		MySet<int> setConcernedTasks;
		for (int i = 0; i < iTaskNum; i++)	setConcernedTasks.insert(i);
		NonpreemptiveMinAvgRT_Exp::SchedConds cDummyFixed;
		TaskSetPriorityStruct cPriorityAssignment(cTaskSet);
		double dMinRTSum = cCalc.ComputeMinRTSum_v2(setConcernedTasks, cDummyFixed, cDummyFixed, cPriorityAssignment);		
		cPriorityAssignment.Print();
#ifdef MCMILPPACT		
		NonpreemptiveRTMILP cMILPModel(cTaskSet);
		int iStatus = cMILPModel.Solve(2);
		if (iStatus != 1)	continue;
		double dMILPObj = cMILPModel.getObj();
		if (!DOUBLE_GE(dMILPObj, dMinRTSum, 1e-6))
		{
			sprintf_s(axBuffer, "CT%d.tskst", iTestN);
			cTaskSet.WriteImageFile(axBuffer);			
		}
		cout << "Alg: " << dMinRTSum << endl;
		cout << "MILP: " << dMILPObj << endl;
		cStat.setItem("Tested", iTestN);
		cStat.setItem("Different", iDiff);
		cStat.WriteStatisticText("NPMinAvgRTAlgTest.txt");
		iTestN++;
#endif
	}
}

//WeightedRTSumBnB

WeightedRTSumBnB::WeightedRTSumBnB()
{
	m_pcTaskSet = NULL;
}

WeightedRTSumBnB::WeightedRTSumBnB(TaskSet & rcTaskSet)
{
	m_pcTaskSet = &rcTaskSet;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorRTTable.push_back(-1);
		m_vectorBestRTTable.push_back(-1);
	}
	m_cOptimalPriorityAssignment = TaskSetPriorityStruct(rcTaskSet);
}

WeightedRTSumBnB::~WeightedRTSumBnB()
{

}

void WeightedRTSumBnB::setWeightTable(vector<double> & rvectorWeightTable)
{
	m_vectorWeightTable = rvectorWeightTable;
}

int WeightedRTSumBnB::Run()
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	TaskSetPriorityStruct cPA(*m_pcTaskSet);
	double dCurrentCost = 0;
	m_dBestCost = 1e74;
	int iRetStatus = Recur(iTaskNum - 1, cPA, m_vectorRTTable, dCurrentCost, m_dBestCost);
	return iRetStatus;
}

double WeightedRTSumBnB::ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPA)
{
	_LORTCalculator cCalc(*m_pcTaskSet);
	double dRT = cCalc.CalculateRTTask(iTaskIndex, rcPA);
	return dRT;
}

int WeightedRTSumBnB::Recur(int iLevel, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorRTTable, double dCurrentCost, double & rdBestCost)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	//Base case
	if (iLevel == -1)
	{
		if (dCurrentCost < rdBestCost)
		{
			m_vectorBestRTTable = rvectorRTTable;
			m_cOptimalPriorityAssignment = rcPriorityAssignment;
			rdBestCost = dCurrentCost;
		}
		return 1;
	}

	//Assign task to current priority level
	bool bSchedStatus = false;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (rcPriorityAssignment.getPriorityByTask(i) != -1)	continue;
		rcPriorityAssignment.setPriority(i, iLevel);
		double dRT = ComputeRT(i, rcPriorityAssignment);	
		if (dRT <= pcTaskArray[i].getDeadline())
		{
			bSchedStatus = true;
			double dThisCurentCost = dCurrentCost + dRT * m_vectorWeightTable[i];
			if (dThisCurentCost < rdBestCost)
			{
				rvectorRTTable[i] = dRT;
				int iRecurStatus = Recur(iLevel - 1, rcPriorityAssignment, rvectorRTTable, dThisCurentCost, rdBestCost);
				if (iRecurStatus == -1)
				{
					return -1;
				}
			}
		}		
		rvectorRTTable[i] = -1;
		rcPriorityAssignment.unset(i);
	}
	if (!bSchedStatus)
		return -1;
	return 1;
}



#ifdef MCMILPPACT

RTFocusRefinement::RTFocusRefinement()
{

}

RTFocusRefinement::RTFocusRefinement(SystemType &rcSystem)
	:GeneralFocusRefinement(rcSystem)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
}

void RTFocusRefinement::SpecifyRTLB(int iTaskIndex, double dLB, double dUB)
{
	m_mapSpecifiedRTRange[iTaskIndex] = { dLB, dUB };
}

void RTFocusRefinement::PrintResult(ostream & os)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	for (SchedConds::iterator iter = m_cSchedCondsConfig.begin(); iter != m_cSchedCondsConfig.end(); iter++)
	{
		os << "Task " << iter->m_iTaskIndex << " / Config: " << iter->m_dUpperBound << " / RT: " << cRTCalc.CalculateRTTask(iter->m_iTaskIndex, this->m_cPriorityAssignmentSol) << endl;
	}
}

void RTFocusRefinement::ClearFR()
{
	GeneralFocusRefinement::ClearFR();
	m_mapSpecifiedRTRange.clear();
}

void RTFocusRefinement::CreateRTModelVar(const IloEnv & rcEnv, IloNumVarArray & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		sprintf_s(axBuffer, "R(%d)", i);
		rcRTVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));
	}
}

void RTFocusRefinement::GenSpecifiedRTRangeConst(const IloEnv & rcEnv, IloNumVarArray & rcRTVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (SpeciryRTRange::iterator iter = m_mapSpecifiedRTRange.begin(); iter != m_mapSpecifiedRTRange.end(); iter++)
	{
		rcConst.add(IloRange(rcEnv, iter->second.first, rcRTVars[iter->first], iter->second.second));
	}	
}

void RTFocusRefinement::GenRTModelConst(const IloEnv & rcEnv, SchedCondVars & rcSchedCondVars, IloNumVarArray & rcRTVars, IloRangeArray & rcConst)
{
	for (SchedCondVars::iterator iter = rcSchedCondVars.begin(); iter != rcSchedCondVars.end(); iter++)
	{
		int iTaskIndex = iter->first.m_iTaskIndex;
		double dQuery = iter->first.m_dUpperBound;
		double dUB = m_cUnschedCoreComputer.getRTSampleUB(iTaskIndex, dQuery);
		rcConst.add(IloRange(rcEnv, 0.0, rcRTVars[iTaskIndex] - (1.0 - iter->second) * dUB, IloInfinity));
	}
}

void RTFocusRefinement::GenSchedCondConsistencyConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst)
{
	int iCurrentTaskIndex = rcSchedCondVars.begin()->first.m_iTaskIndex;
	const IloEnv & rcEnv = rcConst.getEnv();
	for (SchedCondVars::iterator iter = rcSchedCondVars.begin(); iter != rcSchedCondVars.end(); iter++)
	{			
		IloExpr cExpr(rcEnv);
		SchedCondVars::iterator iterNext = iter;
		iterNext++;
		if (iterNext == rcSchedCondVars.end())	break;
		if (iter->first.m_iTaskIndex == iterNext->first.m_iTaskIndex)
		{
			rcConst.add(IloRange(rcEnv, -IloInfinity, iter->second - iterNext->second, 0.0));
		}

	}
}

IloExpr RTFocusRefinement::ObjExpr(IloEnv & rcEnv, IloNumVarArray & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IloExpr cExpr(rcEnv);
	for (int i = 0; i < iTaskNum; i++)
	{
		cExpr += rcRTVars[i];
		//cExpr += (i + 1) * rcRTVars[i];
	}
	return cExpr;
}

int RTFocusRefinement::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		IloNumVarArray cRTVars(cSolveEnv);
		IloRangeArray cConst(cSolveEnv);
		SchedCondVars cSchedCondVars;
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);
		CreateRTModelVar(cSolveEnv, cRTVars);
		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenSchedCondConsistencyConst(cSchedCondVars, cConst);
		GenRTModelConst(cSolveEnv, cSchedCondVars, cRTVars, cConst);
		GenSpecifiedRTRangeConst(cSolveEnv, cRTVars, cConst);
		IloObjective cObjective = IloMinimize(cSolveEnv, ObjExpr(cSolveEnv, cRTVars));
#if 1
		IloNumVar cMinSlack(cSolveEnv, 0.0, IloInfinity, IloNumVar::Float);
		for (int i = 0; i < iTaskNum; i++)
		{
			cConst.add(IloRange(cSolveEnv, 0.0, (pcTaskArray[i].getDeadline() - cRTVars[i]) - cMinSlack, IloInfinity));
		}
		cObjective = IloMinimize(cSolveEnv, -1.0 * cMinSlack);
#endif

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);
		cModel.add(cRTVars);
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		m_cSchedCondsConfig.clear();
		m_dequeSolutionSchedConds.clear();
		m_iBestSolFeasible = 0;
		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		m_cCplexStatus = cSolver.getCplexStatus();

		m_dequeSolutionSchedConds.clear();
		m_cSchedCondsConfig.clear();
		if (bStatus)
		{
			ExtractAllSolution(cSolver, cRTVars);
			MarshalAllSolution(m_dequeSolutionSchedConds);
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}

}

void RTFocusRefinement::ExtractSolution(IloCplex & rcSolver, int iSolIndex, IloNumVarArray & rcRTVars, SchedConds & rcSchedCondsConfig)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		double dValue = rcSolver.getValue(rcRTVars[i], iSolIndex);
		rcSchedCondsConfig.insert({ i, dValue });
	}
}

void RTFocusRefinement::ExtractAllSolution(IloCplex & rcSolver, IloNumVarArray & rcRTVars)
{
	int iSolNum = rcSolver.getSolnPoolNsolns();
	m_dequeSolutionSchedConds.clear();
	for (int i = 0; i < iSolNum; i++)	
	{
		SchedConds rcConfig;
		ExtractSolution(rcSolver, i, rcRTVars, rcConfig);
		double dObj = rcSolver.getObjValue(i);
		m_dequeSolutionSchedConds.push_back({dObj, rcConfig});
	}
}

void DynamicResolutionStrategy(TaskSet & rcTaskSet)
{
	GET_TASKSET_NUM_PTR(&rcTaskSet, iTaskNum, pcTaskArray);	
	double dInitialResolution = 1;
	for (int i = 0; i < iTaskNum; i++) dInitialResolution = max(dInitialResolution, pcTaskArray[i].getPeriod());
	dInitialResolution /= 2;
	dInitialResolution = 10000.0;
	int iClockStart = clock();
	typedef RTFocusRefinement::UnschedCoreComputer::SchedConds SchedConds;
	//Obtained Initial Solution
	RTFocusRefinement::UnschedCoreComputer::SchedConds cOptimalSchedCond;		
	RTFocusRefinement cRTFR(rcTaskSet);
	RTFocusRefinement::UnschedCoreComputer & cUnschedCoreComp = cRTFR.getUnschedCoreComputer();
	cout << "Generating RT Samples..." << endl;		
	cUnschedCoreComp.GenerateRTSamplesByResolution(dInitialResolution);
	cout << "Start Optimization..." << endl;
	//cRTFR.setSubILPDisplay(2);
	cRTFR.setLogFile("RTFRTet_log.txt");
	int iStatus = cRTFR.FocusRefinement(1);
	cOptimalSchedCond = cRTFR.getOptimalSchedCondConfig();
	double dResolution = dInitialResolution;
	double dRefinementLevel = 2;	
	while (true)
	{		
		RTFocusRefinement cRTFRIter(rcTaskSet);
		cRTFRIter.setLogFile("RTFRTet_log.txt");
		RTFocusRefinement::UnschedCoreComputer & cUnschedCoreCompIter = cRTFRIter.getUnschedCoreComputer();		
		cUnschedCoreCompIter.GenerateRTSamplesByResolution(dResolution / dRefinementLevel);
		for (SchedConds::iterator iter = cOptimalSchedCond.begin(); iter != cOptimalSchedCond.end(); iter++)
		{
			//cRTFRIter.SpecifyRTLB(iter->m_iTaskIndex, iter->m_dUpperBound - 5 * dResolution, iter->m_dUpperBound);
			cRTFRIter.SpecifyRTLB(iter->m_iTaskIndex,  0, iter->m_dUpperBound);
			cUnschedCoreCompIter.AddRTSamples(iter->m_iTaskIndex, iter->m_dUpperBound);
		}
		dResolution /= dRefinementLevel;
		cout << "At resolution " << dResolution << endl;
		int iStatus = cRTFRIter.FocusRefinement(1);
		if (iStatus != 1)
		{
			cout << "Infeasible?" << endl;
			while (1);
		}
		cOptimalSchedCond = cRTFRIter.getOptimalSchedCondConfig();
		if (dResolution <= dRefinementLevel)
		{
			PrintSetContent(cOptimalSchedCond.getSetContent());
			break;
		}
	}

	cout << clock() - iClockStart << " ms" << endl;
}

void BinaryResolutionRefinement(TaskSet & rcTaskSet)
{
	GET_TASKSET_NUM_PTR(&rcTaskSet, iTaskNum, pcTaskArray);
	double dOrder = 2;
	double dGreatestPeriod = 0;
	for (int i = 0; i < iTaskNum; i++) dGreatestPeriod = max(dGreatestPeriod, pcTaskArray[i].getPeriod());
	typedef RTFocusRefinement::UnschedCoreComputer::SchedConds SchedConds;
	SchedConds cOptimalSchedConds;
	for (int i = 0; i < iTaskNum; i++)
	{
		cOptimalSchedConds.insert({ i, pcTaskArray[i].getDeadline() });
	}

	while (true)
	{
		RTFocusRefinement cRTFR(rcTaskSet);
		cRTFR.setLogFile("RTFRTet_log.txt");
		RTFocusRefinement::UnschedCoreComputer & cUnschedCoreCompIter = cRTFR.getUnschedCoreComputer();
		for (int i = 0; i < iTaskNum; i++)
		{
			cUnschedCoreCompIter.GenerateRTSamplesByResolution(i, 0, pcTaskArray[i].getDeadline(), max(1.0, pcTaskArray[i].getDeadline() / dOrder));
		}
		for (SchedConds::iterator iter = cOptimalSchedConds.begin(); iter != cOptimalSchedConds.end(); iter++)
		{
			//cRTFRIter.SpecifyRTLB(iter->m_iTaskIndex, iter->m_dUpperBound - 5 * dResolution, iter->m_dUpperBound);
			cRTFR.SpecifyRTLB(iter->m_iTaskIndex, 0, iter->m_dUpperBound);
			cUnschedCoreCompIter.AddRTSamples(iter->m_iTaskIndex, iter->m_dUpperBound);
		}		
		cout << "At order " << dOrder << endl;
		int iStatus = cRTFR.FocusRefinement(1);
		if (iStatus != 1)
		{
			cout << "Infeasible?" << endl;
			while (1);
		}
		cOptimalSchedConds = cRTFR.getOptimalSchedCondConfig();
		if (dOrder >= 1024)
		{
			PrintSetContent(cOptimalSchedConds.getSetContent());
			break;
		}
		dOrder *= 2;
	}
}

void TestRTFocusRefinement()
{
	TaskSet cTaskSet;
#if 1
	cTaskSet.GenRandomTaskSetInteger(30, 0.75, 0.0, 1.0);
	cTaskSet.WriteImageFile("TestTaskSetForRTFocusRefinement.tskst");
	cTaskSet.WriteTextFile("TestTaskSetForRTFocusRefinement.tskst.txt");
#else
	cTaskSet.ReadImageFile("TestTaskSetForRTFocusRefinement.tskst");
#endif
	if (0)
	{
		RTFocusRefinement cRTFR(cTaskSet);
		RTFocusRefinement::UnschedCoreComputer & cUnschedCoreComp = cRTFR.getUnschedCoreComputer();
		cout << "Generating RT Samples..." << endl;

		GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
		double dSmallestPeriod = 1e74;
		for (int i = 0; i < iTaskNum; i++)
		{
			dSmallestPeriod = min(pcTaskArray[i].getPeriod(), dSmallestPeriod);
		}
		cout << dSmallestPeriod << endl;
		cUnschedCoreComp.GenerateRTSamplesByResolution(5000);

		cout << "Start Optimization..." << endl;
		//cRTFR.setSubILPDisplay(2);
		cRTFR.setLogFile("RTFRTet_log.txt");
		int iStatus = cRTFR.FocusRefinement(1);
		if (iStatus == 1)
			PrintSetContent(cRTFR.getOptimalSchedCondConfig().getSetContent());
	}

	if (0)
	{
		//DynamicResolutionStrategy(cTaskSet);
		BinaryResolutionRefinement(cTaskSet);
	}
	system("pause");
	cout << "Full MILP...." << endl;
	MinimizeLORTModel cModel(cTaskSet);
	cModel.Initialize(cTaskSet);
	cModel.SolveModel(1e74, 1);
	system("pause");
}

//Temporary for experimentation
RTFocusRefinement_TempExp::RTFocusRefinement_TempExp()
{

}

RTFocusRefinement_TempExp::RTFocusRefinement_TempExp(SystemType & rcSystem)
	:RTFocusRefinement(rcSystem)
{

}

void RTFocusRefinement_TempExp::ShuffleAndComputeMore(UnschedCores & rcCores)
{
	UnschedCores cLastCores = rcCores;
	UnschedCores cNewCores;
	while (true)
	{
		SchedConds cTotalSchedConds;
		for (UnschedCores::iterator iterCore = cLastCores.begin(); iterCore != cLastCores.end(); iterCore++)
		{
			m_cUnschedCoreComputer.CombineSchedConds(cTotalSchedConds, *iterCore);
		}
		cout << "Total Conditions: ";
		PrintSetContent(cTotalSchedConds.getSetContent());
		int iNewNum = m_cUnschedCoreComputer.ComputeUnschedCores(cTotalSchedConds, m_cSchedCondsFixed, cLastCores, m_iCorePerIter);
		cNewCores.clear();
		int iSize = cLastCores.size();
		for (int i = 0; i < iNewNum; i++)
		{
			cNewCores.push_back(cLastCores[iSize - 1 - i]);
			rcCores.push_back(cNewCores.back());
			cout << "Core: ";
			PrintSetContent(cNewCores.back().getSetContent());
			system("pause");
		}
		cLastCores = cNewCores;
		if (cNewCores.empty())
			break;
	}		

}

void RTFocusRefinement_TempExp::ComputeSchedConsImplication(SchedConds & rcPreCond, SchedConds & rcPostCond, UnschedCores & rcPostCondCores)
{
	SchedConds cSchedCondsFixed = m_cSchedCondsFixed;	
	for (SchedConds::iterator iter = rcPreCond.begin(); iter != rcPreCond.end(); iter++)
	{
		SchedConds::iterator iterFind = cSchedCondsFixed.find(*iter);
		if (iterFind != cSchedCondsFixed.end())
		{
			if (iter->m_dUpperBound > iterFind->m_dUpperBound)
			{
				cSchedCondsFixed.erase(*iterFind);
				cSchedCondsFixed.insert(*iter);
			}
		}
	}

	m_cUnschedCoreComputer.ComputeUnschedCores(rcPostCond, cSchedCondsFixed, rcPostCondCores, 1);
}

void RTFocusRefinement_TempExp::GenCoreImplicationConst(SchedCondsImplications & rcSchedCondsImplications, SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcConst.getEnv();
	for (SchedCondsImplications::iterator iterImp = rcSchedCondsImplications.begin(); iterImp != rcSchedCondsImplications.end(); iterImp++)
	{
		SchedConds & rcPreConds = iterImp->first;
		IloExpr cPreCondSatSum = ScheCondsSatSumExpr(rcEnv, rcPreConds, rcSchedCondVars);

		for (UnschedCores::iterator iterPostCond = iterImp->second.begin(); iterPostCond != iterImp->second.end(); iterPostCond++)
		{
			SchedConds & rcPostConds = *iterPostCond;
			IloExpr cPostCondSatSum = ScheCondsSatSumExpr(rcEnv, rcPreConds, rcSchedCondVars);			
		}		
	}
}

void RTFocusRefinement_TempExp::PostComputeUnschedCore(UnschedCores & rcUnschedCores)
{
	//ShuffleAndComputeMore(rcUnschedCores);
}

int RTFocusRefinement_TempExp::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		IloNumVarArray cRTVars(cSolveEnv);
		IloRangeArray cConst(cSolveEnv);
		SchedCondVars cSchedCondVars;
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);
		CreateRTModelVar(cSolveEnv, cRTVars);
		//GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		//GenSchedCondConsistencyConst(cSchedCondVars, cConst);
		//GenRTModelConst(cSolveEnv, cSchedCondVars, cRTVars, cConst);
		GenConjunctUnschedCoreCost(rcGenUC, cRTVars, cConst);
		GenSpecifiedRTRangeConst(cSolveEnv, cRTVars, cConst);
		IloObjective cObjective = IloMinimize(cSolveEnv, ObjExpr(cSolveEnv, cRTVars));
#if 0
		IloNumVar cMinSlack(cSolveEnv, 0.0, IloInfinity, IloNumVar::Float);
		for (int i = 0; i < iTaskNum; i++)
		{
			cConst.add(IloRange(cSolveEnv, 0.0, (pcTaskArray[i].getDeadline() - cRTVars[i]) - cMinSlack, IloInfinity));
		}
		cObjective = IloMinimize(cSolveEnv, -1.0 * cMinSlack);
#endif

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);
		cModel.add(cRTVars);
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		m_cSchedCondsConfig.clear();
		m_dequeSolutionSchedConds.clear();
		m_iBestSolFeasible = 0;
		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		m_cCplexStatus = cSolver.getCplexStatus();

		m_dequeSolutionSchedConds.clear();
		m_cSchedCondsConfig.clear();
		if (bStatus)
		{
			ExtractAllSolution(cSolver, cRTVars);
			MarshalAllSolution(m_dequeSolutionSchedConds);
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}

}

void RTFocusRefinement_TempExp::GenConjunctUnschedCoreCost(UnschedCores & rcCores, IloNumVarArray & rcRTVars, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcConst.getEnv();
	for (UnschedCores::iterator iterCore = rcCores.begin(); iterCore != rcCores.end(); iterCore++)
	{
		for (SchedConds::iterator iterEle = iterCore->begin(); iterEle != iterCore->end(); iterEle++)
		{
			int iTaskIndex = iterEle->m_iTaskIndex;
			double dBound = m_cUnschedCoreComputer.getRTSampleUB(iTaskIndex, iterEle->m_dUpperBound);
			rcConst.add(IloRange(rcEnv, dBound, rcRTVars[iTaskIndex], IloInfinity));
		}
	}
}

//RTSum Focus Refinement
RTSumFocusRefinement::RTSumFocusRefinement()
{

}

RTSumFocusRefinement::RTSumFocusRefinement(SystemType &rcSystem)
	:GeneralFocusRefinement_MK(rcSystem)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
}

void RTSumFocusRefinement::SpecifyRTLB(int iTaskIndex, double dLB, double dUB)
{
	m_mapSpecifiedRTRange[iTaskIndex] = { dLB, dUB };
}

void RTSumFocusRefinement::PrintResult(ostream & os)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	for (SchedConds::iterator iter = m_cSchedCondsConfig.begin(); iter != m_cSchedCondsConfig.end(); iter++)
	{
		if (iter->first.m_setTasks.size() == 1)
		{
			int iTaskIndex = *iter->first.m_setTasks.begin();
			os << "Task " << iTaskIndex << " / Config: " << iter->second << " / RT: " << cRTCalc.CalculateRTTask(iTaskIndex, this->m_cPriorityAssignmentSol) << endl;
		}		
	}
}

void RTSumFocusRefinement::ClearFR()
{
	GeneralFocusRefinement_MK::ClearFR();
	m_mapSpecifiedRTRange.clear();
}

void RTSumFocusRefinement::AddLinearRTConst(LinearRTConst & rcLinearConst)
{
	m_dequeLinearRTConst.push_back(rcLinearConst);	
}

void RTSumFocusRefinement::CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars)
{	
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		double dLB = pcTaskArray[i].getCLO();
		double dUB = pcTaskArray[i].getDeadline();
		SchedCondType cType;
		cType.m_setTasks.insert(i);
		string stringName;
		stringstream ss(stringName);		
		ss << "R_" << cType;		
		ss >> stringName;			
		//cout << stringName << endl;

		rcSchedCondTypeVars[cType] = IloNumVar(rcEnv, dLB, dUB, IloNumVar::Float, stringName.data());
	}
#if 0
	double dUB = 0;
	double dLB = 0;
	SchedCondType cSummationType;
	for (int i = 0; i < iTaskNum; i++)
	{
		dUB += pcTaskArray[i].getDeadline();
		dLB += pcTaskArray[i].getCLO();
		cSummationType.m_setTasks.insert(i);
	}	
	string stringName;
	stringstream ss(stringName);
	ss << "R_" << cSummationType;		
	ss.getline(axBuffer, 512);		
	rcSchedCondTypeVars[cSummationType] = IloNumVar(rcEnv, dLB, dUB, IloNumVar::Float, axBuffer);
#endif
}

IloNumVar RTSumFocusRefinement::CreateSchedCondTypeVar(IloEnv & rcEnv, MySet<int> & rsetTasks)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dUB = 0;
	double dLB = 0;
	for (MySet<int>::iterator iter = rsetTasks.begin(); iter != rsetTasks.end(); iter++)
	{
		dLB += pcTaskArray[*iter].getCLO();
		dUB += pcTaskArray[*iter].getDeadline();
	}
	RTSumRange_Type cType(rsetTasks);
	string stringName;
	stringstream ss(stringName);
	ss << "R_" << cType;
	ss.flush();
	return IloNumVar(rcEnv, dLB, dUB, IloNumVar::Float, stringName.data());
}

void RTSumFocusRefinement::CreateRTModelVar(const IloEnv & rcEnv, IloNumVarArray & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		sprintf_s(axBuffer, "R(%d)", i);
		rcRTVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));
	}
}

void RTSumFocusRefinement::GenSpecifiedRTRangeConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (SpeciryRTRange::iterator iter = m_mapSpecifiedRTRange.begin(); iter != m_mapSpecifiedRTRange.end(); iter++)
	{
		SchedCondType cQuery;
		cQuery.m_setTasks.insert(iter->first);
		assert(rcSchedCondTypeVars.count(cQuery));
		rcConst.add(IloRange(rcEnv, iter->second.first, rcSchedCondTypeVars[cQuery], iter->second.second));
	}
}

void RTSumFocusRefinement::GenLinearRTConst(deque<LinearRTConst> & rdequeLinearRTConst, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcConst.getEnv();
	for (deque<LinearRTConst>::iterator iter = rdequeLinearRTConst.begin(); iter != rdequeLinearRTConst.end(); iter++)
	{
		double dRHS = 0;
		IloExpr cExpr(rcEnv);
		for (LinearRTConst::iterator iterCoef = iter->begin(); iterCoef != iter->end(); iterCoef++)
		{
			if (iterCoef->first == -1)
			{
				dRHS = iterCoef->second;
				continue;
			}
			cExpr += iterCoef->second * rcSchedCondTypeVars[RTSumRange_Type(iterCoef->first)];
		}
		rcConst.add(IloRange(rcEnv, -IloInfinity, cExpr - dRHS, 0.0));
	}
}

void RTSumFocusRefinement::GenRTSumModelConst(const IloEnv & rcEnv, SchedCondVars & rcSchedCondVars, SchedCondTypeVars & rcRTSumVars, IloRangeArray & rcConst)
{
	for (SchedCondVars::iterator iterType = rcSchedCondVars.begin(); iterType != rcSchedCondVars.end(); iterType++)
	{
		for (SchedCondVars::mapped_type::iterator iterContent = iterType->second.begin(); iterContent != iterType->second.end(); iterContent++)
		{
			double dBound = iterContent->first + 1;
			rcConst.add(IloRange(rcEnv, 0.0, rcRTSumVars[iterType->first] - (1.0 - iterContent->second) * dBound, IloInfinity));
		}			
	}

	for (SchedCondTypeVars::iterator iterType = rcRTSumVars.begin(); iterType != rcRTSumVars.end(); iterType++)
	{
		const MySet<int> & rsetTasks = iterType->first.m_setTasks;
		IloExpr cExpr(rcEnv);
		for (MySet<int>::iterator iterTask = rsetTasks.begin(); iterTask != rsetTasks.end(); iterTask++)
		{
			SchedCondType cQuery;
			cQuery.m_setTasks.insert(*iterTask);
			assert(rcRTSumVars.count(cQuery));
			cExpr += rcRTSumVars[cQuery];
		}
		rcConst.add(IloRange(rcEnv, 0.0, cExpr - rcRTSumVars[iterType->first], IloInfinity));
		//rcConst.add(IloRange(rcEnv, 0.0, cExpr - rcRTSumVars[iterType->first], 0.0));
	}


}

void RTSumFocusRefinement::GenSchedCondConsistencyConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcConst.getEnv();
	for (SchedCondVars::iterator iterType = rcSchedCondVars.begin(); iterType != rcSchedCondVars.end(); iterType++)
	{
		SchedCondVars::mapped_type::iterator iterContent = iterType->second.begin();
		SchedCondVars::mapped_type::iterator iterContentNext = iterContent;
		iterContentNext++;
		for (; iterContentNext != iterType->second.end(); iterContentNext++)
		{
			rcConst.add(IloRange(rcEnv, -IloInfinity, iterContent->second - iterContentNext->second, 0.0));
		}
	}	
}

IloExpr RTSumFocusRefinement::ObjExpr(IloEnv & rcEnv, SchedCondTypeVars & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IloExpr cExpr(rcEnv);
	for (int i = 0; i < iTaskNum; i++)
	{
		SchedCondType cType;
		cType.m_setTasks.insert(i);
		//cExpr += rcRTVars[cType];
		cExpr += (i + 1) * rcRTVars[cType];
	}
	return cExpr;
}

int RTSumFocusRefinement::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;		
		IloRangeArray cConst(cSolveEnv);
		SchedCondVars cSchedCondVars;
		SchedCondTypeVars cSchedCondTypeVars;
		Variable2D cInterVars;
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);		
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);


		IloObjective cObjective;
		IloExpr cObjExpr(cSolveEnv);
#if 1
		SchedCondType cObjSchedCond;		
		for(int i = 0; i < iTaskNum; i++)
		//for (int i = 1; i < 3; i++)
		{
			cObjSchedCond.m_setTasks.insert(i);
		}
		cSchedCondTypeVars[cObjSchedCond] = CreateSchedCondTypeVar(cSolveEnv, cObjSchedCond.m_setTasks);
		assert(cSchedCondTypeVars.count(cObjSchedCond));
		cObjExpr = cSchedCondTypeVars[cObjSchedCond];		
#endif

#if 0
		cObjExpr = cDummyInteger;
		for (int i = 0; i < iTaskNum; i++)		
		{
			//cObjExpr += cSchedCondTypeVars[RTSumRange_Type(i)];
			cObjExpr += (i + 1) * cSchedCondTypeVars[RTSumRange_Type(i)];
		}		
#endif
		cObjective = IloMinimize(cSolveEnv, cObjExpr);

		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenSchedCondConsistencyConst(cSchedCondVars, cConst);		
		GenRTSumModelConst(cSolveEnv, cSchedCondVars, cSchedCondTypeVars, cConst);		
		GenSpecifiedRTRangeConst(cSolveEnv, cSchedCondTypeVars, cConst);
		GenLinearRTConst(m_dequeLinearRTConst, cSchedCondTypeVars, cConst);
		//GenE2ELatencyConst(cSolveEnv, cSchedCondTypeVars, cConst);
		
		//CreateInterfereVariable(cSolveEnv, cInterVars);
		//GenInterfereConst(cSolveEnv, cInterVars, cSchedCondTypeVars, cConst);

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);
		
		for (SchedCondTypeVars::iterator iter = cSchedCondTypeVars.begin(); iter != cSchedCondTypeVars.end(); iter++)
		{
			cModel.add(iter->second);
		}

		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		cSolver.exportModel("Temp.lp");
		int iRetStatus = 0;

		m_cSchedCondsConfig.clear();
		m_dequeSolutionSchedConds.clear();
		m_iBestSolFeasible = 0;
		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		m_cCplexStatus = cSolver.getCplexStatus();

		m_dequeSolutionSchedConds.clear();
		m_cSchedCondsConfig.clear();
		if (bStatus)
		{	
			ExtractAllSolution(cSolver, cSchedCondTypeVars);
			MarshalAllSolution(m_dequeSolutionSchedConds);
			//cout << endl;  m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
			if (1)
			{

				IloModel cAdjModel(cSolveEnv);
				IloExpr cAdjObjExpr(cSolveEnv);
				for (SchedCondTypeVars::iterator iter = cSchedCondTypeVars.begin(); iter != cSchedCondTypeVars.end(); iter++)
				{
					cAdjObjExpr += iter->second;
				}
				IloObjective cObjective = IloMaximize(cSolveEnv, cAdjObjExpr);
				cAdjModel.add(cObjective);
				cAdjModel.add(cConst);
				cAdjModel.add(cDummyInteger);
				double dObj = cSolver.getObjValue();
				cAdjModel.add(IloRange(cSolveEnv, -IloInfinity, cObjExpr - dObj - 0.1, 0.0));
				IloCplex cAdjSolver(cAdjModel);				
				setSolverParam(cAdjSolver);
				EnableSolverDisp(iDisplay, cAdjSolver);
				double dWallTime = cAdjSolver.getCplexTime();
				double dCPUTime = getCPUTimeStamp();
				bool bStatus = cAdjSolver.solve();				
				dWallTime = cAdjSolver.getCplexTime() - dWallTime;
				dCPUTime = getCPUTimeStamp() - dCPUTime;
				m_dSolveWallTime += dWallTime;
				m_dSolveCPUTime += dCPUTime;
				//cout << cAdjModel << endl;
				if (bStatus)
				{
					m_cSchedCondsConfig.clear();
					m_dequeSolutionSchedConds.clear();
					ExtractAllSolution(cAdjSolver, cSchedCondTypeVars);
					for (SolutionSchedCondsDeque::iterator iter = m_dequeSolutionSchedConds.begin();
						iter != m_dequeSolutionSchedConds.end(); iter++)
					{
						iter->first = dObj;
					}
					MarshalAllSolution(m_dequeSolutionSchedConds);
				}
				
				cAdjSolver.end();
				cAdjModel.end();
				cAdjObjExpr.end();
			//	cout << "Adjusted: ";
//				m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
			}
	
			//m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}

}

void RTSumFocusRefinement::ExtractSolution(IloCplex & rcSolver, int iSolIndex, SchedCondTypeVars & rcSchedCondTypeVars, SchedConds & rcSchedCondsConfig)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (SchedCondTypeVars::iterator iter = rcSchedCondTypeVars.begin(); iter != rcSchedCondTypeVars.end(); iter++)
	{
		double dValue = round(rcSolver.getValue(iter->second, iSolIndex));
		rcSchedCondsConfig.insert({ iter->first, dValue });
	}

}

void RTSumFocusRefinement::ExtractAllSolution(IloCplex & rcSolver, SchedCondTypeVars & rcSchedCondTypeVars)
{
	int iSolNum = rcSolver.getSolnPoolNsolns();
	m_dequeSolutionSchedConds.clear();
	for (int i = 0; i < iSolNum; i++)
	{
		SchedConds rcConfig;
		ExtractSolution(rcSolver, i, rcSchedCondTypeVars, rcConfig);
		double dObj = rcSolver.getObjValue(i);		
		m_dequeSolutionSchedConds.push_back({ dObj, rcConfig });
	}
}

//Experimental Strenghthening 
void RTSumFocusRefinement::CreateInterfereVariable(const IloEnv & rcEnv, Variable2D & rcInterVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			rcInterVars[i][j] = IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Float);
		}
	}
}

void RTSumFocusRefinement::GenInterfereConst(const IloEnv & rcEnv, Variable2D & rcInterVars, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		IloExpr cExpr(rcEnv);
		cExpr += pcTaskArray[i].getCLO();
		IloNumVar & rcThisSchedCondType = rcSchedCondTypeVars[RTSumRange_Type(i)];
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			IloNumVar & rcInterVar = rcInterVars[j][i];
			if (rcSchedCondTypeVars.count(j))
			{
				rcConst.add(IloRange(rcEnv, 0.0, rcInterVar - (rcThisSchedCondType - rcSchedCondTypeVars[RTSumRange_Type(j)]), IloInfinity));
			}
			else
			{
				rcConst.add(IloRange(rcEnv, 0.0, rcInterVar - (rcThisSchedCondType - pcTaskArray[j].getDeadline()), IloInfinity));
			}
			cExpr += (rcInterVar / pcTaskArray[j].getPeriod()) * pcTaskArray[j].getCLO();
		}
		rcConst.add(IloRange(rcEnv, 0.0, rcSchedCondTypeVars[RTSumRange_Type(i)] - cExpr));
	}
}

double dInvolvedTasksRatio = 0.5;
double dContrainedDelayRatio = 0.4;
double dLengthRatio = 0.5;

void RTSumFocusRefinement::GenE2ELatencyConst(IloEnv & rcEnv, SchedCondTypeVars & rcRTVars , IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iInvolvedTaskNum = iTaskNum * dInvolvedTasksRatio;
	iInvolvedTaskNum = max(iInvolvedTaskNum, 1);
	int iLength = iInvolvedTaskNum * dLengthRatio;
	iLength = max(1, iLength);
	for (int i = 0; i < iInvolvedTaskNum; i++)
	{
		double dDelay = 0;
		IloExpr cExpr(rcEnv);
		for (int j = 0; j < iLength; j++)
		{
			int iTaskIndex = i + j;
			if (iTaskIndex >= iTaskNum)	break;
			cExpr += rcRTVars[RTSumRange_Type(i)];			
			dDelay += pcTaskArray[iTaskIndex].getDeadline() * dContrainedDelayRatio;
		}
		rcConst.add(IloRange(rcEnv, 0.0, dDelay - cExpr, IloInfinity));
	}

}

void TestRTSumFocusRefinement()
{
	TaskSet cTaskSet;
	if (0)
	{
		cTaskSet.AddTask(5, 5, 0.2, 1.0, false);
		cTaskSet.AddTask(10, 10, 0.3, 1.0, false);
		cTaskSet.AddTask(35, 35, 0.4, 1.0, false);
		cTaskSet.ConstructTaskArray();
	}

	{
#if 0
		cTaskSet.GenRandomTaskSetInteger(40, 0.85, 0.5, 2.0);
		cTaskSet.WriteImageFile("TestTaskSetForRTFocusRefinement.tskst");
		cTaskSet.WriteTextFile("TestTaskSetForRTFocusRefinement.tskst.txt");
#else
		cTaskSet.ReadImageFile("TestTaskSetForRTFocusRefinement.tskst");
#endif
	}
	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
	MySet<int> setTasks;
	for (int i = 0; i < iTaskNum; i++)	setTasks.insert(i);

	int iOpt = 0;	
	cout << "Do MILP ?" << endl;
	cin >> iOpt;
	if (iOpt)
	{
		cout << "Full MILP" << endl;
		//Full MILP
		RTSRTPMILPModel cMILPModel(cTaskSet);
		cMILPModel.Solve(2.0);
		cout << "MILP Result: " << cMILPModel.getObj() << endl;
		cMILPModel.PrintResult(cout);
	}

	iOpt = 0;	
	cout << "Do FR ?" << endl;
	cin >> iOpt;
	if (iOpt)
	{
		typedef RTSumFocusRefinement::SchedConds SchedConds;
		SchedConds cDummy;
		TaskSetPriorityStruct cPriorityStruct(cTaskSet);
		RTSumRangeUnschedCoreComputer cUCCalc(cTaskSet);
		cout << "Min RT Sum: " << cUCCalc.ComputeMinRTSum(setTasks, cDummy, cDummy, cPriorityStruct) << endl;

		cout << "RT Sum FR" << endl;
		cTaskSet.DisplayTaskInfo();
		RTSumFocusRefinement cModel(cTaskSet);
		cModel.setCorePerIter(5);
		cModel.setLogFile("RTSumFR_Log.txt");
		cModel.FocusRefinement(1);

	}




	system("pause");
}

//Experimental Weighted RT sum FR
WeightedRTSumFocusRefinement::WeightedRTSumFocusRefinement()
{

}

WeightedRTSumFocusRefinement::WeightedRTSumFocusRefinement(SystemType & rcSystem)
	:RTSumFocusRefinement(rcSystem)
{

}

void WeightedRTSumFocusRefinement::setWeight(vector<double> & rvectorWeight)
{
	m_vectorWeightTable = rvectorWeight;
}

void WeightedRTSumFocusRefinement::PostComputeUnschedCore(UnschedCores & rcUnschedCores)
{
//	cout << "Post Compute UC" << endl;
	for (UnschedCores::iterator iter = rcUnschedCores.begin(); iter != rcUnschedCores.end(); iter++)
	{
		MySet<int> setElementTasks;
		for (UnschedCore::iterator iterEle = iter->begin(); iterEle != iter->end(); iterEle++)
		{
			setElementTasks.insert(*iterEle->first.m_setTasks.begin());
		}
		SchedConds cDummy;
		TaskSetPriorityStruct cPA(*m_pcTaskSet);
		double dLB = m_cUnschedCoreComputer.ComputeMinRTSum(setElementTasks, cDummy, cDummy, cPA);
		m_dequeUnschedCoreSummationBound.push_back(dLB);
	}	
//	cout << "Post Compute UC End" << endl;
}

void WeightedRTSumFocusRefinement::GenerateUnschedCoreStrengtheningConst(const IloEnv & rcEnv, UnschedCores & rcGenUC, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	//cout << "UC LB Strengthen" << endl;
	assert(m_dequeUnschedCoreSummationBound.size() == rcGenUC.size());
	for (UnschedCores::iterator iterCore = rcGenUC.begin(); iterCore != rcGenUC.end(); iterCore++)
	{
		IloExpr cExpr(rcEnv);
		for (UnschedCore::iterator iterEle = iterCore->begin(); iterEle != iterCore->end(); iterEle++)
		{
			int iTaskIndex = *iterEle->first.m_setTasks.begin();
			cExpr += rcSchedCondTypeVars[RTSumRange_Type(iTaskIndex)];
		}	
		double dLB = m_dequeUnschedCoreSummationBound[iterCore - rcGenUC.begin()];	
		rcConst.add(IloRange(rcEnv, dLB, cExpr, IloInfinity));
	}
	//cout << "UC LB Strengthen end" << endl;
}

int WeightedRTSumFocusRefinement::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		IloRangeArray cConst(cSolveEnv);
		SchedCondVars cSchedCondVars;
		SchedCondTypeVars cSchedCondTypeVars;
		Variable2D cInterVars;
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);


		IloObjective cObjective;
		IloExpr cObjExpr(cSolveEnv);
#if 0
		SchedCondType cObjSchedCond;
		for (int i = 0; i < iTaskNum; i++)			
		{
			cObjSchedCond.m_setTasks.insert(i);
		}
		cSchedCondTypeVars[cObjSchedCond] = CreateSchedCondTypeVar(cSolveEnv, cObjSchedCond.m_setTasks);
		assert(cSchedCondTypeVars.count(cObjSchedCond));

		IloExpr cSumConst(cSolveEnv);
		for (MySet<int>::iterator iter = cObjSchedCond.m_setTasks.begin(); iter != cObjSchedCond.m_setTasks.end(); iter++)
		{
			cSumConst += cSchedCondTypeVars[RTSumRange_Type(*iter)];
		}
		cConst.add(IloRange(cSolveEnv, 0.0, cSchedCondTypeVars[cObjSchedCond] - cSumConst, 0.0));

		assert(m_vectorWeightTable.size() == iTaskNum);
		double dMaxWeight = 0;
		for (int i = 0; i < iTaskNum; i++)
		{
			dMaxWeight = max(dMaxWeight, m_vectorWeightTable[i]);
		}

		cObjExpr = dMaxWeight * cSchedCondTypeVars[cObjSchedCond];
		for (int i = 0; i < iTaskNum; i++)
		{
			cObjExpr -= (dMaxWeight - m_vectorWeightTable[i]) * cSchedCondTypeVars[RTSumRange_Type(i)];
		}

#endif

#if 1
		cObjExpr = IloExpr(cSolveEnv);
		for (int i = 0; i < iTaskNum; i++)
		{			
			if (m_setConcernedTasks.count(i))
				cObjExpr += (m_vectorWeightTable[i]) * cSchedCondTypeVars[RTSumRange_Type(i)];
			else
			{
				cSchedCondTypeVars.erase(RTSumRange_Type(i));
			}
		}
#endif

		cObjective = IloMinimize(cSolveEnv, cObjExpr);

		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenSchedCondConsistencyConst(cSchedCondVars, cConst);
		GenRTSumModelConst(cSolveEnv, cSchedCondVars, cSchedCondTypeVars, cConst);
		GenSpecifiedRTRangeConst(cSolveEnv, cSchedCondTypeVars, cConst);
		GenLinearRTConst(m_dequeLinearRTConst, cSchedCondTypeVars, cConst);
		//GenE2ELatencyConst(cSolveEnv, cSchedCondTypeVars, cConst);

		//CreateInterfereVariable(cSolveEnv, cInterVars);
		//GenInterfereConst(cSolveEnv, cInterVars, cSchedCondTypeVars, cConst);
		GenerateUnschedCoreStrengtheningConst(cSolveEnv, rcGenUC, cSchedCondTypeVars, cConst);

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);

		for (SchedCondTypeVars::iterator iter = cSchedCondTypeVars.begin(); iter != cSchedCondTypeVars.end(); iter++)
		{			
			if (iter->first.m_setTasks.size() == 1)
			{
				int iTask = *iter->first.m_setTasks.begin();
				if (m_setConcernedTasks.count(iTask) == 0)	continue;
			}
			cModel.add(iter->second);
		}

		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);
		cSolver.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-3);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		m_cSchedCondsConfig.clear();
		m_dequeSolutionSchedConds.clear();
		m_iBestSolFeasible = 0;
		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		m_cCplexStatus = cSolver.getCplexStatus();

		m_dequeSolutionSchedConds.clear();
		m_cSchedCondsConfig.clear();
		if (bStatus)
		{
			ExtractAllSolution(cSolver, cSchedCondTypeVars);
			MarshalAllSolution(m_dequeSolutionSchedConds);
			//cout << endl;  m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
			if (0)
			{

				IloModel cAdjModel(cSolveEnv);
				IloExpr cAdjObjExpr(cSolveEnv);
				for (SchedCondTypeVars::iterator iter = cSchedCondTypeVars.begin(); iter != cSchedCondTypeVars.end(); iter++)
				{
					cAdjObjExpr += iter->second;
				}
				IloObjective cObjective = IloMaximize(cSolveEnv, cAdjObjExpr);
				cAdjModel.add(cObjective);
				cAdjModel.add(cConst);
				cAdjModel.add(cDummyInteger);
				double dObj = cSolver.getObjValue();
				cAdjModel.add(IloRange(cSolveEnv, -IloInfinity, cObjExpr - dObj + 0.8, 0.0));
				IloCplex cAdjSolver(cAdjModel);
				setSolverParam(cAdjSolver);
				EnableSolverDisp(iDisplay, cAdjSolver);
				double dWallTime = cAdjSolver.getCplexTime();
				double dCPUTime = getCPUTimeStamp();
				bool bStatus = cAdjSolver.solve();
				dWallTime = cAdjSolver.getCplexTime() - dWallTime;
				dCPUTime = getCPUTimeStamp() - dCPUTime;
				m_dSolveWallTime += dWallTime;
				m_dSolveCPUTime += dCPUTime;

				if (bStatus)
				{
					m_cSchedCondsConfig.clear();
					m_dequeSolutionSchedConds.clear();
					ExtractAllSolution(cAdjSolver, cSchedCondTypeVars);
					for (SolutionSchedCondsDeque::iterator iter = m_dequeSolutionSchedConds.begin();
						iter != m_dequeSolutionSchedConds.end(); iter++)
					{
						iter->first = dObj;
					}
					MarshalAllSolution(m_dequeSolutionSchedConds);
				}

				cAdjSolver.end();
				cAdjModel.end();
				cAdjObjExpr.end();
				//	cout << "Adjusted: ";
				//				m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
			}

			//m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}

}


//RTSum Focus Refinement
WRTSDFocusRefinement::WRTSDFocusRefinement()
{

}

WRTSDFocusRefinement::WRTSDFocusRefinement(SystemType &rcSystem)
	:GeneralFocusRefinement_MK(rcSystem)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
}

void WRTSDFocusRefinement::PrintResult(ostream & os)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	for (SchedConds::iterator iter = m_cSchedCondsConfig.begin(); iter != m_cSchedCondsConfig.end(); iter++)
	{
		if (iter->first.m_setTasks.size() == 1)
		{
			int iTaskIndex = *iter->first.m_setTasks.begin();
			os << "Task " << iTaskIndex << " / Config: " << iter->second << " / RT: " << cRTCalc.CalculateRTTask(iTaskIndex, this->m_cPriorityAssignmentSol) << endl;
		}
	}
}

void WRTSDFocusRefinement::ClearFR()
{
	GeneralFocusRefinement_MK::ClearFR();
	m_mapSpecifiedRTRange.clear();
}

void WRTSDFocusRefinement::AddLinearRTConst(LinearRTConst & rcLinearConst)
{
	m_dequeLinearRTConst.push_back(rcLinearConst);
}

void WRTSDFocusRefinement::CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	const vector<double> & rvectorWeightTable = m_pcTaskSet->getWeightTable();
	for (int i = 0; i < iTaskNum; i++)
	{
		double dLB = pcTaskArray[i].getCLO() * rvectorWeightTable[i];
		double dUB = pcTaskArray[i].getDeadline() * rvectorWeightTable[i];
		SchedCondType cType;
		cType.m_setTasks.insert(i);
		string stringName;
		stringstream ss(stringName);
		ss << "R_" << cType;
		ss >> stringName;
		//cout << stringName << endl;

		rcSchedCondTypeVars[cType] = IloNumVar(rcEnv, dLB, dUB, IloNumVar::Float, stringName.data());
	}
#if 0
	double dUB = 0;
	double dLB = 0;
	SchedCondType cSummationType;
	for (int i = 0; i < iTaskNum; i++)
	{
		dUB += pcTaskArray[i].getDeadline();
		dLB += pcTaskArray[i].getCLO();
		cSummationType.m_setTasks.insert(i);
	}
	string stringName;
	stringstream ss(stringName);
	ss << "R_" << cSummationType;
	ss.getline(axBuffer, 512);
	rcSchedCondTypeVars[cSummationType] = IloNumVar(rcEnv, dLB, dUB, IloNumVar::Float, axBuffer);
#endif
}

IloNumVar WRTSDFocusRefinement::CreateSchedCondTypeVar(IloEnv & rcEnv, MySet<int> & rsetTasks)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dUB = 0;
	double dLB = 0;
	const vector<double> & rvectorWeightTable = m_pcTaskSet->getWeightTable();
	for (MySet<int>::iterator iter = rsetTasks.begin(); iter != rsetTasks.end(); iter++)
	{
		dLB += pcTaskArray[*iter].getCLO() * rvectorWeightTable[*iter];
		dUB += pcTaskArray[*iter].getDeadline() * rvectorWeightTable[*iter];
	}
	RTSumRange_Type cType(rsetTasks);
	string stringName;
	stringstream ss(stringName);
	ss << "R_" << cType;
	ss.flush();		
	return IloNumVar(rcEnv, dLB, dUB, IloNumVar::Float, ss.str().data());
}

void WRTSDFocusRefinement::CreateRTModelVar(const IloEnv & rcEnv, IloNumVarArray & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		sprintf_s(axBuffer, "R(%d)", i);
		rcRTVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));
	}
}

void WRTSDFocusRefinement::GenLinearRTConst(deque<LinearRTConst> & rdequeLinearRTConst, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcConst.getEnv();
	const vector<double> & rvectorWeightTable = m_pcTaskSet->getWeightTable();
	for (deque<LinearRTConst>::iterator iter = rdequeLinearRTConst.begin(); iter != rdequeLinearRTConst.end(); iter++)
	{
		double dRHS = 0;
		IloExpr cExpr(rcEnv);
		for (LinearRTConst::iterator iterCoef = iter->begin(); iterCoef != iter->end(); iterCoef++)
		{
			if (iterCoef->first == -1)
			{
				dRHS = iterCoef->second;
				continue;
			}
			double dWeight = rvectorWeightTable[iterCoef->first];
			assert(dWeight > 0);
			cExpr += iterCoef->second * rcSchedCondTypeVars[RTSumRange_Type(iterCoef->first)] / dWeight;
		}
		rcConst.add(IloRange(rcEnv, -IloInfinity, cExpr - dRHS, 0.0));
	}
}

void WRTSDFocusRefinement::GenRTSumModelConst(const IloEnv & rcEnv, SchedCondVars & rcSchedCondVars, SchedCondTypeVars & rcRTSumVars, IloRangeArray & rcConst)
{
	for (SchedCondVars::iterator iterType = rcSchedCondVars.begin(); iterType != rcSchedCondVars.end(); iterType++)
	{
		for (SchedCondVars::mapped_type::iterator iterContent = iterType->second.begin(); iterContent != iterType->second.end(); iterContent++)
		{
			double dBound = iterContent->first + 1;
			rcConst.add(IloRange(rcEnv, 0.0, rcRTSumVars[iterType->first] - (1.0 - iterContent->second) * dBound, IloInfinity));
		}
	}

	for (SchedCondTypeVars::iterator iterType = rcRTSumVars.begin(); iterType != rcRTSumVars.end(); iterType++)
	{
		const MySet<int> & rsetTasks = iterType->first.m_setTasks;
		IloExpr cExpr(rcEnv);
		for (MySet<int>::iterator iterTask = rsetTasks.begin(); iterTask != rsetTasks.end(); iterTask++)
		{
			SchedCondType cQuery;
			cQuery.m_setTasks.insert(*iterTask);
			assert(rcRTSumVars.count(cQuery));
			cExpr += rcRTSumVars[cQuery];
		}
		rcConst.add(IloRange(rcEnv, 0.0, cExpr - rcRTSumVars[iterType->first], IloInfinity));
		//rcConst.add(IloRange(rcEnv, 0.0, cExpr - rcRTSumVars[iterType->first], 0.0));
	}


}

void WRTSDFocusRefinement::GenSchedCondConsistencyConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcConst.getEnv();
	for (SchedCondVars::iterator iterType = rcSchedCondVars.begin(); iterType != rcSchedCondVars.end(); iterType++)
	{
		SchedCondVars::mapped_type::iterator iterContent = iterType->second.begin();
		SchedCondVars::mapped_type::iterator iterContentNext = iterContent;
		iterContentNext++;
		for (; iterContentNext != iterType->second.end(); iterContentNext++)
		{
			rcConst.add(IloRange(rcEnv, -IloInfinity, iterContent->second - iterContentNext->second, 0.0));
		}
	}
}

IloExpr WRTSDFocusRefinement::ObjExpr(IloEnv & rcEnv, SchedCondTypeVars & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IloExpr cExpr(rcEnv);
	for (int i = 0; i < iTaskNum; i++)
	{
		SchedCondType cType;
		cType.m_setTasks.insert(i);
		//cExpr += rcRTVars[cType];
		cExpr += (i + 1) * rcRTVars[cType];
	}
	return cExpr;
}

int WRTSDFocusRefinement::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		IloRangeArray cConst(cSolveEnv);
		SchedCondVars cSchedCondVars;
		SchedCondTypeVars cSchedCondTypeVars;
		Variable2D cInterVars;
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);


		IloObjective cObjective;
		IloExpr cObjExpr(cSolveEnv);
#if 1
		SchedCondType cObjSchedCond;
		for (int i = 0; i < iTaskNum; i++)
			//for (int i = 1; i < 3; i++)
		{
			cObjSchedCond.m_setTasks.insert(i);
		}
		cSchedCondTypeVars[cObjSchedCond] = CreateSchedCondTypeVar(cSolveEnv, cObjSchedCond.m_setTasks);
		assert(cSchedCondTypeVars.count(cObjSchedCond));
		cObjExpr = cSchedCondTypeVars[cObjSchedCond];
#endif

		cObjective = IloMinimize(cSolveEnv, cObjExpr);

		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenSchedCondConsistencyConst(cSchedCondVars, cConst);
		GenRTSumModelConst(cSolveEnv, cSchedCondVars, cSchedCondTypeVars, cConst);		
		GenLinearRTConst(m_dequeLinearRTConst, cSchedCondTypeVars, cConst);
		//GenE2ELatencyConst(cSolveEnv, cSchedCondTypeVars, cConst);

		//CreateInterfereVariable(cSolveEnv, cInterVars);
		//GenInterfereConst(cSolveEnv, cInterVars, cSchedCondTypeVars, cConst);

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);

		for (SchedCondTypeVars::iterator iter = cSchedCondTypeVars.begin(); iter != cSchedCondTypeVars.end(); iter++)
		{
			cModel.add(iter->second);
		}

		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		cSolver.exportModel("Temp.lp");
		int iRetStatus = 0;

		m_cSchedCondsConfig.clear();
		m_dequeSolutionSchedConds.clear();
		m_iBestSolFeasible = 0;
		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		m_cCplexStatus = cSolver.getCplexStatus();

		m_dequeSolutionSchedConds.clear();
		m_cSchedCondsConfig.clear();
		if (bStatus)
		{
			ExtractAllSolution(cSolver, cSchedCondTypeVars);
			MarshalAllSolution(m_dequeSolutionSchedConds);
			//cout << endl;  m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
			if (1)
			{

				IloModel cAdjModel(cSolveEnv);
				IloExpr cAdjObjExpr(cSolveEnv);
				for (SchedCondTypeVars::iterator iter = cSchedCondTypeVars.begin(); iter != cSchedCondTypeVars.end(); iter++)
				{
					cAdjObjExpr += iter->second;
				}
				IloObjective cObjective = IloMaximize(cSolveEnv, cAdjObjExpr);
				cAdjModel.add(cObjective);
				cAdjModel.add(cConst);
				cAdjModel.add(cDummyInteger);
				double dObj = cSolver.getObjValue();
				cAdjModel.add(IloRange(cSolveEnv, -IloInfinity, cObjExpr - dObj - 0.1, 0.0));
				IloCplex cAdjSolver(cAdjModel);
				setSolverParam(cAdjSolver);
				EnableSolverDisp(iDisplay, cAdjSolver);
				double dWallTime = cAdjSolver.getCplexTime();
				double dCPUTime = getCPUTimeStamp();
				bool bStatus = cAdjSolver.solve();
				dWallTime = cAdjSolver.getCplexTime() - dWallTime;
				dCPUTime = getCPUTimeStamp() - dCPUTime;
				m_dSolveWallTime += dWallTime;
				m_dSolveCPUTime += dCPUTime;
				//cout << cAdjModel << endl;
				if (bStatus)
				{
					m_cSchedCondsConfig.clear();
					m_dequeSolutionSchedConds.clear();
					ExtractAllSolution(cAdjSolver, cSchedCondTypeVars);
					for (SolutionSchedCondsDeque::iterator iter = m_dequeSolutionSchedConds.begin();
						iter != m_dequeSolutionSchedConds.end(); iter++)
					{
						iter->first = dObj;
					}
					MarshalAllSolution(m_dequeSolutionSchedConds);
				}

				cAdjSolver.end();
				cAdjModel.end();
				cAdjObjExpr.end();
				//	cout << "Adjusted: ";
				//				m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
			}

			//m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}

}

void WRTSDFocusRefinement::ExtractSolution(IloCplex & rcSolver, int iSolIndex, SchedCondTypeVars & rcSchedCondTypeVars, SchedConds & rcSchedCondsConfig)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (SchedCondTypeVars::iterator iter = rcSchedCondTypeVars.begin(); iter != rcSchedCondTypeVars.end(); iter++)
	{
		double dValue = round(rcSolver.getValue(iter->second, iSolIndex));
		rcSchedCondsConfig.insert({ iter->first, dValue });
	}

}

void WRTSDFocusRefinement::ExtractAllSolution(IloCplex & rcSolver, SchedCondTypeVars & rcSchedCondTypeVars)
{
	int iSolNum = rcSolver.getSolnPoolNsolns();
	m_dequeSolutionSchedConds.clear();
	for (int i = 0; i < iSolNum; i++)
	{
		SchedConds rcConfig;
		ExtractSolution(rcSolver, i, rcSchedCondTypeVars, rcConfig);
		double dObj = rcSolver.getObjValue(i);
		m_dequeSolutionSchedConds.push_back({ dObj, rcConfig });
	}
}



//Full RT MILP Model

RTSRTPMILPModel::RTSRTPMILPModel()
{
	m_pcTaskSet = nullptr;
}

RTSRTPMILPModel::RTSRTPMILPModel(TaskSet & rcTaskSet)
{
	m_pcTaskSet = &rcTaskSet;
}

void RTSRTPMILPModel::CreateInterInstVariable(const IloEnv & rcEnv, Variable2D & rcInterInstVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			sprintf_s(axBuffer, "I(%d, %d)", i, j);
			rcInterInstVars[i][j] = IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Int, axBuffer);
		}
	}
}

void RTSRTPMILPModel::CreatePriorityVariable(const IloEnv & rcEnv, Variable2D & rcPriorityVariable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			sprintf_s(axBuffer, "p(%d, %d)", i, j);
			rcPriorityVariable[i][j] = IloNumVar(rcEnv, 0.0, 1.0, IloNumVar::Int, axBuffer);
		}
	}

}

void RTSRTPMILPModel::CreatePInterProdVaraible(const IloEnv & rcEnv, Variable2D & rcPIPVariable)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			sprintf_s(axBuffer, "PIP(%d, %d)", i, j);
			rcPIPVariable[i][j] = IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Float, axBuffer);
		}
	}
}

void RTSRTPMILPModel::CreateRTVariable(const IloEnv & rcEnv, Variable1D & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		sprintf_s(axBuffer, "R(%d)", i);
		rcRTVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));
	}
}

void RTSRTPMILPModel::GenInterInstConst(const IloEnv & rcEnv,
	Variable2D & rcInterInstVars, Variable1D & rcRTVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			rcConst.add(IloRange(rcEnv, 0.0, rcInterInstVars[i][j] - rcRTVars[j] / pcTaskArray[i].getPeriod(), IloInfinity));
			//rcConst.add(IloRange(rcEnv, -IloInfinity, rcInterInstVars[i][j] - rcRTVars[j] / pcTaskArray[i].getPeriod() - 1 + 1e-8, 0.0));
		}
	}
}

void RTSRTPMILPModel::GenPIPModelConst(const IloEnv & rcEnv, Variable2D & rcPIPVars, Variable2D & rcInterInstVars, Variable2D & rcPVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			double dBigM = DetermineBigMForPIP(i, j);
			//rcConst.add(IloRange(rcEnv, 0.0, rcPIPVars[i][j] - rcInterInstVars[i][j] + dBigM * rcPVars[j][i], IloInfinity));			
			rcConst.add(IloRange(rcEnv, 0.0, rcPIPVars[i][j] - rcInterInstVars[i][j] + dBigM * (1 - rcPVars[i][j]), IloInfinity));
			rcConst.add(IloRange(rcEnv, 0.0, rcInterInstVars[i][j] - rcPIPVars[i][j], IloInfinity));
		}
	}
}

void RTSRTPMILPModel::GenRTVarConst(const IloEnv & rcEnv, Variable1D & rcRTVars, Variable2D & rcPIPVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		IloExpr cExpr(rcEnv);
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			cExpr += rcPIPVars[j][i] * pcTaskArray[j].getCLO();
		}
		cExpr += pcTaskArray[i].getCLO();
		rcConst.add(IloRange(rcEnv, 0.0, rcRTVars[i] - cExpr, 0.0));
	}
}

void RTSRTPMILPModel::GenPriorityModelConst(const IloEnv & rcEnv, Variable2D & rcPVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			rcConst.add(IloRange(rcEnv, 0.0, rcPVars[i][j] + rcPVars[j][i] - 1.0, 0.0));
		}
	}

	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			for (int k = 0; k < iTaskNum; k++)
			{
				if ((k == j) || (k == i))	continue;
				rcConst.add(IloRange(rcEnv, 0.0, rcPVars[i][j] - rcPVars[i][k] - rcPVars[k][j] + 1.0, IloInfinity));
			}
		}
	}
}

double RTSRTPMILPModel::DetermineBigMForPIP(int i, int j)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	return (ceil(pcTaskArray[j].getPeriod() / pcTaskArray[i].getCLO()));
}

int RTSRTPMILPModel::Solve(int iDisplayLevel, double dTimeout)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		Variable1D cRTVars(cSolveEnv);
		Variable2D cInterInstVars;
		Variable2D cPIPVars;
		Variable2D cPVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateRTVariable(cSolveEnv, cRTVars);
		CreateInterInstVariable(cSolveEnv, cInterInstVars);
		CreatePInterProdVaraible(cSolveEnv, cPIPVars);
		CreatePriorityVariable(cSolveEnv, cPVars);
		//cout << "0" << endl;
		GenInterInstConst(cSolveEnv, cInterInstVars, cRTVars, cConst);
		//cout << "1" << endl;
		GenPIPModelConst(cSolveEnv, cPIPVars, cInterInstVars, cPVars, cConst);
		//cout << "2" << endl;
		GenPriorityModelConst(cSolveEnv, cPVars, cConst);
		//cout << "3" << endl;
		GenRTVarConst(cSolveEnv, cRTVars, cPIPVars, cConst);
		//cout << "4" << endl;
		GenE2ELatencyConst(cSolveEnv, cRTVars, cConst);
#if 1
		IloExpr cObjExpr(cSolveEnv);
		for (int i = 0; i < iTaskNum; i++)
		{
			cObjExpr += cRTVars[i];
			//cObjExpr += (i + 1) * cRTVars[i];
		}
		IloObjective cObjective = IloMinimize(cSolveEnv, cObjExpr);
#endif

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		//cModel.add(cDummyInteger);
		//cModel.add(cRTVars);
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplayLevel, cSolver);
		setSolverParam(cSolver);

		//cSolver.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-9);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		IloCplex::Status m_cCplexStatus = cSolver.getCplexStatus();
		if (bStatus)
		{			
			m_dObjective = cSolver.getObjValue();
			ExtractPrioritySol(cSolver, cPVars, m_cPrioritySol);
			_LORTCalculator cRTCalc(*m_pcTaskSet);
			for (int i = 0; i < iTaskNum; i++)
			{
				assert(cRTCalc.CalculateRTTask(i, m_cPrioritySol) <= pcTaskArray[i].getDeadline());
			}
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}
}

int RTSRTPMILPModel::Solve(MySet<int> & rsetConcernedTasks, int iDisplayLevel, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		Variable1D cRTVars(cSolveEnv);
		Variable2D cInterInstVars;
		Variable2D cPIPVars;
		Variable2D cPVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateRTVariable(cSolveEnv, cRTVars);
		CreateInterInstVariable(cSolveEnv, cInterInstVars);
		CreatePInterProdVaraible(cSolveEnv, cPIPVars);
		CreatePriorityVariable(cSolveEnv, cPVars);
		//cout << "0" << endl;
		GenInterInstConst(cSolveEnv, cInterInstVars, cRTVars, cConst);
		//cout << "1" << endl;
		GenPIPModelConst(cSolveEnv, cPIPVars, cInterInstVars, cPVars, cConst);
		//cout << "2" << endl;
		GenPriorityModelConst(cSolveEnv, cPVars, cConst);
		//cout << "3" << endl;
		GenRTVarConst(cSolveEnv, cRTVars, cPIPVars, cConst);
		//cout << "4" << endl;

#if 1
		IloExpr cObjExpr(cSolveEnv);
		//for (int i = 0; i < iTaskNum; i++)
		for (MySet<int>::iterator iter = rsetConcernedTasks.begin(); iter != rsetConcernedTasks.end(); iter++)
		{
			int i = *iter;
			cObjExpr += cRTVars[i];
		}
		IloObjective cObjective = IloMinimize(cSolveEnv, cObjExpr);
#endif

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		//cModel.add(cDummyInteger);
		//cModel.add(cRTVars);
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplayLevel, cSolver);
		setSolverParam(cSolver);

		//cSolver.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-9);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		IloCplex::Status m_cCplexStatus = cSolver.getCplexStatus();
		if (bStatus)
		{
			m_dObjective = cSolver.getObjValue();
			ExtractPrioritySol(cSolver, cPVars, m_cPrioritySol);
			_LORTCalculator cRTCalc(*m_pcTaskSet);
			for (int i = 0; i < iTaskNum; i++)
			{
				assert(cRTCalc.CalculateRTTask(i, m_cPrioritySol) <= pcTaskArray[i].getDeadline());
			}
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}
}

void RTSRTPMILPModel::EnableSolverDisp(int iLevel, IloCplex & rcSolver)
{
	const IloEnv & rcEnv = rcSolver.getEnv();
	if (iLevel)
	{
		rcSolver.setParam(IloCplex::Param::MIP::Display, iLevel);
	}
	else
	{
		rcSolver.setParam(IloCplex::Param::MIP::Display, 0);
		rcSolver.setWarning(rcEnv.getNullStream());
		rcSolver.setOut(rcEnv.getNullStream());
	}
}

void RTSRTPMILPModel::setSolverParam(IloCplex & rcSolver)
{
	rcSolver.setParam(IloCplex::Param::Emphasis::MIP, 0);
	rcSolver.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, 1e-9);
	rcSolver.setParam(IloCplex::Param::MIP::Tolerances::Integrality, 0);
}

void RTSRTPMILPModel::ExtractPrioritySol(IloCplex & rcSolver, Variable2D & rcPVars, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	rcPriorityAssignment = TaskSetPriorityStruct(*m_pcTaskSet);
	for (int i = 0; i < iTaskNum; i++)
	{
		int iPriorityLevel = 0;
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j) continue;
			iPriorityLevel += round(rcSolver.getValue(rcPVars[j][i]));			
		}		
		rcPriorityAssignment.setPriority(i, iPriorityLevel);
	}
}

void RTSRTPMILPModel::PrintResult(ostream & os)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	double dSum = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		double dRT = cRTCalc.CalculateRTTask(i, m_cPrioritySol);
		sprintf_s(axBuffer, "Task %d / RT: %.2f / P: %d", i, dRT, m_cPrioritySol.getPriorityByTask(i));
		os << axBuffer << endl;
		dSum += dRT;
	}
	os << "RT Sum: " << dSum << endl;
}

void RTSRTPMILPModel::PrintResult(MySet<int> & rsetConcernedTasks, ostream & os)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	double dSum = 0;
	for (MySet<int>::iterator iter = rsetConcernedTasks.begin(); iter != rsetConcernedTasks.end(); iter++)
	{
		int i = *iter;
		double dRT = cRTCalc.CalculateRTTask(i, m_cPrioritySol);
		sprintf_s(axBuffer, "Task %d / RT: %.2f / P: %d", i, dRT, m_cPrioritySol.getPriorityByTask(i));
		os << axBuffer << endl;
		dSum += dRT;
	}
	os << "RT Sum: " << dSum << endl;
}

void RTSRTPMILPModel::GenE2ELatencyConst(IloEnv & rcEnv, IloNumVarArray & rcRTVars, IloRangeArray & rcConst)
{
/*
	double dInvolvedTasksRatio = 0.3;
	double dContrainedDelayRatio = 0.5;
	double dLengthRatio = 0.5;*/

	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iInvolvedTaskNum = iTaskNum * dInvolvedTasksRatio;
	iInvolvedTaskNum = max(iInvolvedTaskNum, 1);
	int iLength = iInvolvedTaskNum * dLengthRatio;
	iLength = max(1, iLength);
	for (int i = 0; i < iInvolvedTaskNum; i++)
	{
		double dDelay = 0;
		IloExpr cExpr(rcEnv);
		for (int j = 0; j < iLength; j++)
		{
			int iTaskIndex = i + j;
			if (iTaskIndex >= iTaskNum)	break;
			cExpr += rcRTVars[i];
			dDelay += pcTaskArray[iTaskIndex].getDeadline() * dContrainedDelayRatio;
		}
		rcConst.add(IloRange(rcEnv, 0.0, dDelay - cExpr, IloInfinity));
	}

}

NonpreemptiveRTMILP::NonpreemptiveRTMILP()
{

}

NonpreemptiveRTMILP::NonpreemptiveRTMILP(TaskSet & rcTaskSet)
	:RTSRTPMILPModel(rcTaskSet)
{

}

NonpreemptiveRTMILP::~NonpreemptiveRTMILP()
{

}

void NonpreemptiveRTMILP::CreateBlockingVars(const IloEnv & rcEnv, Variable1D & rcBlockingVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		rcBlockingVars.add(IloNumVar(rcEnv, 0.0, IloInfinity));
	}
}

void NonpreemptiveRTMILP::GenBlockingVarConst(const IloEnv & rcEnv, Variable1D & rcBlockingVars, Variable2D & rcPVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	for (int i = 0; i < iTaskNum; i++)
	{		
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			rcConst.add(IloRange(rcEnv, 0.0, rcBlockingVars[i] - rcPVars[i][j] * pcTaskArray[j].getCLO()));
			//rcConst.add(IloRange(rcEnv, 0.0, rcBlockingVars[i] - pcTaskArray[j].getCLO()));
		}
		rcConst.add(IloRange(rcEnv, 0.0, rcBlockingVars[i] - pcTaskArray[i].getCLO(), IloInfinity));
	}
}

void NonpreemptiveRTMILP::GenInterInstConst(const IloEnv & rcEnv, Variable2D & rcInterInstVars, Variable1D & rcRTVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			rcConst.add(IloRange(rcEnv, 0.0, rcInterInstVars[i][j] - (rcRTVars[j] - pcTaskArray[j].getCLO()) / pcTaskArray[i].getPeriod(), IloInfinity));
			//rcConst.add(IloRange(rcEnv, -IloInfinity, rcInterInstVars[i][j] - (rcRTVars[j] - pcTaskArray[j].getCLO()) / pcTaskArray[i].getPeriod() - 1 + 1e-8, 0.0));
		}
	}
}

void NonpreemptiveRTMILP::GenRTVarConst(const IloEnv & rcEnv, Variable1D & rcRTVars, Variable1D & rcBlockingVars, Variable2D & rcPIPVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		IloExpr cExpr(rcEnv);
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			cExpr += rcPIPVars[j][i] * pcTaskArray[j].getCLO();
		}
		cExpr += pcTaskArray[i].getCLO() + rcBlockingVars[i];
		rcConst.add(IloRange(rcEnv, 0.0, rcRTVars[i] - cExpr, 0.0));
	}
}

void NonpreemptiveRTMILP::GenPIPModelConst(const IloEnv & rcEnv, Variable2D & rcPIPVars, Variable2D & rcInterInstVars, Variable2D & rcPVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			double dBigM = (ceil((pcTaskArray[j].getPeriod() + pcTaskArray[j].getCLO()) / pcTaskArray[i].getCLO()));
			//rcConst.add(IloRange(rcEnv, 0.0, rcPIPVars[i][j] - rcInterInstVars[i][j] + dBigM * rcPVars[j][i], IloInfinity));			
			rcConst.add(IloRange(rcEnv, 0.0, rcPIPVars[i][j] - rcInterInstVars[i][j] + dBigM * (1 - rcPVars[i][j]), IloInfinity));
			rcConst.add(IloRange(rcEnv, 0.0, rcInterInstVars[i][j] - rcPIPVars[i][j], IloInfinity));
		}
	}
}

int NonpreemptiveRTMILP::Solve(int iDisplayLevel, double dTimeout)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		Variable1D cRTVars(cSolveEnv);
		Variable1D cBlockingVars(cSolveEnv);
		Variable2D cInterInstVars;
		Variable2D cPIPVars;
		Variable2D cPVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateRTVariable(cSolveEnv, cRTVars);
		CreateBlockingVars(cSolveEnv, cBlockingVars);
		CreateInterInstVariable(cSolveEnv, cInterInstVars);
		CreatePInterProdVaraible(cSolveEnv, cPIPVars);
		CreatePriorityVariable(cSolveEnv, cPVars);
		//cout << "0" << endl;
		GenInterInstConst(cSolveEnv, cInterInstVars, cRTVars, cConst);
		//cout << "1" << endl;
		GenPIPModelConst(cSolveEnv, cPIPVars, cInterInstVars, cPVars, cConst);
		//cout << "2" << endl;
		GenPriorityModelConst(cSolveEnv, cPVars, cConst);
		//cout << "3" << endl;
		GenRTVarConst(cSolveEnv, cRTVars, cBlockingVars, cPIPVars, cConst);
		//cout << "4" << endl;
		//GenE2ELatencyConst(cSolveEnv, cRTVars, cConst);
		GenBlockingVarConst(cSolveEnv, cBlockingVars, cPVars, cConst);
#if 1
		IloExpr cObjExpr(cSolveEnv);
		for (int i = 0; i < iTaskNum; i++)
		{
			cObjExpr += cRTVars[i];
			//cObjExpr += (i + 1) * cRTVars[i];
		}
		IloObjective cObjective = IloMinimize(cSolveEnv, cObjExpr);
#endif

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		//cModel.add(cDummyInteger);
		//cModel.add(cRTVars);
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplayLevel, cSolver);
		setSolverParam(cSolver);

		//cSolver.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-9);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		IloCplex::Status m_cCplexStatus = cSolver.getCplexStatus();
		if (bStatus)
		{
			m_dObjective = cSolver.getObjValue();
			ExtractPrioritySol(cSolver, cPVars, m_cPrioritySol);
			_LORTCalculator cRTCalc(*m_pcTaskSet);
			for (int i = 0; i < iTaskNum; i++)
			{
				assert(cRTCalc.CalculateRTTask(i, m_cPrioritySol) <= pcTaskArray[i].getDeadline());
			}
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}
}

void NonpreemptiveRTMILP::PrintResult(ostream & os)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	NonpreemptiveMinAvgRT_Exp cRTCalc(*m_pcTaskSet);
	double dSum = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		double dRT = cRTCalc.ComputeRT(i, m_cPrioritySol);
		sprintf_s(axBuffer, "Task %d / RT: %.2f / P: %d", i, dRT, m_cPrioritySol.getPriorityByTask(i));
		os << axBuffer << endl;
		dSum += dRT;
	}
	os << "RT Sum: " << dSum << endl;
}

//MinWeighted RT Sum MILP
MinWeightedRTSumMILP::MinWeightedRTSumMILP()
{

}

MinWeightedRTSumMILP::MinWeightedRTSumMILP(TaskSet & rcTaskSet)
	:RTSRTPMILPModel(rcTaskSet)
{

}

void MinWeightedRTSumMILP::PrintResult(ostream & os)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	double dSum = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		double dRT = cRTCalc.CalculateRTTask(i, m_cPrioritySol);
		sprintf_s(axBuffer, "Task %d / RT: %.2f / P: %d", i, dRT, m_cPrioritySol.getPriorityByTask(i));
		os << axBuffer << endl;
		dSum += (i + 1) * dRT;
	}
	os << "RT Sum: " << dSum << endl;
}

int MinWeightedRTSumMILP::Solve(int iDisplayLevel, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		assert(m_vectorWeight.size() == iTaskNum);
		IloEnv cSolveEnv;
		Variable1D cRTVars(cSolveEnv);
		Variable2D cInterInstVars;
		Variable2D cPIPVars;
		Variable2D cPVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateRTVariable(cSolveEnv, cRTVars);
		CreateInterInstVariable(cSolveEnv, cInterInstVars);
		CreatePInterProdVaraible(cSolveEnv, cPIPVars);
		CreatePriorityVariable(cSolveEnv, cPVars);
		//cout << "0" << endl;
		GenInterInstConst(cSolveEnv, cInterInstVars, cRTVars, cConst);
		//cout << "1" << endl;
		GenPIPModelConst(cSolveEnv, cPIPVars, cInterInstVars, cPVars, cConst);
		//cout << "2" << endl;
		GenPriorityModelConst(cSolveEnv, cPVars, cConst);
		//cout << "3" << endl;
		GenRTVarConst(cSolveEnv, cRTVars, cPIPVars, cConst);
		//cout << "4" << endl;		
#if 1		
		IloExpr cObjExpr(cSolveEnv);
		for (int i = 0; i < iTaskNum; i++)
		{						
			if(m_setConcernedTasks.count(i))
				cObjExpr += m_vectorWeight[i] * cRTVars[i];
		}
		IloObjective cObjective = IloMinimize(cSolveEnv, cObjExpr);
#endif

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);				
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplayLevel, cSolver);
		setSolverParam(cSolver);
		cSolver.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		IloCplex::Status m_cCplexStatus = cSolver.getCplexStatus();
		if (bStatus)
		{
			m_dObjective = cSolver.getObjValue();
			ExtractPrioritySol(cSolver, cPVars, m_cPrioritySol);
			_LORTCalculator cRTCalc(*m_pcTaskSet);
			for (int i = 0; i < iTaskNum; i++)
			{
				assert(cRTCalc.CalculateRTTask(i, m_cPrioritySol) <= pcTaskArray[i].getDeadline());
			}
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}
}

void MinWeightedRTSumMILP::setWeight(vector<double> & rvectorWeight)
{
	m_vectorWeight = rvectorWeight;
}

//MinWeightedRTSumMILP_NonPreemptive
MinWeightedRTSumMILP_NonPreemptive::MinWeightedRTSumMILP_NonPreemptive()
{

}

MinWeightedRTSumMILP_NonPreemptive::MinWeightedRTSumMILP_NonPreemptive(TaskSet & rcTaskSet)
	:NonpreemptiveRTMILP(rcTaskSet)
{
	m_cStartingSolution = TaskSetPriorityStruct(rcTaskSet);
}

void MinWeightedRTSumMILP_NonPreemptive::PrintResult(ostream & os)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	_LORTCalculator cRTCalc(*m_pcTaskSet);
	double dSum = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		double dRT = cRTCalc.CalculateRTTask(i, m_cPrioritySol);
		sprintf_s(axBuffer, "Task %d / RT: %.2f / P: %d", i, dRT, m_cPrioritySol.getPriorityByTask(i));
		os << axBuffer << endl;
		dSum += (i + 1) * dRT;
	}
	os << "RT Sum: " << dSum << endl;
}

int MinWeightedRTSumMILP_NonPreemptive::Solve(int iDisplayLevel, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		Variable1D cRTVars(cSolveEnv);
		Variable1D cBlockingVars(cSolveEnv);
		Variable2D cInterInstVars;
		Variable2D cPIPVars;
		Variable2D cPVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateRTVariable(cSolveEnv, cRTVars);
		CreateBlockingVars(cSolveEnv, cBlockingVars);
		CreateInterInstVariable(cSolveEnv, cInterInstVars);
		CreatePInterProdVaraible(cSolveEnv, cPIPVars);
		CreatePriorityVariable(cSolveEnv, cPVars);
		//cout << "0" << endl;
		GenInterInstConst(cSolveEnv, cInterInstVars, cRTVars, cConst);
		//cout << "1" << endl;
		GenPIPModelConst(cSolveEnv, cPIPVars, cInterInstVars, cPVars, cConst);
		//cout << "2" << endl;
		GenPriorityModelConst(cSolveEnv, cPVars, cConst);
		//cout << "3" << endl;
		GenRTVarConst(cSolveEnv, cRTVars, cBlockingVars, cPIPVars, cConst);
		//cout << "4" << endl;
		//GenE2ELatencyConst(cSolveEnv, cRTVars, cConst);
		GenBlockingVarConst(cSolveEnv, cBlockingVars, cPVars, cConst);
#if 1
		IloExpr cObjExpr(cSolveEnv);
		for (int i = 0; i < iTaskNum; i++)
		{
			cObjExpr += m_vectorWeight[i] * cRTVars[i];
			//cObjExpr += (i + 1) * cRTVars[i];
		}
		IloObjective cObjective = IloMinimize(cSolveEnv, cObjExpr);
#endif

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		//cModel.add(cDummyInteger);
		//cModel.add(cRTVars);
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplayLevel, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-9);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		//see if there is an starting solution
		if (m_cStartingSolution.getSize() == iTaskNum)
		{
			AddMILPStart(cSolver, m_cStartingSolution,
				cRTVars,
				cInterInstVars,
				cPIPVars,
				cPVars,
				cBlockingVars);
		}

		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		IloCplex::Status m_cCplexStatus = cSolver.getCplexStatus();
		if (bStatus)
		{
			m_dObjective = cSolver.getObjValue();
			ExtractPrioritySol(cSolver, cPVars, m_cPrioritySol);
			_NonPreemptiveLORTCalculator cRTCalc(*m_pcTaskSet);
			for (int i = 0; i < iTaskNum; i++)
			{
				assert(cRTCalc.CalculateRT(i, m_cPrioritySol) <= pcTaskArray[i].getDeadline());
			}
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}
}

void MinWeightedRTSumMILP_NonPreemptive::setWeight(vector<double> & rvectorWeight)
{
	m_vectorWeight = rvectorWeight;
}

int MinWeightedRTSumMILP_NonPreemptive::TestPriorityAssignment(TaskSetPriorityStruct & rcPA, int iDisplayLevel, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		Variable1D cRTVars(cSolveEnv);
		Variable1D cBlockingVars(cSolveEnv);
		Variable2D cInterInstVars;
		Variable2D cPIPVars;
		Variable2D cPVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateRTVariable(cSolveEnv, cRTVars);
		CreateBlockingVars(cSolveEnv, cBlockingVars);
		CreateInterInstVariable(cSolveEnv, cInterInstVars);
		CreatePInterProdVaraible(cSolveEnv, cPIPVars);
		CreatePriorityVariable(cSolveEnv, cPVars);
		//cout << "0" << endl;
		GenInterInstConst(cSolveEnv, cInterInstVars, cRTVars, cConst);		
		GenPIPModelConst(cSolveEnv, cPIPVars, cInterInstVars, cPVars, cConst);		
		GenPriorityModelConst(cSolveEnv, cPVars, cConst);		
		GenRTVarConst(cSolveEnv, cRTVars, cBlockingVars, cPIPVars, cConst);				
		GenBlockingVarConst(cSolveEnv, cBlockingVars, cPVars, cConst);

		for (int i = 0; i < iTaskNum; i++)
		{
			for (int j = 0; j < iTaskNum; j++)
			{
				if (i == j)	continue;
				if (rcPA.getPriorityByTask(i) < rcPA.getPriorityByTask(j))
				{
					//cConst.add(cPVars[i][j] == 1);
					cConst.add(IloRange(cSolveEnv, 1.0, cPVars[i][j], 1.0));
				}
				else
				{
					cConst.add(IloRange(cSolveEnv, 0.0, cPVars[i][j], 0.0));
				}
			}
		}

		IloExpr cObjExpr(cSolveEnv);
		for (int i = 0; i < iTaskNum; i++)
		{
			cObjExpr += cBlockingVars[i];
		}
		IloObjective cObj = IloMinimize(cSolveEnv, cObjExpr);
		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		//cModel.add(cObj);
		IloCplex cSolver(cModel);
		cSolver.exportModel("Temp.lp");
		EnableSolverDisp(iDisplayLevel, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-9);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;

		//see if there is an starting solutionr

		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		IloCplex::Status m_cCplexStatus = cSolver.getCplexStatus();
		if (bStatus)
		{
			cout << "MILP Blocking: " << endl;
			for (int i = 0; i < iTaskNum; i++)
			{
				cout << "Task " << i << ": " << cSolver.getValue(cBlockingVars[i]) << endl;
			}
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			iRetStatus = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			iRetStatus = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			iRetStatus = 0;
		else
			iRetStatus = 0;
		cSolveEnv.end();
		return iRetStatus;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}
}

void MinWeightedRTSumMILP_NonPreemptive::AddMILPStart(IloCplex & rcSolver, TaskSetPriorityStruct & rcPriorityAssignment,
	Variable1D & rcRTVars,
	Variable2D & rcInterInstVars,
	Variable2D & rcPIPVars,
	Variable2D & rcPVars,
	Variable1D & rcBlockingVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	if (m_cStartingSolution.getSize() != iTaskNum)	return;
	vector<double> vectorRTTable;
	vector<double> vectorBlockingTable;
	vectorRTTable.reserve(iTaskNum);
	vectorBlockingTable.reserve(iTaskNum);
	_NonPreemptiveLORTCalculator cCalc(*m_pcTaskSet);
	const IloEnv & rcEnv = rcSolver.getEnv();
	IloNumVarArray cAllVariables(rcEnv);
	IloNumArray cAllValues(rcEnv);
	for (int i = 0; i < iTaskNum; i++)
	{
		double dRT = cCalc.CalculateRT(i, rcPriorityAssignment);
		vectorRTTable.push_back(dRT);
		vectorBlockingTable.push_back(cCalc.ComputeBlocking(i, rcPriorityAssignment));
		if (dRT > pcTaskArray[i].getDeadline())
			return;
	}
#if 1
	//cRTVars
	for (int i = 0; i < iTaskNum; i++)
	{
		cAllVariables.add(rcRTVars[i]);
		cAllValues.add(vectorRTTable[i]);
	}
#endif

#if 1
	//Blocking vars
	for (int i = 0; i < iTaskNum; i++)
	{
		cAllVariables.add(rcBlockingVars[i]);
		cAllValues.add(vectorBlockingTable[i]);
	}
#endif

#if 1
	//rcInterInstVars
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			int iInterInst = round(ceil((vectorRTTable[j] - pcTaskArray[j].getCLO()) / pcTaskArray[i].getPeriod()));
			cAllVariables.add(rcInterInstVars[i][j]);
			cAllValues.add(iInterInst);
		}
	}
#endif

#if 1
	//rcPIPVars
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			//double dBigM = DetermineBigMForPIP(i, j);
			//rcConst.add(IloRange(rcEnv, 0.0, rcPIPVars[i][j] - rcInterInstVars[i][j] + dBigM * rcPVars[j][i], IloInfinity));			
			//rcConst.add(IloRange(rcEnv, 0.0, rcPIPVars[i][j] - rcInterInstVars[i][j] + dBigM * (1 - rcPVars[i][j]), IloInfinity));
			//rcConst.add(IloRange(rcEnv, 0.0, rcInterInstVars[i][j] - rcPIPVars[i][j], IloInfinity));
			double dValue = 0;
			if (rcPriorityAssignment.getPriorityByTask(i) < rcPriorityAssignment.getPriorityByTask(j))
			{
				dValue = round(ceil((vectorRTTable[j] - pcTaskArray[j].getCLO()) / pcTaskArray[i].getPeriod()));
			}
			else
			{
				dValue = 0;
			}
			cAllVariables.add(rcPIPVars[i][j]);
			cAllValues.add(dValue);
		}
	}
#endif

#if 1
	//rcPVars
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			double dValue = 0;
			if (rcPriorityAssignment.getPriorityByTask(i) < rcPriorityAssignment.getPriorityByTask(j))
			{
				dValue = 1;
			}
			else
			{
				dValue = 0;
			}
			cAllVariables.add(rcPVars[i][j]);
			cAllValues.add(dValue);
		}
	}
#endif
	rcSolver.addMIPStart(cAllVariables, cAllValues, IloCplex::MIPStartSolveFixed);
}


void TestRTSumAlgorithm()
{
	TaskSet cTaskSet;
	if (0)
	{
		cTaskSet.AddTask(5, 5, 0.2, 1.0, false);
		cTaskSet.AddTask(10, 10, 0.3, 1.0, false);
		cTaskSet.AddTask(35, 35, 0.4, 1.0, false);
		cTaskSet.ConstructTaskArray();
	}

	{
#if 0
		cTaskSet.GenRandomTaskSetInteger(60, 0.85, 0.5, 2.0);
		cTaskSet.WriteImageFile("TestTaskSetForRTFocusRefinement.tskst");
		cTaskSet.WriteTextFile("TestTaskSetForRTFocusRefinement.tskst.txt");
#else
		cTaskSet.ReadImageFile("TestTaskSetForRTFocusRefinement.tskst");
#endif
	}
	if (0)
	{
		cTaskSet = TaskSet();
		//cTaskSet.ReadImageFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\LDSNotOptimal\\LDSNotOptimal_Better_45.tskst");
		//cTaskSet.ReadImageFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\LDSNotOptimal\\LDSNotOptimal_Worse_0.tskst");
		cTaskSet.ReadImageFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\LDSNotOptimal\\LDSNotOptimal_Better_38735.tskst");
	}

	cTaskSet.DisplayTaskInfo();
	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
	//Rate Monotonic
	
	MySet<int> setConcernedTask;
	for (int i = 0; i < iTaskNum; i++)	setConcernedTask.insert(i);
	


	//MinRTSum Alg
	if (1)
	{
		cout << "Min RT Sum Algorithm" << endl;
		RTSumRangeUnschedCoreComputer cCalc(cTaskSet);		
		RTSumRangeUnschedCoreComputer::SchedConds cDummy;
		TaskSetPriorityStruct cPA(cTaskSet);
		double dSec = getWallTimeSecond();
		double dRTSum = cCalc.ComputeMinRTSum(setConcernedTask, cDummy, cDummy, cPA);
		dSec = getWallTimeSecond() - dSec;
		cout << "Min RT Sum: " << dRTSum << endl;
		cout << "Time: " << dSec * 1000 << "ms" << endl;
	}

	//Full MILP
	{
		cout << "Full MILP...." << endl;
		system("pause");
		{
			RTSRTPMILPModel cModel(cTaskSet);
			int iStatus = cModel.Solve(setConcernedTask, 2, 1e74);
			if (iStatus == 1)
			{
				cModel.PrintResult(setConcernedTask, cout);
			}
		}
		system("pause");
	}


}


void TestMinUnweightAlg()
{	
	WeightedTaskSet cTaskSet;
	{
#if 0
		cTaskSet.GenRandomTaskSetInteger(20, 0.85, 0.5, 2.0);
		cTaskSet.WriteImageFile("TestTaskSetForRTFocusRefinement.tskst");
		cTaskSet.WriteTextFile("TestTaskSetForRTFocusRefinement.tskst.txt");
#else
		cTaskSet.ReadImageFile("TestTaskSetForRTFocusRefinement.tskst");
		//cTaskSet.ReadImageFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\MaxSuboptimality.tskst");
		cTaskSet.ReadImageFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\TaskSet.tskst");
#endif
	}
	cTaskSet.DisplayTaskInfo();
	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
	//Rate Monotonic
	vector<double> vectorWeight;
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorWeight.push_back(i + 1);
	}
	vectorWeight = cTaskSet.getWeightTable();
	MySet<int> setConcernedTask;
	for (int i = 0; i < iTaskNum; i++)	setConcernedTask.insert(i);
		
	//MinRTSum Alg
	if (1)
	{
		cout << "Min Weighted RT Sum Heuristic Algorithm " << endl;
		MinWeightedRTSumHeuristic cCalc(cTaskSet);
		cCalc.setWeight(vectorWeight);
		MinWeightedRTSumHeuristic::SchedConds cDummy;
		TaskSetPriorityStruct cPA(cTaskSet);
		double dSec = getWallTimeSecond();
		double dRTSum = cCalc.ComputeMinWeightedRTSum(setConcernedTask, cDummy, cDummy, cPA);		
		dSec = getWallTimeSecond() - dSec;
		cout << "Min RT Sum by Method 1: " << dRTSum << endl;			
		AnalyzeSuboptimalityFromSmallExample_PrintInfo(cTaskSet, cPA, vectorWeight);
	}

	if (0)
	{
		//Greedy Heuristic
		cout << "Greedy Heuristic" << endl;
		GreedyLowestPriorityMinWeightedRTSum(cTaskSet, vectorWeight);
	}

	//BnB
	if (0)
	{
		WeightedRTSumBnB cBnB(cTaskSet);
		cBnB.setWeightTable(vectorWeight);
		int iStatus = cBnB.Run();
		if (iStatus == 1)
		{
			cout << "By BnB: " << cBnB.m_dBestCost << endl;
		}
		else
		{
			cout << "No solution from BnB: " << iStatus << endl;
		}
	}

	//MinWeightedRTSumFR
	if (0)
	{
		WeightedRTSumFocusRefinement cMinWRTSum(cTaskSet);
		cMinWRTSum.setWeight(vectorWeight);
		cMinWRTSum.setCorePerIter(5);
		//cMinWRTSum.setSubILPDisplay(2);
		cMinWRTSum.setLogFile("MinWeightedRTSumFR_Log.txt");
		cMinWRTSum.FocusRefinement(1);
	}

	//Full MILP
	{
		cout << "Full MILP...." << endl;
		//system("pause");
		if (1)
		{
			MinWeightedRTSumMILP cModel(cTaskSet);
			cModel.setWeight(vectorWeight);
			cModel.setConcernedTasks(setConcernedTask);
			cModel.Solve(5, 20);
			cout << cModel.getObj() << endl;			
			//cModel.PrintResult(cout);
			TaskSetPriorityStruct cPA = cModel.getPrioritySol();
			AnalyzeSuboptimalityFromSmallExample_PrintInfo(cTaskSet, cPA, vectorWeight);
		}
		system("pause");
	}

}

void TestMinUnweightHeuristicSuboptimality()
{
	int iTestNum = 1;
	double dAvgRTSubOptimality_Max = 0;
	double dAvgRTSubOptimality_Sum = 0;
	double dHeuristicSubOptimality_Max = 0;
	double dHeuristicSubOptimality_Sum = 0;
	double dHeuristicRollingSubOptimality_Max = 0;
	double dHeuristicRollingSubOptimality_Sum = 0;
	StatisticSet cStatistic;
		
	while (true)
	{				
		WeightedTaskSet cTaskSet;
		//cTaskSet.GenRandomTaskSetInteger(20, 0.75, 0.5, 2.0);
		cTaskSet.GenRandomTaskset(20, 0.90, 0.0, 1.0, PERIODOPT_LOG);
		cTaskSet.GenRandomWeight(10000);
		cTaskSet.WriteImageFile("CurrentTestCase.tskst");
		cTaskSet.WriteTextFile("CurrentTestCase.txt");
		GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
		MySet<int> setConcernedTask;
		for (int i = 0; i < iTaskNum; i++)	setConcernedTask.insert(i);
		vector<double> vectorWeightTable;
		vectorWeightTable.reserve(iTaskNum);
		vectorWeightTable = cTaskSet.getWeightTable();
		//Full MILP
		MinWeightedRTSumMILP cModel(cTaskSet);
		cModel.setWeight(vectorWeightTable);
		cModel.setConcernedTasks(setConcernedTask);
		int iMILPStatus = 1;
		iMILPStatus = cModel.Solve(2, 600);
		double dMILPResult = cModel.getObj();
		cout << "By MILP: " << dMILPResult << endl;
		if (iMILPStatus != 1)	continue;
		
		//Bnb
#if 0
		WeightedRTSumBnB cBnB(cTaskSet);
		cBnB.setWeightTable(vectorWeightTable);
		int iBnBStatus = cBnB.Run();
		if (iBnBStatus != 1)	continue;
		double dBnBResult = cBnB.m_dBestCost;
		dMILPResult = dBnBResult;
		cout << "By BnB: " << dBnBResult << endl;
#endif
		TaskSetPriorityStruct cPAAvg(cTaskSet);
		RTSumRangeUnschedCoreComputer cMinAvgRT(cTaskSet);
		RTSumRangeUnschedCoreComputer::SchedConds cDummy;
		int iResult = cMinAvgRT.ComputeMinRTSum(setConcernedTask, cDummy, cDummy, cPAAvg);		
		if (iResult < 0)	continue;
		_LORTCalculator cRTCalc(cTaskSet);
		double dAvgRTResult = 0;
		for (int i = 0; i < iTaskNum; i++)
		{
			double dRT = cRTCalc.CalculateRTTask(i, cPAAvg);
			dAvgRTResult += vectorWeightTable[i] * dRT;
		}
		cout << "By Optimal Average RT Algorithm: " << dAvgRTResult << endl;
		double dAvgRTSubOptimality_This = (dAvgRTResult - dMILPResult) / dMILPResult;
		if (dAvgRTResult < dMILPResult)	continue;
		dAvgRTSubOptimality_Max = max(dAvgRTSubOptimality_Max, dAvgRTSubOptimality_This);
		dAvgRTSubOptimality_Sum += dAvgRTSubOptimality_This;


		//MinRTSum Alg
		MinWeightedRTSumHeuristic cHeuristic(cTaskSet);
		cHeuristic.setWeight(vectorWeightTable);
		MinWeightedRTSumHeuristic::SchedConds cDummyHeu;
		TaskSetPriorityStruct cPA(cTaskSet);		
		vector<double> vectorRTTable;
		double dHeuristicResult = cHeuristic.ComputeMinWeightedRTSum(setConcernedTask, cDummyHeu, cDummyHeu, cPA);						
		cout << "By Heuristic: " << dHeuristicResult << endl;
		if (dHeuristicResult < 0)	continue;
		if (dHeuristicResult < dMILPResult)	continue;;
		double dHeuristicSubOptimality_This = (dHeuristicResult - dMILPResult) / dMILPResult;
		dHeuristicSubOptimality_Max = max(dHeuristicSubOptimality_Max, dHeuristicSubOptimality_This);
		dHeuristicSubOptimality_Sum += dHeuristicSubOptimality_This;

		//MinRTSum Alg Rolling Adjustment
		TaskSetPriorityStruct cPARolling(cTaskSet);
#if 0
		double dRollingAdjustment = cHeuristic.ComputeMinWeightedRTSum_RollingAdjustment(setConcernedTask, cDummyHeu, cDummyHeu, cPARolling);
#else
		cPARolling = cPA;
		double dRollingAdjustment = cHeuristic.RollingToFixedPointAdjustment(setConcernedTask, cPARolling, vectorWeightTable);		
#endif
		cout << "By Rolling Adjustment: " << dRollingAdjustment << endl;
		cout << "------------------------------------" << endl;
		double dHeuristicRollingSubOptimality_This = (dRollingAdjustment - dMILPResult) / dMILPResult;		
		dHeuristicRollingSubOptimality_Max = max(dHeuristicRollingSubOptimality_Max, dHeuristicRollingSubOptimality_This);
		dHeuristicRollingSubOptimality_Sum += dHeuristicRollingSubOptimality_This;

		if (DOUBLE_EQUAL(dHeuristicRollingSubOptimality_Max, dHeuristicRollingSubOptimality_This, 1e-6))
		{
			cTaskSet.WriteImageFile("MaxSuboptimality.tskst");
			cTaskSet.WriteTextFile("MaxSuboptimality.tskst.txt");
		}

		cStatistic.setItem("Test Num", iTestNum);
		cStatistic.setItem("AvgRT Maximum Sub-optimality", dAvgRTSubOptimality_Max);
		cStatistic.setItem("Heuristic Maximum Sub-optimality", dHeuristicSubOptimality_Max);
		cStatistic.setItem("AvgRT Average Sub-optimality", dAvgRTSubOptimality_Sum / iTestNum);
		cStatistic.setItem("Heuristic Average Sub-optimality", dHeuristicSubOptimality_Sum / iTestNum);		
		cStatistic.setItem("Rolling Average Sub-optimality", dHeuristicRollingSubOptimality_Sum / iTestNum);		
		cStatistic.setItem("Rolling Maximum Sub-optimality", dHeuristicRollingSubOptimality_Max);
		cStatistic.WriteStatisticText("MinWeightRTSumTest.txt");
		iTestNum++;
	}

}

void AnalyzeSuboptimalityFromSmallExample_PrintInfo(TaskSet & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeight);
void AnalyzeSuboptimalityFromSmallExample_PlayWithPriorityAssignment(TaskSet & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeight);
void AnalyzeSuboptimalityFromSmallExample()
{
	int iTestNum = 1;
	double dAvgRTSubOptimality_Max = 0;
	double dAvgRTSubOptimality_Sum = 0;
	double dHeuristicSubOptimality_Max = 0;
	double dHeuristicSubOptimality_Sum = 0;
	StatisticSet cStatistic;

	while (true)
	{

		WeightedTaskSet cTaskSet;
		cTaskSet.GenRandomTaskSetInteger(7, 0.85, 0.5, 2.0);		
		cTaskSet.GenRandomWeight(10000);		
		//cTaskSet.ReadImageFile("WeightedRTSumAnalyzeFromSmallExample.txt.tskst");
		GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
		MySet<int> setConcernedTask;
		for (int i = 0; i < iTaskNum; i++)	setConcernedTask.insert(i);
		vector<double> vectorWeightTable;
		vectorWeightTable.reserve(iTaskNum);
		for (int i = 0; i < iTaskNum; i++)
		{
			//vectorWeightTable.push_back(i + 1);
			//vectorWeightTable.push_back(rand() % 10000);
			vectorWeightTable.push_back(pcTaskArray[i].getCLO());
		}
		vectorWeightTable = cTaskSet.getWeightTable();
		//Full MILP
		MinWeightedRTSumMILP cModel(cTaskSet);
		cModel.setWeight(vectorWeightTable);
		cModel.setConcernedTasks(setConcernedTask);
		int iMILPStatus = cModel.Solve(2, 60);
		double dMILPResult = cModel.getObj();
		cout << "By MILP: " << dMILPResult << endl;
		if (iMILPStatus != 1)	continue;

	

		//MinRTSum Alg
		MinWeightedRTSumHeuristic cHeuristic(cTaskSet);
		cHeuristic.setWeight(vectorWeightTable);
		MinWeightedRTSumHeuristic::SchedConds cDummyHeu;
		TaskSetPriorityStruct cPA(cTaskSet);
		double dHeuristicResult = cHeuristic.ComputeMinWeightedRTSum(setConcernedTask, cDummyHeu, cDummyHeu, cPA);
		cout << "By Heuristic: " << dHeuristicResult << endl;
		if (dHeuristicResult < 0)	continue;
		if (dHeuristicResult < dMILPResult)	continue;;
		double dHeuristicSubOptimality_This = (dHeuristicResult - dMILPResult) / dMILPResult;
		dHeuristicSubOptimality_Max = max(dHeuristicSubOptimality_Max, dHeuristicSubOptimality_This);
		dHeuristicSubOptimality_Sum += dHeuristicSubOptimality_This;
		char axBuffer[512] = { 0 };
		if (DOUBLE_EQUAL(dHeuristicResult, dMILPResult, 1e-3) == false)
		{
			cTaskSet.WriteImageFile("WeightedRTSumAnalyzeFromSmallExample.txt.tskst");
			cTaskSet.WriteTextFile("WeightedRTSumAnalyzeFromSmallExample.txt");
			cout << dHeuristicResult - dMILPResult << endl;
			cTaskSet.DisplayTaskInfo();
#if 0
			cout << "Result by MILP: " << endl;
			cModel.PrintResult(cout);
#endif
			cout << "Result by Heuristic" << endl;
			_LORTCalculator cRTCalc(cTaskSet);
			double dHeuWeightedRTSum = 0;
#if 0
			for (int i = 0; i < iTaskNum; i++)
			{
				double dRT = cRTCalc.CalculateRTTask(i, cPA);
				dHeuWeightedRTSum += (i + 1) * dRT;
				sprintf_s(axBuffer, "Task %d / RT: %.2f / P: %d / NWCET: %.2f", i, dRT, cPA.getPriorityByTask(i), pcTaskArray[i].getCLO() / vectorWeightTable[i]);
				cout << axBuffer << endl;
			}
			cout << "Heuristic Actual Sum: " << dHeuWeightedRTSum << endl;
#endif			
			cout << "MILP RT Diff: " << endl;
			TaskSetPriorityStruct cMILPPA = cModel.getPrioritySol();
			AnalyzeSuboptimalityFromSmallExample_PrintInfo(cTaskSet, cMILPPA, vectorWeightTable);
			cout << "Heuristic RT Diff" << endl;
			AnalyzeSuboptimalityFromSmallExample_PrintInfo(cTaskSet, cPA, vectorWeightTable);

			AnalyzeSuboptimalityFromSmallExample_PlayWithPriorityAssignment(cTaskSet, cPA, vectorWeightTable);
			system("pause");
			
			break;
		}
	}
}

void AnalyzeSuboptimalityFromSmallExample_PrintInfo(TaskSet & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeight)
{
	GET_TASKSET_NUM_PTR(&rcTaskSet, iTaskNum, pcTaskArray);
	_LORTCalculator cRTCalc(rcTaskSet);
	double dLastRT = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		int iTaskAtThisLevel = rcPriorityAssignment.getTaskByPriority(i);
		double dRT = cRTCalc.CalculateRTTask(iTaskAtThisLevel, rcPriorityAssignment);
		cout << "Priority: " << i << " / Task: " << rcPriorityAssignment.getTaskByPriority(i) << " / RT: " << dRT 
			<< " / Period: " << pcTaskArray[iTaskAtThisLevel].getPeriod()
			<< " / WCET: " << pcTaskArray[iTaskAtThisLevel].getCLO()
			<< " / NWCET: " << pcTaskArray[iTaskAtThisLevel].getCLO() / rvectorWeight[iTaskAtThisLevel]
			<< " / Weight: " << rvectorWeight[iTaskAtThisLevel];
		dLastRT = dRT;

		MySet<int> setLowestSchedSet;
		for (int j = 0; j < iTaskNum; j++)
		{
			if ((pcTaskArray[j].getDeadline() >= dRT) && (rcPriorityAssignment.getPriorityByTask(j) <= i))
			{
				setLowestSchedSet.insert(j);
			}
		}
		cout << " / LowestSchedSet: "; PrintSetContent(setLowestSchedSet.getSetContent());		
	}
}

void AnalyzeSuboptimalityFromSmallExample_PlayWithPriorityAssignment(TaskSet & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeight)
{
	GET_TASKSET_NUM_PTR(&rcTaskSet, iTaskNum, pcTaskArray);
	_LORTCalculator cCalcRT(rcTaskSet);	
	TaskSetPriorityStruct cOriginal(rcTaskSet);
	cOriginal.CopyFrom_Strict(rcPriorityAssignment);
	while (true)
	{
		bool bExit = false;
		cout << "Enter Priority to Swap: " << endl;
		int iTaskA = -1, iTaskB = -1;
		cin >> iTaskA >> iTaskB;
		switch (iTaskA)
		{
		case -1:
			bExit = true;
			break;
		case -2:
			rcPriorityAssignment.CopyFrom_Strict(cOriginal);
			break;
		default:
			rcPriorityAssignment.Swap(iTaskA, iTaskB);
			break;
		}
		if (bExit)
		{
			break;
		}
		AnalyzeSuboptimalityFromSmallExample_PrintInfo(rcTaskSet, rcPriorityAssignment, rvectorWeight);
		double dRTSum = 0;
		for (int i = 0; i < iTaskNum; i++)
		{
			double dRT = cCalcRT.CalculateRTTask(i, rcPriorityAssignment);			
		//	cout << "Task " << i << " / RT: " << dRT << " / P: " << rcPriorityAssignment.getPriorityByTask(i) << " / NWCET: " << pcTaskArray[i].getCLO() / rvectorWeight[i] << endl;
			if (dRT > pcTaskArray[i].getDeadline())
			{
				cout << "Unschedulable" << endl;
				break;
			}
			dRTSum += rvectorWeight[i] * dRT;
		}
		cout << "Weighted RT Sum: " << dRTSum << endl;
	}
}

double GreedyLowestPriorityMinWeightedRTSum(TaskSet & rcTaskSet, vector<double> & rvectorWeightTable)
{
	GET_TASKSET_NUM_PTR(&rcTaskSet, iTaskNum, pcTaskArray);
	TaskSetPriorityStruct cPA(rcTaskSet);
	_LORTCalculator cRTCalc(rcTaskSet);
	double dRTSum = 0;
	for (int iPriorityLevel = iTaskNum - 1; iPriorityLevel >= 0; iPriorityLevel--)
	{
		MySet<int> setLowestSchedulable;	
		for (int i = 0; i < iTaskNum; i++)
		{
			if (cPA.getPriorityByTask(i) != -1)	continue;
			cPA.setPriority(i, iPriorityLevel);
			double dRT = cRTCalc.CalculateRTTask(i, cPA);
			if (dRT <= pcTaskArray[i].getDeadline())
			{
				setLowestSchedulable.insert(i);
			}
			cPA.unset(i);
		}			
		
		cout << "Lowest Schedulable Set: " << endl;
		PrintSetContent(setLowestSchedulable.getSetContent());
		int iCandidate = -1;
		TaskSetPriorityStruct cPAThis;
		if (1)
		{
			MinWeightedRTSumMILP cMILPModel(rcTaskSet);
			cMILPModel.setConcernedTasks(setLowestSchedulable);
			cMILPModel.setWeight(rvectorWeightTable);
			cMILPModel.Solve(0);
			cPAThis = cMILPModel.getPrioritySol();
		}

		if (0)
		{
			WeightedRTSumFocusRefinement cFR(rcTaskSet);
			cFR.setWeight(rvectorWeightTable);
			cFR.setConcernedTasks(setLowestSchedulable);
			cFR.FocusRefinement(0);
			cPAThis = cFR.getPriorityAssignmentSol();			
		}
		iCandidate = cPAThis.getTaskByPriority(iPriorityLevel);
		assert(setLowestSchedulable.count(iCandidate));		
		cPA.setPriority(iCandidate, iPriorityLevel);
		double dRT = cRTCalc.CalculateRTTask(iCandidate, cPA);
		dRTSum += dRT * rvectorWeightTable[iCandidate];
		cout << "Priority " << iPriorityLevel << ": " << iCandidate << ", RT: " << dRT << endl;
	}
	cout << "Weighted RT Sum: " << dRTSum << endl;
	cPA.Print();
	return dRTSum;
}


int MinWeightedRTSumTest_NonPreemptive_USweep(int argc, char * argv[])
{
	vector<double> vectorUtil;
	for (double dUtil = 0.05; dUtil <= 0.95; dUtil += 0.05)
	{
		vectorUtil.push_back(dUtil);
	}
	char axBuffer[512] = { 0 };
	int iSystemSize = 20;

	// do the test
	chdir("SubOptimality");
	MakePath("MinWeightedRTSumTest_NonPreemptive_USweep");
	chdir("MinWeightedRTSumTest_NonPreemptive_USweep");
	ofstream ofstreamRawDataPointFile("MinWeightedRTSumTest_NonPreemptive_USweep_DataPoint.txt", ios::out);
	ofstreamRawDataPointFile << left << setw(40) << "#Utilization";
	ofstreamRawDataPointFile << left << setw(40) << "Scaled-WCET Monotonic";
	ofstreamRawDataPointFile << left << setw(40) << "Scaled-WCET Monotonic + Sifting"; ofstreamRawDataPointFile << endl;
	ofstreamRawDataPointFile.close();
	StatisticSet cOverall;
	int iErrorIndex = 0;
	for (vector<double>::iterator iter = vectorUtil.begin(); iter != vectorUtil.end(); iter++)
	{
		bool bContinue = false;
		double dUtil = *iter;
		sprintf_s(axBuffer, "U%.2f_N%d", *iter, iSystemSize);
		MakePath(axBuffer);
		cout << endl;
		for (int i = 0; i < 1000;)
		{
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", *iter, iSystemSize, i);
			printf_s("\rUtiliation: %.2f, #Case: %d            ", *iter, i);
			if (IsFileExist(axBuffer))
			{
				//cout << "File already exists. Skipping " << i << endl;
				i++;
				continue;
			}

			WeightedTaskSet cTaskSet;
			cTaskSet.GenRandomTaskSetInteger(iSystemSize, dUtil, 0.0, 1.0, PERIODOPT_LOG);
			cTaskSet.GenRandomWeight();
			vector<double> vectorWeightTable = cTaskSet.getWeightTable();
			GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
			MySet<int> setConcernedTask;
			for (int j = 0; j < iTaskNum; j++)	setConcernedTask.insert(j);
			StatisticSet cStatistic;
			MinWeightedRTSumHeuristic_NonPreemptive cHeuristic(cTaskSet);
			cHeuristic.setWeight(vectorWeightTable);
			MinWeightedRTSumHeuristic::SchedConds cDummy;
			_NonPreemptiveLORTCalculator cRTCalc(cTaskSet);
			//NominalWCET Heuristic
			TaskSetPriorityStruct cPA_NonminalWCET(cTaskSet);
			double dNominalWCETTime = clock();
			double dNominalWCETResult = cHeuristic.ComputeMinWeightedRTSum(setConcernedTask, cDummy, cDummy, cPA_NonminalWCET);
			dNominalWCETTime = clock() - dNominalWCETTime;
			if (dNominalWCETResult < 0)
			{
				continue;
			}
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst", *iter, iSystemSize, i);
			cTaskSet.WriteImageFile(axBuffer);
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.txt", *iter, iSystemSize, i);
			cTaskSet.WriteTextFile(axBuffer);
			cStatistic.setItem("Nominal WCET Heuristic Result", dNominalWCETResult);
			cStatistic.setItem("Nominal WCET Heuristic Time", dNominalWCETTime);

			//Rolling Down Adjustment
			TaskSetPriorityStruct cPA_RollingDown = cPA_NonminalWCET;
			double dRollingDownTime = clock();
			double dRollingDownResult = cHeuristic.RollingDownToFixedPointAdjustment(setConcernedTask, cPA_RollingDown, vectorWeightTable);
			dRollingDownTime = clock() - dRollingDownTime;
			cStatistic.setItem("Rolling Down Adjustment Result", dRollingDownResult);
			cStatistic.setItem("Rolling Down Adjustment Time", dRollingDownTime);
			for (int k = 0; k < iTaskNum; k++)
			{
				double dThisRT = cRTCalc.CalculateRT(k, cPA_RollingDown);
				if (!(dThisRT <= pcTaskArray[k].getDeadline()))
				{
					cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
					sprintf_s(axBuffer, "U%.2f_N%d/Error", *iter, iSystemSize);
					MakePath(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst", *iter, iSystemSize, iErrorIndex++);
					cTaskSet.WriteImageFile(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst.txt", *iter, iSystemSize, iErrorIndex++);
					cTaskSet.WriteTextFile(axBuffer);
					break;
				}
				//assert(dThisRT <= pcTaskArray[k].getDeadline());

			}
			if (bContinue)	continue;

			//Rolling Up Adjustment
			TaskSetPriorityStruct cPA_RollingUp = cPA_NonminalWCET;
			double dRollingUpTime = clock();
			double dRollingUpResult = cHeuristic.RollingUpToFixedPointAdjustment(setConcernedTask, cPA_RollingUp, vectorWeightTable);
			dRollingUpTime = clock() - dRollingUpTime;
			cStatistic.setItem("Rolling Up Adjustment Result", dRollingUpResult);
			cStatistic.setItem("Rolling Up Adjustment Time", dRollingUpTime);
			for (int k = 0; k < iTaskNum; k++)
			{
				double dThisRT = cRTCalc.CalculateRT(k, cPA_RollingUp);
				if (!DOUBLE_GE(pcTaskArray[k].getDeadline(), dThisRT, 1e-7))
				{
					cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
					sprintf_s(axBuffer, "U%.2f_N%d/Error", *iter, iSystemSize);
					MakePath(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst", *iter, iSystemSize, iErrorIndex++);
					cTaskSet.WriteImageFile(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst.txt", *iter, iSystemSize, iErrorIndex++);
					cTaskSet.WriteTextFile(axBuffer);
					bContinue = true;
					break;
				}
				//assert(dThisRT <= pcTaskArray[k].getDeadline());
			}
			if (bContinue)	continue;
			//Rolling Down-Up Adjustment
			TaskSetPriorityStruct cPA_Rolling = cPA_NonminalWCET;
			double dRollingTime = clock();
			double dRollingResult = cHeuristic.RollingToFixedPointAdjustment(setConcernedTask, cPA_Rolling, vectorWeightTable);
			dRollingTime = clock() - dRollingTime;
			cStatistic.setItem("Rolling Adjustment Result", dRollingResult);
			cStatistic.setItem("Rolling Adjustment Time", dRollingTime);
			for (int k = 0; k < iTaskNum; k++)
			{
				double dThisRT = cRTCalc.CalculateRT(k, cPA_Rolling);
				if (!DOUBLE_GE(pcTaskArray[k].getDeadline(), dThisRT, 1e-7))
				{
					cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
					sprintf_s(axBuffer, "U%.2f_N%d/Error", *iter, iSystemSize);
					MakePath(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst", *iter, iSystemSize, iErrorIndex++);
					cTaskSet.WriteImageFile(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst.txt", *iter, iSystemSize, iErrorIndex++);
					cTaskSet.WriteTextFile(axBuffer);
					bContinue = true;
					break;
				}
				//assert(dThisRT <= pcTaskArray[k].getDeadline());
			}
			if (bContinue)	continue;

			//MILP
			MinWeightedRTSumMILP_NonPreemptive cMILP(cTaskSet);
			cMILP.setWeight(vectorWeightTable);
			cMILP.setConcernedTasks(setConcernedTask);
			cMILP.setStartingSolution(cPA_Rolling);
			int iStatus = cMILP.Solve(0, 600);
			if (iStatus != 1)
			{
				cout << "MILP Failed ?" << endl;
				continue;
			}
			double dMILPResult = cMILP.getObj();

			dMILPResult = 0;
			TaskSetPriorityStruct cMILPPA = cMILP.getPrioritySol();
			for (int k = 0; k < iTaskNum; k++)
			{
				dMILPResult += cRTCalc.CalculateRT(k, cMILPPA) * vectorWeightTable[k];
			}
			double dMILPWallTime = cMILP.getWallTime();
			double dMILPCPUTime = cMILP.getCPUTime();
			cStatistic.setItem("MILP Result", dMILPResult);
			cStatistic.setItem("MILP Wall Time", dMILPWallTime);
			cStatistic.setItem("MILP CPU Time", dMILPCPUTime);
			double dBestHeuristicResult = dNominalWCETResult;
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingDownResult);
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingUpResult);
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingResult);
			if (dMILPResult - dBestHeuristicResult < 1e-7)
			{
				sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", *iter, iSystemSize, i);
				cStatistic.WriteStatisticImage(axBuffer);
				sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt.txt", *iter, iSystemSize, i);
				cStatistic.WriteStatisticText(axBuffer);
				i++;
			}
			else
			{
				//cout << "MILP Not Optimal" << endl;
			}
		}

		//Marshal the experiment result		
		double dNominalWCETResult = 0;
		double dNominalWCETTime = 0;
		double dNominalWCETSubOpt = 0;
		double dNominalWCETMaxSubOpt = 0;

		double dRollingDownResult = 0;
		double dRollingDownTime = 0;
		double dRollingDownSubOpt = 0;
		double dRollingDownMaxSubOpt = 0;

		double dRollingUpResult = 0;
		double dRollingUpTime = 0;
		double dRollingUpSubOpt = 0;
		double dRollingUpMaxSubOpt = 0;

		double dRollingResult = 0;
		double dRollingTime = 0;
		double dRollingSubOpt = 0;
		double dRollingMaxSubOpt = 0;

		double dMILPCPUTime = 0;
		double dMILPWallTime = 0;
		double dMILPResult = 0;

		sprintf_s(axBuffer, "U%.2f_N%d", *iter, iSystemSize);
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", *iter, iSystemSize, i);
			StatisticSet cStatistic;
			cStatistic.ReadStatisticImage(axBuffer);
			//NominalWCET Heuristic		
			dNominalWCETResult += cStatistic.getItem("Nominal WCET Heuristic Result").getValue();
			dNominalWCETTime += cStatistic.getItem("Nominal WCET Heuristic Time").getValue();

			//Rolling Down Adjustment			
			dRollingDownResult += cStatistic.getItem("Rolling Down Adjustment Result").getValue();
			dRollingDownTime += cStatistic.getItem("Rolling Down Adjustment Time").getValue();

			//Rolling Up Adjustment		
			dRollingUpResult += cStatistic.getItem("Rolling Up Adjustment Result").getValue();
			dRollingUpTime += cStatistic.getItem("Rolling Up Adjustment Time").getValue();

			//Rolling Down-Up Adjustment			
			dRollingResult += cStatistic.getItem("Rolling Adjustment Result").getValue();
			dRollingTime += cStatistic.getItem("Rolling Adjustment Time").getValue();

			//MILP			
			dMILPResult += cStatistic.getItem("MILP Result").getValue();
			dMILPWallTime += cStatistic.getItem("MILP Wall Time").getValue();
			dMILPCPUTime += cStatistic.getItem("MILP CPU Time").getValue();

			double dBestHeuristicResult = dNominalWCETResult;
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingDownResult);
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingUpResult);
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingResult);
			if ((dMILPResult - dBestHeuristicResult) < 1e-7)
			{
			}
			else
			{
				//cout << "MILP Not Optimal" << endl;
			}


			dNominalWCETMaxSubOpt = max(dNominalWCETMaxSubOpt, (dNominalWCETResult - dMILPResult) / dMILPResult);
			dRollingDownMaxSubOpt = max(dRollingDownMaxSubOpt, (dRollingDownResult - dMILPResult) / dMILPResult);
			dRollingUpMaxSubOpt = max(dRollingUpMaxSubOpt, (dRollingUpResult - dMILPResult) / dMILPResult);
			dRollingMaxSubOpt = max(dRollingMaxSubOpt, (dRollingResult - dMILPResult) / dMILPResult);

			dNominalWCETSubOpt += (dNominalWCETResult - dMILPResult) / dMILPResult;
			dRollingDownSubOpt += (dRollingDownResult - dMILPResult) / dMILPResult;
			dRollingUpSubOpt += (dRollingUpResult - dMILPResult) / dMILPResult;
			dRollingSubOpt += (dRollingResult - dMILPResult) / dMILPResult;
		}
		int iUtilOffSet = iter - vectorUtil.begin();
		sprintf_s(axBuffer, "%c.%.2f NonminalWCET Average Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dNominalWCETSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%.2f NonminalWCET Max Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dNominalWCETMaxSubOpt);

		sprintf_s(axBuffer, "%c.%.2f Rolling Down Average Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingDownSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%.2f Rolling Down Max Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingDownMaxSubOpt);

		sprintf_s(axBuffer, "%c.%.2f Rolling Up Average Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingUpSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%.2f Rolling Up Max Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingUpMaxSubOpt);

		sprintf_s(axBuffer, "%c.%.2f Rolling Average Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%.2f Rolling Max Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingMaxSubOpt);
		cOverall.WriteStatisticText("MinWeightedRTSumHeuristicTest_USweep.txt");

		//Write raw data point file
		ofstream ofstreamRawDataPointFile("MinWeightedRTSumTest_NonPreemptive_USweep_DataPoint.txt", ios::app);
		ofstreamRawDataPointFile << left << setw(40) << *iter;
		ofstreamRawDataPointFile << left << setw(40) << dNominalWCETSubOpt / 1000.0;
		ofstreamRawDataPointFile << left << setw(40) << dRollingSubOpt / 1000.0; ofstreamRawDataPointFile << endl;
		ofstreamRawDataPointFile.close();
	}

	return 0;
}

int MinWeightedRTSumTest_NonPreemptive_NSweep(int argc, char * argv[])
{
	vector<int> vectorN;
	for (int iN = 6; iN <= 24; iN += 2)		
	{
		vectorN.push_back(iN);
	}	
	double dUtil = 0.9;
	char axBuffer[512] = { 0 };

	// do the test
	chdir("SubOptimality");
	MakePath("MinWeightedRTSumTest_NonPreemptive_NSweep");
	chdir("MinWeightedRTSumTest_NonPreemptive_NSweep");
	ofstream ofstreamRawDataPointFile("MinWeightedRTSumTest_NonPreemptive_NSweep_DataPoint.txt", ios::out);
	ofstreamRawDataPointFile << left << setw(40) << "#System Size";
	ofstreamRawDataPointFile << left << setw(40) << "Scaled-WCET Monotonic";
	ofstreamRawDataPointFile << left << setw(40) << "Scaled-WCET Monotonic + Sifting"; ofstreamRawDataPointFile << endl;
	ofstreamRawDataPointFile.close();
	StatisticSet cOverall;
	for (vector<int>::iterator iter = vectorN.begin(); iter != vectorN.end(); iter++)
	{
		int iN = *iter;
		int iSystemSize = iN;

		sprintf_s(axBuffer, "U%.2f_N%d", dUtil, iN);
		MakePath(axBuffer);
		cout << endl;
		for (int i = 0; i < 1000;)
		{
			bool bContinue = false;
			int iErrorIndex = 0;
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", dUtil, iSystemSize, i);
			printf_s("\rSystem Size: %d, #Case: %d            ", *iter, i);
			if (IsFileExist(axBuffer))
			{
				//cout << "Skipping " << i << endl;
				i++;
				continue;
			}

			WeightedTaskSet cTaskSet;
			cTaskSet.GenRandomTaskSetInteger(iSystemSize, dUtil, 0.0, 1.0, PERIODOPT_LOG);
			cTaskSet.GenRandomWeight();
			vector<double> vectorWeightTable = cTaskSet.getWeightTable();
			GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
			MySet<int> setConcernedTask;
			for (int j = 0; j < iTaskNum; j++)	setConcernedTask.insert(j);
			StatisticSet cStatistic;
			_NonPreemptiveLORTCalculator cRTCalc(cTaskSet);
			MinWeightedRTSumHeuristic_NonPreemptive cHeuristic(cTaskSet);
			cHeuristic.setWeight(vectorWeightTable);
			MinWeightedRTSumHeuristic_NonPreemptive::SchedConds cDummy;

			//NominalWCET Heuristic
			TaskSetPriorityStruct cPA_NonminalWCET(cTaskSet);
			double dNominalWCETTime = clock();
			double dNominalWCETResult = cHeuristic.ComputeMinWeightedRTSum(setConcernedTask, cDummy, cDummy, cPA_NonminalWCET);
			dNominalWCETTime = clock() - dNominalWCETTime;
			if (dNominalWCETResult < 0)
			{
				continue;
			}
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst", dUtil, iSystemSize, i);
			cTaskSet.WriteImageFile(axBuffer);
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.txt", dUtil, iSystemSize, i);
			cTaskSet.WriteTextFile(axBuffer);
			cStatistic.setItem("Nominal WCET Heuristic Result", dNominalWCETResult);
			cStatistic.setItem("Nominal WCET Heuristic Time", dNominalWCETTime);


			//Rolling Down Adjustment
			TaskSetPriorityStruct cPA_RollingDown = cPA_NonminalWCET;
			double dRollingDownTime = clock();
			double dRollingDownResult = cHeuristic.RollingDownToFixedPointAdjustment(setConcernedTask, cPA_RollingDown, vectorWeightTable);
			dRollingDownTime = clock() - dRollingDownTime;
			cStatistic.setItem("Rolling Down Adjustment Result", dRollingDownResult);
			cStatistic.setItem("Rolling Down Adjustment Time", dRollingDownTime);
			for (int k = 0; k < iTaskNum; k++)
			{
				double dThisRT = cRTCalc.CalculateRT(k, cPA_RollingDown);
				if (!(dThisRT <= pcTaskArray[k].getDeadline()))
				{
					cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
					sprintf_s(axBuffer, "U%.2f_N%d/Error", *iter, iSystemSize);
					MakePath(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst", dUtil, iSystemSize, iErrorIndex);
					cTaskSet.WriteImageFile(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst.txt", dUtil, iSystemSize, iErrorIndex++);
					cTaskSet.WriteTextFile(axBuffer);
					break;
				}
				//assert(dThisRT <= pcTaskArray[k].getDeadline());

			}
			if (bContinue)	continue;

			//Rolling Up Adjustment
			TaskSetPriorityStruct cPA_RollingUp = cPA_NonminalWCET;
			double dRollingUpTime = clock();
			double dRollingUpResult = cHeuristic.RollingUpToFixedPointAdjustment(setConcernedTask, cPA_RollingUp, vectorWeightTable);
			dRollingUpTime = clock() - dRollingUpTime;
			cStatistic.setItem("Rolling Up Adjustment Result", dRollingUpResult);
			cStatistic.setItem("Rolling Up Adjustment Time", dRollingUpTime);
			for (int k = 0; k < iTaskNum; k++)
			{
				double dThisRT = cRTCalc.CalculateRT(k, cPA_RollingUp);
				if (!DOUBLE_GE(pcTaskArray[k].getDeadline(), dThisRT, 1e-7))
				{
					cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
					sprintf_s(axBuffer, "U%.2f_N%d/Error", *iter, iSystemSize);
					MakePath(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst", dUtil, iSystemSize, iErrorIndex++);
					cTaskSet.WriteImageFile(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst.txt", dUtil, iSystemSize, iErrorIndex++);
					cTaskSet.WriteTextFile(axBuffer);
					bContinue = true;
					break;
				}
				//assert(dThisRT <= pcTaskArray[k].getDeadline());
			}
			if (bContinue)	continue;


			//Rolling Down-Up Adjustment
			TaskSetPriorityStruct cPA_Rolling = cPA_NonminalWCET;
			double dRollingTime = clock();
			double dRollingResult = cHeuristic.RollingToFixedPointAdjustment(setConcernedTask, cPA_Rolling, vectorWeightTable);
			dRollingTime = clock() - dRollingTime;
			cStatistic.setItem("Rolling Adjustment Result", dRollingResult);
			cStatistic.setItem("Rolling Adjustment Time", dRollingTime);
			for (int k = 0; k < iTaskNum; k++)
			{
				double dThisRT = cRTCalc.CalculateRT(k, cPA_Rolling);
				if (!DOUBLE_GE(pcTaskArray[k].getDeadline(), dThisRT, 1e-7))
				{
					cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
					sprintf_s(axBuffer, "U%.2f_N%d/Error", *iter, iSystemSize);
					MakePath(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst", dUtil, iSystemSize, iErrorIndex++);
					cTaskSet.WriteImageFile(axBuffer);
					sprintf_s(axBuffer, "U%.2f_N%d/Error/TaskSetErr%d.tskst.txt", dUtil, iSystemSize, iErrorIndex++);
					cTaskSet.WriteTextFile(axBuffer);
					bContinue = true;
					break;
				}
				//assert(dThisRT <= pcTaskArray[k].getDeadline());
			}
			if (bContinue)	continue;



			//MILP
			MinWeightedRTSumMILP_NonPreemptive cMILP(cTaskSet);
			cMILP.setWeight(vectorWeightTable);
			cMILP.setStartingSolution(cPA_Rolling);
			cMILP.setConcernedTasks(setConcernedTask);
			int iStatus = cMILP.Solve(0, 900);
			double dMILPResult = cMILP.getObj();
			if (iStatus != 1)
			{
				cout << "MILP Failed ?" << endl;
				continue;
			}
			dMILPResult = 0;
			TaskSetPriorityStruct  cMILPPA = cMILP.getPrioritySol();
			for (int k = 0; k < iTaskNum; k++)
			{
				dMILPResult += cRTCalc.CalculateRT(k, cMILPPA) * vectorWeightTable[k];
			}
			double dMILPWallTime = cMILP.getWallTime();
			double dMILPCPUTime = cMILP.getCPUTime();
			cStatistic.setItem("MILP Result", dMILPResult);
			cStatistic.setItem("MILP Wall Time", dMILPWallTime);
			cStatistic.setItem("MILP CPU Time", dMILPCPUTime);

			double dBestHeuristicResult = dNominalWCETResult;
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingDownResult);
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingUpResult);
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingResult);
			if (dMILPResult - dBestHeuristicResult < 1e-7)
			{
				sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", dUtil, iSystemSize, i);
				cStatistic.WriteStatisticImage(axBuffer);
				sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt.txt", dUtil, iSystemSize, i);
				cStatistic.WriteStatisticText(axBuffer);
				i++;
			}
			else
			{
				//cout << "MILP Not Optimal" << endl;
			}
		}

		//Marshal the experiment result		
		double dNominalWCETResult = 0;
		double dNominalWCETTime = 0;
		double dNominalWCETSubOpt = 0;
		double dNominalWCETMaxSubOpt = 0;

		double dRollingDownResult = 0;
		double dRollingDownTime = 0;
		double dRollingDownSubOpt = 0;
		double dRollingDownMaxSubOpt = 0;

		double dRollingUpResult = 0;
		double dRollingUpTime = 0;
		double dRollingUpSubOpt = 0;
		double dRollingUpMaxSubOpt = 0;

		double dRollingResult = 0;
		double dRollingTime = 0;
		double dRollingSubOpt = 0;
		double dRollingMaxSubOpt = 0;

		double dMILPCPUTime = 0;
		double dMILPWallTime = 0;
		double dMILPResult = 0;

		sprintf_s(axBuffer, "U%.2f_N%d", dUtil, iSystemSize);
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", dUtil, iSystemSize, i);
			StatisticSet cStatistic;
			cStatistic.ReadStatisticImage(axBuffer);
			//NominalWCET Heuristic		
			dNominalWCETResult += cStatistic.getItem("Nominal WCET Heuristic Result").getValue();
			dNominalWCETTime += cStatistic.getItem("Nominal WCET Heuristic Time").getValue();

			//Rolling Down Adjustment			
			dRollingDownResult += cStatistic.getItem("Rolling Down Adjustment Result").getValue();
			dRollingDownTime += cStatistic.getItem("Rolling Down Adjustment Time").getValue();

			//Rolling Up Adjustment		
			dRollingUpResult += cStatistic.getItem("Rolling Up Adjustment Result").getValue();
			dRollingUpTime += cStatistic.getItem("Rolling Up Adjustment Time").getValue();

			//Rolling Down-Up Adjustment			
			dRollingResult += cStatistic.getItem("Rolling Adjustment Result").getValue();
			dRollingTime += cStatistic.getItem("Rolling Adjustment Time").getValue();

			//MILP			
			dMILPResult += cStatistic.getItem("MILP Result").getValue();
			dMILPWallTime += cStatistic.getItem("MILP Wall Time").getValue();
			dMILPCPUTime += cStatistic.getItem("MILP CPU Time").getValue();
			double dBestHeuristicResult = dNominalWCETResult;
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingDownResult);
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingUpResult);
			dBestHeuristicResult = min(dBestHeuristicResult, dRollingResult);
			if ((dMILPResult - dBestHeuristicResult) < 1e-7)
			{
			}
			else
			{

				//cout << "MILP Not Optimal" << endl;
			}

			dNominalWCETMaxSubOpt = max(dNominalWCETMaxSubOpt, (dNominalWCETResult - dMILPResult) / dMILPResult);
			dRollingDownMaxSubOpt = max(dRollingDownMaxSubOpt, (dRollingDownResult - dMILPResult) / dMILPResult);
			dRollingUpMaxSubOpt = max(dRollingUpMaxSubOpt, (dRollingUpResult - dMILPResult) / dMILPResult);
			dRollingMaxSubOpt = max(dRollingMaxSubOpt, (dRollingResult - dMILPResult) / dMILPResult);

			dNominalWCETSubOpt += (dNominalWCETResult - dMILPResult) / dMILPResult;
			dRollingDownSubOpt += (dRollingDownResult - dMILPResult) / dMILPResult;
			dRollingUpSubOpt += (dRollingUpResult - dMILPResult) / dMILPResult;
			dRollingSubOpt += (dRollingResult - dMILPResult) / dMILPResult;
		}
		int iNOffset = iter - vectorN.begin();
		sprintf_s(axBuffer, "%c.%d NonminalWCET Average Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dNominalWCETSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%d NonminalWCET Max Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dNominalWCETMaxSubOpt);

		sprintf_s(axBuffer, "%c.%d Rolling Down Average Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingDownSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%d Rolling Down Max Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingDownMaxSubOpt);

		sprintf_s(axBuffer, "%c.%d Rolling Up Average Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingUpSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%d Rolling Up Max Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingUpMaxSubOpt);

		sprintf_s(axBuffer, "%c.%d Rolling Average Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%d Rolling Max Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingMaxSubOpt);

		cOverall.WriteStatisticText("MinWeightedRTSumHeuristicTest_NSweep.txt");

		ofstream ofstreamRawDataPointFile("MinWeightedRTSumTest_NonPreemptive_NSweep_DataPoint.txt", ios::app);
		ofstreamRawDataPointFile << left << setw(40) << *iter;
		ofstreamRawDataPointFile << left << setw(40) << dNominalWCETSubOpt / 1000.0;
		ofstreamRawDataPointFile << left << setw(40) << dRollingSubOpt / 1000.0; ofstreamRawDataPointFile << endl;
		ofstreamRawDataPointFile.close();
	}
	return 0;
}

int MinWeightedRTSumTest_Preemptive_USweep(int argc, char * argv[])
{
	vector<double> vectorUtil;
	for (double dUtil = 0.05; dUtil <= 0.95; dUtil += 0.05)
	{
		vectorUtil.push_back(dUtil);
	}
	char axBuffer[512] = { 0 };
	int iSystemSize = 20;


	// do the test
	chdir("SubOptimality");
	MakePath("MinWeightedRTSumTest_Preemptive_USweep");
	chdir("MinWeightedRTSumTest_Preemptive_USweep");

	ofstream ofstreamRawDataPointFile("MinWeightedRTSumTest_Preemptive_USweep_DataPoint.txt", ios::out);
	ofstreamRawDataPointFile << left << setw(40) << "#Utilization";
	ofstreamRawDataPointFile << left << setw(40) << "Scaled-WCET Monotonic";
	ofstreamRawDataPointFile << left << setw(40) << "Scaled-WCET Monotonic + Sifting"; ofstreamRawDataPointFile << endl;
	ofstreamRawDataPointFile.close();

	StatisticSet cOverall;
	for (vector<double>::iterator iter = vectorUtil.begin(); iter != vectorUtil.end(); iter++)
	{
		double dUtil = *iter;
		sprintf_s(axBuffer, "U%.2f_N%d", *iter, iSystemSize);
		MakePath(axBuffer);
		cout << endl;
		for (int i = 0; i < 1000;)
		{
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", *iter, iSystemSize, i);
			printf_s("\rUtiliation: %.2f, #Case: %d            ", *iter, i);
			if (IsFileExist(axBuffer))
			{
				//cout << "Skipping " << i << endl;
				i++;
				continue;
			}

			WeightedTaskSet cTaskSet;
			cTaskSet.GenRandomTaskset(iSystemSize, dUtil, 0.0, 1.0, PERIODOPT_LOG);
			cTaskSet.GenRandomWeight();
			vector<double> vectorWeightTable = cTaskSet.getWeightTable();
			GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
			MySet<int> setConcernedTask;
			for (int j = 0; j < iTaskNum; j++)	setConcernedTask.insert(j);
			StatisticSet cStatistic;
			MinWeightedRTSumHeuristic cHeuristic(cTaskSet);
			cHeuristic.setWeight(vectorWeightTable);
			MinWeightedRTSumHeuristic::SchedConds cDummy;

			//NominalWCET Heuristic
			TaskSetPriorityStruct cPA_NonminalWCET(cTaskSet);
			double dNominalWCETTime = clock();
			double dNominalWCETResult = cHeuristic.ComputeMinWeightedRTSum(setConcernedTask, cDummy, cDummy, cPA_NonminalWCET);
			dNominalWCETTime = clock() - dNominalWCETTime;
			if (dNominalWCETResult < 0)
			{
				continue;
			}
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst", *iter, iSystemSize, i);
			cTaskSet.WriteImageFile(axBuffer);
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.txt", *iter, iSystemSize, i);
			cTaskSet.WriteTextFile(axBuffer);
			cStatistic.setItem("Nominal WCET Heuristic Result", dNominalWCETResult);
			cStatistic.setItem("Nominal WCET Heuristic Time", dNominalWCETTime);

			//Rolling Down Adjustment
			TaskSetPriorityStruct cPA_RollingDown = cPA_NonminalWCET;
			double dRollingDownTime = clock();
			double dRollingDownResult = cHeuristic.RollingDownToFixedPointAdjustment(setConcernedTask, cPA_RollingDown, vectorWeightTable);
			dRollingDownTime = clock() - dRollingDownTime;
			cStatistic.setItem("Rolling Down Adjustment Result", dRollingDownResult);
			cStatistic.setItem("Rolling Down Adjustment Time", dRollingDownTime);

			//Rolling Up Adjustment
			TaskSetPriorityStruct cPA_RollingUp = cPA_NonminalWCET;
			double dRollingUpTime = clock();
			double dRollingUpResult = cHeuristic.RollingUpToFixedPointAdjustment(setConcernedTask, cPA_RollingUp, vectorWeightTable);
			dRollingUpTime = clock() - dRollingUpTime;
			cStatistic.setItem("Rolling Up Adjustment Result", dRollingUpResult);
			cStatistic.setItem("Rolling Up Adjustment Time", dRollingUpTime);

			//Rolling Down-Up Adjustment
			TaskSetPriorityStruct cPA_Rolling = cPA_NonminalWCET;
			double dRollingTime = clock();
			double dRollingResult = cHeuristic.RollingToFixedPointAdjustment(setConcernedTask, cPA_Rolling, vectorWeightTable);
			dRollingTime = clock() - dRollingTime;
			cStatistic.setItem("Rolling Adjustment Result", dRollingResult);
			cStatistic.setItem("Rolling Adjustment Time", dRollingTime);



			//MILP
			MinWeightedRTSumMILP cMILP(cTaskSet);
			cMILP.setWeight(vectorWeightTable);
			cMILP.setConcernedTasks(setConcernedTask);
			int iStatus = cMILP.Solve(0, 600);
			double dMILPResult = cMILP.getObj();
			_LORTCalculator cRTCalc(cTaskSet);
			dMILPResult = 0;
			TaskSetPriorityStruct cMILPPA = cMILP.getPrioritySol();
			for (int k = 0; k < iTaskNum; k++)
			{
				dMILPResult += cRTCalc.CalculateRTTask(k, cMILPPA) * vectorWeightTable[k];
			}
			double dMILPWallTime = cMILP.getWallTime();
			double dMILPCPUTime = cMILP.getCPUTime();
			cStatistic.setItem("MILP Result", dMILPResult);
			cStatistic.setItem("MILP Wall Time", dMILPWallTime);
			cStatistic.setItem("MILP CPU Time", dMILPCPUTime);
			if (
				(dMILPResult <= dNominalWCETResult)
				&& (dMILPResult <= dRollingDownResult)
				&& (dMILPResult <= dRollingUpResult)
				&& (dMILPResult <= dRollingResult))
			{
				sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", *iter, iSystemSize, i);
				cStatistic.WriteStatisticImage(axBuffer);
				sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt.txt", *iter, iSystemSize, i);
				cStatistic.WriteStatisticText(axBuffer);
				i++;
			}
		}

		//Marshal the experiment result		
		double dNominalWCETResult = 0;
		double dNominalWCETTime = 0;
		double dNominalWCETSubOpt = 0;
		double dNominalWCETMaxSubOpt = 0;

		double dRollingDownResult = 0;
		double dRollingDownTime = 0;
		double dRollingDownSubOpt = 0;
		double dRollingDownMaxSubOpt = 0;

		double dRollingUpResult = 0;
		double dRollingUpTime = 0;
		double dRollingUpSubOpt = 0;
		double dRollingUpMaxSubOpt = 0;

		double dRollingResult = 0;
		double dRollingTime = 0;
		double dRollingSubOpt = 0;
		double dRollingMaxSubOpt = 0;

		double dMILPCPUTime = 0;
		double dMILPWallTime = 0;
		double dMILPResult = 0;

		sprintf_s(axBuffer, "U%.2f_N%d", *iter, iSystemSize);
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", *iter, iSystemSize, i);
			StatisticSet cStatistic;
			cStatistic.ReadStatisticImage(axBuffer);
			//NominalWCET Heuristic		
			dNominalWCETResult += cStatistic.getItem("Nominal WCET Heuristic Result").getValue();
			dNominalWCETTime += cStatistic.getItem("Nominal WCET Heuristic Time").getValue();

			//Rolling Down Adjustment			
			dRollingDownResult += cStatistic.getItem("Rolling Down Adjustment Result").getValue();
			dRollingDownTime += cStatistic.getItem("Rolling Down Adjustment Time").getValue();

			//Rolling Up Adjustment		
			dRollingUpResult += cStatistic.getItem("Rolling Up Adjustment Result").getValue();
			dRollingUpTime += cStatistic.getItem("Rolling Up Adjustment Time").getValue();

			//Rolling Down-Up Adjustment			
			dRollingResult += cStatistic.getItem("Rolling Adjustment Result").getValue();
			dRollingTime += cStatistic.getItem("Rolling Adjustment Time").getValue();

			//MILP			
			dMILPResult += cStatistic.getItem("MILP Result").getValue();
			dMILPWallTime += cStatistic.getItem("MILP Wall Time").getValue();
			dMILPCPUTime += cStatistic.getItem("MILP CPU Time").getValue();
			if ((dMILPResult <= dNominalWCETResult)
				&& (dMILPResult <= dRollingDownResult)
				&& (dMILPResult <= dRollingUpResult)
				&& (dMILPResult <= dRollingResult))
			{

			}
			else
			{
				cout << "Shouldn't have inferior MILP Result" << endl;
				while (1);
			}

			dNominalWCETMaxSubOpt = max(dNominalWCETMaxSubOpt, (dNominalWCETResult - dMILPResult) / dMILPResult);
			dRollingDownMaxSubOpt = max(dRollingDownMaxSubOpt, (dRollingDownResult - dMILPResult) / dMILPResult);
			dRollingUpMaxSubOpt = max(dRollingUpMaxSubOpt, (dRollingUpResult - dMILPResult) / dMILPResult);
			dRollingMaxSubOpt = max(dRollingMaxSubOpt, (dRollingResult - dMILPResult) / dMILPResult);

			dNominalWCETSubOpt += (dNominalWCETResult - dMILPResult) / dMILPResult;
			dRollingDownSubOpt += (dRollingDownResult - dMILPResult) / dMILPResult;
			dRollingUpSubOpt += (dRollingUpResult - dMILPResult) / dMILPResult;
			dRollingSubOpt += (dRollingResult - dMILPResult) / dMILPResult;
		}
		int iUtilOffSet = iter - vectorUtil.begin();
		sprintf_s(axBuffer, "%c.%.2f NonminalWCET Average Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dNominalWCETSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%.2f NonminalWCET Max Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dNominalWCETMaxSubOpt);

		sprintf_s(axBuffer, "%c.%.2f Rolling Down Average Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingDownSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%.2f Rolling Down Max Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingDownMaxSubOpt);

		sprintf_s(axBuffer, "%c.%.2f Rolling Up Average Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingUpSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%.2f Rolling Up Max Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingUpMaxSubOpt);

		sprintf_s(axBuffer, "%c.%.2f Rolling Average Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%.2f Rolling Max Sub-optimality", 'A' + iUtilOffSet, *iter);
		cOverall.setItem(axBuffer, dRollingMaxSubOpt);

		cOverall.WriteStatisticText("MinWeightedRTSumHeuristicTest_USweep.txt");
		ofstream ofstreamRawDataPointFile("MinWeightedRTSumTest_Preemptive_USweep_DataPoint.txt", ios::app);
		ofstreamRawDataPointFile << left << setw(40) << *iter;
		ofstreamRawDataPointFile << left << setw(40) << dNominalWCETSubOpt / 1000.0;
		ofstreamRawDataPointFile << left << setw(40) << dRollingSubOpt / 1000.0; ofstreamRawDataPointFile << endl;
		ofstreamRawDataPointFile.close();
	}
	return 0;
}

int MinWeightedRTSumTest_Preemptive_NSweep(int argc, char * argv[])
{
	vector<int> vectorN;
	for (int iN = 6; iN <= 30; iN += 2)
	{
		vectorN.push_back(iN);
	}
	char axBuffer[512] = { 0 };
	double dUtil = 0.9;

	// do the test
	chdir("SubOptimality");
	MakePath("MinWeightedRTSumTest_Preemptive_NSweep");
	chdir("MinWeightedRTSumTest_Preemptive_NSweep");
	ofstream ofstreamRawDataPointFile("MinWeightedRTSumTest_Preemptive_NSweep_DataPoint.txt", ios::out);
	ofstreamRawDataPointFile << left << setw(40) << "#System Size";
	ofstreamRawDataPointFile << left << setw(40) << "Scaled-WCET Monotonic";
	ofstreamRawDataPointFile << left << setw(40) << "Scaled-WCET Monotonic + Sifting"; ofstreamRawDataPointFile << endl;
	ofstreamRawDataPointFile.close();
	StatisticSet cOverall;
	for (vector<int>::iterator iter = vectorN.begin(); iter != vectorN.end(); iter++)
	{
		int iN = *iter;
		int iSystemSize = iN;
		sprintf_s(axBuffer, "U%.2f_N%d", dUtil, iN);
		MakePath(axBuffer);
		cout << endl;
		for (int i = 0; i < 1000;)
		{
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", dUtil, iSystemSize, i);
			printf_s("\rSystem Size: %d, #Case: %d            ", *iter, i);
			if (IsFileExist(axBuffer))
			{
				//cout << "Skipping " << i << endl;
				i++;
				continue;
			}

			WeightedTaskSet cTaskSet;
			cTaskSet.GenRandomTaskset(iSystemSize, dUtil, 0.0, 1.0, PERIODOPT_LOG);
			cTaskSet.GenRandomWeight();
			vector<double> vectorWeightTable = cTaskSet.getWeightTable();
			GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
			MySet<int> setConcernedTask;
			for (int j = 0; j < iTaskNum; j++)	setConcernedTask.insert(j);
			StatisticSet cStatistic;
			MinWeightedRTSumHeuristic cHeuristic(cTaskSet);
			cHeuristic.setWeight(vectorWeightTable);
			MinWeightedRTSumHeuristic::SchedConds cDummy;

			//NominalWCET Heuristic
			TaskSetPriorityStruct cPA_NonminalWCET(cTaskSet);
			double dNominalWCETTime = clock();
			double dNominalWCETResult = cHeuristic.ComputeMinWeightedRTSum(setConcernedTask, cDummy, cDummy, cPA_NonminalWCET);
			dNominalWCETTime = clock() - dNominalWCETTime;
			if (dNominalWCETResult < 0)
			{
				continue;
			}
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst", dUtil, iSystemSize, i);
			cTaskSet.WriteImageFile(axBuffer);
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.txt", dUtil, iSystemSize, i);
			cTaskSet.WriteTextFile(axBuffer);
			cStatistic.setItem("Nominal WCET Heuristic Result", dNominalWCETResult);
			cStatistic.setItem("Nominal WCET Heuristic Time", dNominalWCETTime);

			//Rolling Down Adjustment
			TaskSetPriorityStruct cPA_RollingDown = cPA_NonminalWCET;
			double dRollingDownTime = clock();
			double dRollingDownResult = cHeuristic.RollingDownToFixedPointAdjustment(setConcernedTask, cPA_RollingDown, vectorWeightTable);
			dRollingDownTime = clock() - dRollingDownTime;
			cStatistic.setItem("Rolling Down Adjustment Result", dRollingDownResult);
			cStatistic.setItem("Rolling Down Adjustment Time", dRollingDownTime);

			//Rolling Up Adjustment
			TaskSetPriorityStruct cPA_RollingUp = cPA_NonminalWCET;
			double dRollingUpTime = clock();
			double dRollingUpResult = cHeuristic.RollingUpToFixedPointAdjustment(setConcernedTask, cPA_RollingUp, vectorWeightTable);
			dRollingUpTime = clock() - dRollingUpTime;
			cStatistic.setItem("Rolling Up Adjustment Result", dRollingUpResult);
			cStatistic.setItem("Rolling Up Adjustment Time", dRollingUpTime);

			//Rolling Down-Up Adjustment
			TaskSetPriorityStruct cPA_Rolling = cPA_NonminalWCET;
			double dRollingTime = clock();
			double dRollingResult = cHeuristic.RollingToFixedPointAdjustment(setConcernedTask, cPA_Rolling, vectorWeightTable);
			dRollingTime = clock() - dRollingTime;
			cStatistic.setItem("Rolling Adjustment Result", dRollingResult);
			cStatistic.setItem("Rolling Adjustment Time", dRollingTime);



			//MILP
			MinWeightedRTSumMILP cMILP(cTaskSet);
			cMILP.setWeight(vectorWeightTable);
			cMILP.setConcernedTasks(setConcernedTask);
			int iStatus = cMILP.Solve(0, 600);
			double dMILPResult = cMILP.getObj();
			_LORTCalculator cRTCalc(cTaskSet);
			dMILPResult = 0;
			TaskSetPriorityStruct  cMILPPA = cMILP.getPrioritySol();
			for (int k = 0; k < iTaskNum; k++)
			{

				dMILPResult += cRTCalc.CalculateRTTask(k, cMILPPA) * vectorWeightTable[k];
			}
			double dMILPWallTime = cMILP.getWallTime();
			double dMILPCPUTime = cMILP.getCPUTime();
			cStatistic.setItem("MILP Result", dMILPResult);
			cStatistic.setItem("MILP Wall Time", dMILPWallTime);
			cStatistic.setItem("MILP CPU Time", dMILPCPUTime);
			if (
				(dMILPResult <= dNominalWCETResult)
				&& (dMILPResult <= dRollingDownResult)
				&& (dMILPResult <= dRollingUpResult)
				&& (dMILPResult <= dRollingResult))
			{
				sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", dUtil, iSystemSize, i);
				cStatistic.WriteStatisticImage(axBuffer);
				sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt.txt", dUtil, iSystemSize, i);
				cStatistic.WriteStatisticText(axBuffer);
				i++;
			}
		}

		//Marshal the experiment result		
		double dNominalWCETResult = 0;
		double dNominalWCETTime = 0;
		double dNominalWCETSubOpt = 0;
		double dNominalWCETMaxSubOpt = 0;

		double dRollingDownResult = 0;
		double dRollingDownTime = 0;
		double dRollingDownSubOpt = 0;
		double dRollingDownMaxSubOpt = 0;

		double dRollingUpResult = 0;
		double dRollingUpTime = 0;
		double dRollingUpSubOpt = 0;
		double dRollingUpMaxSubOpt = 0;

		double dRollingResult = 0;
		double dRollingTime = 0;
		double dRollingSubOpt = 0;
		double dRollingMaxSubOpt = 0;

		double dMILPCPUTime = 0;
		double dMILPWallTime = 0;
		double dMILPResult = 0;

		sprintf_s(axBuffer, "U%.2f_N%d", dUtil, iSystemSize);
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "U%.2f_N%d/TaskSet%d.tskst.rslt", dUtil, iSystemSize, i);
			StatisticSet cStatistic;
			cStatistic.ReadStatisticImage(axBuffer);
			//NominalWCET Heuristic		
			dNominalWCETResult += cStatistic.getItem("Nominal WCET Heuristic Result").getValue();
			dNominalWCETTime += cStatistic.getItem("Nominal WCET Heuristic Time").getValue();

			//Rolling Down Adjustment			
			dRollingDownResult += cStatistic.getItem("Rolling Down Adjustment Result").getValue();
			dRollingDownTime += cStatistic.getItem("Rolling Down Adjustment Time").getValue();

			//Rolling Up Adjustment		
			dRollingUpResult += cStatistic.getItem("Rolling Up Adjustment Result").getValue();
			dRollingUpTime += cStatistic.getItem("Rolling Up Adjustment Time").getValue();

			//Rolling Down-Up Adjustment			
			dRollingResult += cStatistic.getItem("Rolling Adjustment Result").getValue();
			dRollingTime += cStatistic.getItem("Rolling Adjustment Time").getValue();

			//MILP			
			dMILPResult += cStatistic.getItem("MILP Result").getValue();
			dMILPWallTime += cStatistic.getItem("MILP Wall Time").getValue();
			dMILPCPUTime += cStatistic.getItem("MILP CPU Time").getValue();
			if ((dMILPResult <= dNominalWCETResult)
				&& (dMILPResult <= dRollingDownResult)
				&& (dMILPResult <= dRollingUpResult)
				&& (dMILPResult <= dRollingResult))
			{

			}
			else
			{
				cout << "Shouldn't have inferior MILP Result" << endl;
				while (1);
			}

			dNominalWCETMaxSubOpt = max(dNominalWCETMaxSubOpt, (dNominalWCETResult - dMILPResult) / dMILPResult);
			dRollingDownMaxSubOpt = max(dRollingDownMaxSubOpt, (dRollingDownResult - dMILPResult) / dMILPResult);
			dRollingUpMaxSubOpt = max(dRollingUpMaxSubOpt, (dRollingUpResult - dMILPResult) / dMILPResult);
			dRollingMaxSubOpt = max(dRollingMaxSubOpt, (dRollingResult - dMILPResult) / dMILPResult);

			dNominalWCETSubOpt += (dNominalWCETResult - dMILPResult) / dMILPResult;
			dRollingDownSubOpt += (dRollingDownResult - dMILPResult) / dMILPResult;
			dRollingUpSubOpt += (dRollingUpResult - dMILPResult) / dMILPResult;
			dRollingSubOpt += (dRollingResult - dMILPResult) / dMILPResult;
		}
		int iNOffset = iter - vectorN.begin();
		sprintf_s(axBuffer, "%c.%d NonminalWCET Average Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dNominalWCETSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%d NonminalWCET Max Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dNominalWCETMaxSubOpt);

		sprintf_s(axBuffer, "%c.%d Rolling Down Average Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingDownSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%d Rolling Down Max Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingDownMaxSubOpt);

		sprintf_s(axBuffer, "%c.%d Rolling Up Average Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingUpSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%d Rolling Up Max Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingUpMaxSubOpt);

		sprintf_s(axBuffer, "%c.%d Rolling Average Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingSubOpt / 1000.0);
		sprintf_s(axBuffer, "%c.%d Rolling Max Sub-optimality", 'A' + iNOffset, *iter);
		cOverall.setItem(axBuffer, dRollingMaxSubOpt);

		cOverall.WriteStatisticText("MinWeightedRTSumHeuristicTest_NSweep.txt");

		ofstream ofstreamRawDataPointFile("MinWeightedRTSumTest_Preemptive_NSweep_DataPoint.txt", ios::app);
		ofstreamRawDataPointFile << left << setw(40) << *iter;
		ofstreamRawDataPointFile << left << setw(40) << dNominalWCETSubOpt / 1000.0;
		ofstreamRawDataPointFile << left << setw(40) << dRollingSubOpt / 1000.0; ofstreamRawDataPointFile << endl;
		ofstreamRawDataPointFile.close();
	}
	return 0;
}

void testMinWeightedAvgRTMILP()
{
	while (true)
	{
		int i = 1;
		int iSystemSize = 20;
		double dUtil = 0.30;
		char axBuffer[512] = { 0 };
		WeightedTaskSet cTaskSet;
		cTaskSet.GenRandomTaskset(iSystemSize, dUtil, 0.0, 1.0, PERIODOPT_LOG);
		cTaskSet.GenRandomWeight();		
		vector<double> vectorWeightTable = cTaskSet.getWeightTable();
		GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
		MySet<int> setConcernedTask;
		for (int j = 0; j < iTaskNum; j++)	setConcernedTask.insert(j);
		StatisticSet cStatistic;
		MinWeightedRTSumHeuristic_NonPreemptive cHeuristic(cTaskSet);
		cHeuristic.setWeight(vectorWeightTable);
		MinWeightedRTSumHeuristic::SchedConds cDummy;
		_NonPreemptiveLORTCalculator cRTCalc(cTaskSet);
		//NominalWCET Heuristic
		TaskSetPriorityStruct cPA_NonminalWCET(cTaskSet);
		double dNominalWCETTime = clock();
		double dNominalWCETResult = cHeuristic.ComputeMinWeightedRTSum(setConcernedTask, cDummy, cDummy, cPA_NonminalWCET);
		dNominalWCETTime = clock() - dNominalWCETTime;
		if (dNominalWCETResult < 0)
		{
			continue;
			cout << "Unschedulable" << endl;
			while (1);
		}
		sprintf_s(axBuffer, "TestMinWARTMILP_TaskSet%d.tskst", i);
		cTaskSet.WriteImageFile(axBuffer);
		sprintf_s(axBuffer, "TestMinWARTMILP_TaskSet%d.tskst.txt", i);
		cTaskSet.WriteTextFile(axBuffer);
		cStatistic.setItem("Nominal WCET Heuristic Result", dNominalWCETResult);
		cStatistic.setItem("Nominal WCET Heuristic Time", dNominalWCETTime);

		//Rolling Down Adjustment
		TaskSetPriorityStruct cPA_RollingDown = cPA_NonminalWCET;
		double dRollingDownTime = clock();
		double dRollingDownResult = cHeuristic.RollingDownToFixedPointAdjustment(setConcernedTask, cPA_RollingDown, vectorWeightTable);
		dRollingDownTime = clock() - dRollingDownTime;
		cStatistic.setItem("Rolling Down Adjustment Result", dRollingDownResult);
		cStatistic.setItem("Rolling Down Adjustment Time", dRollingDownTime);
		for (int k = 0; k < iTaskNum; k++)
		{
			double dThisRT = cRTCalc.CalculateRT(k, cPA_RollingDown);
			if (!(dThisRT <= pcTaskArray[k].getDeadline()))
			{
				cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
			}
			assert(dThisRT <= pcTaskArray[k].getDeadline());
		}

		//Rolling Up Adjustment
		TaskSetPriorityStruct cPA_RollingUp = cPA_NonminalWCET;
		double dRollingUpTime = clock();
		double dRollingUpResult = cHeuristic.RollingUpToFixedPointAdjustment(setConcernedTask, cPA_RollingUp, vectorWeightTable);
		dRollingUpTime = clock() - dRollingUpTime;
		cStatistic.setItem("Rolling Up Adjustment Result", dRollingUpResult);
		cStatistic.setItem("Rolling Up Adjustment Time", dRollingUpTime);
		for (int k = 0; k < iTaskNum; k++)
		{
			double dThisRT = cRTCalc.CalculateRT(k, cPA_RollingUp);
			if (!(dThisRT <= pcTaskArray[k].getDeadline()))
			{
				cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
			}
			assert(dThisRT <= pcTaskArray[k].getDeadline());
		}

		//Rolling Down-Up Adjustment
		TaskSetPriorityStruct cPA_Rolling = cPA_NonminalWCET;
		double dRollingTime = clock();
		double dRollingResult = cHeuristic.RollingToFixedPointAdjustment(setConcernedTask, cPA_Rolling, vectorWeightTable);
		dRollingTime = clock() - dRollingTime;
		cStatistic.setItem("Rolling Adjustment Result", dRollingResult);
		cStatistic.setItem("Rolling Adjustment Time", dRollingTime);
		for (int k = 0; k < iTaskNum; k++)
		{
			double dThisRT = cRTCalc.CalculateRT(k, cPA_Rolling);
			if (!(dThisRT <= pcTaskArray[k].getDeadline()))
			{
				cout << dThisRT << ", " << pcTaskArray[k].getDeadline() << endl;
			}
			assert(dThisRT <= pcTaskArray[k].getDeadline());
		}
		//cout << "NominalWCET: " << dNominalWCETResult << endl;
		printf_s("NominalWCET: %.5f\n", dNominalWCETResult);
//		cout << "Rolling Down: " << dRollingDownResult << endl;
		printf_s("Rolling Down: %.5f\n", dRollingDownResult);
		//cout << "Rolling Up: " << dRollingUpResult << endl;
		printf_s("Rolling Up: %.5f\n", dRollingUpResult);
		//cout << "Rolling Both: " << dRollingResult << endl;
		printf_s("Rolling Both: %.5f\n", dRollingResult);
		cout << "Blocking Info: " << endl;
		for (int k = 0; k < iTaskNum; k++)
		{
			cout << "Task " << k << ": " << cRTCalc.ComputeBlocking(k, cPA_Rolling) << endl;
		}

		//MILP		
		MinWeightedRTSumMILP_NonPreemptive cMILP(cTaskSet);
		cout << "Verify by MILP:" << endl;
		cout << "Rolling Both" << endl;
		cout << cMILP.TestPriorityAssignment(cPA_Rolling, 2) << endl;
#if 0
		cout << "Rolling Up" << endl;
		cout << cMILP.TestPriorityAssignment(cPA_RollingUp, 2) << endl;
		cout << "Rolling Down" << endl;
		cout << cMILP.TestPriorityAssignment(cPA_RollingDown, 2) << endl;
#endif
		cout << "Perform with MILP" << endl;
		cMILP.setWeight(vectorWeightTable);
		cMILP.setConcernedTasks(setConcernedTask);
		cMILP.setStartingSolution(cPA_Rolling);
		int iStatus = cMILP.Solve(2, 600);
		if (iStatus != 1)
		{
			cout << "MILP Failed ?" << endl;
			while (1);
		}
		double dMILPResult = cMILP.getObj();

		dMILPResult = 0;
		TaskSetPriorityStruct cMILPPA = cMILP.getPrioritySol();
		for (int k = 0; k < iTaskNum; k++)
		{
			dMILPResult += cRTCalc.CalculateRT(k, cMILPPA) * vectorWeightTable[k];
		}
		double dMILPWallTime = cMILP.getWallTime();
		double dMILPCPUTime = cMILP.getCPUTime();	
	
		//cout << "MILP Result: " << dMILPResult << endl;
		printf_s("MILP Result: %.5f\n", dMILPResult);
		printf_s("MILP Result Diff: %.10f\n", dMILPResult - dRollingResult);
		cout << "------------------------------------------" << endl;
		double dBestHeuristicResult = dNominalWCETResult;
		dBestHeuristicResult = min(dBestHeuristicResult, dRollingDownResult);
		dBestHeuristicResult = min(dBestHeuristicResult, dRollingUpResult);
		dBestHeuristicResult = min(dBestHeuristicResult, dRollingResult);
		if ((dMILPResult - dBestHeuristicResult) < 1e-7)
		{			
		}
		else
		{
			cout << "MILP Not Optimal" << endl;
		}
		break;
	}
	

}

#endif


/*

void Recur(int iSignal)
{
	double offset[] = f(period(iSignal));
	for (int i = 0; i < sizeof(offset); i++)
	{
		offset[i];
		Recur(iSignal + 1);
	}	
}*/

