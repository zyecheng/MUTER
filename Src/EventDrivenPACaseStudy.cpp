#include "stdafx.h"
#include "EventDrivenPACaseStudy.h"
#include <direct.h>
#include "StatisticSet.h"

DistributedEventDrivenTaskSet::End2EndPath::End2EndPath()
{

}

void DistributedEventDrivenTaskSet::End2EndPath::AddNode(int iObjIndex)
{
	m_dequeNodes.push_back(iObjIndex);
}

const deque<int> & DistributedEventDrivenTaskSet::End2EndPath::getNodes() const
{
	return m_dequeNodes;
}

void DistributedEventDrivenTaskSet::End2EndPath::WriteImage(ofstream & rcOutputFile)
{
	double dValue = m_dDeadline;
	rcOutputFile.write((const char *)&dValue, sizeof(double));

	int iNodeSize = m_dequeNodes.size();	
	rcOutputFile.write((const char *)&iNodeSize, sizeof(int));

	for (int i = 0; i < iNodeSize; i++)
	{
		int iValue = m_dequeNodes[i];
		rcOutputFile.write((const char *)&iValue, sizeof(int));
	}
}

void DistributedEventDrivenTaskSet::End2EndPath::ReadImage(ifstream & rcIfnputFile)
{	
	rcIfnputFile.read((char *)&m_dDeadline, sizeof(double));

	int iNodeSize = 0;
	rcIfnputFile.read((char *)&iNodeSize, sizeof(int));

	for (int i = 0; i < iNodeSize; i++)
	{
		int iValue = 0;
		rcIfnputFile.read((char *)&iValue, sizeof(int));
		m_dequeNodes.push_back(iValue);
	}
}

void DistributedEventDrivenTaskSet::InitializeDistributedSystemData()
{
	assert(m_pcTaskArray);
	assert(m_iTaskNum);
	m_vectorAllocation.clear();
	m_vectorJitterInheritanceSource.clear();
	m_vectorObjectType.clear();
	m_vectorAllocation.reserve(m_iTaskNum);
	m_vectorJitterInheritanceSource.reserve(m_iTaskNum);
	m_vectorObjectType.reserve(m_iTaskNum);
	for (int i = 0; i < m_iTaskNum; i++)
	{	
		m_vectorAllocation.push_back(-1);
		m_vectorJitterInheritanceSource.push_back(-1);
		m_vectorObjectType.push_back(0);
	}
}

void DistributedEventDrivenTaskSet::setAllocation(int iTaskIndex, int iAllocation)
{
	assert(m_vectorAllocation.size() > iTaskIndex);
	m_vectorAllocation[iTaskIndex] = iAllocation;
}

void DistributedEventDrivenTaskSet::setJiterInheritanceSource(int iTaskIndex, int iSource)
{
	assert(m_vectorJitterInheritanceSource.size() > iTaskIndex);
	m_vectorJitterInheritanceSource[iTaskIndex] = iSource;
}

void DistributedEventDrivenTaskSet::setObjectType(int iTaskIndex, int iType)
{
	assert(iType == 0 || iType == 1);
	assert(m_vectorObjectType.size() == m_iTaskNum);
	m_vectorObjectType[iTaskIndex] = iType;
}

void DistributedEventDrivenTaskSet::AddEnd2EndPaths(End2EndPath & rcEnd2EndPath)
{
	m_dequeEnd2EndPaths.push_back(rcEnd2EndPath);
}

void DistributedEventDrivenTaskSet::InitializeDerivedData()
{
	DerivePartition();
	DeriveHyperperiod();
	ResolveObjectsOnPaths();
	ResolveJitterPropagator();
	DeriveTighterDeadlineBound();
	DeriveWaitingTimeBound();
	DerivePartitionUtilization();
}

int DistributedEventDrivenTaskSet::getPartitionObjectType(int iPartitionIndex)
{
	set<int> & rsetPartition = m_umapPartition[iPartitionIndex];
	if (rsetPartition.empty())	return -1;
	return m_vectorObjectType[*rsetPartition.begin()];
}

void DistributedEventDrivenTaskSet::DerivePartition()
{
	assert(m_vectorAllocation.size() == m_iTaskNum);
	assert(m_vectorJitterInheritanceSource.size() == m_iTaskNum);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		int iAllocation = m_vectorAllocation[i];
		m_umapPartition[iAllocation].insert(i);
		m_setAllocationIndices.insert(iAllocation);
	}	
}

void DistributedEventDrivenTaskSet::DeriveHyperperiod()
{
	assert(m_vectorAllocation.size() == m_iTaskNum);
	assert(m_vectorJitterInheritanceSource.size() == m_iTaskNum);
	for (unordered_map<int, set<int> >::iterator iter = m_umapPartition.begin(); iter != m_umapPartition.end(); iter++)
	{
		int iAllocationIndex = iter->first;
		set<int> & rsetObjects = iter->second;
		double dHyperperiod = 1;
		for (set<int>::iterator iterObj = rsetObjects.begin(); iterObj != rsetObjects.end(); iterObj++)
		{
			dHyperperiod = lcm(dHyperperiod, m_pcTaskArray[*iterObj].getPeriod());
		}
		m_umapHyperperiod[iAllocationIndex] = dHyperperiod;
	}
}

void DistributedEventDrivenTaskSet::ResolveObjectsOnPaths()
{
	for (deque<End2EndPath>::const_iterator iter = m_dequeEnd2EndPaths.begin(); iter != m_dequeEnd2EndPaths.end(); iter++)
	{
		const deque<int> & rdequeNodes = iter->getNodes();		
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{
			int iNodeIndex = *iterNode;		
			m_setObjectsOnPaths.insert(iNodeIndex);
		}		
	}
}

void DistributedEventDrivenTaskSet::ResolveJitterPropagator()
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (m_vectorJitterInheritanceSource[i] == -1) continue;
		m_setJitterPropagator.insert(m_vectorJitterInheritanceSource[i]);
	}
}

const set<int> & DistributedEventDrivenTaskSet::getPartition(int iTaskIndex)
{
	assert(m_vectorAllocation.size() == m_iTaskNum);
	assert(m_vectorJitterInheritanceSource.size() == m_iTaskNum);
	return m_umapPartition[m_vectorAllocation[iTaskIndex]];
}

const set<int> & DistributedEventDrivenTaskSet::getPartition_ByAllocation(int iAllocationIndex)
{
	assert(m_vectorAllocation.size() == m_iTaskNum);
	assert(m_vectorJitterInheritanceSource.size() == m_iTaskNum);
	assert(m_umapPartition.count(iAllocationIndex));
	return m_umapPartition[iAllocationIndex];
}

double DistributedEventDrivenTaskSet::getPartitionHyperperiod_ByAllocation(int iAllocation)
{
	assert(m_umapPartition.count(iAllocation));
	return m_umapHyperperiod[iAllocation];
}

double DistributedEventDrivenTaskSet::getPartitionHyperperiod_ByTask(int iTaskIndex)
{
	int iAllocationIndex = m_vectorAllocation[iTaskIndex];
	return getPartitionHyperperiod_ByAllocation(iAllocationIndex);
}

const deque<DistributedEventDrivenTaskSet::End2EndPath> DistributedEventDrivenTaskSet::getEnd2EndPaths()
{
	return m_dequeEnd2EndPaths;
}

void DistributedEventDrivenTaskSet::DeriveTighterDeadlineBound()
{
	//Task that propogate jitter to other task cannot have response time larger than their partition hyperperiod
	DistributedEventDrivenTaskSet * m_pcTaskSet = this;
	int iTaskNum = m_iTaskNum;
	m_vectorDeadlineBound.clear();
	m_vectorDeadlineBound.reserve(iTaskNum);	
	for (int i = 0; i < iTaskNum; i++) m_vectorDeadlineBound.push_back(m_pcTaskSet->getPartitionHyperperiod_ByTask(i));
	for (int i = 0; i < iTaskNum; i++)
	{		
		int iJitterSource = m_pcTaskSet->getJitterSource(i);
		if (iJitterSource != -1)
		{
			m_vectorDeadlineBound[iJitterSource] = min(m_vectorDeadlineBound[iJitterSource], m_pcTaskSet->getPartitionHyperperiod_ByTask(i));
		}
	}	
}

void DistributedEventDrivenTaskSet::DeriveWaitingTimeBound()
{	
	assert(m_vectorDeadlineBound.size() == m_iTaskNum);//assume that deadline bound has been derived
	m_vectorWaitingTimeBound.clear();
	m_vectorWaitingTimeBound.reserve(m_iTaskNum);	
	const set<int> & rsetObjonPaths = getObjectsOnPaths();	//this has to be resolved first
	for (int i = 0; i < m_iTaskNum; i++)
	{
		double dBound = m_vectorDeadlineBound[i];
		m_vectorWaitingTimeBound.push_back(dBound);
	}

	const deque<End2EndPath> & rdequePaths = m_dequeEnd2EndPaths;
	for (deque<End2EndPath>::iterator iterPath = m_dequeEnd2EndPaths.begin(); iterPath != m_dequeEnd2EndPaths.end(); iterPath++)
	{
		const deque<int> & rdequeNodes = iterPath->getNodes();
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{
			m_vectorWaitingTimeBound[*iterNode] = min(m_vectorWaitingTimeBound[*iterNode], iterPath->getDeadline());
		}
	}
}

double DistributedEventDrivenTaskSet::getWaitingTimeBound(int iTaskIndex)
{
	assert(m_vectorWaitingTimeBound.size() > iTaskIndex);
	return m_vectorWaitingTimeBound[iTaskIndex];
}

double DistributedEventDrivenTaskSet::getPartitionUtilization_ByObjIndex(int iObjIndex)
{
	int iAllocationIndex = m_vectorAllocation[iObjIndex];
	assert(m_mapUtilization.count(iAllocationIndex));
	return getPartitionUtilization_ByPartition(iAllocationIndex);
}

double DistributedEventDrivenTaskSet::getPartitionUtilization_ByPartition(int iPartitionIndex)
{
	const set<int> & rsetPartiiton = getPartition_ByAllocation(iPartitionIndex);
	double dUtil = 0;
	for (set<int>::const_iterator iter = rsetPartiiton.begin(); iter != rsetPartiiton.end(); iter++)
	{
		dUtil += m_pcTaskArray[*iter].getUtilization();
	}
	return dUtil;
}

double DistributedEventDrivenTaskSet::ScalePartitionUtil(int iPartitionIndex, double dUtil)
{
	const set<int> & rsetPartiiton = getPartition_ByAllocation(iPartitionIndex);
	double dCurrentUtil = 0;
	for (set<int>::const_iterator iter = rsetPartiiton.begin(); iter != rsetPartiiton.end(); iter++)
	{
		int iTaskIndex = *iter;
		dCurrentUtil += m_pcTaskArray[iTaskIndex].getUtilization();
	}
	double dScale = dUtil / dCurrentUtil;
	double dActualScaledUtil = 0;
	for (set<int>::const_iterator iter = rsetPartiiton.begin(); iter != rsetPartiiton.end(); iter++)
	{
		int iTaskIndex = *iter;
		double dPrevUtil = m_pcTaskArray[iTaskIndex].getUtilization();
		double dOrigWCET = m_pcTaskArray[iTaskIndex].getCLO();
		double dOrigWCET_FromUtil = m_pcTaskArray[iTaskIndex].getUtilization() * m_pcTaskArray[iTaskIndex].getPeriod();
		double dNewUtil = dPrevUtil * dScale;
		double dNewWCET = dNewUtil * m_pcTaskArray[iTaskIndex].getPeriod();
		dNewWCET = round(dNewWCET);
		dNewUtil = dNewWCET / m_pcTaskArray[iTaskIndex].getPeriod();
		m_pcTaskArray[iTaskIndex].setUtilization(dNewUtil);
		m_pcTaskArray[iTaskIndex].Construct();
		dActualScaledUtil += dNewUtil;
	}
	return dActualScaledUtil;
}

void DistributedEventDrivenTaskSet::WriteExtendedParameterImage(ofstream & OutputFile)
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		int dValue = m_vectorJitterInheritanceSource[i];
		OutputFile.write((const char *)&dValue, sizeof(int));

		dValue = m_vectorAllocation[i];
		OutputFile.write((const char *)&dValue, sizeof(int));

		dValue = m_vectorObjectType[i];
		OutputFile.write((const char *)&dValue, sizeof(int));		
	}

	int iEnd2EndPathNum = m_dequeEnd2EndPaths.size();	
	OutputFile.write((const char *)&iEnd2EndPathNum, sizeof(int));

	for (int i = 0; i < iEnd2EndPathNum; i++)
	{
		End2EndPath & rcPath = m_dequeEnd2EndPaths[i];
		rcPath.WriteImage(OutputFile);
	}
}

void DistributedEventDrivenTaskSet::ReadExtendedParameterImage(ifstream & InputFile)
{
	assert(m_iTaskNum > 0);
	InitializeDistributedSystemData();
	for (int i = 0; i < m_iTaskNum; i++)
	{
		int dValue = 0;
		InputFile.read((char *)&dValue, sizeof(int));
		m_vectorJitterInheritanceSource[i] = dValue;
		
		InputFile.read((char *)&dValue, sizeof(int));
		m_vectorAllocation[i] = dValue;

		InputFile.read((char *)&dValue, sizeof(int));
		m_vectorObjectType[i] = dValue;
	}

	int iEnd2EndPathNum = m_dequeEnd2EndPaths.size();
	InputFile.read((char *)&iEnd2EndPathNum, sizeof(int));

	for (int i = 0; i < iEnd2EndPathNum; i++)
	{
		End2EndPath rcPath;
		rcPath.ReadImage(InputFile);
		m_dequeEnd2EndPaths.push_back(rcPath);
	}
}

void DistributedEventDrivenTaskSet::DeriveSchedTestInstancesBound()
{	
	//Based on the fact that the waiting time w cannot be greater than the end-to-end latency

	for (deque<End2EndPath>::iterator iter = m_dequeEnd2EndPaths.begin(); iter != m_dequeEnd2EndPaths.end(); iter++)
	{
		const deque<int> & rdequeNodes = iter->getNodes();
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{

		}
	}
}

double DistributedEventDrivenTaskSet::getDeadlineBound(int iTaskIndex)
{
	assert(m_vectorDeadlineBound.size() > iTaskIndex);
	return m_vectorDeadlineBound[iTaskIndex];
}

void DistributedEventDrivenTaskSet::DerivePartitionUtilization()
{
	assert(m_vectorAllocation.size() == m_iTaskNum);
	assert(m_vectorJitterInheritanceSource.size() == m_iTaskNum);
	for (unordered_map<int, set<int> >::iterator iter = m_umapPartition.begin(); iter != m_umapPartition.end(); iter++)
	{
		int iAllocationIndex = iter->first;
		set<int> & rsetObjects = iter->second;
		double dUtil = 0;
		for (set<int>::iterator iterObj = rsetObjects.begin(); iterObj != rsetObjects.end(); iterObj++)
		{
			dUtil += m_pcTaskArray[*iterObj].getUtilization();
		}
		m_mapUtilization[iAllocationIndex] = dUtil;
	}
}

//DistributedEventDrivenResponseTimeCalculator

DistributedEventDrivenResponseTimeCalculator::DistributedEventDrivenResponseTimeCalculator()
{
	m_pcSystem = NULL;
}

DistributedEventDrivenResponseTimeCalculator::DistributedEventDrivenResponseTimeCalculator(DistributedEventDrivenTaskSet & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment)
{
	m_pcSystem = &rcTaskSet;
	m_pcPriorityAssignment = &rcPriorityAssignment;
}

double DistributedEventDrivenResponseTimeCalculator::RHS_Task(int iInstance, int iTaskIndex, double d_w, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	const set<int> & rsetPartitions = m_pcSystem->getPartition(iTaskIndex);
	double dRHS = (iInstance + 1) * pcTaskArray[iTaskIndex].getCLO();
	double d_t = d_w;
	for (set<int>::iterator iter = rsetPartitions.begin(); iter != rsetPartitions.end(); iter++)
	{
		int iThisTaskIndex = *iter;
		if (iThisTaskIndex == iTaskIndex)	continue;
		if (m_pcPriorityAssignment->getPriorityByTask(iThisTaskIndex) > m_pcPriorityAssignment->getPriorityByTask(iTaskIndex)) continue;
		int iThisTaskJitterSource = m_pcSystem->getJitterSource(iThisTaskIndex);
		double dJitter = 0;
#if 0
		if (iThisTaskJitterSource == iTaskIndex)
		{
			dJitter = d_w + getJitter(iTaskIndex, rvectorResponseTimes);
		}
		else
		{
			dJitter = getJitter(iThisTaskIndex, rvectorResponseTimes);
		}
#else
		dJitter = getJitter(iThisTaskIndex, rvectorResponseTimes);
#endif 
		
		//dRHS += ceil((d_t + dJitter) / pcTaskArray[iThisTaskIndex].getPeriod()) * pcTaskArray[iThisTaskIndex].getCLO();
		dRHS += ceil_tol((d_t + dJitter) / pcTaskArray[iThisTaskIndex].getPeriod()) * pcTaskArray[iThisTaskIndex].getCLO();
	}
	return dRHS;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWaitingTimeQ_Task(int iInstance, int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	double dPartitionHyperperiod = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex);
	double d_w_q_rhs = 0;
	double d_w_q = pcTaskArray[iTaskIndex].getCLO();
	double dInsatnceStartTime = (iInstance)* pcTaskArray[iTaskIndex].getPeriod();
	double dTaskJitter = getJitter(iTaskIndex, rvectorResponseTimes);
	while (!DOUBLE_EQUAL(d_w_q, d_w_q_rhs, 1e-5))
	{
		d_w_q_rhs = d_w_q;
		d_w_q = RHS_Task(iInstance, iTaskIndex, d_w_q_rhs, rvectorResponseTimes);
		//double dTempRT = d_w_q - dInsatnceStartTime + dTaskJitter;
		if (d_w_q > dWaitingTimeLimit)
		{
			return -1;
		}

		if (d_w_q > dPartitionHyperperiod)
		{
			return -1;
		}
	}
	return d_w_q;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWaitingTime_Task(int iInstance, int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	double dPartitionHyperperiod = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex);	
	double dInsatnceStartTime = (iInstance)* pcTaskArray[iTaskIndex].getPeriod();
	double dTaskJitter = getJitter(iTaskIndex, rvectorResponseTimes);
	double dWaitingTimeQLimit = dWaitingTimeLimit + dInsatnceStartTime;
	double dWaitingTimeQ = ComputeWaitingTimeQ_Task(iInstance, iTaskIndex, dWaitingTimeQLimit, rvectorResponseTimes);
	if (dWaitingTimeQ == -1)
	{
		return -1;
	}
	return dWaitingTimeQ - dInsatnceStartTime;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeResponseTime_Task(int iInstance, int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	double dPartitionHyperperiod = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex);
	double d_w = 0;
	double dInsatnceStartTime = (iInstance)* pcTaskArray[iTaskIndex].getPeriod();
	double dTaskJitter = getJitter(iTaskIndex, rvectorResponseTimes);
	double dWaitingTimeLimit = dRTLimit - dTaskJitter;
	d_w = ComputeWaitingTime_Task(iInstance, iTaskIndex, dWaitingTimeLimit, rvectorResponseTimes);
	if (d_w == -1)
	{
		return -1;
	}
	double dRT = d_w + dTaskJitter;
	return dRT;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWorstCaseWaitingTime_Task(int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iInstance = 0;
	double dThisWaitingTime = 0;
	double dWorstWaitingTime = 0;
	int iJitterSource = m_pcSystem->getJitterSource(iTaskIndex);
	double dTaskJitter = (iJitterSource == -1) ? 0.0 : rvectorResponseTimes[iJitterSource];
	if (dWaitingTimeLimit < 0)
	{
		dWaitingTimeLimit = m_pcSystem->getWaitingTimeBound(iTaskIndex);
	}
	else
	{
		dWaitingTimeLimit = min(dWaitingTimeLimit, m_pcSystem->getWaitingTimeBound(iTaskIndex));
	}

	int iInstanceUpperbound = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex) / pcTaskArray[iTaskIndex].getPeriod();

	while (true)
	{
		dThisWaitingTime = ComputeWaitingTime_Task(iInstance, iTaskIndex, dWaitingTimeLimit, rvectorResponseTimes);
		if (dThisWaitingTime == -1)
		{
			return -1;
		}
		dWorstWaitingTime = max(dWorstWaitingTime, dThisWaitingTime);
		if ((dThisWaitingTime + dTaskJitter) <= pcTaskArray[iTaskIndex].getPeriod())
		{
			break;
		}
		iInstance++;
		if (iInstance >= iInstanceUpperbound)
		{
			return -1;
		}
	}
	return dWorstWaitingTime;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWorstCaseResponseTime_Task(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iInstance = 0;
	double dThisRT = 0;
	double dWorstRT = 0;	
	if (dRTLimit < 0)
	{
		dRTLimit = m_pcSystem->getDeadlineBound(iTaskIndex);
	}
	else
	{
		dRTLimit = min(dRTLimit, m_pcSystem->getDeadlineBound(iTaskIndex));
	}

	int iInstanceUpperbound = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex) / pcTaskArray[iTaskIndex].getPeriod();

	while (true)
	{
		dThisRT = ComputeResponseTime_Task(iInstance, iTaskIndex, dRTLimit, rvectorResponseTimes);
		if (dThisRT == -1)
		{
			return -1;
		}
		dWorstRT = max(dWorstRT, dThisRT);
		if (dThisRT <= pcTaskArray[iTaskIndex].getPeriod())
		{
			break;
		}
		iInstance++;
		if (iInstance >= iInstanceUpperbound)
		{
			return -1;
		}
	}
	return dWorstRT;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWorstCaseResponseTime_Task_Approx(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	if (dRTLimit < 0)
	{
		dRTLimit = pcTaskArray[iTaskIndex].getPeriod();
	}
	else
	{
		dRTLimit = min(dRTLimit, pcTaskArray[iTaskIndex].getPeriod());
	}

	double dRT = ComputeResponseTime_Task(0, iTaskIndex, dRTLimit, rvectorResponseTimes);
	if (dRT > pcTaskArray[iTaskIndex].getPeriod())
	{
		return -1;
	}
	else
	{
		return dRT;
	}
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWorstCaseWaitingTime_Task_Approx(int iTaskIndex, double dWTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iJitterSource = m_pcSystem->getJitterSource(iTaskIndex);
	double dTaskJitter = (iJitterSource == -1) ? 0.0 : rvectorResponseTimes[iJitterSource];
	if (dWTLimit < 0)
	{
		dWTLimit = min(pcTaskArray[iTaskIndex].getPeriod(), m_pcSystem->getWaitingTimeBound(iTaskIndex));
	}
	else
	{
		dWTLimit = min(dWTLimit, pcTaskArray[iTaskIndex].getPeriod());
		dWTLimit = min(dWTLimit, m_pcSystem->getWaitingTimeBound(iTaskIndex));
	}

	double dWT = ComputeWaitingTime_Task(0, iTaskIndex, dWTLimit, rvectorResponseTimes);
	if ((dWT + dTaskJitter) > pcTaskArray[iTaskIndex].getPeriod())
	{
		return -1;
	}
	else
	{
		return dWT;
	}
}

double DistributedEventDrivenResponseTimeCalculator::RHS_Message(int iInstance, int iTaskIndex, double d_w, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	const set<int> & rsetPartitions = m_pcSystem->getPartition(iTaskIndex);
	double dBlocking = DerivedBlocking(iTaskIndex);
	double dRHS = (iInstance) * pcTaskArray[iTaskIndex].getCLO() + dBlocking;
	double d_t = d_w;	
	for (set<int>::iterator iter = rsetPartitions.begin(); iter != rsetPartitions.end(); iter++)
	{
		int iThisTaskIndex = *iter;
		if (iThisTaskIndex == iTaskIndex)	continue;
		if (m_pcPriorityAssignment->getPriorityByTask(iThisTaskIndex) > m_pcPriorityAssignment->getPriorityByTask(iTaskIndex)) continue;

		int iThisTaskJitterSource = m_pcSystem->getJitterSource(iThisTaskIndex);
		double dJitter = 0;
#if 0
		if (iThisTaskJitterSource == iTaskIndex)
		{
			dJitter = d_w + getJitter(iTaskIndex, rvectorResponseTimes);
		}
		else
		{
			dJitter = getJitter(iThisTaskIndex, rvectorResponseTimes);
		}
#else

		dJitter = getJitter(iThisTaskIndex, rvectorResponseTimes);
#endif
		


		//double dJitter = getJitter(iThisTaskIndex, rvectorResponseTimes);
		//dRHS += ceil((d_t + dJitter) / pcTaskArray[iThisTaskIndex].getPeriod()) * pcTaskArray[iThisTaskIndex].getCLO();
		dRHS += ceil_tol((d_t + dJitter) / pcTaskArray[iThisTaskIndex].getPeriod()) * pcTaskArray[iThisTaskIndex].getCLO();
	}
	return dRHS;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWaitingTimeQ_Message(int iInstance, int iTaskIndex, double dWaitingTimeQLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	double d_w_q_rhs = 0;
	double dBlocking = DerivedBlocking(iTaskIndex);
	int iTaskPriority = m_pcPriorityAssignment->getPriorityByTask(iTaskIndex);
	double d_w_q = dBlocking;
	if (dBlocking == 0)
	{
		const set<int> & rsetPartition = m_pcSystem->getPartition(iTaskIndex);
		for (set<int>::const_iterator iter = rsetPartition.begin(); iter != rsetPartition.end(); iter++)
		{
			if (m_pcPriorityAssignment->getPriorityByTask(*iter) < iTaskPriority)
			{
				if (pcTaskArray[*iter].getCLO() > 0)
				{
					d_w_q = pcTaskArray[*iter].getCLO();
					break;
				}
			}
		}
	}

	double dPartitionHyperperiod = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex);
	double dInstanceStartTime = (iInstance)* pcTaskArray[iTaskIndex].getPeriod();
	double dTaskJitter = getJitter(iTaskIndex, rvectorResponseTimes);
	while (!DOUBLE_EQUAL(d_w_q, d_w_q_rhs, 1e-5))
	{
		d_w_q_rhs = d_w_q;
		d_w_q = RHS_Message(iInstance, iTaskIndex, d_w_q_rhs, rvectorResponseTimes);		
		if (d_w_q > dWaitingTimeQLimit)
		{
			return -1;
		}

		if (d_w_q > dPartitionHyperperiod)
		{
			return -1;
		}
	}
	//double dRT = pcTaskArray[iTaskIndex].getCLO() + d_w_q - dInstanceStartTime + dTaskJitter;
	return d_w_q;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWaitingTime_Message(int iInstance, int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	double d_w_q_rhs = 0;	
	int iTaskPriority = m_pcPriorityAssignment->getPriorityByTask(iTaskIndex);
	double d_w_q = 0;
	double dInstanceStartTime = iInstance * pcTaskArray[iTaskIndex].getPeriod();
	double dWaitingTimeQLimit = dWaitingTimeLimit + dInstanceStartTime - pcTaskArray[iTaskIndex].getCLO();
	d_w_q = ComputeWaitingTimeQ_Message(iInstance, iTaskIndex, dWaitingTimeQLimit, rvectorResponseTimes);
	if (d_w_q == -1)
	{
		return -1;
	}
	double dRet = d_w_q - dInstanceStartTime + pcTaskArray[iTaskIndex].getCLO();
	return dRet;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeResponseTime_Message(int iInstance, int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	double d_w_q_rhs = 0;	
	int iTaskPriority = m_pcPriorityAssignment->getPriorityByTask(iTaskIndex);
	double d_w = 0;
	double dPartitionHyperperiod = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex);
	double dInstanceStartTime = (iInstance)* pcTaskArray[iTaskIndex].getPeriod();
	double dTaskJitter = getJitter(iTaskIndex, rvectorResponseTimes);
	double dWaitingTimeLimit = dRTLimit + dInstanceStartTime - dTaskJitter - pcTaskArray[iTaskIndex].getCLO();
	d_w = ComputeWaitingTime_Message(iInstance, iTaskIndex, dWaitingTimeLimit, rvectorResponseTimes);
	double dRT = d_w + dTaskJitter;
	return dRT;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWorstCaseResponseTime_Message(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iInstance = 0;
	double dThisRT = 0;
	double dWorstRT = 0;	
	int iInstanceUpperbound = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex) / pcTaskArray[iTaskIndex].getPeriod();
	if (dRTLimit < 0)
	{
		dRTLimit = m_pcSystem->getDeadlineBound(iTaskIndex);
	}
	else
	{
		dRTLimit = min(dRTLimit, m_pcSystem->getDeadlineBound(iTaskIndex));
	}
	while (true)
	{
		dThisRT = ComputeResponseTime_Message(iInstance, iTaskIndex, dRTLimit, rvectorResponseTimes);
		if (dThisRT == -1)
		{
			return -1;
		}
		dWorstRT = max(dWorstRT, dThisRT);
		if (dThisRT <= pcTaskArray[iTaskIndex].getPeriod())
		{
			break;
		}
		iInstance++;
		if (iInstance >= iInstanceUpperbound)
		{
			return -1;
		}
	}
	return dWorstRT;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWorstCaseWaitingTime_Message(int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iInstance = 0;
	double dThisWT = 0;
	double dWorstWT = 0;
	int iJitterSource = m_pcSystem->getJitterSource(iTaskIndex);
	double dTaskJitter = (iJitterSource == -1) ? 0.0 : rvectorResponseTimes[iJitterSource];
	int iInstanceUpperbound = m_pcSystem->getPartitionHyperperiod_ByTask(iTaskIndex) / pcTaskArray[iTaskIndex].getPeriod();
	if (dWaitingTimeLimit < 0)
	{
		dWaitingTimeLimit = m_pcSystem->getWaitingTimeBound(iTaskIndex);
	}
	else
	{
		dWaitingTimeLimit = min(dWaitingTimeLimit, m_pcSystem->getDeadlineBound(iTaskIndex));
		dWaitingTimeLimit = min(dWaitingTimeLimit, m_pcSystem->getWaitingTimeBound(iTaskIndex));
	}
	while (true)
	{
		dThisWT = ComputeWaitingTime_Message(iInstance, iTaskIndex, dWaitingTimeLimit, rvectorResponseTimes);
		if (dThisWT == -1)
		{
			return -1;
		}
		dWorstWT = max(dWorstWT, dThisWT);
		if ((dThisWT + dTaskJitter) <= pcTaskArray[iTaskIndex].getPeriod())
		{
			break;
		}
		iInstance++;
		if (iInstance >= iInstanceUpperbound)
		{
			return -1;
		}
	}
	return dWorstWT;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWorstCaseResponseTime_Message_Approx(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	if (dRTLimit < 0)
	{
		dRTLimit = pcTaskArray[iTaskIndex].getPeriod();
	}
	else
	{
		dRTLimit = min(dRTLimit, pcTaskArray[iTaskIndex].getPeriod());
	}
	double dRT = ComputeResponseTime_Message(0, iTaskIndex, dRTLimit, rvectorResponseTimes);
	if (dRT > pcTaskArray[iTaskIndex].getPeriod())
	{
		return -1;
	}
	else
	{
		return dRT;
	}
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWorstCaseWaitingTime_Message_Approx(int iTaskIndex, double dWTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iJitterSource = m_pcSystem->getJitterSource(iTaskIndex);
	double dTaskJitter = (iJitterSource == -1) ? 0.0 : rvectorResponseTimes[iJitterSource];
	if (dWTLimit < 0)
	{
		dWTLimit = pcTaskArray[iTaskIndex].getPeriod();
	}
	else
	{
		dWTLimit = min(dWTLimit, pcTaskArray[iTaskIndex].getPeriod());
	}
	double dWT = ComputeWaitingTime_Message(0, iTaskIndex, dWTLimit, rvectorResponseTimes);
	if ((dWT + dTaskJitter) > pcTaskArray[iTaskIndex].getPeriod())
	{
		return -1;
	}
	else
	{
		return dWT;
	}
}

double DistributedEventDrivenResponseTimeCalculator::ComputeResponseTime_SpecifiedResponseTimes(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iObjType = m_pcSystem->getObjectType(iTaskIndex);
	double dRT = 0;
	if (iObjType == 0)
	{
		dRT = ComputeWorstCaseResponseTime_Task(iTaskIndex, dRTLimit, rvectorResponseTimes);
	}
	else if (iObjType == 1)
	{
		dRT = ComputeWorstCaseResponseTime_Message(iTaskIndex, dRTLimit, rvectorResponseTimes);
	}
	else
	{
		cout << "Unknown Object Type" << endl;
		assert(0);
	}
	return dRT;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWaitingTime_SpecifiedResponseTimes(int iTaskIndex, double dWTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iObjType = m_pcSystem->getObjectType(iTaskIndex);
	double dWT = 0;
	if (iObjType == 0)
	{
		dWT = ComputeWorstCaseWaitingTime_Task(iTaskIndex, dWTLimit, rvectorResponseTimes);
	}
	else if (iObjType == 1)
	{
		dWT = ComputeWorstCaseWaitingTime_Message(iTaskIndex, dWTLimit, rvectorResponseTimes);
	}
	else
	{
		cout << "Unknown Object Type" << endl;
		assert(0);
	}
	return dWT;
}

bool ComponentWiseCompare_LE(vector<double> & rvectorRT_LHS, vector<double> & rvectorRT_RHS)
{
	assert(rvectorRT_RHS.size() == rvectorRT_LHS.size());
	int iNum = rvectorRT_LHS.size();
	for (int i = 0; i < iNum; i++)
	{
		if (rvectorRT_LHS[i] > rvectorRT_RHS[i])
		{
			return false;
		}
	}
	return true;
}

int DistributedEventDrivenResponseTimeCalculator::ComputeAllResponseTimeExact(vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	rvectorResponseTimes.clear();
	rvectorResponseTimes.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		rvectorResponseTimes.push_back(pcTaskArray[i].getCLO());
	}
	bool bChange = true;
	while (bChange)
	{	
		bChange = false;
		for (int i = 0; i < iTaskNum; i++)
		{
			double dWCRT = 0;
			int iObjType = m_pcSystem->getObjectType(i);
			if (iObjType == 0)
			{
				dWCRT = ComputeWorstCaseResponseTime_Task(i, -1, rvectorResponseTimes);
			}
			else if (iObjType == 1)
			{
				dWCRT = ComputeWorstCaseResponseTime_Message(i, -1, rvectorResponseTimes);
			}
			else
			{
				cout << "Unkonwn Object Type" << endl;
				assert(false);
			}

			if (dWCRT == -1)
			{
				return 0;
			}

			if (!DOUBLE_EQUAL(rvectorResponseTimes[i], dWCRT, 1e-7))
			{
				bChange = true;
				rvectorResponseTimes[i] = dWCRT;
			}			
		}
	}	
	return 1;
}

int DistributedEventDrivenResponseTimeCalculator::ComputeAllResponseTime_Approx(vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	rvectorResponseTimes.clear();
	rvectorResponseTimes.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		rvectorResponseTimes.push_back(pcTaskArray[i].getCLO());
	}
	bool bChange = true;
	while (bChange)
	{
		bChange = false;
		for (int i = 0; i < iTaskNum; i++)
		{
			double dWCRT = 0;
			int iObjType = m_pcSystem->getObjectType(i);
			if (iObjType == 0)
			{
				dWCRT = ComputeWorstCaseResponseTime_Task_Approx(i, pcTaskArray[i].getPeriod(), rvectorResponseTimes);
			}
			else if (iObjType == 1)
			{
				dWCRT = ComputeWorstCaseResponseTime_Message_Approx(i, pcTaskArray[i].getPeriod(), rvectorResponseTimes);
			}
			else
			{
				cout << "Unkonwn Object Type" << endl;
				assert(false);
			}

			if (dWCRT == -1)
			{
				return 0;
			}


			if (!DOUBLE_EQUAL(rvectorResponseTimes[i], dWCRT, 1e-7))
			{
				bChange = true;
				rvectorResponseTimes[i] = dWCRT;
			}
		}
	}
	return 1;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeResponseTime_SpecifiedResponseTimes_Approx(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iObjType = m_pcSystem->getObjectType(iTaskIndex);
	double dRT = 0;

	if (dRTLimit < 0)
	{
		dRTLimit = pcTaskArray[iTaskIndex].getPeriod();
	}
	else
	{
		dRTLimit = min(dRTLimit, pcTaskArray[iTaskIndex].getPeriod());
	}

	if (iObjType == 0)
	{
		dRT = ComputeWorstCaseResponseTime_Task_Approx(iTaskIndex, dRTLimit, rvectorResponseTimes);
	}
	else if (iObjType == 1)
	{
		dRT = ComputeWorstCaseResponseTime_Message_Approx(iTaskIndex, dRTLimit, rvectorResponseTimes);
	}
	else
	{
		cout << "Unknown Object Type" << endl;
		assert(0);
	}
	return dRT;
}

double DistributedEventDrivenResponseTimeCalculator::getWaitingTime(int iTaskIndex, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	int iObjType = m_pcSystem->getObjectType(iTaskIndex);
	int iJitterSource = m_pcSystem->getJitterSource(iTaskIndex);
	double dJitter = (iJitterSource == -1) ? 0.0 : rvectorResponseTimes[iJitterSource];
	return rvectorResponseTimes[iTaskIndex] - dJitter;
}

double DistributedEventDrivenResponseTimeCalculator::getJitter(int iTaskIndex, const vector<double> & rvectorResponseTimes)
{
	int iJitterSource = m_pcSystem->getJitterSource(iTaskIndex);
	if (iJitterSource == -1)	return 0;
	return rvectorResponseTimes[iJitterSource];
}

double DistributedEventDrivenResponseTimeCalculator::DerivedBlocking(int iTaskIndex)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	const set<int> & rsetPartitions = m_pcSystem->getPartition(iTaskIndex);
	double dBlocking = 0;
	for (set<int>::iterator iter = rsetPartitions.begin(); iter != rsetPartitions.end(); iter++)
	{
		int iThisTaskIndex = *iter;
		if (iThisTaskIndex == iTaskIndex) continue;
		if (m_pcPriorityAssignment->getPriorityByTask(iThisTaskIndex) < m_pcPriorityAssignment->getPriorityByTask(iTaskIndex))	continue;
		dBlocking = max(dBlocking, pcTaskArray[iThisTaskIndex].getCLO());
	}
	return dBlocking;
}

double DistributedEventDrivenResponseTimeCalculator::ComputeWaitingTime_SpecifiedResponseTimes_Approx(int iTaskIndex, double dWTLimit, const vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcSystem, iTaskNum, pcTaskArray);
	vector<double> vectorResponseTimes;	
	int iObjType = m_pcSystem->getObjectType(iTaskIndex);
	double dWT = 0;

	if (dWTLimit < 0)
	{
		dWTLimit = pcTaskArray[iTaskIndex].getPeriod();
	}
	else
	{
		dWTLimit = min(dWTLimit, pcTaskArray[iTaskIndex].getPeriod());
	}

	if (iObjType == 0)
	{
		dWT = ComputeWorstCaseWaitingTime_Task_Approx(iTaskIndex, dWTLimit, rvectorResponseTimes);
	}
	else if (iObjType == 1)
	{
		dWT = ComputeWorstCaseWaitingTime_Message_Approx(iTaskIndex, dWTLimit, rvectorResponseTimes);
	}
	else
	{
		cout << "Unknown Object Type" << endl;
		assert(0);
	}
	return dWT;
}

//DistributedEventDrivenSchedCondType
bool operator < (const DistributedEventDrivenSchedCondType & rcLHS, const DistributedEventDrivenSchedCondType & rcRHS)
{
	if (rcLHS.m_iTaskIndex == rcRHS.m_iTaskIndex)
	{
		return rcLHS.m_enumCondType < rcRHS.m_enumCondType;
	}
	return rcLHS.m_iTaskIndex < rcRHS.m_iTaskIndex;
}

ostream & operator << (ostream & rcOS, const DistributedEventDrivenSchedCondType & rcRHS)
{
	rcOS << "{";
	rcOS << rcRHS.m_iTaskIndex << ", ";
	switch (rcRHS.m_enumCondType)
	{
	case DistributedEventDrivenSchedCondType::UB:
		rcOS << "DUB";
		break;
	case DistributedEventDrivenSchedCondType::LB:
		rcOS << "DLB";
		break;
	default:
		cout << "Unknown SchedCond Type" << endl;
		assert(0);
		break;
	}
	rcOS << "}";
	return rcOS;
}

istream & operator >> (istream & rcIS, DistributedEventDrivenSchedCondType & rcLHS)
{
	char xChar;
	char axBuffer[32] = { 0 };
	rcLHS.m_iTaskIndex = -1;
	rcLHS.m_enumCondType = DistributedEventDrivenSchedCondType::UB;
	DistributedEventDrivenSchedCondType cTemp;
	rcIS >> xChar;
	if (xChar != '{')	return rcIS;
	rcIS >> cTemp.m_iTaskIndex;
	rcIS >> xChar;
	if (xChar != ',')	return rcIS;
	rcIS >> axBuffer[0] >> axBuffer[1] >> axBuffer[2];
	if (axBuffer[0] != 'D')	return rcIS;
	if (axBuffer[2] != 'B')	return rcIS;
	if ((axBuffer[1] == 'U'))
	{
		cTemp.m_enumCondType = cTemp.UB;
	}
	else if ((axBuffer[1] == 'L'))
	{
		cTemp.m_enumCondType = cTemp.LB;
	}
	else
	{
		cout << "Instream DistributedEventDrivenSchedCondType error. Do not proceed." << endl;
		while (1);
		return rcIS;
	}
	rcIS >> xChar;
	if (xChar != '}')
	{
		cout << "Instream DistributedEventDrivenSchedCondType error. Do not proceed." << endl;
		while (1);
		return rcIS;
	}
	rcLHS = cTemp;
	return rcIS;
}

DistributedEventDrivenSchedCondType::DistributedEventDrivenSchedCondType(const char axStr[])
{
	stringstream ss(axStr);
	ss >> *this;
}

//DistributedEventDrivenMUDARComputer

DistributedEventDrivenMUDARComputer::DistributedEventDrivenMUDARComputer()
{

}

DistributedEventDrivenMUDARComputer::DistributedEventDrivenMUDARComputer(SystemType & rcSystem)
	:GeneralUnschedCoreComputer_MonoInt(rcSystem)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	m_vectorDeadlineLB.reserve(iTaskNum);
	m_vectorDeadlineUB.reserve(iTaskNum);	
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorDeadlineLB.push_back(-1);
		m_vectorDeadlineUB.push_back(-1);
	}
	m_cPreAssignment = TaskSetPriorityStruct(*m_pcTaskSet);
}

DistributedEventDrivenMUDARComputer::Monotonicity DistributedEventDrivenMUDARComputer::SchedCondMonotonicity(const SchedCondType & rcSchedCondType)
{
	switch (rcSchedCondType.m_enumCondType)
	{
	case SchedCondType::UB:
		return Monotonicity::Up;
		break;
	case SchedCondType::LB:
		return Monotonicity::Down;
		break;
	default:
		cout << "Unknown SchedCond Type" << endl;
		assert(0);
		break;
	}
	return Monotonicity::Up;
}

DistributedEventDrivenMUDARComputer::SchedCondContent DistributedEventDrivenMUDARComputer::SchedCondLowerBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;	
	return pcTaskArray[iTaskIndex].getCLO();
}

DistributedEventDrivenMUDARComputer::SchedCondContent DistributedEventDrivenMUDARComputer::SchedCondUpperBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;	
	//return pcTaskArray[iTaskNum].getPeriod();
	double dDeadlineBound = m_pcTaskSet->getDeadlineBound(iTaskIndex);
	return dDeadlineBound;
}

void DistributedEventDrivenMUDARComputer::ExtractDeadlineUBLB(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, vector<double> & rvectorDeadlineUB, vector<double> & rvectorDeadlineLB)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		//m_vectorDeadlineLB[i] = pcTaskArray[i].getCLO();
//		m_vectorDeadlineUB[i] = m_pcTaskSet->getPartitionHyperperiod_ByTask(i);

		m_vectorDeadlineLB[i] = SchedCondLowerBound(SchedCondType(i, SchedCondType::LB));
		m_vectorDeadlineUB[i] = SchedCondUpperBound(SchedCondType(i, SchedCondType::UB));
	}

	for (SchedConds::iterator iter = rcSchedCondsFlex.begin(); iter != rcSchedCondsFlex.end(); iter++)
	{
		int iTaskIndex = iter->first.m_iTaskIndex;
		switch (iter->first.m_enumCondType)
		{
		case DistributedEventDrivenSchedCondType::LB:
			m_vectorDeadlineLB[iTaskIndex] = max(m_vectorDeadlineLB[iTaskIndex], iter->second + 0.0);
			break;
		case DistributedEventDrivenSchedCondType::UB:
			m_vectorDeadlineUB[iTaskIndex] = min(m_vectorDeadlineUB[iTaskIndex], iter->second + 0.0);
			break;
		default:
			cout << "Unknown SchedCondType" << endl;
			break;
		}
	}

	for (SchedConds::iterator iter = rcSchedCondsFixed.begin(); iter != rcSchedCondsFixed.end(); iter++)
	{
		int iTaskIndex = iter->first.m_iTaskIndex;
		switch (iter->first.m_enumCondType)
		{
		case DistributedEventDrivenSchedCondType::LB:
			m_vectorDeadlineLB[iTaskIndex] = max(m_vectorDeadlineLB[iTaskIndex], iter->second + 0.0);
			break;
		case DistributedEventDrivenSchedCondType::UB:
			m_vectorDeadlineUB[iTaskIndex] = min(m_vectorDeadlineUB[iTaskIndex], iter->second + 0.0);
			break;
		default:
			cout << "Unknown SchedCondType" << endl;
			break;
		}
	}

}

bool DistributedEventDrivenMUDARComputer::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, rcPriorityAssignment);
	double dRTLimit = m_vectorDeadlineUB[iTaskIndex];
	double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes(iTaskIndex, dRTLimit, m_vectorDeadlineLB);

	if (dRT == -1)
	{
		return false;
	}

	if (dRT <= m_vectorDeadlineUB[iTaskIndex])
	{
		return true;
	}
	else
	{
		return false;
	}
}

void DistributedEventDrivenMUDARComputer::IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	ExtractDeadlineUBLB(rcSchedCondsFlex, rcSchedCondsFixed, m_vectorDeadlineUB, m_vectorDeadlineLB);
}

bool DistributedEventDrivenMUDARComputer::IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	TaskSetPriorityStruct cPriorityAssignment(*m_pcTaskSet);
	return IsSchedulable(rcSchedCondsFlex, rcSchedCondsFixed, cPriorityAssignment);
}

bool DistributedEventDrivenMUDARComputer::IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	//There are hundreds of task on different execution resources. Has to perform carefully.
	IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	
	//if the given rcPriorityAssignment is empty, copy from existing pre assignment
	if (rcPriorityAssignment.getSize() == 0)
	{
		rcPriorityAssignment = m_cPreAssignment;
	}

	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
	int iCurrPriorityLevel = iTaskNum - 1;
	for (set<int>::iterator iterPartition = rsetAllocationIndices.begin(); iterPartition != rsetAllocationIndices.end(); iterPartition++)
	{		
		const set<int> & rsetTasksOnPartition = m_pcTaskSet->getPartition_ByAllocation(*iterPartition);				
		int iPriorityLevelLB = iCurrPriorityLevel - rsetTasksOnPartition.size() + 1;
		for (; iCurrPriorityLevel >= iPriorityLevelLB; iCurrPriorityLevel--)
		{
			//Test if already assigned
			if (rcPriorityAssignment.getTaskByPriority(iCurrPriorityLevel) != -1)	continue;

			//If not, iterate through all candidates.
			bool bAssigned = false;			
			for (set<int>::iterator iterTask = rsetTasksOnPartition.begin(); iterTask != rsetTasksOnPartition.end(); iterTask++)			
			{
				int iCurrentTask = *iterTask;
				//test if already assigned;
				if (rcPriorityAssignment.getPriorityByTask(iCurrentTask) != -1)
				{
					continue;
				}

				//Assign the priority and perform schedulability 
				rcPriorityAssignment.setPriority(iCurrentTask, iCurrPriorityLevel);		
				bool bSchedTestStatus = SchedTest(iCurrentTask, rcPriorityAssignment, rcSchedCondsFlex, rcSchedCondsFixed);
				if (!bSchedTestStatus)
				{
					rcPriorityAssignment.unset(iCurrentTask);
				}
				else
				{					
					bAssigned = true;
					break;
				}
			}

			if (!bAssigned)
			{
				return false;
			}
		}		
	}
	assert(rcPriorityAssignment.getSize() == iTaskNum);
	return true;
}

void DistributedEventDrivenMUDARComputer::DeriveSafePriorityAssignment(TaskSetPriorityStruct & rcPriorityAssignment)
{
	/*
	If a task,
	1) does not propagate jitter
	2) is not involved in any path
	3) schedulable assuming worst case response time of jitter source task at lowest unassigned priority
	then can safely assign the lowest unassigned priority
	*/		
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
	const set<int> & rsetTasksOnPaths = m_pcTaskSet->getObjectsOnPaths();
	const set<int> & rsetJitterPropagators = m_pcTaskSet->getJitterPropogator();
	int iCurrPriorityLevel = iTaskNum - 1;
	vector<double> vectorMaximumResponseTimes;
	vectorMaximumResponseTimes.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorMaximumResponseTimes.push_back(SchedCondUpperBound(SchedCondType(i, SchedCondType::UB)));
	}

	for (set<int>::iterator iterPartition = rsetAllocationIndices.begin(); iterPartition != rsetAllocationIndices.end(); iterPartition++)
	{
		const set<int> & rsetTasksOnPartition = m_pcTaskSet->getPartition_ByAllocation(*iterPartition);
		int iPriorityLevelLB = iCurrPriorityLevel - rsetTasksOnPartition.size() + 1;
		for (; iCurrPriorityLevel >= iPriorityLevelLB; iCurrPriorityLevel--)
		{
			//Test if already assigned
			if (rcPriorityAssignment.getTaskByPriority(iCurrPriorityLevel) != -1)	continue;

			//If not, iterate through all candidates.
			bool bAssigned = false;
			for (set<int>::iterator iterTask = rsetTasksOnPartition.begin(); iterTask != rsetTasksOnPartition.end(); iterTask++)
			{
				int iCurrentTask = *iterTask;
				//test if already assigned;
				if (rcPriorityAssignment.getPriorityByTask(iCurrentTask) != -1)
				{
					continue;
				}

				//test if satisfies 1 and 2
				if (rsetTasksOnPaths.count(iCurrentTask) || rsetJitterPropagators.count(iCurrentTask))
				{
					continue;
				}				


				//Assign the priority and perform schedulability 
				rcPriorityAssignment.setPriority(iCurrentTask, iCurrPriorityLevel);
				DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, rcPriorityAssignment);
				double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes(iCurrentTask, -1, vectorMaximumResponseTimes);
				bool bSchedTestStatus = (dRT != -1);
				if (!bSchedTestStatus)
				{
					rcPriorityAssignment.unset(iCurrentTask);
				}
				else
				{
					bAssigned = true;
					break;
				}
			}

			if (!bAssigned)
			{
				iCurrPriorityLevel = iPriorityLevelLB - 1;
				break;
			}
		}
	}	
}

void DistributedEventDrivenMUDARComputer::DeriveSafePriorityAssignment_Approx(TaskSetPriorityStruct & rcPriorityAssignment)
{
	/*
	If a task,
	1) does not propagate jitter
	2) is not involved in any path
	3) schedulable assuming worst case response time of jitter source task at lowest unassigned priority
	then can safely assign the lowest unassigned priority
	*/
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
	const set<int> & rsetTasksOnPaths = m_pcTaskSet->getObjectsOnPaths();
	const set<int> & rsetJitterPropagators = m_pcTaskSet->getJitterPropogator();
	int iCurrPriorityLevel = iTaskNum - 1;
	vector<double> vectorMaximumResponseTimes;
	vectorMaximumResponseTimes.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorMaximumResponseTimes.push_back(pcTaskArray[i].getPeriod());
	}

	for (set<int>::iterator iterPartition = rsetAllocationIndices.begin(); iterPartition != rsetAllocationIndices.end(); iterPartition++)
	{
		const set<int> & rsetTasksOnPartition = m_pcTaskSet->getPartition_ByAllocation(*iterPartition);
		int iPriorityLevelLB = iCurrPriorityLevel - rsetTasksOnPartition.size() + 1;
		for (; iCurrPriorityLevel >= iPriorityLevelLB; iCurrPriorityLevel--)
		{
			//Test if already assigned
			if (rcPriorityAssignment.getTaskByPriority(iCurrPriorityLevel) != -1)	continue;

			//If not, iterate through all candidates.
			bool bAssigned = false;
			for (set<int>::iterator iterTask = rsetTasksOnPartition.begin(); iterTask != rsetTasksOnPartition.end(); iterTask++)
			{
				int iCurrentTask = *iterTask;
				//test if already assigned;
				if (rcPriorityAssignment.getPriorityByTask(iCurrentTask) != -1)
				{
					continue;
				}

				//test if satisfies 1 and 2
				if (rsetTasksOnPaths.count(iCurrentTask) || rsetJitterPropagators.count(iCurrentTask))
				{
					continue;
				}


				//Assign the priority and perform schedulability 
				rcPriorityAssignment.setPriority(iCurrentTask, iCurrPriorityLevel);
				DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, rcPriorityAssignment);
				double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes_Approx(iCurrentTask, -1, vectorMaximumResponseTimes);
				bool bSchedTestStatus = (dRT != -1);
				if (!bSchedTestStatus)
				{
					rcPriorityAssignment.unset(iCurrentTask);
				}
				else
				{
					bAssigned = true;
					break;
				}
			}

			if (!bAssigned)
			{
				iCurrPriorityLevel = iPriorityLevelLB - 1;
				break;
			}
		}
	}
}

//DistributedEventDrivenMUDARComputer_Approx

DistributedEventDrivenMUDARComputer_Approx::DistributedEventDrivenMUDARComputer_Approx()
{

}

DistributedEventDrivenMUDARComputer_Approx::DistributedEventDrivenMUDARComputer_Approx(SystemType & rcSystem)
	:DistributedEventDrivenMUDARComputer(rcSystem)
{

}

bool DistributedEventDrivenMUDARComputer_Approx::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, rcPriorityAssignment);
	double dRTLimit = m_vectorDeadlineUB[iTaskIndex];
	double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes_Approx(iTaskIndex, dRTLimit, m_vectorDeadlineLB);

	if (dRT == -1)
	{
		return false;
	}	
	if (dRT <= m_vectorDeadlineUB[iTaskIndex])
	{
		return true;
	}
	else
	{
		return false;
	}
}

DistributedEventDrivenMUDARComputer_Approx::SchedCondContent DistributedEventDrivenMUDARComputer_Approx::SchedCondUpperBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;
	double dDeadlineBound = pcTaskArray[iTaskIndex].getPeriod();
	return dDeadlineBound;
}

//DistributedEventDrivenMUDARComputer_WaitingTimeBased

DistributedEventDrivenMUDARComputer_WaitingTimeBased::DistributedEventDrivenMUDARComputer_WaitingTimeBased()
{

}

DistributedEventDrivenMUDARComputer_WaitingTimeBased::DistributedEventDrivenMUDARComputer_WaitingTimeBased(SystemType & rcSystem)
	:DistributedEventDrivenMUDARComputer(rcSystem)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	m_vectorDerivedResponseTime.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorDerivedResponseTime.push_back(-1);
	}
}

DistributedEventDrivenMUDARComputer_WaitingTimeBased::SchedCondContent DistributedEventDrivenMUDARComputer_WaitingTimeBased::SchedCondUpperBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;
	double dBound = m_pcTaskSet->getWaitingTimeBound(iTaskIndex);
	return dBound;
}

DistributedEventDrivenMUDARComputer_WaitingTimeBased::SchedCondContent DistributedEventDrivenMUDARComputer_WaitingTimeBased::SchedCondLowerBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;
	int iObjType = m_pcTaskSet->getObjectType(iTaskIndex);
	double dBound = 0;
	dBound = pcTaskArray[iTaskIndex].getCLO();
	return dBound;
}

void DistributedEventDrivenMUDARComputer_WaitingTimeBased::IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	DistributedEventDrivenMUDARComputer::IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);	
	ConstructResponseTimeFromWaitingTime(m_vectorDeadlineLB, m_vectorDerivedResponseTime);//Derived Response Time are essentially used as jitter. thus derived from lower bound.
}

bool DistributedEventDrivenMUDARComputer_WaitingTimeBased::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	/*
	two conditions:
	1) Schedulable RT <= T
	2) Waiting time no larger than specified bound
	*/
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, rcPriorityAssignment);
	double dWTLimit = m_vectorDeadlineUB[iTaskIndex];
	double dWT = cRTCalc.ComputeWaitingTime_SpecifiedResponseTimes(iTaskIndex, dWTLimit, m_vectorDerivedResponseTime);
	if (dWT < 0)
	{
		return false;
	}

	if (dWT <= m_vectorDeadlineUB[iTaskIndex])
	{
		return true;
	}
	else
	{
		return false;
	}
}

void DistributedEventDrivenMUDARComputer_WaitingTimeBased::ConstructResponseTimeFromWaitingTime(const vector<double> & rvectorWaitingTimes, vector<double> & rvectorResponseTimes)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	if (rvectorResponseTimes.size() != iTaskNum)
	{
		rvectorResponseTimes.clear();
		rvectorResponseTimes.reserve(iTaskNum);
		for (int i = 0; i < iTaskNum; i++)
		{
			rvectorResponseTimes.push_back(-1);
		}
	}
	else
	{
		for (int i = 0; i < iTaskNum; i++)
		{
			if (m_pcTaskSet->getJitterSource(i) == -1)
			{
				rvectorResponseTimes[i] = rvectorWaitingTimes[i];
			}
			else
			{
				rvectorResponseTimes[i] = -1;
			}
		}
	}

	for (int i = 0; i < iTaskNum; i++)
	{
		if (rvectorResponseTimes[i] == -1)
			ResolveResponseTime(i, rvectorWaitingTimes, rvectorResponseTimes);
		assert(rvectorResponseTimes[i] >= 0);
	}
}

double DistributedEventDrivenMUDARComputer_WaitingTimeBased::ResolveResponseTime(int iTaskIndex, const vector<double> & rvectorWaitingTimes, vector<double> & rvectorResponseTimes)
{
	int iJitterSource = m_pcTaskSet->getJitterSource(iTaskIndex);
	if (iJitterSource == -1)
	{
		rvectorResponseTimes[iTaskIndex] = rvectorWaitingTimes[iTaskIndex];
		return rvectorWaitingTimes[iTaskIndex];
	}
	else
	{
		if (rvectorResponseTimes[iJitterSource] == -1)
		{
			double dRT = ResolveResponseTime(iJitterSource, rvectorWaitingTimes, rvectorResponseTimes);
			if (rvectorResponseTimes[iJitterSource] != dRT)
			{
				cout << "ResolveResponseTime Fail" << endl;
				cout << dRT << " " << rvectorResponseTimes[iJitterSource] << endl;
				while (1);
			}
		}
		rvectorResponseTimes[iTaskIndex] = rvectorWaitingTimes[iTaskIndex] + rvectorResponseTimes[iJitterSource];
		return rvectorResponseTimes[iTaskIndex];
	}
}

//DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx

DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx()
{

}

DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx(SystemType & rcSystem)
	:DistributedEventDrivenMUDARComputer_WaitingTimeBased(rcSystem)
{

}

DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::SchedCondContent DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::SchedCondUpperBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;
	double dBound = min(m_pcTaskSet->getWaitingTimeBound(iTaskIndex), pcTaskArray[iTaskIndex].getPeriod());
	return dBound;
}

bool DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	/*
	two conditions:
	1) Schedulable RT <= T
	2) Waiting time no larger than specified bound
	*/
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, rcPriorityAssignment);
	double dWTLimit = m_vectorDeadlineUB[iTaskIndex];
	double dWT = cRTCalc.ComputeWaitingTime_SpecifiedResponseTimes_Approx(iTaskIndex, dWTLimit, m_vectorDerivedResponseTime);
	if (dWT == -1)
	{
		return false;
	}

	if (dWT <= m_vectorDeadlineUB[iTaskIndex])
	{
		return true;
	}
	else
	{
		return false;
	}
}

//DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime

DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime::DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime()
{

}

DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime::DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime(SystemType & rcSystem)
	:DistributedEventDrivenMUDARComputer_WaitingTimeBased(rcSystem)
{

}

DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime::SchedCondContent DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime::SchedCondLowerBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;
	int iObjType = m_pcTaskSet->getObjectType(iTaskIndex);
	double dBound = 0;
	switch (rcSchedCondType.m_enumCondType)
	{
	case SchedCondType::UB:
		dBound = pcTaskArray[iTaskIndex].getCLO();
		break;
	case SchedCondType::LB:
		return pcTaskArray[iTaskIndex].getCLO();
		break;
	default:
		break;
	}
	
	return dBound;
}

DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime::SchedCondContent DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime::SchedCondUpperBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;
	double dBound = 0;
	switch (rcSchedCondType.m_enumCondType)
	{
	case SchedCondType::UB:
		dBound = m_pcTaskSet->getWaitingTimeBound(iTaskIndex);
		break;
	case SchedCondType::LB:
		dBound = m_pcTaskSet->getDeadlineBound(iTaskIndex);
		break;
	default:
		break;
	}	
	return dBound;
}

bool DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, rcPriorityAssignment);
	double dWTLimit = m_vectorDeadlineUB[iTaskIndex];
	double dWT = cRTCalc.ComputeWaitingTime_SpecifiedResponseTimes(iTaskIndex, dWTLimit, m_vectorDeadlineLB);
	if (dWT < 0)
	{
		return false;
	}

	if (dWT <= m_vectorDeadlineUB[iTaskIndex])
	{
		return true;
	}
	else
	{
		return false;
	}
}

//DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx

DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx::DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx()
{

}

DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx::DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx(SystemType & rcSystem)
	:DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime(rcSystem)
{

}

DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx::SchedCondContent DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx::SchedCondUpperBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iTaskIndex = rcSchedCondType.m_iTaskIndex;
	double dBound = 0;
	switch (rcSchedCondType.m_enumCondType)
	{
	case SchedCondType::UB:
		dBound = m_pcTaskSet->getWaitingTimeBound(iTaskIndex);
		dBound = min(dBound, pcTaskArray[iTaskIndex].getPeriod());
		break;
	case SchedCondType::LB:
		dBound = pcTaskArray[iTaskIndex].getPeriod();
		break;
	default:
		break;
	}
	return dBound;
}

bool DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, rcPriorityAssignment);
	double dWTLimit = m_vectorDeadlineUB[iTaskIndex];
	double dWT = cRTCalc.ComputeWaitingTime_SpecifiedResponseTimes_Approx(iTaskIndex, dWTLimit, m_vectorDeadlineLB);
	if (dWT < 0)
	{
		return false;
	}

	if (dWT <= m_vectorDeadlineUB[iTaskIndex])
	{
		return true;
	}
	else
	{
		return false;
	}
}

#ifdef MCMILPPACT
//DistributedEventDrivenMUDARGuidedOpt
DistributedEventDrivenMUDARGuidedOpt::DistributedEventDrivenMUDARGuidedOpt()
{

}

DistributedEventDrivenMUDARGuidedOpt::DistributedEventDrivenMUDARGuidedOpt(SystemType & rcSystem)
	:GeneralFocusRefinement_MonoInt(rcSystem)
{

}

void DistributedEventDrivenMUDARGuidedOpt::CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars)
{
	//Got to figure out what are the necessary task to export sched conds.
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	//All tasks that propagate jitter should have a lower and upper bound
	const set<int> & rsetJitterPropagator = m_pcTaskSet->getJitterPropogator();	
	for (set<int>::const_iterator iter = rsetJitterPropagator.begin(); iter != rsetJitterPropagator.end(); iter++)
	{
		int iTaskIndex = *iter;
		SchedCondType cLBType(iTaskIndex, SchedCondType::LB);
		SchedCondType cUBType(iTaskIndex, SchedCondType::UB);
		rcSchedCondTypeVars[cLBType] = IloNumVar(rcEnv, m_cUnschedCoreComputer.SchedCondLowerBound(cLBType), m_cUnschedCoreComputer.SchedCondUpperBound(cLBType), IloNumVar::Int);
		rcSchedCondTypeVars[cUBType] = IloNumVar(rcEnv, m_cUnschedCoreComputer.SchedCondLowerBound(cUBType), m_cUnschedCoreComputer.SchedCondUpperBound(cUBType), IloNumVar::Int);
	}

	//All tasks involved on a path should have a upper bound
	const set<int> & rsetObjectsOnPaths = m_pcTaskSet->getObjectsOnPaths();
	for (set<int>::const_iterator iter = rsetObjectsOnPaths.begin(); iter != rsetObjectsOnPaths.end(); iter++)
	{
		int iTaskIndex = *iter;
		SchedCondType cUBType(iTaskIndex, SchedCondType::UB);
		if (rcSchedCondTypeVars.count(cUBType)) continue;
		rcSchedCondTypeVars[cUBType] = IloNumVar(rcEnv, m_cUnschedCoreComputer.SchedCondLowerBound(cUBType), m_cUnschedCoreComputer.SchedCondUpperBound(cUBType), IloNumVar::Int);
	}

}

int DistributedEventDrivenMUDARGuidedOpt::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout)
{
	try
	{
		IloEnv cSolveEnv;
		IloRangeArray cConst(cSolveEnv);		
		SchedCondTypeVars cSchedCondTypeVars;
		SchedCondVars cSchedCondVars;
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);
		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);

		GenUBLBEqualConst(cSolveEnv, cSchedCondTypeVars, cConst);
		GenEnd2EndConst(cSolveEnv, cSchedCondTypeVars, cConst);
		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenSchedCondTypeConst(cSolveEnv, cSchedCondTypeVars, cSchedCondVars, cConst);
		GenPositiveWatingTimeConst(cSolveEnv, cSchedCondTypeVars, cConst);

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
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
			ExtractAllSolution(cSolver, cSchedCondTypeVars);
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

void DistributedEventDrivenMUDARGuidedOpt::GenEnd2EndConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	/*
	\sum w_i <= L
	w_i = R_i - J_i
	*/
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	typedef DistributedEventDrivenTaskSet::End2EndPath End2EndPath;
	const deque<End2EndPath> & rdequeEnd2EndPaths = m_pcTaskSet->getEnd2EndPaths();
	for (deque<End2EndPath>::const_iterator iter = rdequeEnd2EndPaths.begin(); iter != rdequeEnd2EndPaths.end(); iter++)
	{
		const deque<int> & rdequeNodes = iter->getNodes();
		IloExpr cEnd2EndLatency(rcEnv);
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{
			int iNodeIndex = *iterNode;
			int iJitterSource = m_pcTaskSet->getJitterSource(iNodeIndex);
			assert(rcSchedCondTypeVars.count(SchedCondType(iNodeIndex, SchedCondType::UB)) == 1);
			cEnd2EndLatency += rcSchedCondTypeVars[SchedCondType(iNodeIndex, SchedCondType::UB)];
			if (iJitterSource != -1)
			{
				assert(rcSchedCondTypeVars.count(SchedCondType(iJitterSource, SchedCondType::UB)) == 1);
				cEnd2EndLatency -= rcSchedCondTypeVars[SchedCondType(iJitterSource, SchedCondType::UB)];
			}
		}
		rcConst.add(cEnd2EndLatency <= iter->getDeadline());
	}
}

void DistributedEventDrivenMUDARGuidedOpt::GenUBLBEqualConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (SchedCondTypeVars::iterator iter = rcSchedCondTypeVars.begin(); iter != rcSchedCondTypeVars.end(); iter++)
	{
		int iTaskIndex = iter->first.m_iTaskIndex;
		if (iter->first.m_enumCondType == SchedCondType::UB)
		{
			if (rcSchedCondTypeVars.count(SchedCondType(iTaskIndex, SchedCondType::LB)))
			{
				rcConst.add(IloRange(rcEnv, 0.0,
					iter->second - rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::LB)],
					0.0
					));
			}			
		}
		else if (iter->first.m_enumCondType == SchedCondType::LB)
		{
			if (rcSchedCondTypeVars.count(SchedCondType(iTaskIndex, SchedCondType::UB)))
			{
				rcConst.add(IloRange(rcEnv, 0.0,
					iter->second - rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::UB)],
					0.0
					));
			}
		}
	}
}

IloExpr DistributedEventDrivenMUDARGuidedOpt::WaitingTimeExpr(IloEnv & rcEnv, int iTaskIndex, SchedCondTypeVars & rcSchedCondTypeVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iJitterSource = m_pcTaskSet->getJitterSource(iTaskIndex);
	IloExpr cWaitingTimeExpr(rcEnv);

	if (iJitterSource == -1)
	{
		cWaitingTimeExpr =
			rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::UB)];
	}
	else
	{
		cWaitingTimeExpr =
			rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::UB)] -
			rcSchedCondTypeVars[SchedCondType(iJitterSource, SchedCondType::UB)];
	}
	return cWaitingTimeExpr;
}

void DistributedEventDrivenMUDARGuidedOpt::GenPositiveWatingTimeConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (SchedCondTypeVars::iterator iter = rcSchedCondTypeVars.begin(); iter != rcSchedCondTypeVars.end(); iter++)
	{
		int iTaskIndex = iter->first.m_iTaskIndex;
		int iJitterSource = m_pcTaskSet->getJitterSource(iTaskIndex);
		if (iJitterSource != -1)
		{
			IloExpr cWaitingTimeExpr(rcEnv);
			if (m_pcTaskSet->getObjectType(iTaskIndex) == 0)
			{
				cWaitingTimeExpr =
					rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::UB)] - rcSchedCondTypeVars[SchedCondType(iJitterSource, SchedCondType::UB)];
				rcConst.add(IloRange(rcEnv,
					pcTaskArray[iTaskIndex].getCLO(),
					cWaitingTimeExpr,
					IloInfinity
					));
			}
			else
			{
				cWaitingTimeExpr =
					rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::UB)] -
					rcSchedCondTypeVars[SchedCondType(iJitterSource, SchedCondType::UB)] - pcTaskArray[iTaskIndex].getCLO();
				rcConst.add(IloRange(rcEnv,
					0.0,
					cWaitingTimeExpr,
					IloInfinity
					));
			}

		}
	}
}

//DistributedEventDrivenMUDARGuidedOpt_Approx
DistributedEventDrivenMUDARGuidedOpt_Approx::DistributedEventDrivenMUDARGuidedOpt_Approx()
{

}

DistributedEventDrivenMUDARGuidedOpt_Approx::DistributedEventDrivenMUDARGuidedOpt_Approx(SystemType & rcSystem)
	:GeneralFocusRefinement_MonoInt(rcSystem)
{

}

void DistributedEventDrivenMUDARGuidedOpt_Approx::CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars)
{
	//Got to figure out what are the necessary task to export sched conds.
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	//All tasks that propagate jitter should have a lower and upper bound
	const set<int> & rsetJitterPropagator = m_pcTaskSet->getJitterPropogator();
	for (set<int>::const_iterator iter = rsetJitterPropagator.begin(); iter != rsetJitterPropagator.end(); iter++)
	{
		int iTaskIndex = *iter;
		SchedCondType cLBType(iTaskIndex, SchedCondType::LB);
		SchedCondType cUBType(iTaskIndex, SchedCondType::UB);
		rcSchedCondTypeVars[cLBType] = IloNumVar(rcEnv, m_cUnschedCoreComputer.SchedCondLowerBound(cLBType), m_cUnschedCoreComputer.SchedCondUpperBound(cLBType), IloNumVar::Int);
		rcSchedCondTypeVars[cUBType] = IloNumVar(rcEnv, m_cUnschedCoreComputer.SchedCondLowerBound(cUBType), m_cUnschedCoreComputer.SchedCondUpperBound(cUBType), IloNumVar::Int);
	}

	//All tasks involved on a path should have a upper bound
	const set<int> & rsetObjectsOnPaths = m_pcTaskSet->getObjectsOnPaths();
	for (set<int>::const_iterator iter = rsetObjectsOnPaths.begin(); iter != rsetObjectsOnPaths.end(); iter++)
	{
		int iTaskIndex = *iter;
		SchedCondType cUBType(iTaskIndex, SchedCondType::UB);
		if (rcSchedCondTypeVars.count(cUBType)) continue;
		rcSchedCondTypeVars[cUBType] = IloNumVar(rcEnv, m_cUnschedCoreComputer.SchedCondLowerBound(cUBType), m_cUnschedCoreComputer.SchedCondUpperBound(cUBType), IloNumVar::Int);
	}

}

int DistributedEventDrivenMUDARGuidedOpt_Approx::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout)
{
	try
	{
		IloEnv cSolveEnv;
		IloRangeArray cConst(cSolveEnv);
		SchedCondTypeVars cSchedCondTypeVars;
		SchedCondVars cSchedCondVars;		
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);
		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);

		GenUBLBEqualConst(cSolveEnv, cSchedCondTypeVars, cConst);
		GenEnd2EndConst(cSolveEnv, cSchedCondTypeVars, cConst);
		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenSchedCondTypeConst(cSolveEnv, cSchedCondTypeVars, cSchedCondVars, cConst);
		GenPositiveWatingTimeConst(cSolveEnv, cSchedCondTypeVars, cConst);
		IloModel cModel(cSolveEnv);		
		cModel.add(cConst);
		GenMaxMinLaxityModel(cSolveEnv, cSchedCondTypeVars, cModel);

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
			ExtractAllSolution(cSolver, cSchedCondTypeVars);
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

void DistributedEventDrivenMUDARGuidedOpt_Approx::GenPositiveWatingTimeConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (SchedCondTypeVars::iterator iter = rcSchedCondTypeVars.begin(); iter != rcSchedCondTypeVars.end(); iter++)
	{
		int iTaskIndex = iter->first.m_iTaskIndex;
		int iJitterSource = m_pcTaskSet->getJitterSource(iTaskIndex);
		if (iJitterSource != -1)
		{
			IloExpr cWaitingTimeExpr(rcEnv);
			if (m_pcTaskSet->getObjectType(iTaskIndex) == 0)
			{
				cWaitingTimeExpr =
					rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::UB)] - rcSchedCondTypeVars[SchedCondType(iJitterSource, SchedCondType::UB)];
				rcConst.add(IloRange(rcEnv,
					pcTaskArray[iTaskIndex].getCLO(),
					cWaitingTimeExpr,
					IloInfinity
					));
			}
			else
			{
				cWaitingTimeExpr =
					rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::UB)] -
					rcSchedCondTypeVars[SchedCondType(iJitterSource, SchedCondType::UB)] - pcTaskArray[iTaskIndex].getCLO();
				rcConst.add(IloRange(rcEnv,
					0.0,
					cWaitingTimeExpr,
					IloInfinity
					));
			}

		}
	}
}

void DistributedEventDrivenMUDARGuidedOpt_Approx::GenEnd2EndConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	/*
	\sum w_i <= L
	w_i = R_i - J_i
	*/
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	typedef DistributedEventDrivenTaskSet::End2EndPath End2EndPath;
	const deque<End2EndPath> & rdequeEnd2EndPaths = m_pcTaskSet->getEnd2EndPaths();
	for (deque<End2EndPath>::const_iterator iter = rdequeEnd2EndPaths.begin(); iter != rdequeEnd2EndPaths.end(); iter++)
	{
		const deque<int> & rdequeNodes = iter->getNodes();
		IloExpr cEnd2EndLatency(rcEnv);
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{
			int iNodeIndex = *iterNode;
			int iJitterSource = m_pcTaskSet->getJitterSource(iNodeIndex);
			assert(rcSchedCondTypeVars.count(SchedCondType(iNodeIndex, SchedCondType::UB)) == 1);
			cEnd2EndLatency += rcSchedCondTypeVars[SchedCondType(iNodeIndex, SchedCondType::UB)];
			if (iJitterSource != -1)
			{
				assert(rcSchedCondTypeVars.count(SchedCondType(iJitterSource, SchedCondType::UB)) == 1);
				cEnd2EndLatency -= rcSchedCondTypeVars[SchedCondType(iJitterSource, SchedCondType::UB)];
			}
		}
		rcConst.add(cEnd2EndLatency <= iter->getDeadline());
	}
}

void DistributedEventDrivenMUDARGuidedOpt_Approx::GenUBLBEqualConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const set<int> & rsetJitterPropagator = m_pcTaskSet->getJitterPropogator();
	for (SchedCondTypeVars::iterator iter = rcSchedCondTypeVars.begin(); iter != rcSchedCondTypeVars.end(); iter++)
	{
		int iTaskIndex = iter->first.m_iTaskIndex;				
		if (iter->first.m_enumCondType == SchedCondType::UB)
		{
			if (rcSchedCondTypeVars.count(SchedCondType(iTaskIndex, SchedCondType::LB)))
			{
				rcConst.add(IloRange(rcEnv, 0.0,
					iter->second - rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::LB)],
					0.0
					));
			}
		}
		else if (iter->first.m_enumCondType == SchedCondType::LB)
		{
			if (rcSchedCondTypeVars.count(SchedCondType(iTaskIndex, SchedCondType::UB)))
			{
				rcConst.add(IloRange(rcEnv, 0.0,
					iter->second - rcSchedCondTypeVars[SchedCondType(iTaskIndex, SchedCondType::UB)],
					0.0
					));
			}
		}
	}
}

void DistributedEventDrivenMUDARGuidedOpt_Approx::GenMaxMinLaxityModel(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	IloNumVar cMaxLaxity(rcEnv, 0.0, IloInfinity, IloNumVar::Float);
	IloRangeArray cConst(rcEnv);
	for (SchedCondTypeVars::iterator iter = rcSchedCondTypeVars.begin(); iter != rcSchedCondTypeVars.end(); iter++)
	{
		int iTaskIndex = iter->first.m_iTaskIndex;
		cConst.add(IloRange(rcEnv,
			-IloInfinity,
			cMaxLaxity - (pcTaskArray[iTaskIndex].getPeriod() - iter->second),
			0.0
			));
	}

	rcModel.add(cConst);
	rcModel.add(IloMinimize(rcEnv, -cMaxLaxity));
}

//DistributedEventDrivenFullMILP

BinaryArrayInteger::BinaryArrayInteger(const IloEnv & rcEnv, IloNum dLB, IloNum dUB)
	:m_cBinVars(rcEnv)
{
	m_dLB = dLB;
	m_dUB = dUB;
	CreateBinVars(rcEnv, m_dUB);
}

void BinaryArrayInteger::CreateBinVars(const IloEnv & rcEnv, IloNum dUB)
{
	int iSize = ceil(log2(dUB));	
	for (int i = 0; i < iSize; i++)
	{
		m_cBinVars.add(IloNumVar(rcEnv, 0.0, 1.0, IloNumVar::Int));		
	}
}

void BinaryArrayInteger::GenBoundConst(IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcConst.getEnv();
	rcConst.add(IloRange(rcEnv,
		m_dLB,
		getExpr(),
		m_dUB
		));
}

IloExpr BinaryArrayInteger::getExpr()
{
	int iSize = m_cBinVars.getSize();
	const IloEnv & rcEnv = m_cBinVars.getEnv();
	double dCoef = 1;
	IloExpr cExpr(rcEnv);
	for (int i = 0; i < iSize; i++)
	{
		cExpr += dCoef * m_cBinVars[i];
		dCoef *= 2;
	}
	return cExpr;
}

double BinaryArrayInteger::getValue(IloCplex & rcSolver)
{
	return rcSolver.getValue(getExpr());	
}

IloExpr BinaryArrayInteger::getLinearizeProduct(const IloEnv & rcEnv, IloNumVar & rcFactor, IloNum dFactorUB, IloRangeArray & rcConst)
{
	IloNumVarArray cBinProductVars(rcEnv);
	int iSize = m_cBinVars.getSize();
	double dCoef = 1;
	IloExpr cExpr(rcEnv);
	for (int i = 0; i < iSize; i++)
	{
		cBinProductVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Float));
		rcConst.add(IloRange(rcEnv,
			0.0,
			cBinProductVars[i] - rcFactor * dCoef + (1 - m_cBinVars[i]) * dFactorUB * dCoef,
			IloInfinity
			));
#if 1
		rcConst.add(IloRange(rcEnv,
			-IloInfinity,
			cBinProductVars[i] - rcFactor * dCoef,
			0.0
			));

		rcConst.add(IloRange(rcEnv,
			-IloInfinity,
			cBinProductVars[i] - (m_cBinVars[i]) * dFactorUB * dCoef,
			0.0
			));
#endif
		cExpr += cBinProductVars[i];
		dCoef *= 2;
	}
	return cExpr;
}

DistributedEventDrivenFullMILP::DistributedEventDrivenFullMILP(DistributedEventDrivenTaskSet & rcTaskSet)
	:DistributedEventDrivenFullMILP()
{
	m_pcTaskSet = &rcTaskSet;
	m_cSolPA = TaskSetPriorityStruct(rcTaskSet);
	m_cPreAssignment = TaskSetPriorityStruct(rcTaskSet);
}

DistributedEventDrivenFullMILP::DistributedEventDrivenFullMILP()
{
	m_dSolveCPUTime = -1;
	m_dSolveWallTime = -1;
	m_dObjective = -1;
}

void DistributedEventDrivenFullMILP::CreatePVars(const IloEnv & rcEnv, Variable2D & rcPVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			rcPVars[i][j] = IloNumVar(rcEnv, 0.0, 1.0, IloNumVar::Int);
		}
	}
}

void DistributedEventDrivenFullMILP::CreateInterVars(const IloEnv & rcEnv, Variable2D & rcInterVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iNumTasks, pcTaskArray);
	for (int i = 0; i < iNumTasks; i++)
	{
		for (int j = 0; j < iNumTasks; j++)
		{
			rcInterVars[i][j] = IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Int);
		}
	}
}

void DistributedEventDrivenFullMILP::CreatePIPVars(const IloEnv & rcEnv, Variable2D & rcPIPVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iNumTasks, pcTaskArray);
	for (int i = 0; i < iNumTasks; i++)
	{
		for (int j = 0; j < iNumTasks; j++)
		{
			rcPIPVars[i][j] = IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Float);
		}
	}
}

void DistributedEventDrivenFullMILP::CreateRTVars(const IloEnv & rcEnv, Variable1D & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		rcRTVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float));
	}
}

void DistributedEventDrivenFullMILP::GenPVarConst(const IloEnv & rcEnv, int iAllocation, Variable2D & rcPVars, IloRangeArray & rcConst)
{
	const set<int> & rsetPartitions = m_pcTaskSet->getPartition_ByAllocation(iAllocation);
	int iTaskNum = rsetPartitions.size();
	for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
	{
		int i = *iter_i;
		for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
		{
			int j = *iter_j;
			if (i == j)	continue;
			rcConst.add(IloRange(rcEnv, 0.0, rcPVars[i][j] + rcPVars[j][i] - 1.0, 0.0));
		}
	}

	for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
	{
		int i = *iter_i;
		for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
		{
			int j = *iter_j;
			if (i == j)	continue;
			for (set<int>::const_iterator iter_k = rsetPartitions.begin(); iter_k != rsetPartitions.end(); iter_k++)
			{
				int k = *iter_k;
				if ((k == j) || (k == i))	continue;
				rcConst.add(IloRange(rcEnv, 0.0, rcPVars[i][j] - rcPVars[i][k] - rcPVars[k][j] + 1.0, IloInfinity));
			}
		}
	}
}

void DistributedEventDrivenFullMILP::GenInterVarConst(const IloEnv & rcEnv, int iAllocation, Variable2D & rcInterVars, Variable1D & rcRTVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTotalTaskNum, pcTaskArray);
	const set<int> & rsetPartitions = m_pcTaskSet->getPartition_ByAllocation(iAllocation);
	int iTaskNum = rsetPartitions.size();
	for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
	{
		int i = *iter_i;
		IloExpr cWaitingTimeExpr = WaitingTimeExpr(i, rcRTVars);		
		int iObjType = m_pcTaskSet->getObjectType(i);

		for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
		{
			int j = *iter_j;
			if (i == j)	continue;			
			assert(m_pcTaskSet->getObjectType(j) == iObjType);
			int iJitterSource_j = m_pcTaskSet->getJitterSource(j);
			IloExpr cLBExpr(rcEnv);			

			if (iJitterSource_j == -1)			
			{
				if (iObjType == 1)
				{
					cLBExpr = (cWaitingTimeExpr - pcTaskArray[i].getCLO()) / pcTaskArray[j].getPeriod();
				}
				else
				{
					cLBExpr = (cWaitingTimeExpr) / pcTaskArray[j].getPeriod();
				}				
			}
			else
			{
				if (iObjType == 1)
				{
					cLBExpr = (cWaitingTimeExpr + rcRTVars[iJitterSource_j] - pcTaskArray[i].getCLO()) / pcTaskArray[j].getPeriod();
				}
				else
				{
					cLBExpr = (cWaitingTimeExpr + rcRTVars[iJitterSource_j]) / pcTaskArray[j].getPeriod();
				}
			}
			
			
			m_mapInterVarLBExpr[j][i] = cLBExpr;
			rcConst.add(IloRange(rcEnv,
				0.0,
				rcInterVars[j][i] - cLBExpr,
				IloInfinity
				));
		}
	}
}

double DistributedEventDrivenFullMILP::DeterminedPIPBigM(int iTask_i, int iTask_j)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iJitterSource_j = m_pcTaskSet->getJitterSource(iTask_j);
	double dPIPBigM = (pcTaskArray[iTask_i].getDeadline() + pcTaskArray[iJitterSource_j].getDeadline()) / pcTaskArray[iTask_j].getPeriod();
	dPIPBigM = ceil(dPIPBigM) + 1;		
	return 2 * dPIPBigM;
}

void DistributedEventDrivenFullMILP::GenPIPVarConst(const IloEnv & rcEnv, int iAllocation, Variable2D & rcPVars, Variable2D & rcInterVars, Variable2D & rcPIPVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTotalTaskNum, pcTaskArray);
	const set<int> & rsetPartitions = m_pcTaskSet->getPartition_ByAllocation(iAllocation);
	for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
	{
		int i = *iter_i;
		for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
		{
			int j = *iter_j;
			if (i == j)	continue;
			double dBigM = DeterminedPIPBigM(i, j);
			IloExpr cExprA(rcEnv);
			cExprA = rcPIPVars[j][i] - rcInterVars[j][i] + (rcPVars[i][j]) * dBigM;
			rcConst.add(IloRange(
				rcEnv, 0.0,
				cExprA,
				IloInfinity
				));
#if 0
			rcConst.add(IloRange(rcEnv,
				-IloInfinity,
				rcPIPVars[j][i] - rcPVars[j][i] * dBigM, 
				0.0
				));
#endif
		}
	}
}

IloExpr DistributedEventDrivenFullMILP::WaitingTimeExpr(int iTaskIndex, Variable1D & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcRTVars.getEnv();
	IloExpr cWaitingTimeExpr(rcEnv);	
	int iJitterSource = m_pcTaskSet->getJitterSource(iTaskIndex);
	int iObjType = m_pcTaskSet->getObjectType(iTaskIndex);
	cWaitingTimeExpr = rcRTVars[iTaskIndex];
	if (iJitterSource != -1)
	{
		cWaitingTimeExpr -= rcRTVars[iJitterSource];
	}
	return cWaitingTimeExpr;
}

void DistributedEventDrivenFullMILP::GenRTVarConst(const IloEnv & rcEnv, int iAllocation, Variable2D & rcPVars, Variable2D & rcPIPIVars, Variable1D & rcRTVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTotalTaskNum, pcTaskArray);
	const set<int> & rsetPartitions = m_pcTaskSet->getPartition_ByAllocation(iAllocation);
	for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
	{
		int i = *iter_i;
		IloExpr cRHS(rcEnv);
		IloNumVar cBlocking(rcEnv, 0.0, IloInfinity, IloNumVar::Float);
		IloExpr cWaitingTimeExpr = WaitingTimeExpr(i, rcRTVars);

		int iObjType = m_pcTaskSet->getObjectType(i);
		if (iObjType == 0)
		{
			cRHS += pcTaskArray[i].getCLO();
		}
		else
		{
			cRHS += cBlocking;
		}

		for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
		{
			int j = *iter_j;
			assert(m_pcTaskSet->getObjectType(j) == iObjType);
			if (i == j)	continue;
			rcConst.add(IloRange(rcEnv,
				0.0,
				cBlocking - rcPVars[i][j] * pcTaskArray[j].getCLO(),
				IloInfinity
				));

			cRHS += rcPIPIVars[j][i] * pcTaskArray[j].getCLO();
		}

		if (iObjType == 0)
		{
			rcConst.add(IloRange(rcEnv,
				0.0,
				cWaitingTimeExpr - cRHS,
				IloInfinity
				));
		}
		else
		{
			rcConst.add(IloRange(rcEnv,
				0.0,
				cWaitingTimeExpr - pcTaskArray[i].getCLO() - cRHS,
				IloInfinity
				));
		}
	
	}
}

void DistributedEventDrivenFullMILP::GenBreakDownUtilization_Model(const IloEnv & rcEnv, int iAllocation, Variable2D & rcPVars, Variable2D & rcPIPVars, Variable2D & rcInterVars, Variable1D & rcRTVars, IloModel & rcModel)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTotalTaskNum, pcTaskArray);
	const set<int> & rsetPartitions = m_pcTaskSet->getPartition_ByAllocation(iAllocation);
	IloNumVar cScalingFactor(rcEnv, 1.0, 1.0 / m_pcTaskSet->getPartitionUtilization_ByPartition(iAllocation), IloNumVar::Float);
	double dScalingFactorUB = 2.0 / m_pcTaskSet->getPartitionUtilization_ByPartition(iAllocation);	
	IloRangeArray rcConst(rcEnv);

	for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
	{
		int i = *iter_i;
		IloExpr cWaitingTimeExpr = WaitingTimeExpr(i, rcRTVars);
		int iObjType = m_pcTaskSet->getObjectType(i);

		for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
		{
			int j = *iter_j;
			if (i == j)	continue;
			assert(m_pcTaskSet->getObjectType(j) == iObjType);
			int iJitterSource_j = m_pcTaskSet->getJitterSource(j);
			IloExpr cLBExpr(rcEnv);

			if (iJitterSource_j == -1)
			{
				if (iObjType == 1)
				{
					cLBExpr = (cWaitingTimeExpr - pcTaskArray[i].getCLO() * cScalingFactor) / pcTaskArray[j].getPeriod();
				}
				else
				{
					cLBExpr = (cWaitingTimeExpr) / pcTaskArray[j].getPeriod();
				}
			}
			else
			{
				if (iObjType == 1)
				{
					cLBExpr = (cWaitingTimeExpr + rcRTVars[iJitterSource_j] - pcTaskArray[i].getCLO() * cScalingFactor) / pcTaskArray[j].getPeriod();
				}
				else
				{
					cLBExpr = (cWaitingTimeExpr + rcRTVars[iJitterSource_j]) / pcTaskArray[j].getPeriod();
				}
			}


			m_mapInterVarLBExpr[j][i] = cLBExpr;

			rcConst.add(IloRange(rcEnv,
				0.0,
				cLBExpr,
				IloInfinity
				));

			rcConst.add(IloRange(rcEnv,
				0.0,
				rcInterVars[j][i] - cLBExpr,
				IloInfinity
				));	
			
		}
	}
#if 1
	map<int, map<int, BinaryArrayInteger> > mapBinaryInteger;
	for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
	{
		int i = *iter_i;		
		for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
		{
			int j = *iter_j;
			double dBigM = DeterminedPIPBigM(i, j);
			double dPIPUB = ceil(m_pcTaskSet->getPartitionHyperperiod_ByTask(i) / pcTaskArray[j].getPeriod());
			mapBinaryInteger[j][i] = BinaryArrayInteger(rcEnv, 0.0, dBigM);
			mapBinaryInteger[j][i].GenBoundConst(rcConst);
			if (i == j)	continue;			
			IloExpr cExprA(rcEnv);
			cExprA = mapBinaryInteger[j][i].getExpr() - rcInterVars[j][i] + (rcPVars[i][j]) * dBigM;
#if 1
			rcConst.add(IloRange(
				rcEnv, 0.0,
				cExprA,
				IloInfinity
				));
#endif
		}
	}
#endif
#if 1
	map<int, map<int, IloExpr> > mapScalingFactorBinaryIntegerProduct;
	map<int, map<int, IloNumVar> > mapScalingPProductVars;
	for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
	{
		int i = *iter_i;
		IloExpr cRHS(rcEnv);
		IloNumVar cBlocking(rcEnv, 0.0, IloInfinity, IloNumVar::Float);
		IloExpr cWaitingTimeExpr = WaitingTimeExpr(i, rcRTVars);

		int iObjType = m_pcTaskSet->getObjectType(i);
		if (iObjType == 0)
		{
			cRHS += pcTaskArray[i].getCLO() * cScalingFactor;
		}
		else
		{
			cRHS += cBlocking;
		}

		for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
		{
			int j = *iter_j;
			mapScalingPProductVars[i][j] = IloNumVar(rcEnv, 0.0, dScalingFactorUB, IloNumVar::Float);
			mapScalingFactorBinaryIntegerProduct[j][i] = mapBinaryInteger[j][i].getLinearizeProduct(rcEnv, cScalingFactor, dScalingFactorUB, rcConst);
			assert(m_pcTaskSet->getObjectType(j) == iObjType);
			if (i == j)	continue;

			rcConst.add(IloRange(rcEnv,
				0.0,
				mapScalingPProductVars[i][j] - cScalingFactor + (1 - rcPVars[i][j]) * 2 * dScalingFactorUB,
				IloInfinity
				));

			rcConst.add(IloRange(rcEnv,
				0.0,
				cBlocking - mapScalingPProductVars[i][j] * pcTaskArray[j].getCLO(),
				IloInfinity
				));

			cRHS += mapScalingFactorBinaryIntegerProduct[j][i] * pcTaskArray[j].getCLO();
		}

		if (iObjType == 0)
		{
			rcConst.add(IloRange(rcEnv,
				0.0,
				cWaitingTimeExpr - cRHS,
				IloInfinity
				));
		}
		else
		{
			rcConst.add(IloRange(rcEnv,
				0.0,
				cWaitingTimeExpr - pcTaskArray[i].getCLO() * cScalingFactor - cRHS,
				IloInfinity
				));
		}
	}
#endif

	IloObjective cObjective = IloMaximize(rcEnv, cScalingFactor);
	rcModel.add(rcConst);
	rcModel.add(cObjective);
}

int DistributedEventDrivenFullMILP::SolveBreakDownUtilization(int iECUIndex, int iDisplayLevel, double dTimeout /* = 1e74 */)
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

		CreatePVars(cSolveEnv, cPVars);
		CreateInterVars(cSolveEnv, cInterInstVars);
		CreateRTVars(cSolveEnv, cRTVars);
		CreatePIPVars(cSolveEnv, cPIPVars);

		const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
		for (set<int>::const_iterator iterAllocInd = rsetAllocationIndices.begin(); iterAllocInd != rsetAllocationIndices.end(); iterAllocInd++)
		{
			int iAllocationIndex = *iterAllocInd;
			GenPVarConst(cSolveEnv, iAllocationIndex, cPVars, cConst);
			GenInterVarConst(cSolveEnv, iAllocationIndex, cInterInstVars, cRTVars, cConst);
			GenPIPVarConst(cSolveEnv, iAllocationIndex, cPVars, cInterInstVars, cPIPVars, cConst);			
			GenRTVarConst(cSolveEnv, iAllocationIndex, cPVars, cPIPVars, cRTVars, cConst);
		}		
		GenPreAssignmentConst(cPVars, cConst);
		GenEnd2EndConst(cSolveEnv, cRTVars, cConst);
		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);		
		switch (m_enumObjective)
		{
		case DistributedEventDrivenFullMILP::None:
			break;
		case DistributedEventDrivenFullMILP::MaxMinEnd2EndLaxity:
			GenMaxMinEnd2EndLaxityModel(cSolveEnv, cRTVars, cModel);
			break;
		default:
			break;
		}
		GenBreakDownUtilization_Model(cSolveEnv, iECUIndex, cPVars, cPIPVars, cInterInstVars, cRTVars, cModel);
		IloCplex cSolver(cModel);
		EnableSolverDisp(iDisplayLevel, cSolver);
		setSolverParam(cSolver);

		//cSolver.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-9);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;
		m_dObjective = -1;
		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;
		IloCplex::Status m_cCplexStatus = cSolver.getCplexStatus();
		if (bStatus)
		{
			m_dObjective = cSolver.getObjValue();
			ExtractSolutionPA(cPVars, cSolver, m_cSolPA);
			ExtractSolutionRT(cRTVars, cSolver, m_vectorSolResponseTimes);
			ExtractSolutionWaitingTimes(cRTVars, cSolver, m_vectorSolWatingTimes);
			VerifyResult(cRTVars, cPVars, cInterInstVars, cPIPVars, cSolver);
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

void DistributedEventDrivenFullMILP::GenEnd2EndConst(const IloEnv & rcEnv, Variable1D & rcRTVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = m_pcTaskSet->getEnd2EndPaths();
	for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
		iter != rdequePaths.end(); iter++)
	{
		const deque<int> & rdequeNodes = iter->getNodes();
		IloExpr cEnd2EndLatency(rcEnv);
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{
			int iNodeIndex = *iterNode;
			IloExpr cWaitingTimeExpr = WaitingTimeExpr(iNodeIndex, rcRTVars);
			cEnd2EndLatency += cWaitingTimeExpr;
		}
		rcConst.add(IloRange(rcEnv,
			-IloInfinity,
			cEnd2EndLatency - iter->getDeadline(),
			0.0
			));
	}
}

void DistributedEventDrivenFullMILP::GenMaxMinEnd2EndLaxityModel(const IloEnv & rcEnv, Variable1D rcRTVars, IloModel & rcModel)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IloNumVar cMinEnd2EndLaxity(rcEnv, 0.0, IloInfinity);
	IloRangeArray cConst(rcEnv);
	const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = m_pcTaskSet->getEnd2EndPaths();
	for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
		iter != rdequePaths.end(); iter++)
	{
		const deque<int> & rdequeNodes = iter->getNodes();
		IloExpr cEnd2EndLatency(rcEnv);
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{
			int iNodeIndex = *iterNode;
			IloExpr cWaitingTimeExpr = WaitingTimeExpr(iNodeIndex, rcRTVars);
			cEnd2EndLatency += cWaitingTimeExpr;
		}
		cConst.add(IloRange(rcEnv,
			-IloInfinity,
			cMinEnd2EndLaxity - (iter->getDeadline() - cEnd2EndLatency),
			0.0
			));
	}
	rcModel.add(cConst);
	rcModel.add(IloMinimize(rcEnv, -cMinEnd2EndLaxity));
}

void DistributedEventDrivenFullMILP::GenMinMaxNormalizedEnd2EndLaxityModel(const IloEnv & rcEnv, Variable1D rcRTVars, IloModel & rcModel)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IloNumVar cMaxNormalizedEnd2EndLatency(rcEnv, 0.0, IloInfinity);
	IloRangeArray cConst(rcEnv);
	const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = m_pcTaskSet->getEnd2EndPaths();

	for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
		iter != rdequePaths.end(); iter++)
	{
		const deque<int> & rdequeNodes = iter->getNodes();
		IloExpr cEnd2EndLatency(rcEnv);
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{
			int iNodeIndex = *iterNode;
			IloExpr cWaitingTimeExpr = WaitingTimeExpr(iNodeIndex, rcRTVars);
			cEnd2EndLatency += cWaitingTimeExpr;
		}
		cConst.add(IloRange(rcEnv,
			0.0,
			cMaxNormalizedEnd2EndLatency - (cEnd2EndLatency / iter->getDeadline()),
			IloInfinity
			));
	}
	rcModel.add(cConst);
	rcModel.add(IloMinimize(rcEnv, cMaxNormalizedEnd2EndLatency));
}

void DistributedEventDrivenFullMILP::EnableSolverDisp(int iLevel, IloCplex & rcSolver)
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

void DistributedEventDrivenFullMILP::setSolverParam(IloCplex & rcSolver)
{
	rcSolver.setParam(IloCplex::Param::Emphasis::MIP, 0);
	rcSolver.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, 1e-9);
	rcSolver.setParam(IloCplex::Param::MIP::Tolerances::Integrality, 0);
}

int DistributedEventDrivenFullMILP::Solve(int iDisplayLevel, double dTimeout /* = 1e74 */)
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

		CreatePVars(cSolveEnv, cPVars);
		CreateInterVars(cSolveEnv, cInterInstVars);
		CreateRTVars(cSolveEnv, cRTVars);
		CreatePIPVars(cSolveEnv, cPIPVars);

		const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
		for (set<int>::const_iterator iterAllocInd = rsetAllocationIndices.begin(); iterAllocInd != rsetAllocationIndices.end(); iterAllocInd++)
		{
			int iAllocationIndex = *iterAllocInd;			
			GenPVarConst(cSolveEnv, iAllocationIndex, cPVars, cConst);			
			GenInterVarConst(cSolveEnv, iAllocationIndex, cInterInstVars, cRTVars, cConst);			
			GenPIPVarConst(cSolveEnv, iAllocationIndex, cPVars, cInterInstVars, cPIPVars, cConst);			
			GenRTVarConst(cSolveEnv, iAllocationIndex, cPVars, cPIPVars, cRTVars, cConst);
		}
		GenPreAssignmentConst(cPVars, cConst);
		GenEnd2EndConst(cSolveEnv, cRTVars, cConst);
		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);
		switch (m_enumObjective)
		{
		case DistributedEventDrivenFullMILP::None:
			break;
		case DistributedEventDrivenFullMILP::MaxMinEnd2EndLaxity:
			GenMaxMinEnd2EndLaxityModel(cSolveEnv, cRTVars, cModel);
			break;
		default:
			break;
		}
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
			ExtractSolutionPA(cPVars, cSolver, m_cSolPA);
			ExtractSolutionRT(cRTVars, cSolver, m_vectorSolResponseTimes);
			ExtractSolutionWaitingTimes(cRTVars, cSolver, m_vectorSolWatingTimes);
			VerifyResult(cRTVars, cPVars, cInterInstVars, cPIPVars, cSolver);			
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

int DistributedEventDrivenFullMILP::VerifyPA(TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorResponseTimes, int iDisplayLevel, double dTimeout)
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

		CreatePVars(cSolveEnv, cPVars);
		CreateInterVars(cSolveEnv, cInterInstVars);
		CreateRTVars(cSolveEnv, cRTVars);
		CreatePIPVars(cSolveEnv, cPIPVars);

		const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
		for (set<int>::const_iterator iterAllocInd = rsetAllocationIndices.begin(); iterAllocInd != rsetAllocationIndices.end(); iterAllocInd++)
		{
			int iAllocationIndex = *iterAllocInd;
			GenPVarConst(cSolveEnv, iAllocationIndex, cPVars, cConst);
			GenInterVarConst(cSolveEnv, iAllocationIndex, cInterInstVars, cRTVars, cConst);
			GenPIPVarConst(cSolveEnv, iAllocationIndex, cPVars, cInterInstVars, cPIPVars, cConst);
			GenRTVarConst(cSolveEnv, iAllocationIndex, cPVars, cPIPVars, cRTVars, cConst);
		}
		GenEnd2EndConst(cSolveEnv, cRTVars, cConst);
		//Target Priority Assignment Constraint
		//Must be complete priority assignment
		assert(rcPriorityAssignment.getSize() == iTaskNum);		
		for (set<int>::const_iterator iterAlloc = rsetAllocationIndices.begin(); iterAlloc != rsetAllocationIndices.end(); iterAlloc++)
		{
			const set<int> & rsetPartition = m_pcTaskSet->getPartition_ByAllocation(*iterAlloc);
			for (set<int>::const_iterator iter_i = rsetPartition.begin(); iter_i != rsetPartition.end(); iter_i++)
			{
				int i = *iter_i;
				if (rcPriorityAssignment.getPriorityByTask(i) == -1)	continue;
				for (set<int>::const_iterator iter_j = rsetPartition.begin(); iter_j != rsetPartition.end(); iter_j++)
				{
					int j = *iter_j;
					if (rcPriorityAssignment.getPriorityByTask(j) < rcPriorityAssignment.getPriorityByTask(i))
					{
						cConst.add(IloRange(cSolveEnv,
							0.0,
							cPVars[i][j],
							0.0
							));

						cConst.add(IloRange(cSolveEnv,
							1.0,
							cPVars[j][i],
							1.0
							));
					}
				}
			}
		}
		
		//Find out the least fixed point. Thus should minimized overall response time
		IloExpr cObjExpr(cSolveEnv);
		for (int i = 0; i < iTaskNum; i++)
		{
			cObjExpr += cRTVars[i];
		}
		IloObjective cObjective = IloMinimize(cSolveEnv, cObjExpr);
		//GenEnd2EndConst(cSolveEnv, cRTVars, cConst);
		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);
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
			if (rvectorResponseTimes.size() != iTaskNum)
			{
				rvectorResponseTimes.clear();
				rvectorResponseTimes.reserve(iTaskNum);
				for (int i = 0; i < iTaskNum; i++)	
					rvectorResponseTimes.push_back(cSolver.getValue(cRTVars[i]));
			}
			else
			{
				for (int i = 0; i < iTaskNum; i++)
				{
					rvectorResponseTimes[i] = cSolver.getValue(cRTVars[i]);
				}
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

void DistributedEventDrivenFullMILP::ExtractSolutionPA(Variable2D & rcPVars, IloCplex & cSolver, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTotalTaskNum, pcTaskArray);
	rcPriorityAssignment = TaskSetPriorityStruct(*m_pcTaskSet);
	const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
	int iPriorityLevelStart = iTotalTaskNum - 1;
	for (set<int>::const_iterator iterAllocationIndices = rsetAllocationIndices.begin(); iterAllocationIndices != rsetAllocationIndices.end(); iterAllocationIndices++)
	{
		int iAllocationIndex = *iterAllocationIndices;		
		const set<int> & rsetPartition = m_pcTaskSet->getPartition_ByAllocation(iAllocationIndex);
	//	cout << "Next Range Start: " << iPriorityLevelStart << endl;
		for (set<int>::const_iterator iter_i = rsetPartition.begin(); iter_i != rsetPartition.end(); iter_i++)
		{
			int i = *iter_i;
			int iPriorityLevel = iPriorityLevelStart;
			for (set<int>::const_iterator iter_j = rsetPartition.begin(); iter_j != rsetPartition.end(); iter_j++)
			{				
				int j = *iter_j;
				if (i == j)	continue;
				double dValue = round(cSolver.getValue(rcPVars[i][j]));
				if (DOUBLE_EQUAL(dValue, 1.0, 1e-7))
				{
					iPriorityLevel--;
				}
			}
			rcPriorityAssignment.setPriority(i, iPriorityLevel);
			//cout << "Task " << i << " at " << iPriorityLevel << endl;
		}
		iPriorityLevelStart = iPriorityLevelStart - rsetPartition.size();
	}
	assert(rcPriorityAssignment.getSize() == iTotalTaskNum);
}

void DistributedEventDrivenFullMILP::ExtractSolutionRT(Variable1D & rcRTVars, IloCplex & rcSolver, vector<double> & rvectorResponsetimes)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	rvectorResponsetimes.clear();
	rvectorResponsetimes.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		rvectorResponsetimes.push_back(rcSolver.getValue(rcRTVars[i]));
	}
}

void DistributedEventDrivenFullMILP::ExtractSolutionWaitingTimes(Variable1D & rcRTVars, IloCplex & rcSolver, vector<double> & rvectorWaitingTimes)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	rvectorWaitingTimes.clear();
	rvectorWaitingTimes.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		rvectorWaitingTimes.push_back(rcSolver.getValue(WaitingTimeExpr(i, rcRTVars)));
	}
}

void DistributedEventDrivenFullMILP::GenPreAssignmentConst(Variable2D & rcPVars, IloRangeArray & rcConst)
{
	//The pre assigned priorities must be continuous starting from the lowest priority within each partition.
	const IloEnv & rcEnv = rcConst.getEnv();
	const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
	for (set<int>::const_iterator iterAlloc = rsetAllocationIndices.begin(); iterAlloc != rsetAllocationIndices.end(); iterAlloc++)
	{
		const set<int> & rsetPartition = m_pcTaskSet->getPartition_ByAllocation(*iterAlloc);
		for (set<int>::const_iterator iter_i = rsetPartition.begin(); iter_i != rsetPartition.end(); iter_i++)
		{
			int i = *iter_i;
			if (m_cPreAssignment.getPriorityByTask(i) == -1)	continue;
			for (set<int>::const_iterator iter_j = rsetPartition.begin(); iter_j != rsetPartition.end(); iter_j++)
			{
				int j = *iter_j;
				if (m_cPreAssignment.getPriorityByTask(j) < m_cPreAssignment.getPriorityByTask(i))
				{
					rcConst.add(IloRange(rcEnv,
						0.0,
						rcPVars[i][j],
						0.0
						));

					rcConst.add(IloRange(rcEnv,
						1.0,
						rcPVars[j][i],
						1.0
						));
				}
			}
		}
	}
}

void DistributedEventDrivenFullMILP::VerifyResult(Variable1D & rcRTVars, Variable2D & rcPVars, Variable2D & rcInterVars, Variable2D & rcPIPVars, IloCplex & rcSolver)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const set<int> & rsetAllocationIndices = m_pcTaskSet->getAllocationIndices();
	double dRoundingError = 1e-5;
	cout << "Begin verifying MILP result" << endl;
	//Verify PIP vars first
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		int iAlloInd = *iter;
		const set<int> & rsetPartitions = m_pcTaskSet->getPartition_ByAllocation(iAlloInd);
		for (set<int>::const_iterator iter_i = rsetPartitions.begin(); iter_i != rsetPartitions.end(); iter_i++)
		{
			int i = *iter_i;
			double d_w_i = rcSolver.getValue(WaitingTimeExpr(i, rcRTVars));
			if (m_pcTaskSet->getObjectType(i) == 1)
			{
				d_w_i -= pcTaskArray[i].getCLO();
			}
			for (set<int>::const_iterator iter_j = rsetPartitions.begin(); iter_j != rsetPartitions.end(); iter_j++)
			{
				int j = *iter_j;
				if (i == j)	continue;

				double dPVar_MILP = round(rcSolver.getValue(rcPVars[i][j]));
				if (dPVar_MILP == 1)
				{
					if (m_cSolPA.getPriorityByTask(i) > m_cSolPA.getPriorityByTask(j))
					{
						cout << "Priority Extracted Wrongly" << endl;
					}
				}


				int iJitterSource_j = m_pcTaskSet->getJitterSource(j);
				double dJitter_j_MILP = 0;
				if (iJitterSource_j != -1)
					dJitter_j_MILP = rcSolver.getValue(rcRTVars[iJitterSource_j]);
				double d_LBValue_Verify = (d_w_i + dJitter_j_MILP) / pcTaskArray[j].getPeriod();
				double d_InterVar_ji_Verify = ceil_tol(d_LBValue_Verify);				
				double d_InterVar_ji_MILP = rcSolver.getValue(rcInterVars[j][i]);
				if (d_InterVar_ji_Verify > d_InterVar_ji_MILP + dRoundingError)
				{
					cout << "InterVar Mismatch" << endl;
					cout << "Obj Type: " << m_pcTaskSet->getObjectType(i) << endl;
					cout << j << " over " << i << endl;
					cout << d_InterVar_ji_Verify << " " << d_InterVar_ji_MILP << endl;
					cout << "From formulation: " << rcSolver.getValue(m_mapInterVarLBExpr[j][i]) << endl;
					cout << m_vectorSolWatingTimes[i] << " " << iJitterSource_j << " " << pcTaskArray[i].getCLO() << endl;
					double d_LBValue_MILP = 0;
					if (m_pcTaskSet->getObjectType(i) == 0)
					{
						if (iJitterSource_j != -1)
							d_LBValue_MILP = rcSolver.getValue((WaitingTimeExpr(i, rcRTVars) + rcRTVars[iJitterSource_j]) / pcTaskArray[j].getPeriod());
						else
							d_LBValue_MILP = rcSolver.getValue((WaitingTimeExpr(i, rcRTVars)) / pcTaskArray[j].getPeriod());
					}
					else
					{
						if (iJitterSource_j != -1)
							d_LBValue_MILP = rcSolver.getValue((WaitingTimeExpr(i, rcRTVars) + rcRTVars[iJitterSource_j] - pcTaskArray[i].getCLO()) / pcTaskArray[j].getPeriod());
						else
							d_LBValue_MILP = rcSolver.getValue((WaitingTimeExpr(i, rcRTVars) - pcTaskArray[i].getCLO()) / pcTaskArray[j].getPeriod());
					}

					printf_s("MILP LB Value: %.10f \n", d_LBValue_MILP);
					
					printf_s("Verify LB Value: %.10f \n", d_LBValue_Verify);
					cout << (d_LBValue_MILP - round(d_LBValue_MILP)) << " " << (d_LBValue_Verify - round(d_LBValue_Verify)) << endl;
					system("pause");
				}

				double d_PIP_ji_Verify = 0;
				if (m_cSolPA.getPriorityByTask(i) > m_cSolPA.getPriorityByTask(j))
				{
					d_PIP_ji_Verify = d_InterVar_ji_Verify;
				}
				double d_PIP_ji_MILP = rcSolver.getValue(rcInterVars[j][i]);
				if (d_PIP_ji_Verify > d_PIP_ji_MILP + dRoundingError)
				{
					cout << "PIP Mismatch:" << endl;
					cout << j << " over " << i << endl;
					cout << d_PIP_ji_Verify << " " << d_PIP_ji_MILP << endl;
					system("pause");
				}

			}
		}
	}


	//Start from the waiting time;
	for (int i = 0; i < iTaskNum; i++)
	{
		//fixed point computation must first be satisfied
		double d_w_i = m_vectorSolWatingTimes[i];
		if (m_pcTaskSet->getObjectType(i) == 1)
		{
			d_w_i -= pcTaskArray[i].getCLO();
		}
		const set<int> & rsetPartitions = m_pcTaskSet->getPartition(i);
		int iObjType = m_pcTaskSet->getObjectType(i);
		double dRHS = 0;		
		if (m_vectorSolResponseTimes[i] < m_vectorSolWatingTimes[i])
		{ 
			cout << "waiting response time mismatch " << endl;
			cout << "Task " << i << " " << m_vectorSolResponseTimes[i] << m_vectorSolWatingTimes[i] << endl;
			system("pause");
		}

		if (iObjType == 0) //Task
		{
			dRHS += pcTaskArray[i].getCLO();			
			for (set<int>::const_iterator iter = rsetPartitions.begin(); iter != rsetPartitions.end(); iter++)
			{
				int j = *iter;
				if (j == i)	continue;
				if (m_cSolPA.getPriorityByTask(j) > m_cSolPA.getPriorityByTask(i)) continue;
				int iJitterSource_j = m_pcTaskSet->getJitterSource(j);
				double dBusyPeriod = d_w_i;
				if (iJitterSource_j != -1) dBusyPeriod += m_vectorSolResponseTimes[iJitterSource_j];
				dRHS += ceil_tol(dBusyPeriod / pcTaskArray[j].getPeriod()) * pcTaskArray[j].getCLO();
			}
		}
		else// Message
		{
			double dBlocking = 0;
			for (set<int>::const_iterator iter = rsetPartitions.begin(); iter != rsetPartitions.end(); iter++)
			{
				int j = *iter;
				if (j == i)	continue;
				if (m_cSolPA.getPriorityByTask(j) > m_cSolPA.getPriorityByTask(i))
				{
					dBlocking = max(dBlocking, pcTaskArray[j].getCLO());
					continue;
				}
				int iJitterSource_j = m_pcTaskSet->getJitterSource(j);
				double dBusyPeriod = d_w_i;
				if (iJitterSource_j != -1) dBusyPeriod += m_vectorSolResponseTimes[iJitterSource_j];
				dRHS += ceil_tol(dBusyPeriod / pcTaskArray[j].getPeriod()) * pcTaskArray[j].getCLO();				
			}
			dRHS += dBlocking;
		}

		if (d_w_i + dRoundingError < dRHS)
		{
			cout << "waiting time mismatch" << endl;
			cout << "Task " << i << " w: " << d_w_i << " rhs: " << dRHS << endl;
			cout << "diff: " << d_w_i - dRHS << endl;
			system("pause");
		}
	}

	cout << "Computed Reesponse Time Given MILP Response Times" << endl;
	//m_cSolPA.WriteTextFile("MILPDEDPA.txt");
	//return;
	DistributedEventDrivenResponseTimeCalculator cRTCalc(*m_pcTaskSet, m_cSolPA);
	for (int i = 0; i < iTaskNum; i++)
	{
		double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes_Approx(i, -1, m_vectorSolResponseTimes);		
		//continue;
		if (dRT > m_vectorSolResponseTimes[i] + dRoundingError)
		{			
			cout << "Mismatch detected" << endl;
			cout << "Task " << i << " " << dRT << " " << m_vectorSolResponseTimes[i] << endl;
			cout << "diff: " << dRT - m_vectorSolResponseTimes[i] << endl;
			double d_w_i = m_vectorSolWatingTimes[i];
			if (m_pcTaskSet->getObjectType(i) == 1)
			{
				d_w_i -= pcTaskArray[i].getCLO();
			}
			double d_w_i_from_rt = 0;
			const set<int> & rsetPartitions = m_pcTaskSet->getPartition(i);
			int iObjType = m_pcTaskSet->getObjectType(i);
			double dRHS = 0;	
			double d_raw_rt = 0;
			int iJitterSource_i = m_pcTaskSet->getJitterSource(i);
			double dJitter = (iJitterSource_i == -1) ? 0.0 : m_vectorSolResponseTimes[iJitterSource_i];
			if (iObjType == 0) //Task
			{
				d_w_i_from_rt = m_vectorSolResponseTimes[i] - dJitter;
				dRHS += pcTaskArray[i].getCLO();
				for (set<int>::const_iterator iter = rsetPartitions.begin(); iter != rsetPartitions.end(); iter++)
				{
					int j = *iter;
					if (j == i)	continue;
					if (m_cSolPA.getPriorityByTask(j) > m_cSolPA.getPriorityByTask(i)) continue;
					int iJitterSource_j = m_pcTaskSet->getJitterSource(j);
					double dBusyPeriod = d_w_i;
					if (iJitterSource_j != -1) dBusyPeriod += m_vectorSolResponseTimes[iJitterSource_j];
					dRHS += ceil_tol(dBusyPeriod / pcTaskArray[j].getPeriod()) * pcTaskArray[j].getCLO();
				}				
			}
			else// Message
			{
				d_w_i_from_rt = m_vectorSolResponseTimes[i] - dJitter - pcTaskArray[i].getCLO();
				double dBlocking = 0;
				for (set<int>::const_iterator iter = rsetPartitions.begin(); iter != rsetPartitions.end(); iter++)
				{
					int j = *iter;
					if (j == i)	continue;
					if (m_cSolPA.getPriorityByTask(j) > m_cSolPA.getPriorityByTask(i))
					{
						dBlocking = max(dBlocking, pcTaskArray[j].getCLO());
						continue;
					}
					int iJitterSource_j = m_pcTaskSet->getJitterSource(j);
					double dBusyPeriod = d_w_i;
					if (iJitterSource_j != -1) dBusyPeriod += m_vectorSolResponseTimes[iJitterSource_j];
					dRHS += ceil_tol(dBusyPeriod / pcTaskArray[j].getPeriod()) * pcTaskArray[j].getCLO();
				}
				dRHS += dBlocking;

			}
			cout << "waiting and rhs" << endl;
			cout << d_w_i_from_rt << " " << d_w_i << " " << dRHS << endl;
			if (d_w_i + dRoundingError < dRHS)
			{
				cout << "waiting time mismatch" << endl;
				cout << "Task " << i << " w: " << d_w_i << " rhs: " << dRHS << endl;
				cout << "diff: " << d_w_i - dRHS << endl;
				system("pause");
			}

			system("pause");
		}		
	}


	//Verify End2End latency
	const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = m_pcTaskSet->getEnd2EndPaths();
	for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
		iter != rdequePaths.end(); iter++)
	{
		const deque<int> & rdequeNodes = iter->getNodes();
		double dLatency = 0;
		for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
		{
			dLatency += m_vectorSolWatingTimes[*iterNode];
		}		
		if (dLatency > iter->getDeadline())
		{
			cout << "Latency Violated" << endl;
			system("pause");
		}
	}

}

#endif


//Case Study
void ReadDAC07CaseStudy(const char axFileName[], DistributedEventDrivenTaskSet & rcTaskSet, double dWCETScale)
{
	struct ExecutableInfo
	{
		bool bIsMessage;
		int iAllocation;
		double dComputation;
		double dPeriod;
		string stringName;
		deque<string> dequeSources;
		deque<string> dequeDestination;
		set<int> setSources;
	};
	double dScaleUp = 1000;
	ifstream ifstreamInfile(axFileName, ios::in);
	char axLineData[2048] = { 0 };
	int iAliasTasks = 0;
	int iAliasCAN = 0;
	double dEnd2EndScaled = 1;
	map<int, int> mapAllocation2AliasTask, mapAllocation2AliasCAN;
	map<int, int> mapNodeCurrentTaskIndex, mapNodeCurrentMessageIndex;
	map<string, int, StringCompObj> mapObjectName2Index;
	deque<ExecutableInfo> dequeExecutableInfos;
	deque<DistributedEventDrivenTaskSet::End2EndPath> dequePaths;
	set<int> setTasksOnPath;

	//if there is a scaling factor file
	map<int, double> mapPartitionScalingFactor;
	if (IsFileExist("PartitionScalingConfig.txt"))
	{
		//cout << "PartitionScalingConfig File Found" << endl;
		ifstream ifstreamFile("PartitionScalingConfig.txt");
		while (!ifstreamFile.eof())
		{
			int iPartition = -1;
			double dFactor = 1.0;
			ifstreamFile >> iPartition >> dFactor;
			mapPartitionScalingFactor[iPartition] = dFactor;
		}
		ifstreamFile.close();
	}

	while (!ifstreamInfile.eof())
	{
		string stringLine;
		ifstreamInfile.getline(axLineData, 2048);
		stringstream ss(axLineData);
		char xType = axLineData[0];
		if (xType == '#')	continue;
		if (xType == 'N')
		{
			string stringIsMessage;
			double dComputation = 0;
			double dPeriod = 0;
			int iAllocation = -1;
			int iPriority = -1;
			ExecutableInfo cNewExecutable;
			ss >> xType >> cNewExecutable.stringName >> stringIsMessage >> dComputation >> dPeriod >> iAllocation >> iPriority;
			cNewExecutable.bIsMessage = (stringIsMessage.compare("y") == 0);
			cNewExecutable.dPeriod = round(dPeriod * dScaleUp);
			cNewExecutable.dComputation = round(dComputation * dScaleUp);
			cNewExecutable.iAllocation = iAllocation;
			dequeExecutableInfos.push_back(cNewExecutable);
			//rcTaskSet.AddTask(dPeriod, dPeriod, dComputation / dPeriod, 1.0, 0.0);
			mapObjectName2Index[cNewExecutable.stringName] = dequeExecutableInfos.size() - 1;
		}
		else if (xType == 'C')
		{
			DistributedEventDrivenTaskSet::End2EndPath cNewPath;
			double dMaxLatency;
			ss >> xType >> dMaxLatency;			
			dMaxLatency = round(dMaxLatency * dScaleUp * dEnd2EndScaled);
			cNewPath.setDeadline(dMaxLatency);
			int iPrevNode = -1;
			while (!ss.eof())
			{
				string stringNode;
				ss >> stringNode;
				if (stringNode.empty())	break;
				if (mapObjectName2Index.count(stringNode) == 0)
				{
					cout << "Object " << stringNode.data() << " hasn't been assigned an index" << endl;
					while (1);
				}

				int iNodeIndex = mapObjectName2Index[stringNode];
				if (iPrevNode != -1)
				{
					dequeExecutableInfos[iNodeIndex].setSources.insert(iPrevNode);
				}

				iPrevNode = iNodeIndex;

				cNewPath.AddNode(iNodeIndex);
				setTasksOnPath.insert(iNodeIndex);
			}
			dequePaths.push_back(cNewPath);
		}
		else if (xType == 'E')
		{
			string stringSource, stringDestination;
			ss >> xType >> stringSource >> stringDestination;
		}
	}

	for (deque<ExecutableInfo>::iterator iter = dequeExecutableInfos.begin(); iter != dequeExecutableInfos.end(); iter++)
	{
		int iIndex = iter - dequeExecutableInfos.begin();
		double dScale = dWCETScale;
		if (mapPartitionScalingFactor.count(iter->iAllocation))
		{
			dScale = mapPartitionScalingFactor[iter->iAllocation];
		}
		
		double dNewComp = iter->dComputation * dScale;
		dNewComp = round(dNewComp);		
		if(dNewComp == 0.0)
		{
#if 0
			cout << "zero WCET encounter" << endl;
			cout << iter->dComputation << endl;
			system("pause");
#endif
			dNewComp = 1.0;
		}
		iter->dComputation = dNewComp;
	}



	for (deque<ExecutableInfo>::iterator iter = dequeExecutableInfos.begin(); iter != dequeExecutableInfos.end(); iter++)
	{
		rcTaskSet.AddTask(iter->dPeriod, iter->dPeriod, iter->dComputation / iter->dPeriod, 1.0, 0.0);
	}
	rcTaskSet.ConstructTaskArray();
	rcTaskSet.InitializeDistributedSystemData();

	//determine jitter inheritance source
	for (deque<ExecutableInfo>::iterator iter = dequeExecutableInfos.begin(); iter != dequeExecutableInfos.end(); iter++)
	{
		int iTaskIndex = iter - dequeExecutableInfos.begin();
		set<int> & rsetSources = iter->setSources;
		bool bFound = false;
		for (set<int>::iterator iterSource = rsetSources.begin(); iterSource != rsetSources.end(); iterSource++)
		{
			if (dequeExecutableInfos[*iterSource].dPeriod == dequeExecutableInfos[iTaskIndex].dPeriod)
			{
				bFound = true;
				assert(iTaskIndex != *iterSource);
				rcTaskSet.setJiterInheritanceSource(iTaskIndex, *iterSource);
				break;
			}	
		}

		if ((!bFound) && (!rsetSources.empty()))
		{
			assert(iTaskIndex != *rsetSources.begin());
			rcTaskSet.setJiterInheritanceSource(iTaskIndex, *rsetSources.begin());
		}

		rcTaskSet.setAllocation(iTaskIndex, iter->iAllocation);
		rcTaskSet.setObjectType(iTaskIndex, iter->bIsMessage);
	}

	for (deque<DistributedEventDrivenTaskSet::End2EndPath>::iterator iter = dequePaths.begin(); iter != dequePaths.end(); iter++)
	{
		rcTaskSet.AddEnd2EndPaths(*iter);
	}

	rcTaskSet.InitializeDerivedData();
}

void DAC07CaseStudy()
{
	DistributedEventDrivenTaskSet cTaskSet;
	ReadDAC07CaseStudy("CSV.txt", cTaskSet, 0.43);
	cTaskSet.WriteBasicTextFile("VehicleSystem.tskst.txt");
	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
	vector<double> vectorResponseTimes;
	vectorResponseTimes.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorResponseTimes.push_back(0);
	}
	typedef DistributedEventDrivenMUDARComputer::SchedConds SchedConds;
	typedef DistributedEventDrivenMUDARComputer::SchedCondType SchedCondType;
	SchedConds cDummy;
	TaskSetPriorityStruct cPA(cTaskSet);
	DistributedEventDrivenMUDARComputer cUnschedCoreComputer(cTaskSet);
	
	TaskSetPriorityStruct cSafePriorityAssignment(cTaskSet);
	cUnschedCoreComputer.DeriveSafePriorityAssignment(cSafePriorityAssignment);
	cout << "Accurate Test Pre-assignment: " << endl;
	cout << cSafePriorityAssignment.getSize() << " out of " << iTaskNum << " assigned" << endl;

	TaskSetPriorityStruct cSafePA_Approx(cTaskSet);
	cUnschedCoreComputer.DeriveSafePriorityAssignment_Approx(cSafePA_Approx);
	cout << "Approximate Test Pre-assignment: " << endl;
	cout << cSafePA_Approx.getSize() << " out of " << iTaskNum << " assigned" << endl;

#ifdef MCMILPPACT
	//test MILP
	if (0)
	{
		cout << currentDateTime() << endl;
		DistributedEventDrivenFullMILP cMILP(cTaskSet);
		//cMILP.setPreAssignment(cSafePA_Approx);
		int iStatus = cMILP.Solve(2);
		cout << "MILP Status: " << iStatus << endl;
		if (iStatus == 1)
		{
			TaskSetPriorityStruct cResultPA(cTaskSet);
			cResultPA = cMILP.getSolPA();
			vector<double> vectorResponseTimes;
			DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cResultPA);
			cout << "MILP Response Times" << endl;
			vector<double> vectorMILPResponseTimes = cMILP.getSolRT();
			for (int i = 0; i < iTaskNum; i++)
			{
				double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes_Approx(i, -1, vectorMILPResponseTimes);
				//cout << "Task " << i << " " << dRT << " " << vectorMILPResponseTimes[i] << endl;
			}
			int iVerifyStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTimes);
			cout << "Schedulability Verification Status: " << iVerifyStatus << endl;			
#if 0
			for (int i = 0; i < iTaskNum; i++)
			{
				
				if (vectorResponseTimes[i] > vectorMILPResponseTimes[i])
				{
					cout << "Task " << i << " RTCalc: " << vectorResponseTimes[i] << " MILP: " << vectorMILPResponseTimes[i];
					cout << " **";					
				}
				cout << endl;
			}
#endif
		}
	}

	//test MUDAR guided opt approx
	if (0)
	{
		cout << "Start MUDAR guided opt" << endl;
		system("pause");
		DistributedEventDrivenMUDARGuidedOpt_Approx cMUDARGuided(cTaskSet);
		cMUDARGuided.setCorePerIter(5);
		//cMUDARGuided.setSubILPDisplay(2);
		cMUDARGuided.setLogFile("MUDAR_Vehicle_System_Log.txt");
		int iStatus = cMUDARGuided.FocusRefinement(1);		
		if (iStatus == 1)
		{
			//Verify Solution
			TaskSetPriorityStruct cSolPA = cMUDARGuided.getSolPA();
			cSolPA.WriteTextFile("VehicleSystem_MUDAR_PA.txt");
			DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cSolPA);
			vector<double> vectorResponseTimes;
			int iVerifyStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTimes);
			cout << "Verify Status: " << iVerifyStatus << endl;
			const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = cTaskSet.getEnd2EndPaths();
			for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
				iter != rdequePaths.end(); iter++)
			{
				double dLatency = 0;
				const deque<int> & rdequeNodes = iter->getNodes();
				for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
				{
					dLatency += cRTCalc.getWaitingTime(*iterNode, vectorResponseTimes);
				}
				if (dLatency > iter->getDeadline())
				{
					cout << "Latency Deadline Violated" << endl;
					system("pause");
				}
			}

			//Verify using MILP
			{
				cout << "Verify using MILP" << endl;
				DistributedEventDrivenFullMILP cMILP(cTaskSet);
				vector<double> vectorMILPResponseTime;
				TaskSetPriorityStruct cPAtoVerify(cTaskSet);
				cPAtoVerify.ReadTextFile("VehicleSystem_MUDAR_PA.txt");
				int iMILPVerifyStatus = cMILP.VerifyPA(cPAtoVerify, vectorMILPResponseTime, 0);
				cout << "MILP Verify Status: " << iMILPVerifyStatus << endl;
			}

			//The priority assignment should pass accurate analysis
			{
				DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cSolPA);
				vector<double> vectorResponseTimeAccurateAnalysis;
				int iStatus = cRTCalc.ComputeAllResponseTimeExact(vectorResponseTimeAccurateAnalysis);
				cout << "Accurate Analysis Status: " << iStatus << endl;
				for (int i = 0; i < iTaskNum; i++)
				{
					if (!DOUBLE_EQUAL(vectorResponseTimes[i], vectorResponseTimeAccurateAnalysis[i], 1e-5))
					{
						cout << "Mismatch between accurate and approximate analysis: " << endl;
						cout << "Task " << i << " " << vectorResponseTimeAccurateAnalysis[i] << " " << vectorResponseTimes[i] << endl;
					}
				}

				DistributedEventDrivenMUDARComputer cMUDARCompAccurate(cTaskSet);
				DistributedEventDrivenMUDARComputer::SchedConds cDummy;
				DistributedEventDrivenMUDARComputer::SchedConds cSol = (DistributedEventDrivenMUDARComputer::SchedConds)cMUDARGuided.getOptimalSchedCondConfig();
				cout << "SchedCondFig Accurate Test: " << 
				cMUDARCompAccurate.IsSchedulable(cSol, cDummy) << endl;
			}
		}
	}

	//MUDAR Guided Opt Accurate Analysis
	if (1)
	{
		cout << "Start MUDAR Accurate guided opt" << endl;
		system("pause");
		DistributedEventDrivenMUDARGuidedOpt cMUDARGuided(cTaskSet);
#if 0
		TaskSetPriorityStruct cSafePA(cTaskSet);
		cMUDARGuided.getUnschedCoreComputer().DeriveSafePriorityAssignment(cSafePA);
		cMUDARGuided.getUnschedCoreComputer().setPreAssignment(cSafePA);
#endif
		cMUDARGuided.setCorePerIter(5);
		//cMUDARGuided.setSubILPDisplay(2);
		
		cMUDARGuided.setLogFile("MUDAR_Accurate_Vehicle_System_Log.txt");
		int iStatus = cMUDARGuided.FocusRefinement(1);
		if (iStatus == 1)
		{
			//Verify Solution
			TaskSetPriorityStruct cSolPA = cMUDARGuided.getSolPA();
			cSolPA.WriteTextFile("VehicleSystem_MUDAR_Accurate_PA.txt");
			DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cSolPA);
			vector<double> vectorResponseTimes;
			int iVerifyStatus = cRTCalc.ComputeAllResponseTimeExact(vectorResponseTimes);
			cout << "Verify Status: " << iVerifyStatus << endl;
			const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = cTaskSet.getEnd2EndPaths();
			for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
				iter != rdequePaths.end(); iter++)
			{
				double dLatency = 0;
				const deque<int> & rdequeNodes = iter->getNodes();
				for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
				{
					dLatency += cRTCalc.getWaitingTime(*iterNode, vectorResponseTimes);
				}
				if (dLatency > iter->getDeadline())
				{
					cout << "Latency Deadline Violated" << endl;
					system("pause");
				}
			}
		}
	}
#endif

}

void DAC07CaseStudy_WaitingTimeBasedFR()
{
	DistributedEventDrivenTaskSet cTaskSet; 
#if 1
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	double dScale = 1.0;
	cout << "WCET Scale: " << dScale << endl;
	ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
	cTaskSet.WriteBasicTextFile("VehicleSystem.tskst.txt");
#else
	cTaskSet.ReadImageFile("JitterPA_DAC07CaseStudy_MaxSchedUtil_Approx.tskst");	
	cTaskSet.InitializeDerivedData();
#endif
	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
	typedef DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::SchedConds SchedConds;
	typedef DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::SchedCondType SchedCondType;
	SchedConds cDummy;
	DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx cWTMUDARComp_Approx(cTaskSet);
	cout << cWTMUDARComp_Approx.IsSchedulable(cDummy, cDummy) << endl;
	bool bUseMaxMinLaxityObj = true;
#ifdef MCMILPPACT	
	//test MUDAR waitingtime based guided opt approx
	if (1)
	{
		cout << "Start MUDAR guided opt approx" << endl;
		system("pause");
		DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cTaskSet);
		cMUDARGuided.setCorePerIter(5);
		//cMUDARGuided.setSubILPDisplay(2);
		cMUDARGuided.setLogFile("MUDAR_WTB_Vehicle_System_Log.txt");
		if (bUseMaxMinLaxityObj)
		{
			cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			//cMUDARGuided.setObjective(cMUDARGuided.MinMaxNormalizedLaxity);
		}
		int iStatus = cMUDARGuided.FocusRefinement(1);
		if (iStatus == 1)
		{
			TaskSetPriorityStruct cSolPA = cMUDARGuided.getSolPA();
			cSolPA.WriteTextFile("VehicleSystem_MUDAR_WTB_PA.txt");
			cMUDARGuided.GenerateStatisticFile("VehicleSystem_MUDAR_WTB_Stat.txt");
			DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cSolPA);
			//Verify based on given waiting time
			SchedConds cSchedCondConfig = cMUDARGuided.getOptimalSchedCondConfig();
			vector<double> vectorWaitingTime;
			vector<double> vectorDerivedResponseTime;
			for (int i = 0; i < iTaskNum; i++)
			{
				if (cSchedCondConfig.count(SchedCondType(i, SchedCondType::UB)))
				{
					vectorWaitingTime.push_back(cSchedCondConfig[SchedCondType(i, SchedCondType::UB)]);
				}
				else
				{
					vectorWaitingTime.push_back(pcTaskArray[i].getPeriod());
				}
			}
			cMUDARGuided.getUnschedCoreComputer().ConstructResponseTimeFromWaitingTime(vectorWaitingTime, vectorDerivedResponseTime);
			cout << vectorDerivedResponseTime.size() << endl;
			for (int i = 0; i < iTaskNum; i++)
			{
				double dWaitingTime = cRTCalc.ComputeWaitingTime_SpecifiedResponseTimes_Approx(i, -1, vectorDerivedResponseTime);
				if (!(dWaitingTime <= vectorWaitingTime[i]))
				{
					cout << "Waiting Time Failed" << endl;
					cout << "Task " << i << ": " << dWaitingTime << " " << vectorWaitingTime[i] << endl;
				}								
			}

			//Verify Solution					
			vector<double> vectorResponseTimes;
			int iVerifyStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTimes);
			cout << "Verify Status: " << iVerifyStatus << endl;
			const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = cTaskSet.getEnd2EndPaths();
			for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
				iter != rdequePaths.end(); iter++)
			{
				double dLatency = 0;
				const deque<int> & rdequeNodes = iter->getNodes();
				for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
				{
					dLatency += cRTCalc.getWaitingTime(*iterNode, vectorResponseTimes);
				}
				if (dLatency > iter->getDeadline())
				{
					cout << "Latency Deadline Violated" << endl;
					system("pause");
				}
			}

			//Verify using MILP
			{
				cout << "Verify using MILP" << endl;
				DistributedEventDrivenFullMILP cMILP(cTaskSet);
				vector<double> vectorMILPResponseTime;
				TaskSetPriorityStruct cPAtoVerify(cTaskSet);
				cPAtoVerify.ReadTextFile("VehicleSystem_MUDAR_WTB_PA.txt");
				int iMILPVerifyStatus = cMILP.VerifyPA(cPAtoVerify, vectorMILPResponseTime, 0);
				cout << "MILP Verify Status: " << iMILPVerifyStatus << endl;
			}

			//Should also be acceptable to accurate test
			{
				DistributedEventDrivenMUDARComputer_WaitingTimeBased cAccurateMUDARComp(cTaskSet);
				SchedConds cDummy;
				cout << "Accurate Test Status: " << cAccurateMUDARComp.IsSchedulable(cSchedCondConfig, cDummy) << endl;
			}
		}
	}
	//test MDUAR waiting time based guided opt accurate
	if (1)
	{
		cout << "Start MUDAR guided opt accurate " << endl;
		DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased<> cMUDARGuided(cTaskSet);
		cMUDARGuided.setCorePerIter(5);
		//cMUDARGuided.setSubILPDisplay(2);
		cMUDARGuided.setLogFile("MUDAR_WTB_Accurate_Vehicle_System_Log.txt");
		if (bUseMaxMinLaxityObj)
		{
			cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			//cMUDARGuided.setObjective(cMUDARGuided.MinMaxNormalizedLaxity);
		}
		int iStatus = cMUDARGuided.FocusRefinement(1);
		if (iStatus == 1)
		{
			TaskSetPriorityStruct cSolPA = cMUDARGuided.getSolPA();
			cSolPA.WriteTextFile("VehicleSystem_MUDAR_WTB_Accurate_PA.txt");
			cMUDARGuided.GenerateStatisticFile("VehicleSystem_MUDAR_WTB_Accurate_Stat.txt");
			DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cSolPA);
			//Verify based on given waiting time
			SchedConds cSchedCondConfig = cMUDARGuided.getOptimalSchedCondConfig();
			SchedConds cDummy;
			DistributedEventDrivenMUDARComputer_WaitingTimeBased cMUDAComp(cTaskSet);
			cout << cMUDAComp.IsSchedulable(cSchedCondConfig, cDummy) << endl;
			vector<double> vectorWaitingTime;
			vector<double> vectorDerivedResponseTime;
			for (int i = 0; i < iTaskNum; i++)
			{
				if (cSchedCondConfig.count(SchedCondType(i, SchedCondType::UB)))
				{
					vectorWaitingTime.push_back(cSchedCondConfig[SchedCondType(i, SchedCondType::UB)]);
				}
				else
				{
					vectorWaitingTime.push_back(cTaskSet.getPartitionHyperperiod_ByTask(i));
				}
				//cout << "Task " << i << " " << vectorWaitingTime[i] << endl;
			}
			cMUDARGuided.getUnschedCoreComputer().ConstructResponseTimeFromWaitingTime(vectorWaitingTime, vectorDerivedResponseTime);
			cout << vectorDerivedResponseTime.size() << endl;
			for (int i = 0; i < iTaskNum; i++)
			{
				double dWaitingTime = cRTCalc.ComputeWaitingTime_SpecifiedResponseTimes(i, -1, vectorDerivedResponseTime);
				if (!(dWaitingTime <= vectorWaitingTime[i]))
				{
					cout << "Waiting Time Failed" << endl;
					cout << "Task " << i << ": " << dWaitingTime << " " << vectorWaitingTime[i] << endl;
				}
			}

			//Verify Solution					
			vector<double> vectorResponseTimes;
			int iVerifyStatus = cRTCalc.ComputeAllResponseTimeExact(vectorResponseTimes);
			cout << "Verify Status: " << iVerifyStatus << endl;
			const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = cTaskSet.getEnd2EndPaths();
			for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
				iter != rdequePaths.end(); iter++)
			{
				double dLatency = 0;
				const deque<int> & rdequeNodes = iter->getNodes();
				for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
				{
					dLatency += cRTCalc.getWaitingTime(*iterNode, vectorResponseTimes);
				}
				if (dLatency > iter->getDeadline())
				{
					cout << "Latency Deadline Violated" << endl;
					system("pause");
				}
			}
		
		}
	}


	//MILP
	if (1)
	{
		cout << currentDateTime() << endl;
		DistributedEventDrivenFullMILP cMILP(cTaskSet);
		if (bUseMaxMinLaxityObj)
		{
			cMILP.setObjective(DistributedEventDrivenFullMILP::MaxMinEnd2EndLaxity);
		}
		int iStatus = cMILP.Solve(2);
		cout << "MILP Status: " << iStatus << endl;
		if (iStatus == 1)
		{
			TaskSetPriorityStruct cResultPA(cTaskSet);
			cResultPA = cMILP.getSolPA();
			vector<double> vectorResponseTimes;
			DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cResultPA);
			cout << "MILP Response Times" << endl;
			vector<double> vectorMILPResponseTimes = cMILP.getSolRT();
			for (int i = 0; i < iTaskNum; i++)
			{
				double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes_Approx(i, -1, vectorMILPResponseTimes);
				//cout << "Task " << i << " " << dRT << " " << vectorMILPResponseTimes[i] << endl;
			}
			int iVerifyStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTimes);
			cout << "Schedulability Verification Status: " << iVerifyStatus << endl;
		}
	}
#endif
}

void DAC07CaseStudy_MaxSchedulableUtil(int argc, char * argv[])
{
	double dResolution = (1.0) / 20;
	int iStartingPartition = -1;
	int iObjective = 0;
	string stringLoadFile;
	//assume to be called by DummyTemp
	if (argc >= 3)
	{
		dResolution = atof(argv[2]);
	}

	if (argc >= 4)
	{
		iStartingPartition = atoi(argv[3]);
	}

	if (argc >= 5)
	{
		iObjective = atoi(argv[4]);
	}

	if (argc >= 6)
	{
		stringLoadFile = argv[5];
	}
	cout << "Resolution: " << dResolution << endl;
	cout << "Starting Partition: " << iStartingPartition << endl;
	

	DistributedEventDrivenTaskSet cTaskSet;
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	char axBuffer[512];
	if (stringLoadFile.empty())
	{
		double dScale = 1.0;
		cout << "WCET Scale: " << dScale << endl;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
	}
	else
	{
		cTaskSet.ReadImageFile(argv[4]);
		cTaskSet.InitializeDerivedData();
	}
	sprintf_s(axBuffer, "JitterPA_DAC07CaseStudy_MaxSchedUtil_Approx_%.2f.tskst", dResolution);
	cout << currentDateTime().data() << endl;
	int iTime = time(NULL);
	const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		//Use Binary Search to find out the maximal util schedulable
		if (*iter < iStartingPartition)	continue;
		int iPartition = *iter;		
		double dLB = cTaskSet.getPartitionUtilization_ByPartition(iPartition);		
		double dUB = 1.0;
		
		double dOriginal = dLB;
		while (true)
		{
			double dCurrent = (dLB + dUB) / 2;
			double dScaled = cTaskSet.ScalePartitionUtil(iPartition, dCurrent);
			if ((dScaled >= dUB) || (dScaled <= dLB))
			{
				cout << "rounding quit" << endl;
				break;
			}
			dCurrent = dScaled;
			cout << "-------------------------------------" << endl;
			cout << "Partition: " << iPartition << endl;
			cout << "Obj Type: " << cTaskSet.getObjectType(*(cTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
			cout << "Current Utilization Level: " << dCurrent << endl;
			cout << "Util Range: " << dLB << " ~ " << dUB << endl;
#ifdef MCMILPPACT
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cTaskSet);
			cMUDARGuided.setCorePerIter(5);
			if (iObjective)
			{								
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			}
			int iStatus = cMUDARGuided.FocusRefinement(1);
#else
			int iStatus = 1;
#endif
			if (iStatus == 1)
			{
				dLB = dCurrent;	
				cTaskSet.WriteImageFile(axBuffer);
			}
			else
			{
				dUB = dCurrent;
			}

			if ((dUB - dLB) <= dResolution)
			{
				cout << "Resolution quit" << endl;
				cout << cTaskSet.ScalePartitionUtil(iPartition, dLB) << endl;
				cout << "***************" << endl;
				cout << "Partition " << iPartition << " Final Utilization: " << dLB << " From " << dOriginal << endl;				
				cout << "***************" << endl;
				break;
			}
		}
	}
	iTime = time(NULL) - iTime;
	cout << "Total Time: " << iTime << "s" << endl;
}

void DAC07CaseStudy_MaxSchedulableUtil_v2()
{
	//The key is to maintain integrality of pararmeters
	DistributedEventDrivenTaskSet cTaskSet;
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
#if 0
	double dScale = 1.0;
	cout << "WCET Scale: " << dScale << endl;
	ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
#else
	cTaskSet.ReadImageFile("JitterPA_DAC07CaseStudy_MaxSchedUtil_Approx.tskst");
	cTaskSet.InitializeDerivedData();
	cTaskSet.DisplayTaskInfo();
	system("pause");
#endif
	const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
	typedef DistributedEventDrivenMUDARComputer::SchedCondType SchedCondType;
	DistributedEventDrivenMUDARComputer cUnschedCoreComp(cTaskSet);
	cout << cUnschedCoreComp.SchedCondLowerBound(SchedCondType(0, SchedCondType::LB)) << endl;
	typedef DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::SchedConds SchedConds;
	SchedConds cLastFeaSchedCond;
	DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::UnschedCores cLastFeaUnschedCores;	
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		//Use Binary Search to find out the maximal util schedulable
		int iPartition = *iter;
		double dOriginal = cTaskSet.getPartitionUtilization_ByPartition(iPartition);
		int iUB = floor(1.0 / dOriginal);
		int iLB = 1;			
		double dUtilResolution = 0.05;

		while (true)
		{
			int iCurrent = (iLB + iUB) / 2;
			double dNewUtil = iCurrent * dOriginal;
			double dScaled = cTaskSet.ScalePartitionUtil(iPartition, dNewUtil);
			if (DOUBLE_EQUAL(dNewUtil, dScaled, 1e-7) == false)
			{
				cout << "Possible integrality error occurs" << endl;
				cout << dNewUtil << " " << dScaled << endl;
				while (1);
			}
			cout << "-------------------------------------" << endl;
			cout << "Partition: " << iPartition << endl;
			cout << "Obj Type: " << cTaskSet.getObjectType(*(cTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
			cout << "Current Utilization Level: " << dNewUtil << endl;
			cout << "Util Range: " << iLB * dOriginal << " ~ " << iUB * dOriginal << endl;
#ifdef MCMILPPACT
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cTaskSet);						
			cMUDARGuided.setCorePerIter(5);
			if (1)
			{
				cMUDARGuided.setReferenceSchedConds(cLastFeaSchedCond);
				//cMUDARGuided.LoadUnschedCores(cLastFeaUnschedCores);
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			}	

			int iStatus = cMUDARGuided.FocusRefinement(1);
#else
			int iStatus = 1;
#endif
			if (iStatus == 1)
			{
				iLB = iCurrent;
				cTaskSet.WriteImageFile("JitterPA_DAC07CaseStudy_MaxSchedUtil_Approx.tskst");
#ifdef MCMILPPACT
				cLastFeaSchedCond = cMUDARGuided.getOptimalSchedCondConfig();
				cLastFeaUnschedCores = cMUDARGuided.getUnschedCores();
#endif
			}
			else
			{
				iUB = iCurrent;
			}

			if (((iUB - iLB) <= 1) || (((iUB - iLB) * dOriginal) <= dUtilResolution))
			{
				cTaskSet.ScalePartitionUtil(iPartition, iLB * dOriginal);
				cout << "***************" << endl;
				cout << "Partition " << iPartition << " Final Utilization: " << iLB * dOriginal << " From " << dOriginal << endl;
				cout << "***************" << endl;
				break;
			}


		}
	}

}

void DAC07CaseStudy_MaxSchedulableUtil_Accurate(int argc, char * argv[])
{
	double dResolution = (1.0) / 20;
	int iStartingPartition = -1;
	int iObjective = 0;
	string stringLoadFile;
	//assume to be called by DummyTemp
	if (argc >= 3)
	{
		dResolution = atof(argv[2]);
	}

	if (argc >= 4)
	{
		iStartingPartition = atoi(argv[3]);
	}

	if (argc >= 5)
	{
		iObjective = atoi(argv[4]);
	}

	if (argc >= 6)
	{
		stringLoadFile = argv[5];
	}
	cout << "Resolution: " << dResolution << endl;
	cout << "Starting Partition: " << iStartingPartition << endl;


	DistributedEventDrivenTaskSet cTaskSet;
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	char axBuffer[512];
	if (stringLoadFile.empty())
	{
		double dScale = 1.0;
		cout << "WCET Scale: " << dScale << endl;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
	}
	else
	{
		cTaskSet.ReadImageFile(argv[5]);
		cTaskSet.InitializeDerivedData();
	}
	sprintf_s(axBuffer, "JitterPA_DAC07CaseStudy_MaxSchedUtil_Accurate_%.2f.tskst", dResolution);
	cout << currentDateTime().data() << endl;
	int iTime = time(NULL);
	const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		//Use Binary Search to find out the maximal util schedulable
		if (*iter < iStartingPartition)	continue;
		int iPartition = *iter;
		double dLB = cTaskSet.getPartitionUtilization_ByPartition(iPartition);
		double dUB = 1.0;

		double dOriginal = dLB;
		while (true)
		{
			double dCurrent = (dLB + dUB) / 2;		
			cout << "-------------------------------------" << endl;
			cout << "Partition: " << iPartition << endl;
			cout << "Obj Type: " << cTaskSet.getObjectType(*(cTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
			cout << "Current Utilization Level: " << dCurrent << endl;
			cout << "Util Range: " << dLB << " ~ " << dUB << endl;

			double dScaled = cTaskSet.ScalePartitionUtil(iPartition, dCurrent);			
			dCurrent = dScaled;

			if (((dUB - dLB) <= dResolution) || ((dScaled >= dUB) || (dScaled <= dLB)))
			{
				cout << "Resolution quit" << endl;
				cout << cTaskSet.ScalePartitionUtil(iPartition, dLB) << endl;
				cout << "***************" << endl;
				cout << "Partition " << iPartition << " Final Utilization: " << dLB << " From " << dOriginal << endl;
				cout << "***************" << endl;
				break;
			}

#ifdef MCMILPPACT
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased<> cMUDARGuided(cTaskSet);
			cMUDARGuided.setCorePerIter(5);
			if (iObjective)
			{
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			}
			int iStatus = cMUDARGuided.FocusRefinement(1);
#else
			int iStatus = 1;
#endif
			if (iStatus == 1)
			{
				dLB = dCurrent;
				cTaskSet.WriteImageFile(axBuffer);
			}
			else
			{
				dUB = dCurrent;
			}		
		}
}
	iTime = time(NULL) - iTime;
	cout << "Total Time: " << iTime << "s" << endl;
}

void DAC07CaseStudy_MaxSchedulableUtil_Accurate_v2()
{
	//The key is to maintain integrality of pararmeters
	DistributedEventDrivenTaskSet cTaskSet;
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	double dScale = 0.3;
	cout << "WCET Scale: " << dScale << endl;
	ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
	const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
	typedef DistributedEventDrivenMUDARComputer::SchedCondType SchedCondType;
	DistributedEventDrivenMUDARComputer cUnschedCoreComp(cTaskSet);
	cout << cUnschedCoreComp.SchedCondLowerBound(SchedCondType(0, SchedCondType::LB)) << endl;
	typedef DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::SchedConds SchedConds;
	SchedConds cLastFeaSchedCond;
	DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx::UnschedCores cLastFeaUnschedCores;
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		//Use Binary Search to find out the maximal util schedulable
		int iPartition = *iter;
		double dOriginal = cTaskSet.getPartitionUtilization_ByPartition(iPartition);
		int iUB = floor(1.0 / dOriginal);
		int iLB = 1;
		double dUtilResolution = 0.40;

		while (true)
		{
			int iCurrent = (iLB + iUB) / 2;
			double dNewUtil = iCurrent * dOriginal;
			double dScaled = cTaskSet.ScalePartitionUtil(iPartition, dNewUtil);
			if (DOUBLE_EQUAL(dNewUtil, dScaled, 1e-7) == false)
			{
				cout << "Possible integrality error occurs" << endl;
				cout << dNewUtil << " " << dScaled << endl;
				while (1);
			}
			cout << "-------------------------------------" << endl;
			cout << "Partition: " << iPartition << endl;
			cout << "Obj Type: " << cTaskSet.getObjectType(*(cTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
			cout << "Current Utilization Level: " << dNewUtil << endl;
			cout << "Util Range: " << iLB * dOriginal << " ~ " << iUB * dOriginal << endl;
#ifdef MCMILPPACT
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cTaskSet);
			cMUDARGuided.setCorePerIter(5);
			if (1)
			{
				cMUDARGuided.setReferenceSchedConds(cLastFeaSchedCond);
				//cMUDARGuided.LoadUnschedCores(cLastFeaUnschedCores);
				cMUDARGuided.setObjective(cMUDARGuided.ReferenceGuided);
			}

			int iStatus = cMUDARGuided.FocusRefinement(1);
#else
			int iStatus = 1;
#endif
			if (iStatus == 1)
			{
				iLB = iCurrent;
				cTaskSet.WriteImageFile("JitterPA_DAC07CaseStudy_MaxSchedUtil_Approx.tskst");
#ifdef MCMILPPACT
				cLastFeaSchedCond = cMUDARGuided.getOptimalSchedCondConfig();
				cLastFeaUnschedCores = cMUDARGuided.getUnschedCores();
#endif
			}
			else
			{
				iUB = iCurrent;
			}

			if (((iUB - iLB) <= 1) || (((iUB - iLB) * dOriginal) <= dUtilResolution))
			{
				cTaskSet.ScalePartitionUtil(iPartition, iLB * dOriginal);
				cout << "***************" << endl;
				cout << "Partition " << iPartition << " Final Utilization: " << iLB * dOriginal << " From " << dOriginal << endl;
				cout << "***************" << endl;
				break;
			}


		}
	}

}

void DisplayMaxSchedUtilResult()
{
	DistributedEventDrivenTaskSet cOriginal, cTaskSetAccurate, cTaskSetApprox;
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	ReadDAC07CaseStudy("CSV.txt", cOriginal, 1.0);
	cTaskSetAccurate.ReadImageFile("JitterPA_DAC07CaseStudy_MaxSchedUtil_Accurate_0.03.tskst");
	cTaskSetAccurate.InitializeDerivedData();
	cTaskSetApprox.ReadImageFile("JitterPA_DAC07CaseStudy_MaxSchedUtil_Approx_0.03.tskst");
	cTaskSetApprox.InitializeDerivedData();
	const set<int> & rsetAllocationIndices = cTaskSetAccurate.getAllocationIndices();
	vector<double> vectorApprox, vectorAccurate, vectorOriginal;
	vector<int> vectorECUIndex;
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		cout << "**************************" << endl;
		cout << "ECU " << *iter << endl;
		cout << "Approx: " << cTaskSetApprox.getPartitionUtilization_ByPartition(*iter) << endl;
		cout << "Accurate: " << cTaskSetAccurate.getPartitionUtilization_ByPartition(*iter) << endl;
		vectorApprox.push_back(cTaskSetApprox.getPartitionUtilization_ByPartition(*iter));
		vectorAccurate.push_back(cTaskSetAccurate.getPartitionUtilization_ByPartition(*iter));
		vectorECUIndex.push_back(*iter);
		vectorOriginal.push_back(cOriginal.getPartitionUtilization_ByPartition(*iter));
	}
	int iStretch = vectorAccurate.size() / 2;
	for (int i = 0; i < floor(vectorAccurate.size() / 2); i++)
	{		

		if (vectorApprox[i] + 0.01 < vectorAccurate[i])
		{
			printf_s("%d\t & %.3f\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t",
				vectorECUIndex[i], vectorOriginal[i], vectorApprox[i], vectorAccurate[i]		
				);
		}
		else
		{
			printf_s("%d\t & %.3f\t & %.3f\t & %.3f\t",
				vectorECUIndex[i], vectorOriginal[i], vectorApprox[i], vectorAccurate[i]
				);
		}

		if (vectorApprox[i + iStretch] + 0.01 < vectorAccurate[i + iStretch])
		{
			printf_s("& %d\t & %.3f\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t \\\\\\hline \n",
				vectorECUIndex[i + iStretch], vectorOriginal[i + iStretch], vectorApprox[i + iStretch], vectorAccurate[i + iStretch]
				);
		}
		else
		{
			printf_s("& %d\t & %.3f\t & %.3f\t & %.3f \\\\\\hline \n",				
				vectorECUIndex[i + iStretch], vectorOriginal[i+iStretch], vectorApprox[i + iStretch], vectorAccurate[i + iStretch]
				);
		}
	}
	if (vectorAccurate.size() % 2 == 0)	return;
	int i = vectorAccurate.size() - 1;
	if (vectorApprox[i] + 0.01 < vectorAccurate[i])
	{
		printf_s("%d\t & %.3f\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t",
			vectorECUIndex[i], vectorOriginal[i], vectorApprox[i], vectorAccurate[i]
			);
	}
	else
	{
		printf_s("%d\t & %.3f\t & %.3f\t & %.3f\t",
			vectorECUIndex[i], vectorOriginal[i], vectorApprox[i], vectorAccurate[i]
			);
	}

	printf_s("&\t & \t &\\\\\\hline \n");
}

void TestMILPBreakDownUtilization()
{
	DistributedEventDrivenTaskSet cTaskSet;
#if 1
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	double dScale = 1.0;
	cout << "WCET Scale: " << dScale << endl;
	ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
	cTaskSet.WriteBasicTextFile("VehicleSystem.tskst.txt");
#else
	cTaskSet.ReadImageFile("JitterPA_DAC07CaseStudy_MaxSchedUtil_Approx.tskst");
	cTaskSet.InitializeDerivedData();
#endif
#ifdef MCMILPPACT
	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
	if (1)
	{
		cout << currentDateTime() << endl;
		DistributedEventDrivenFullMILP cMILP(cTaskSet);		
		cMILP.setObjective(DistributedEventDrivenFullMILP::None);
		int iStatus = cMILP.SolveBreakDownUtilization(0,2);
		cout << "MILP Status: " << iStatus << endl;
		if (iStatus == 1)
		{
			TaskSetPriorityStruct cResultPA(cTaskSet);
			cResultPA = cMILP.getSolPA();
			vector<double> vectorResponseTimes;
			DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cResultPA);
			cout << "MILP Response Times" << endl;
			vector<double> vectorMILPResponseTimes = cMILP.getSolRT();
			for (int i = 0; i < iTaskNum; i++)
			{
				double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes_Approx(i, -1, vectorMILPResponseTimes);
				//cout << "Task " << i << " " << dRT << " " << vectorMILPResponseTimes[i] << endl;
			}
			int iVerifyStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTimes);
			cout << "Schedulability Verification Status: " << iVerifyStatus << endl;
		}
	}
#endif
}

void TestBinaryInteger()
{
#ifdef MCMILPPACT
	IloEnv cSolveEnv;
	BinaryArrayInteger cBinaryInteger(cSolveEnv, 2, 1000);
	IloNumVar cVar(cSolveEnv, 2, 1000, IloNumVar::Int);
	IloRangeArray cConst(cSolveEnv);
	IloExpr cExpr = cBinaryInteger.getLinearizeProduct(cSolveEnv, cVar, 1000, cConst);
	cBinaryInteger.GenBoundConst(cConst);

	IloModel cModel(cSolveEnv);
	cModel.add(cConst);
	cModel.add(cExpr == 8);
	cout << cModel << endl;
	IloCplex cSolver(cModel);
	bool bStatus = cSolver.solve();
	if (bStatus)
	{
		cout << "Feasible" << endl;
		cout << "Binary Integer: " << cBinaryInteger.getValue(cSolver) << endl;
		cout << "Var: " << cSolver.getValue(cVar) << endl;
	}
	else
	{
		cout << "Infeasible" << endl;
	}
#endif
}

#ifdef MCMILPPACT

void DAC07CaseStudy_BreakDownUtilization_Approx(int argc, char * argv[])
{

	double dResolution = 0.03;
	int iStartingPartition = -1;
	int iObjective = 0;
	string stringLoadFile;
	//assume to be called by DummyTemp
	if (argc >= 3)
	{
		dResolution = atof(argv[2]);
	}

	if (argc >= 4)
	{
		iStartingPartition = atoi(argv[3]);
	}

	if (argc >= 5)
	{
		iObjective = atoi(argv[4]);
	}

	if (argc >= 6)
	{
		stringLoadFile = argv[5];
	}
	cout << "Resolution: " << dResolution << endl;
	cout << "Starting Partition: " << iStartingPartition << endl;


	DistributedEventDrivenTaskSet cTaskSet;
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	char axBuffer[512];
	if (stringLoadFile.empty())
	{
		double dScale = 1.0;
		cout << "WCET Scale: " << dScale << endl;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
	}
	else
	{
		cTaskSet.ReadImageFile(argv[4]);
		cTaskSet.InitializeDerivedData();
	}	
	cout << currentDateTime().data() << endl;
	int iTime = time(NULL);
	const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
	StatisticSet cResult;
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		DistributedEventDrivenTaskSet cThisTaskSet;		

		if (stringLoadFile.empty())
		{
			double dScale = 1.0;
			cout << "WCET Scale: " << dScale << endl;
			ReadDAC07CaseStudy("CSV.txt", cThisTaskSet, dScale);
		}
		else
		{
			cThisTaskSet.ReadImageFile(argv[4]);
			cThisTaskSet.InitializeDerivedData();
		}
		time_t iThisTotalTime = time(NULL);
		IloEnv cTimerEnv;
		IloCplex cTimer(cTimerEnv);
		double dThisTotalTime = cTimer.getCplexTime();
		int iPartition = *iter;
		double dLB = cThisTaskSet.getPartitionUtilization_ByPartition(*iter);
		double dUB = 1.0;
		double dOriginal = dLB;

		cout << "--------------------------------" << endl;
		cout << "Partition: " << *iter << endl;
		cout << "Obj Type: " << cTaskSet.getObjectType(*(cThisTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
		cout << "Test extreme 1.0: " << endl;
		double dActual = cThisTaskSet.ScalePartitionUtil(*iter, 1.0);
#ifdef MCMILPPACT
		DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cThisTaskSet);
		cMUDARGuided.setCorePerIter(5);
		if (iObjective)
		{
			cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
		}
		int iStatus = cMUDARGuided.FocusRefinement(1);
#else
		int iStatus = 1;
#endif
		if (iStatus == 1)
		{
			dLB = dActual;
		}
		else
		{
			cThisTaskSet.ScalePartitionUtil(iPartition, dOriginal);
		}

		sprintf_s(axBuffer, "%d", *iter);
		std::string stringIndex(axBuffer);
		cResult.setItem((char *)stringIndex.data(), dLB);
		cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Aprox.txt");
		cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Aprox.rslt");
		while (true)
		{
			double dCurrent = (dLB + dUB) / 2;
			double dScaled = cThisTaskSet.ScalePartitionUtil(iPartition, dCurrent);
			if ((dScaled >= dUB) || (dScaled <= dLB))
			{
				cout << "rounding quit" << endl;
				break;
			}
			dCurrent = dScaled;
			cout << "-------------------------------------" << endl;
			cout << "Partition: " << iPartition << endl;
			cout << "Obj Type: " << cTaskSet.getObjectType(*(cThisTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
			cout << "Current Utilization Level: " << dCurrent << endl;
			cout << "Util Range: " << dLB << " ~ " << dUB << endl;
#ifdef MCMILPPACT
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cThisTaskSet);
			cMUDARGuided.setCorePerIter(5);
			if (iObjective)
			{
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			}
			int iStatus = cMUDARGuided.FocusRefinement(1);
#else
			int iStatus = 1;
#endif
			if (iStatus == 1)
			{
				dLB = dCurrent;
				cResult.setItem((char *)stringIndex.data(), dLB);
				cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Aprox.txt");
				cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Aprox.rslt");
			}
			else
			{
				dUB = dCurrent;
			}

			if ((dUB - dLB) <= dResolution)
			{
				cout << "Resolution quit" << endl;
				cout << cThisTaskSet.ScalePartitionUtil(iPartition, dLB) << endl;
				cout << "***************" << endl;
				cout << "Partition " << iPartition << " Final Utilization: " << dLB << " From " << dOriginal << endl;
				cout << "***************" << endl;
				break;
			}
		}
		sprintf_s(axBuffer, "%d Time", *iter);
		iThisTotalTime = time(NULL) - iThisTotalTime;
		dThisTotalTime = cTimer.getCplexTime() - dThisTotalTime;
		cResult.setItem(axBuffer, (double)dThisTotalTime);
	}
	iTime = time(NULL) - iTime;
	cout << "Total Time: " << iTime << "s" << endl;
	cResult.setItem("Total Time", (double)iTime);
	cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Aprox.txt");
	cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Aprox.rslt");
}

void DAC07CaseStudy_BreakDownUtilization_Accurate(int argc, char * argv[])
{

	double dResolution = 0.03;
	int iStartingPartition = -1;
	int iObjective = 0;
	string stringLoadFile;
	//assume to be called by DummyTemp
	if (argc >= 3)
	{
		dResolution = atof(argv[2]);
	}

	if (argc >= 4)
	{
		iStartingPartition = atoi(argv[3]);
	}

	if (argc >= 5)
	{
		iObjective = atoi(argv[4]);
	}

	if (argc >= 6)
	{
		stringLoadFile = argv[5];
	}
	cout << "Resolution: " << dResolution << endl;
	cout << "Starting Partition: " << iStartingPartition << endl;

	DistributedEventDrivenTaskSet cTaskSet;
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	char axBuffer[512];
	if (stringLoadFile.empty())
	{
		double dScale = 1.0;
		cout << "WCET Scale: " << dScale << endl;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
	}
	else
	{
		cTaskSet.ReadImageFile(argv[4]);
		cTaskSet.InitializeDerivedData();
	}

	cout << currentDateTime().data() << endl;
	int iTime = time(NULL);
	const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
	StatisticSet cResult;
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		DistributedEventDrivenTaskSet cThisTaskSet;
		if (stringLoadFile.empty())
		{
			double dScale = 1.0;
			cout << "WCET Scale: " << dScale << endl;
			ReadDAC07CaseStudy("CSV.txt", cThisTaskSet, dScale);
		}
		else
		{
			cThisTaskSet.ReadImageFile(argv[4]);
			cThisTaskSet.InitializeDerivedData();
		}
		int iPartition = *iter;
		double dLB = cThisTaskSet.getPartitionUtilization_ByPartition(*iter);
		double dUB = 1.0;
		double dOriginal = dLB;		
		time_t iThisTotalTime = time(NULL);
		IloEnv cTimerEnv;
		IloCplex cTimer(cTimerEnv);
		double dThisTotalTime = cTimer.getCplexTime();
		cout << "--------------------------------" << endl;
		cout << "Partition: " << *iter << endl;
		cout << "Obj Type: " << cTaskSet.getObjectType(*(cThisTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
		cout << "Test extreme 1.0: " << endl;
		double dActual = cThisTaskSet.ScalePartitionUtil(*iter, 1.0);
#ifdef MCMILPPACT
		DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased<> cMUDARGuided(cThisTaskSet);
		cMUDARGuided.setCorePerIter(5);
		if (iObjective)
		{
			cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
		}
		int iStatus = cMUDARGuided.FocusRefinement(1);
#else
		int iStatus = 1;
#endif
		if (iStatus == 1)
		{
			dLB = dActual;
		}
		else
		{
			cThisTaskSet.ScalePartitionUtil(iPartition, dOriginal);
		}

		sprintf_s(axBuffer, "%d", *iter);
		std::string stringIndex(axBuffer);
		cResult.setItem((char *)stringIndex.data(), dLB);
		cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Accurate.txt");
		cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Accurate.rslt");
		while (true)
		{
			double dCurrent = (dLB + dUB) / 2;
			double dScaled = cThisTaskSet.ScalePartitionUtil(iPartition, dCurrent);
			if ((dScaled >= dUB) || (dScaled <= dLB))
			{
				cout << "rounding quit" << endl;
				break;
			}
			dCurrent = dScaled;
			cout << "-------------------------------------" << endl;
			cout << "Partition: " << iPartition << endl;
			cout << "Obj Type: " << cTaskSet.getObjectType(*(cThisTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
			cout << "Current Utilization Level: " << dCurrent << endl;
			cout << "Util Range: " << dLB << " ~ " << dUB << endl;
#ifdef MCMILPPACT
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased<> cMUDARGuided(cThisTaskSet);
			cMUDARGuided.setCorePerIter(5);
			if (iObjective)
			{
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			}
			int iStatus = cMUDARGuided.FocusRefinement(1);
#else
			int iStatus = 1;
#endif
			if (iStatus == 1)
			{
				dLB = dCurrent;
				cResult.setItem((char *)stringIndex.data(), dLB);
				cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Accurate.txt");
				cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Accurate.rslt");
			}
			else
			{
				dUB = dCurrent;
			}

			if ((dUB - dLB) <= dResolution)
			{
				cout << "Resolution quit" << endl;
				cout << cThisTaskSet.ScalePartitionUtil(iPartition, dLB) << endl;
				cout << "***************" << endl;
				cout << "Partition " << iPartition << " Final Utilization: " << dLB << " From " << dOriginal << endl;
				cout << "***************" << endl;
				break;
			}
		}
		sprintf_s(axBuffer, "%d Time", *iter);
		iThisTotalTime = time(NULL) - iThisTotalTime;
		dThisTotalTime = cTimer.getCplexTime() - dThisTotalTime;
		cResult.setItem(axBuffer, (double)dThisTotalTime);
	}
	iTime = time(NULL) - iTime;
	
	cout << "Total Time: " << iTime << "s" << endl;
	cResult.setItem("Total Time", (double)iTime);
	cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Accurate.txt");
	cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Accurate.rslt");
}

#endif

void MarshallBreakDownUtilizationResult()
{
	DistributedEventDrivenTaskSet cTaskSet;
	cout << _chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace") << endl;
	char axBuffer[512];
	ReadDAC07CaseStudy("CSV.txt", cTaskSet, 1.0);
	cout << _chdir("07DACCaseStudyBreakdownUtil") << endl;
	GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
	StatisticSet cApprox, cAccurate, cMILP;
	//vector<double> vectorApprox(iTaskNum, 0), vectorAccurate(iTaskNum, 0), vectorMILP(iTaskNum, 0);
	map<int, double> mapApprox, mapAccurate, mapMILP;
	vector<double> vectorAccurate, vectorApprox, vectorMILP, vectorOriginal;
	vector<double> vectorAccurateTime, vectorApproxTime, vectorMILPTime;
	vector<int> vectorECUIndex;
	cApprox.ReadStatisticImage("DAC07CaseStudy_BreakdownUtil_Aprox.rslt");
	cAccurate.ReadStatisticImage("DAC07CaseStudy_BreakdownUtil_Accurate.rslt");
	cMILP.ReadStatisticImage("DAC07CaseStudy_BreakDownUtil_MILP.rslt");
	const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
	StatisticSet cResult;
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		char axBuffer[512] = { 0 };
		sprintf_s(axBuffer, "%d", *iter);
		mapApprox[*iter] = cApprox.getItem(axBuffer).getValue();
		vectorApprox.push_back(cApprox.getItem(axBuffer).getValue());
		mapAccurate[*iter] = cAccurate.getItem(axBuffer).getValue();
		vectorAccurate.push_back(cAccurate.getItem(axBuffer).getValue());
		mapMILP[*iter] = cMILP.getItem(axBuffer).getValue() ;
		vectorMILP.push_back(cMILP.getItem(axBuffer).getValue());
		if (vectorMILP.back() < 0) vectorMILP.back() = -1;
		vectorECUIndex.push_back(*iter);
		vectorOriginal.push_back(cTaskSet.getPartitionUtilization_ByPartition(*iter));

		sprintf_s(axBuffer, "%d Time", *iter);
		vectorAccurateTime.push_back(cAccurate.getItem(axBuffer).getValue());
		vectorApproxTime.push_back(cApprox.getItem(axBuffer).getValue());
		vectorMILPTime.push_back(cMILP.getItem(axBuffer).getValue());


		cout << "**************************" << endl;
		cout << "ECU " << *iter << endl;
		cout << "Approx: " << cApprox.getItem(axBuffer).getValue() << endl;
		cout << "Accurate: " << cAccurate.getItem(axBuffer).getValue() << endl;
		cout << "MILP: " << mapMILP[*iter] << endl;
	}
	vectorOriginal = vectorMILP;
	int iStretch = vectorAccurate.size() / 2;
	for (int i = 0; i < floor(vectorAccurate.size() / 2); i++)
	{

		if (vectorApprox[i] + 0.01 < vectorAccurate[i])
		{
			if (vectorMILP[i] > 0)
			printf_s("%d\t & %.3f\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t",
			vectorECUIndex[i], vectorMILP[i], vectorApprox[i], vectorAccurate[i]
				);
			else
				printf_s("%d\t & -\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t",
				vectorECUIndex[i], vectorApprox[i], vectorAccurate[i]
				);
		}
		else
		{
			if (vectorMILP[i] > 0)
				printf_s("%d\t & %.3f\t & %.3f\t & %.3f\t",
				vectorECUIndex[i], vectorMILP[i], vectorApprox[i], vectorAccurate[i]
				);
			else
				printf_s("%d\t & -\t & %.3f\t & %.3f\t",
				vectorECUIndex[i], vectorApprox[i], vectorAccurate[i]
				);
		}

		if (vectorApprox[i + iStretch] + 0.01 < vectorAccurate[i + iStretch])
		{
			if (vectorMILP[i + iStretch] > 0)
			printf_s("& %d\t & %.3f\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t \\\\\\hline \n",
			vectorECUIndex[i + iStretch], vectorMILP[i + iStretch], vectorApprox[i + iStretch], vectorAccurate[i + iStretch]
				);
			else
				printf_s("& %d\t & -\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t \\\\\\hline \n",
				vectorECUIndex[i + iStretch], vectorApprox[i + iStretch], vectorAccurate[i + iStretch]
				);
		}
		else
		{
			if (vectorMILP[i + iStretch] > 0)
			printf_s("& %d\t & %.3f\t & %.3f\t & %.3f \\\\\\hline \n",
			vectorECUIndex[i + iStretch], vectorMILP[i + iStretch], vectorApprox[i + iStretch], vectorAccurate[i + iStretch]
				);
			else
				printf_s("& %d\t & -\t & %.3f\t & %.3f \\\\\\hline \n",
				vectorECUIndex[i + iStretch], vectorApprox[i + iStretch], vectorAccurate[i + iStretch]
				);
		}
	}
	if (vectorAccurate.size() % 2 == 0)	return;
	int i = vectorAccurate.size() - 1;
	if (vectorApprox[i] + 0.01 < vectorAccurate[i])
	{
		if (vectorMILP[i] > 0)
			printf_s("%d\t & %.3f\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t",
			vectorECUIndex[i], vectorMILP[i], vectorApprox[i], vectorAccurate[i]
			);
		else
			printf_s("%d\t & -\t & \\textcolor{red}{%.3f}\t & \\textcolor{red}{%.3f}\t",
			vectorECUIndex[i], vectorApprox[i], vectorAccurate[i]
			);
	}
	else
	{
		if (vectorMILP[i] > 0)
			printf_s("%d\t & %.3f\t & %.3f\t & %.3f\t",
			vectorECUIndex[i], vectorMILP[i], vectorApprox[i], vectorAccurate[i]
			);
		else
			printf_s("%d\t & -\t & %.3f\t & %.3f\t",
			vectorECUIndex[i], vectorApprox[i], vectorAccurate[i]
			);
	}

	printf_s("&\t & \t &\\\\\\hline \n");


	//Print Time
	cout << "---------------------------------------" << endl;
	for (int i = 0; i < floor(vectorAccurateTime.size() / 2); i++)
	{

		if (vectorApproxTime[i] + 0.01 < vectorAccurateTime[i] && false)
		{
			printf_s("%d\t & %.0f\t & \\textcolor{red}{%.0f}\t & \\textcolor{red}{%.0f}\t",
				vectorECUIndex[i], vectorMILPTime[i], vectorApproxTime[i], vectorAccurateTime[i]
				);
		}
		else
		{
			printf_s("%d\t & %.0f\t & %.0f\t & %.0f\t",
				vectorECUIndex[i], vectorMILPTime[i], vectorApproxTime[i], vectorAccurateTime[i]
				);
		}

		if (vectorApproxTime[i + iStretch] + 0.01 < vectorAccurateTime[i + iStretch] && false)
		{
			printf_s("& %d\t & %.0f\t & \\textcolor{red}{%.0f}\t & \\textcolor{red}{%.0f}\t \\\\\\hline \n",
				vectorECUIndex[i + iStretch], vectorMILPTime[i + iStretch], vectorApproxTime[i + iStretch], vectorAccurateTime[i + iStretch]
				);
		}
		else
		{
			printf_s("& %d\t & %.0f\t & %.0f\t & %.0f \\\\\\hline \n",
				vectorECUIndex[i + iStretch], vectorMILPTime[i + iStretch], vectorApproxTime[i + iStretch], vectorAccurateTime[i + iStretch]
				);
		}
	}
	if (vectorAccurateTime.size() % 2 == 0)	return;
	i = vectorAccurateTime.size() - 1;
	if (vectorApproxTime[i] + 0.01 < vectorAccurateTime[i] && false)
	{
		printf_s("%d\t & %.0f\t & \\textcolor{red}{%.0f}\t & \\textcolor{red}{%.0f}\t",
			vectorECUIndex[i], vectorMILPTime[i], vectorApproxTime[i], vectorAccurateTime[i]
			);
	}
	else
	{
		printf_s("%d\t & %.0f\t & %.0f\t & %.0f\t",
			vectorECUIndex[i], vectorMILPTime[i], vectorApproxTime[i], vectorAccurateTime[i]
			);
	}

	printf_s("&\t & \t &\\\\\\hline \n");

	cout << "---------------------------------------" << endl;
	//combined 
	for (int i = 0; i < vectorAccurate.size(); i++)
	{
		bool bTextRed = false;
		if (vectorAccurate[i] > vectorApprox[i] && vectorAccurate[i] > vectorMILP[i])
		{
			bTextRed = true;
		}
		if (i < 4)
		{
			printf_s("Bus%d \t & ", i + 1);
		}
		else
		{
			printf_s("ECU%d \t & ", i + 1 - 4);
		}
		

		if (vectorMILP[i] < 0)
		{
			if (bTextRed)
				printf_s("\\textcolor{red}{-} \t & ");
			else
				printf_s("- \t & ");
		}
		else
		{
			if (bTextRed)
				printf_s("\\textcolor{red}{%.3f} \t & ", vectorMILP[i]);
			else
				printf_s("%.3f \t & ", vectorMILP[i]);
		}

		if (bTextRed)
			printf_s("\\textcolor{red}{%.3f} \t & ", vectorApprox[i]);
		else
			printf_s("%.3f \t & ", vectorApprox[i]);

		if (bTextRed)
			printf_s("\\textcolor{red}{%.3f} \t & ", vectorAccurate[i]);
		else
			printf_s("%.3f \t & ", vectorAccurate[i]);

		if (vectorMILPTime[i] >= 7200)
		{
			printf_s("Timeout \t & ");
		}
		else
		{
			printf_s("%.0f \t & ", vectorMILPTime[i]);
		}

		if (vectorApproxTime[i] < 10)
		{
			printf("%.1f \t &", vectorApproxTime[i]);
		}
		else
		{
			printf("%.0f \t &", vectorApproxTime[i]);
		}

		if (vectorAccurateTime[i] < 10)
		{
			printf("%.1f \t", vectorAccurateTime[i]);
		}
		else
		{
			printf("%.0f \t", vectorAccurateTime[i]);
		}

		

		printf_s("\\\\ \\hline\n");
	}
}

#ifdef MCMILPPACT
void DAC07CaseStudy_BreakDownUtilization_MILP(int argc, char * argv[])
{
	double dResolution = (1.0) / 20;
	int iStartingPartition = -1;
	int iTimeout = 3600 * 2;
	string stringLoadFile;
	//assume to be called by DummyTemp
	if (argc >= 3)
	{
		dResolution = atof(argv[2]);
	}

	if (argc >= 4)
	{
		iStartingPartition = atoi(argv[3]);
	}

	if (argc >= 5)
	{
		iTimeout = atoi(argv[4]);
	}

	if (argc >= 6)
	{
		stringLoadFile = argv[5];
	}
	cout << "Resolution: " << dResolution << endl;
	cout << "Starting Partition: " << iStartingPartition << endl;

	DistributedEventDrivenTaskSet cTaskSet;
	_chdir("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace");
	char axBuffer[512];
	if (stringLoadFile.empty())
	{
		double dScale = 1.0;
		cout << "WCET Scale: " << dScale << endl;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
	}
	else
	{
		cTaskSet.ReadImageFile(argv[4]);
		cTaskSet.InitializeDerivedData();
	}	
	cout << currentDateTime().data() << endl;
	int iTime = time(NULL);
	const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
	map<int, double> mapObjective;
	StatisticSet cResult;
	for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
	{
		if (*iter < iStartingPartition)	continue;
		char axBuffer[512] = { 0 };		
		DistributedEventDrivenFullMILP cMILP(cTaskSet);
		cout << "---------------------------------------------" << endl;
		cout << "Allocation: " << *iter << endl;
		cout << "Original: " << cTaskSet.getPartitionUtilization_ByPartition(*iter) << endl;
		cout << "Object Type: " << cTaskSet.getPartitionObjectType(*iter) << endl;
		cMILP.setObjective(DistributedEventDrivenFullMILP::None);
		int iStatus = cMILP.SolveBreakDownUtilization(*iter, 2, iTimeout);
		double dObjective = cMILP.getObjective();
		sprintf_s(axBuffer, "%d", *iter);
		cResult.setItem(axBuffer, dObjective * cTaskSet.getPartitionUtilization_ByPartition(*iter));
		double dWallTime = cMILP.getWallTime();
		sprintf_s(axBuffer, "%d Time", *iter);
		cResult.setItem(axBuffer, dWallTime);
		cResult.WriteStatisticImage("DAC07CaseStudy_BreakDownUtil_MILP.rslt");
		cResult.WriteStatisticText("DAC07CaseStudy_BreakDownUtil_MILP.txt");
		cout << "From " << cTaskSet.getPartitionUtilization_ByPartition(*iter) << " to " << dObjective * cTaskSet.getPartitionUtilization_ByPartition(*iter) << endl;
		if (dObjective > 1)
		{
			double dOriginal = cTaskSet.getPartitionUtilization_ByPartition(*iter);
			TaskSetPriorityStruct cPA = cMILP.getSolPA();
			cTaskSet.ScalePartitionUtil(*iter, dOriginal * dObjective);
			DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cPA);
			vector<double> vectorResponseTime;
			int iStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTime);
			if (iStatus != 1)
			{
				cout << "Schedulability Verification Failed" << endl;				
			}
			cTaskSet.ScalePartitionUtil(*iter, dOriginal);
		}
	}
	iTime = time(NULL) - iTime;
	cout << "Total Time: " << iTime << "s" << endl;
	cResult.setItem("Total Time", (double)iTime);
}
#endif

#ifdef MCMILPPACT
namespace EventDrivenPACaseStudy_ArtifactEvaluation
{
	void VehicleSystem_All(int argc, char * argv[])
	{
		bool bUseMaxMinLaxityObj = true;
		DistributedEventDrivenTaskSet cTaskSet;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, 1.0);				
		GET_TASKSET_NUM_PTR(&cTaskSet, iTaskNum, pcTaskArray);
		vector<double> vectorResponseTimes;
		vectorResponseTimes.reserve(iTaskNum);
		for (int i = 0; i < iTaskNum; i++)
		{
			vectorResponseTimes.push_back(0);
		}
		typedef DistributedEventDrivenMUDARComputer::SchedConds SchedConds;
		typedef DistributedEventDrivenMUDARComputer::SchedCondType SchedCondType;
		SchedConds cDummy;
		TaskSetPriorityStruct cPA(cTaskSet);
		DistributedEventDrivenMUDARComputer cUnschedCoreComputer(cTaskSet);		

#ifdef MCMILPPACT	
		//test MUDAR guided opt approx
		if (1)
		{
			cout << "-----------------------------------------------" << endl;
			cout << "Test MUTER-Approx" << endl;			
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cTaskSet);
			cMUDARGuided.setCorePerIter(5);
			//cMUDARGuided.setSubILPDisplay(2);			
			if (bUseMaxMinLaxityObj)
			{
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
				//cMUDARGuided.setObjective(cMUDARGuided.MinMaxNormalizedLaxity);
			}
			int iStatus = cMUDARGuided.FocusRefinement(1);
			if (iStatus == 1)
			{
				//Verify Solution
				TaskSetPriorityStruct cSolPA = cMUDARGuided.getSolPA();
				//cSolPA.WriteTextFile("VehicleSystem_MUDAR_PA.txt");
				DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cSolPA);
				vector<double> vectorResponseTimes;
				int iVerifyStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTimes);
				cout << "Verify Status: " << iVerifyStatus << endl;
				const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = cTaskSet.getEnd2EndPaths();
				for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
					iter != rdequePaths.end(); iter++)
				{
					double dLatency = 0;
					const deque<int> & rdequeNodes = iter->getNodes();
					for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
					{
						dLatency += cRTCalc.getWaitingTime(*iterNode, vectorResponseTimes);
					}
					if (dLatency > iter->getDeadline())
					{
						cout << "Latency Deadline Violated" << endl;
						system("pause");
					}
				}

				//Verify using MILP
				if (0)
				{
					cout << "Verify using MILP" << endl;
					DistributedEventDrivenFullMILP cMILP(cTaskSet);
					vector<double> vectorMILPResponseTime;
					TaskSetPriorityStruct cPAtoVerify(cTaskSet);
					cPAtoVerify.ReadTextFile("VehicleSystem_MUDAR_PA.txt");
					int iMILPVerifyStatus = cMILP.VerifyPA(cPAtoVerify, vectorMILPResponseTime, 0);
					cout << "MILP Verify Status: " << iMILPVerifyStatus << endl;
				}

				//The priority assignment should pass accurate analysis
				if (0)
				{
					DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cSolPA);
					vector<double> vectorResponseTimeAccurateAnalysis;
					int iStatus = cRTCalc.ComputeAllResponseTimeExact(vectorResponseTimeAccurateAnalysis);
					cout << "Accurate Analysis Status: " << iStatus << endl;
					for (int i = 0; i < iTaskNum; i++)
					{
						if (!DOUBLE_EQUAL(vectorResponseTimes[i], vectorResponseTimeAccurateAnalysis[i], 1e-5))
						{
							cout << "Mismatch between accurate and approximate analysis: " << endl;
							cout << "Task " << i << " " << vectorResponseTimeAccurateAnalysis[i] << " " << vectorResponseTimes[i] << endl;
						}
					}

					DistributedEventDrivenMUDARComputer cMUDARCompAccurate(cTaskSet);
					DistributedEventDrivenMUDARComputer::SchedConds cDummy;
					DistributedEventDrivenMUDARComputer::SchedConds cSol = (DistributedEventDrivenMUDARComputer::SchedConds)cMUDARGuided.getOptimalSchedCondConfig();
					cout << "SchedCondFig Accurate Test: " <<
						cMUDARCompAccurate.IsSchedulable(cSol, cDummy) << endl;
				}
				cout << "Objective: " << cMUDARGuided.getObj() << endl;				
			}
		}

		//MUDAR Guided Opt Accurate Analysis
		if (1)
		{
			cout << "-----------------------------------------------" << endl;
			cout << "Test MUTER-Accurate" << endl;			
			//DistributedEventDrivenMUDARGuidedOpt cMUDARGuided(cTaskSet);			
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased<> cMUDARGuided(cTaskSet);
			cMUDARGuided.setCorePerIter(5);
			//cMUDARGuided.setSubILPDisplay(2);			
			if (bUseMaxMinLaxityObj)
			{
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
				//cMUDARGuided.setObjective(cMUDARGuided.MinMaxNormalizedLaxity);
			}
			int iStatus = cMUDARGuided.FocusRefinement(1);
			if (iStatus == 1)
			{
				//Verify Solution
				TaskSetPriorityStruct cSolPA = cMUDARGuided.getSolPA();
				//cSolPA.WriteTextFile("VehicleSystem_MUDAR_Accurate_PA.txt");
				DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cSolPA);
				vector<double> vectorResponseTimes;
				int iVerifyStatus = cRTCalc.ComputeAllResponseTimeExact(vectorResponseTimes);
				cout << "Verify Status: " << iVerifyStatus << endl;
				const deque<DistributedEventDrivenTaskSet::End2EndPath> & rdequePaths = cTaskSet.getEnd2EndPaths();
				for (deque<DistributedEventDrivenTaskSet::End2EndPath>::const_iterator iter = rdequePaths.begin();
					iter != rdequePaths.end(); iter++)
				{
					double dLatency = 0;
					const deque<int> & rdequeNodes = iter->getNodes();
					for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
					{
						dLatency += cRTCalc.getWaitingTime(*iterNode, vectorResponseTimes);
					}
					if (dLatency > iter->getDeadline())
					{
						cout << "Latency Deadline Violated" << endl;
						system("pause");
					}
				}
				cout << "Objective: " << cMUDARGuided.getObj() << endl;
			}
		}

		//test MILP
		if (1)
		{
			cout << "-----------------------------------------------" << endl;
			cout << "Testing MILP-Approx" << endl;
			cout << currentDateTime() << endl;
			DistributedEventDrivenFullMILP cMILP(cTaskSet);
			cMILP.setObjective(DistributedEventDrivenFullMILP::MaxMinEnd2EndLaxity);
			cout << "******************CPLEX Solver Output******************" << endl;
			int iStatus = cMILP.Solve(2);
			cout << "******************CPLEX Solver Complete******************" << endl;
			cout << "MILP Status: " << iStatus << endl;
			if (iStatus == 1)
			{
				TaskSetPriorityStruct cResultPA(cTaskSet);
				cResultPA = cMILP.getSolPA();
				vector<double> vectorResponseTimes;
				DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cResultPA);
				cout << "MILP Response Times" << endl;
				vector<double> vectorMILPResponseTimes = cMILP.getSolRT();
				for (int i = 0; i < iTaskNum; i++)
				{
					double dRT = cRTCalc.ComputeResponseTime_SpecifiedResponseTimes_Approx(i, -1, vectorMILPResponseTimes);
				}
				int iVerifyStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTimes);
				cout << "Schedulability Verification Status: " << iVerifyStatus << endl;
			}
			cout << "Objective: " << cMILP.getObjective() << endl;
			cout << "RunTime: " << cMILP.getWallTime() << endl;
		}
#endif

	}

	void VechicleSystem_BreakDownUtil_MILPApprox(int argc, char * argv[])
	{
		double dResolution = (1.0) / 20;
		int iStartingPartition = -1;
		int iTimeout = 3600 * 2;
		string stringLoadFile;		
		ofstream ofstreamOutpuFile("MILP-Approx_BreakDownUtil_Result.txt");
		ofstreamOutpuFile << "ECU/Bus\t" << "Breakdown_Utilization\t" << "RunTime(Seconds)" << endl;
		DistributedEventDrivenTaskSet cTaskSet;		
		char axBuffer[512];
		double dScale = 1.0;
		cout << "WCET Scale: " << dScale << endl;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
		cout << currentDateTime().data() << endl;
		int iTime = time(NULL);
		const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
		map<int, double> mapObjective;
		StatisticSet cResult;
		for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
		{
			if (*iter < iStartingPartition)	continue;
			char axBuffer[512] = { 0 };
			DistributedEventDrivenFullMILP cMILP(cTaskSet);
			cout << "---------------------------------------------" << endl;
			if (*iter < 4)
			{
				sprintf_s(axBuffer, "Bus%d", *iter);
			}
			else
			{
				sprintf_s(axBuffer, "ECU%d", *iter - 6);
			}
			cout << "Allocation: " << axBuffer << endl;						
			cMILP.setObjective(DistributedEventDrivenFullMILP::None);
			int iStatus = cMILP.SolveBreakDownUtilization(*iter, 2, iTimeout);
			double dObjective = cMILP.getObjective();
			ofstreamOutpuFile << axBuffer << " " << dObjective * cTaskSet.getPartitionUtilization_ByPartition(*iter) << "\t" << cMILP.getWallTime() << endl;
			sprintf_s(axBuffer, "%d", *iter);
			cResult.setItem(axBuffer, dObjective * cTaskSet.getPartitionUtilization_ByPartition(*iter));
			double dWallTime = cMILP.getWallTime();
			sprintf_s(axBuffer, "%d Time", *iter);
			cResult.setItem(axBuffer, dWallTime);
			//cResult.WriteStatisticImage("DAC07CaseStudy_BreakDownUtil_MILP.rslt");
			//cResult.WriteStatisticText("DAC07CaseStudy_BreakDownUtil_MILP.txt");
			//cout << "From " << cTaskSet.getPartitionUtilization_ByPartition(*iter) << " to " << dObjective * cTaskSet.getPartitionUtilization_ByPartition(*iter) << endl;			
			if (dObjective > 1)
			{
				double dOriginal = cTaskSet.getPartitionUtilization_ByPartition(*iter);
				TaskSetPriorityStruct cPA = cMILP.getSolPA();
				cTaskSet.ScalePartitionUtil(*iter, dOriginal * dObjective);
				DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cPA);
				vector<double> vectorResponseTime;
				int iStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTime);
				if (iStatus != 1)
				{
					cout << "Schedulability Verification Failed" << endl;
				}
				cTaskSet.ScalePartitionUtil(*iter, dOriginal);
			}
		}
		iTime = time(NULL) - iTime;
		cout << "Total Time: " << iTime << "s" << endl;
		cResult.setItem("Total Time", (double)iTime);

	}

	void VechicleSystem_BreakDownUtil_MUTERApprox(int argc, char * argv[])
	{

		double dResolution = 0.03;
		int iStartingPartition = -1;
		int iObjective = 1;
		string stringLoadFile;			

		ofstream ofstreamOutputFile("MUTER-Approx_BreakdownUtilization_Result.txt");
		ofstreamOutputFile << "ECU/Bus\t" << "Breakdown_Utilization\t" << "RunTime(Seconds)" << endl;
		DistributedEventDrivenTaskSet cTaskSet;		
		char axBuffer[512];
		double dScale = 1.0;		
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
		cout << currentDateTime().data() << endl;
		int iTime = time(NULL);
		const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
		StatisticSet cResult;
		for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
		{
			DistributedEventDrivenTaskSet cThisTaskSet;
			double dScale = 1.0;			
			ReadDAC07CaseStudy("CSV.txt", cThisTaskSet, dScale);
			time_t iThisTotalTime = time(NULL);
			IloEnv cTimerEnv;
			IloCplex cTimer(cTimerEnv);
			double dThisTotalTime = cTimer.getCplexTime();
			int iPartition = *iter;
			double dLB = cThisTaskSet.getPartitionUtilization_ByPartition(*iter);
			double dUB = 1.0;
			double dOriginal = dLB;

			cout << "--------------------------------" << endl;
			//cout << "Partition: " << *iter << endl;
			//cout << "Obj Type: " << cTaskSet.getObjectType(*(cThisTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
			//cout << "Test extreme 1.0: " << endl;
			if (*iter < 4)
			{
				sprintf_s(axBuffer, "Bus%d", *iter);
			}
			else
			{
				sprintf_s(axBuffer, "ECU%d", *iter - 6);
			}
			cout << "Computing Breakdown Utilization for " << axBuffer << "..." << endl;
			double dActual = cThisTaskSet.ScalePartitionUtil(*iter, 1.0);
#ifdef MCMILPPACT
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cThisTaskSet);
			cMUDARGuided.setCorePerIter(5);
			if (iObjective)
			{
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			}
			int iStatus = cMUDARGuided.FocusRefinement(0);
#else
			int iStatus = 1;
#endif
			if (iStatus == 1)
			{
				dLB = dActual;
			}
			else
			{
				cThisTaskSet.ScalePartitionUtil(iPartition, dOriginal);
			}

			sprintf_s(axBuffer, "%d", *iter);
			std::string stringIndex(axBuffer);
			cResult.setItem((char *)stringIndex.data(), dLB);
		//	cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Aprox.txt");
			//cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Aprox.rslt");
			while (true)
			{
				double dCurrent = (dLB + dUB) / 2;
				double dScaled = cThisTaskSet.ScalePartitionUtil(iPartition, dCurrent);
				if ((dScaled >= dUB) || (dScaled <= dLB))
				{
					break;
				}
				dCurrent = dScaled;			
#ifdef MCMILPPACT
				DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cThisTaskSet);
				cMUDARGuided.setCorePerIter(5);
				if (iObjective)
				{
					cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
				}
				int iStatus = cMUDARGuided.FocusRefinement(0);
#else
				int iStatus = 1;
#endif
				if (iStatus == 1)
				{
					dLB = dCurrent;
					cResult.setItem((char *)stringIndex.data(), dLB);
					//cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Aprox.txt");
					//cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Aprox.rslt");
				}
				else
				{
					dUB = dCurrent;
				}

				if ((dUB - dLB) <= dResolution)
				{
					//cout << "Resolution quit" << endl;
					//cout << cThisTaskSet.ScalePartitionUtil(iPartition, dLB) << endl;									
					break;
				}
				sprintf_s(axBuffer, "Binary search upper- and lower-bound: %.2f~%.2f      ", dLB, dUB);				
				cout << axBuffer << endl;;
			}
			cout << endl;
			sprintf_s(axBuffer, "%d Time", *iter);
			iThisTotalTime = time(NULL) - iThisTotalTime;
			dThisTotalTime = cTimer.getCplexTime() - dThisTotalTime;
			cResult.setItem(axBuffer, (double)dThisTotalTime);
			if (*iter < 4)
			{
				sprintf_s(axBuffer, "Bus%d", *iter);
			}
			else
			{
				sprintf_s(axBuffer, "ECU%d", *iter - 6);
			}
			cout << axBuffer << "\t " << (dLB + dUB) / 2 << "\t " << dThisTotalTime << " seconds" << endl;
			ofstreamOutputFile << axBuffer << "\t " << (dLB + dUB) / 2 << "\t " << dThisTotalTime << " seconds" << endl;
		}
		iTime = time(NULL) - iTime;
		cout << "Total Time: " << iTime << "s" << endl;
		cResult.setItem("Total Time", (double)iTime);
		//cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Aprox.txt");
		//cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Aprox.rslt");
	}

	void VechicleSystem_BreakDownUtil_MUTERAccurate(int argc, char * argv[])
	{

		double dResolution = 0.03;
		int iStartingPartition = -1;
		int iObjective = 1;
		string stringLoadFile;

		ofstream ofstreamOutputFile("MUTER-Accurate_BreakdownUtilization_Result.txt");
		ofstreamOutputFile << "ECU/Bus\t" << "Breakdown_Utilization\t" << "RunTime(Seconds)" << endl;
		DistributedEventDrivenTaskSet cTaskSet;
		char axBuffer[512];
		double dScale = 1.0;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
		cout << currentDateTime().data() << endl;
		int iTime = time(NULL);
		const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
		StatisticSet cResult;
		for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
		{
			DistributedEventDrivenTaskSet cThisTaskSet;
			double dScale = 1.0;
			ReadDAC07CaseStudy("CSV.txt", cThisTaskSet, dScale);
			time_t iThisTotalTime = time(NULL);
			IloEnv cTimerEnv;
			IloCplex cTimer(cTimerEnv);
			double dThisTotalTime = cTimer.getCplexTime();
			int iPartition = *iter;
			double dLB = cThisTaskSet.getPartitionUtilization_ByPartition(*iter);
			double dUB = 1.0;
			double dOriginal = dLB;

			cout << "--------------------------------" << endl;
			//cout << "Partition: " << *iter << endl;
			//cout << "Obj Type: " << cTaskSet.getObjectType(*(cThisTaskSet.getPartition_ByAllocation(iPartition).begin())) << endl;
			//cout << "Test extreme 1.0: " << endl;
			if (*iter < 4)
			{
				sprintf_s(axBuffer, "Bus%d", *iter);
			}
			else
			{
				sprintf_s(axBuffer, "ECU%d", *iter - 6);
			}
			cout << "Computing Breakdown Utilization for " << axBuffer << "..." << endl;
			double dActual = cThisTaskSet.ScalePartitionUtil(*iter, 1.0);
#ifdef MCMILPPACT
			DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased<> cMUDARGuided(cThisTaskSet); 
			cMUDARGuided.setCorePerIter(5);
			if (iObjective)
			{
				cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
			}
			int iStatus = cMUDARGuided.FocusRefinement(0);
#else
			int iStatus = 1;
#endif
			if (iStatus == 1)
			{
				dLB = dActual;
			}
			else
			{
				cThisTaskSet.ScalePartitionUtil(iPartition, dOriginal);
			}

			sprintf_s(axBuffer, "%d", *iter);
			std::string stringIndex(axBuffer);
			cResult.setItem((char *)stringIndex.data(), dLB);
			cResult.WriteStatisticText("DAC07CaseStudy_BreakdownUtil_Aprox.txt");
			cResult.WriteStatisticImage("DAC07CaseStudy_BreakdownUtil_Aprox.rslt");
			while (true)
			{
				double dCurrent = (dLB + dUB) / 2;
				double dScaled = cThisTaskSet.ScalePartitionUtil(iPartition, dCurrent);
				if ((dScaled >= dUB) || (dScaled <= dLB))
				{
					break;
				}
				dCurrent = dScaled;
#ifdef MCMILPPACT
				DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cThisTaskSet);
				cMUDARGuided.setCorePerIter(5);
				if (iObjective)
				{
					cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
				}
				int iStatus = cMUDARGuided.FocusRefinement(0);
#else
				int iStatus = 1;
#endif
				if (iStatus == 1)
				{
					dLB = dCurrent;
					cResult.setItem((char *)stringIndex.data(), dLB);										
				}
				else
				{
					dUB = dCurrent;
				}

				if ((dUB - dLB) <= dResolution)
				{
					//cout << "Resolution quit" << endl;
					//cout << cThisTaskSet.ScalePartitionUtil(iPartition, dLB) << endl;
					break;
				}
				sprintf_s(axBuffer, "Binary search upper- and lower-bound: %.2f~%.2f      ", dLB, dUB);
				cout << axBuffer << endl;;
			}
			sprintf_s(axBuffer, "%d Time", *iter);
			iThisTotalTime = time(NULL) - iThisTotalTime;
			dThisTotalTime = cTimer.getCplexTime() - dThisTotalTime;
			cResult.setItem(axBuffer, (double)dThisTotalTime);
			if (*iter < 4)
			{
				sprintf_s(axBuffer, "Bus%d", *iter);
			}
			else
			{
				sprintf_s(axBuffer, "ECU%d", *iter - 6);
			}
			cout << axBuffer << "\t " << (dLB + dUB) / 2 << "\t " << dThisTotalTime << " seconds" << endl;
			ofstreamOutputFile << axBuffer << "\t " << (dLB + dUB) / 2 << "\t " << dThisTotalTime << " seconds" << endl;
		}
		iTime = time(NULL) - iTime;
		cout << "Total Time: " << iTime << "s" << endl;
		cResult.setItem("Total Time", (double)iTime);				
	}

	void VechicleSystem_BreakDownUtil_PerAllocation(int argc, char * argv[])
	{
		//Usage: MUTER.exe VehicleSystem_BreakDownUtil [BnB, MUTER, MILP] [AllocationID]
		if (argc != 4)
		{
			cout << "illegal number of argument" << endl;
			cout << "MUTER.exe VehicleSystem_BreakDownUtil [MUTER-Accurate, MUTER-Approx, MILP-Approx] [AllocationID]" << endl;
			return;
		}
		double dResolution = 0.03;
		int iStartingPartition = -1;
		int iObjective = 1;
		string stringLoadFile;
		int iTimeout = 3600 * 2;
		DistributedEventDrivenTaskSet cTaskSet;
		char axBuffer[512];
		double dScale = 1.0;
		ReadDAC07CaseStudy("CSV.txt", cTaskSet, dScale);
		cout << currentDateTime().data() << endl;
		int iTime = time(NULL);
		const set<int> & rsetAllocationIndices = cTaskSet.getAllocationIndices();
		bool bFound = false;
		for (set<int>::const_iterator iter = rsetAllocationIndices.begin(); iter != rsetAllocationIndices.end(); iter++)
		{
			if (*iter < 4)
			{
				sprintf_s(axBuffer, "Bus%d", *iter + 1);
			}
			else
			{
				sprintf_s(axBuffer, "ECU%d", *iter - 5);
			}

			if (strcmp(axBuffer, argv[3]) == 0)
			{
				bFound = true;
			}
			else
			{
				continue;
			}

			cout << "Computing Breakdown Utilization for " << axBuffer << "..." << endl;
			cout << "Method: " << argv[2] << endl;
			if ((strcmp(argv[2], "MUTER-Accurate") == 0) || (strcmp(argv[2], "MUTER-Approx") == 0))
			{
				time_t iThisTotalTime = time(NULL);
				IloEnv cTimerEnv;
				IloCplex cTimer(cTimerEnv);
				double dThisTotalTime = cTimer.getCplexTime();
				int iPartition = *iter;
				double dLB = cTaskSet.getPartitionUtilization_ByPartition(*iter);
				double dUB = 1.0;
				double dOriginal = dLB;

				double dActual = cTaskSet.ScalePartitionUtil(*iter, 1.0);
#ifdef MCMILPPACT
				DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cTaskSet);
				cMUDARGuided.setCorePerIter(5);
				if (iObjective)
				{
					cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
				}
				cout << "Test whether schedulable at utilization=1.0 .... ";
				int iStatus = cMUDARGuided.FocusRefinement(0);
#else
				int iStatus = 1;
#endif
				if (iStatus == 1)
				{
					cout << "Yes" << endl;
					dLB = dActual;
				}
				else
				{
					cout << "No" << endl;
					cout << "Performing binary search..." << endl;
					cTaskSet.ScalePartitionUtil(iPartition, dOriginal);
				}

				while (true)
				{
					double dCurrent = (dLB + dUB) / 2;
					double dScaled = cTaskSet.ScalePartitionUtil(iPartition, dCurrent);
					if ((dScaled >= dUB) || (dScaled <= dLB))
					{
						break;
					}
					dCurrent = dScaled;
#ifdef MCMILPPACT
					int iStatus = 0;
					if ((strcmp(argv[2], "MUTER-Accurate") == 0))
					{
						DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased<>  cMUDARGuided(cTaskSet);
						cMUDARGuided.setCorePerIter(5);
						if (iObjective)
						{
							cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
						}
						iStatus = cMUDARGuided.FocusRefinement(0);
					}
					else if ((strcmp(argv[2], "MUTER-Approx") == 0))
					{
						DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx cMUDARGuided(cTaskSet);
						cMUDARGuided.setCorePerIter(5);
						if (iObjective)
						{
							cMUDARGuided.setObjective(cMUDARGuided.MaxMinEnd2EndLaxity);
						}
						iStatus = cMUDARGuided.FocusRefinement(0);
					}										
#else
					int iStatus = 1;
#endif
					if (iStatus == 1)
					{
						dLB = dCurrent;
					}
					else
					{
						dUB = dCurrent;
					}

					if ((dUB - dLB) <= dResolution)
					{
						sprintf_s(axBuffer, "Binary search upper- and lower-bound: %.3f~%.3f      ", dLB, dUB);
						cout << axBuffer << endl;;
						break;
					}
					sprintf_s(axBuffer, "Binary search upper- and lower-bound: %.3f~%.3f      ", dLB, dUB);
					cout << axBuffer << endl;;
				}
				cout << "Complete" << endl;
				printf_s("Breakdown Utilization: %.3f +/- %.3f\n", min(dLB + dResolution / 2.0, 1.0), dResolution / 2.0);
				dThisTotalTime = cTimer.getCplexTime() - dThisTotalTime;
				cout << "Time Elapsed: " << dThisTotalTime << " seconds" << endl;
			}
			else if (strcmp(argv[2], "MILP-Approx") == 0)
			{
				DistributedEventDrivenFullMILP cMILP(cTaskSet);				
				cMILP.setObjective(DistributedEventDrivenFullMILP::None);
				cout << "******************CPLEX Solver Output******************" << endl;
				int iStatus = cMILP.SolveBreakDownUtilization(*iter, 2, iTimeout);
				cout << "******************CPLEX Solver Complete******************" << endl;
				double dObjective = cMILP.getObjective();												
				double dWallTime = cMILP.getWallTime();
				double dOriginal = cTaskSet.getPartitionUtilization_ByPartition(*iter);
				if (dObjective > 1 && 0)
				{
					
					TaskSetPriorityStruct cPA = cMILP.getSolPA();
					cTaskSet.ScalePartitionUtil(*iter, dOriginal * dObjective);
					DistributedEventDrivenResponseTimeCalculator cRTCalc(cTaskSet, cPA);
					vector<double> vectorResponseTime;
					int iStatus = cRTCalc.ComputeAllResponseTime_Approx(vectorResponseTime);
					if (iStatus != 1)
					{
						cout << "Schedulability Verification Failed" << endl;
					}
					cTaskSet.ScalePartitionUtil(*iter, dOriginal);
				}
				cout << "Complete" << endl;
				if (dObjective > 1)
				{
					cout << "Breakdown utilization: " << dOriginal * dObjective << endl;
				}
				else 
				{
					cout << "Breakdown utilization: N/A" << dOriginal * dObjective << endl;
					cout << "Timeout" << endl;
				}
				cout << "Time Elapsed: " << cMILP.getWallTime() << " seconds" << endl;
			}
			else
			{
				cout << "Method must be with in [MUTER-Accurate, MUTER-Approx, MILP-Approx]" << endl;
				return;
			}
			break;
		}		

		if (!bFound)
		{
			cout << "illegal ECU/Bus name. should be Bus[1-4]/ECU[1-29]" << endl;
			return;
		}

	}
}
#endif
