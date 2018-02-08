#include "stdafx.h"
#include "ResponseTimeCalculator.h"
#include <algorithm>
#include <assert.h>
#include "Util.h"


extern void my_assert(bool bCondition, char axInfo[]);

ResponseTimeCalculator::ResponseTimeCalculator()
{
	m_pdWorstCaseCritiChange = NULL;
	m_pdResponseTime = NULL;
	m_pcTargetTaskSet = NULL;
	m_piPriorityTable = NULL;
}


ResponseTimeCalculator::~ResponseTimeCalculator()
{
	ClearTable();
}

void ResponseTimeCalculator::Initialize(TaskSet & rcTaskSet)
{
	ClearTable();
	m_pcTargetTaskSet = &rcTaskSet;
	CreateTable();
}

void ResponseTimeCalculator::Calculate(int iRTType)
{
	my_assert(m_pdResponseTime && m_pdWorstCaseCritiChange && m_pcTargetTaskSet && m_piPriorityTable,
		"Response Time Calculator Uninitialized!!");
	CalcResponseTime(iRTType);
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	m_bIsMissingDeadline = false;
	for (int i = 0; i < iTaskNum; i++)
	{
		//if (m_pdResponseTime[i] > m_pcTargetTaskSet->m_pcTaskArray[i].getPeriod())
		if (m_pdResponseTime[i] - m_pcTargetTaskSet->m_pcTaskArray[i].getDeadline() > 1e-9)
		{
#if 0
			cout << m_pdResponseTime[i] - m_pcTargetTaskSet->m_pcTaskArray[i].getPeriod() << endl;
			cout << m_pdResponseTime[i] << " " << m_pcTargetTaskSet->m_pcTaskArray[i].getPeriod() << endl;
#endif
			m_bIsMissingDeadline = true;
			break;
		}
	}
}

void ResponseTimeCalculator::CreateTable()
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	m_pdWorstCaseCritiChange = new double[iTaskNum];
	m_pdResponseTime = new double[iTaskNum];
	m_piPriorityTable = new int[iTaskNum];
	for (int i = 0; i < iTaskNum; i++)
	{
		m_pdWorstCaseCritiChange[i] = 0;
		m_pdResponseTime[i] = 0;
		m_piPriorityTable[i] = -1;
	}
	m_bIsMissingDeadline = false;
}

void ResponseTimeCalculator::ClearTable()
{
	if (m_pdWorstCaseCritiChange)
	{
		delete m_pdWorstCaseCritiChange;
	}
	m_pdWorstCaseCritiChange = NULL;

	if (m_pdResponseTime)
	{
		delete m_pdResponseTime;
	}
	m_pdResponseTime = NULL;

	if (m_piPriorityTable)
	{
		delete m_piPriorityTable;
	}
	m_piPriorityTable = NULL;

	m_bIsMissingDeadline = false;
}

void ResponseTimeCalculator::setPrioirty(int iTaskIndex, int iPriority)
{
	my_assert(m_pdResponseTime && m_pdWorstCaseCritiChange && m_pcTargetTaskSet && m_piPriorityTable,
		"Response Time Calculator Uninitialized!!");
	my_assert(iTaskIndex < m_pcTargetTaskSet->getTaskNum(), "In setPrioirty. Index Out Of Bound.");
	m_piPriorityTable[iTaskIndex] = iPriority;
}

int ResponseTimeCalculator::getPriority(int iTaskIndex)
{
	return m_piPriorityTable[iTaskIndex];
}

double ResponseTimeCalculator::LOResponseTime_RHS(int iTaskIndex, double fGuess)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = pcTaskArray[iTaskIndex].getCLO();
	int iPriority = m_piPriorityTable[iTaskIndex];
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex) continue;
		if (m_piPriorityTable[i] == iPriority)
		{
			iPriority = iPriority;
		}
		assert(m_piPriorityTable[i] != iPriority);

		if (m_piPriorityTable[i] < iPriority)
		{
			dResponseTime += ceil(fGuess / pcTaskArray[i].getPeriod()) * pcTaskArray[i].getCLO();
		}
	}
	return dResponseTime;
}

double ResponseTimeCalculator::HIResponseTime_RHS(int iTaskIndex, double fGuess)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = pcTaskArray[iTaskIndex].getCHI();
	int iPriority = m_piPriorityTable[iTaskIndex];
	for (int i = 0; i < iTaskNum; i++)
	{
		if ((m_piPriorityTable[i] < iPriority) && (pcTaskArray[i].getCriticality()))
		{
			dResponseTime += ceil(fGuess / pcTaskArray[i].getPeriod()) * pcTaskArray[i].getCHI();
		}
	}
	return dResponseTime;
}

double ResponseTimeCalculator::MixResponseTime_RHS(int iTaskIndex, double fGuess, double fCritiChange)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = pcTaskArray[iTaskIndex].getCHI();
	int iPriority = m_piPriorityTable[iTaskIndex];
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex) continue;
		assert(m_piPriorityTable[i] != iPriority);
		if (m_piPriorityTable[i] < iPriority)
		{
			if (pcTaskArray[i].getCriticality())
			{				
				int iHIInstances = min(
					ceil((fGuess - fCritiChange - (pcTaskArray[i].getPeriod() - pcTaskArray[i].getDeadline())) / pcTaskArray[i].getPeriod()) + 1.0, 
					ceil(fGuess / pcTaskArray[i].getPeriod())
					);
				int iLOInsatnces = ceil(fGuess / pcTaskArray[i].getPeriod()) - iHIInstances;
				dResponseTime += iLOInsatnces * pcTaskArray[i].getCLO() + iHIInstances * pcTaskArray[i].getCHI();
			}
			else
			{
				int iLOInstances = floor(fCritiChange / pcTaskArray[i].getPeriod()) + 1;
				//int iLOInstances = ceil(fCritiChange / pcTaskArray[i].getPeriod());
				dResponseTime += iLOInstances * pcTaskArray[i].getCLO();
			}
		}
	}
	return dResponseTime;
}

double ResponseTimeCalculator::MixResponseTimeHuang_RHS(int iTaskIndex, double fGuess, double fCritiChange)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = pcTaskArray[iTaskIndex].getCHI();
	int iPriority = m_piPriorityTable[iTaskIndex];
	if (fCritiChange == 16)
	{
		fCritiChange = 16;
	}
	for (int i = 0; i < iTaskNum; i++)
	{
		if (m_piPriorityTable[i] < iPriority)
		{
			if (pcTaskArray[i].getCriticality())
			{
				int iM = max(floor((fCritiChange - pcTaskArray[i].getDeadline()) / pcTaskArray[i].getPeriod()) + 1.0, (double)0.0);
				int iMHI = (ceil(fGuess / pcTaskArray[i].getPeriod()) - iM);
				dResponseTime += iM * pcTaskArray[i].getCLO() + iMHI * pcTaskArray[i].getCHI();
			}
			else
			{
				//int iLOInstances = floor(fCritiChange / pcTaskArray[i].getPeriod()) + 1;
				int iLOInstances = ceil(fCritiChange / pcTaskArray[i].getPeriod());
				dResponseTime += iLOInstances * pcTaskArray[i].getCLO();
			}
		}
	}
	return dResponseTime;
}

double ResponseTimeCalculator::LOResponseTime(int iTaskIndex)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task & rcTask = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = 0;
	double dResponseTime_RHS = rcTask.getCLO();
	double fPeriod = m_pcTargetTaskSet->getTaskArrayPtr()[iTaskIndex].getPeriod();
	while (dResponseTime != dResponseTime_RHS)
	{
		dResponseTime = dResponseTime_RHS;
		if (dResponseTime > fPeriod)
		{
			break;
		}
		dResponseTime_RHS = LOResponseTime_RHS(iTaskIndex, dResponseTime);
	}
	return dResponseTime;
}

double ResponseTimeCalculator::HIResponseTime(int iTaskIndex)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task & rcTask = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = 0;
	double dResponseTime_RHS = rcTask.getCHI();
	double fPeriod = m_pcTargetTaskSet->getTaskArrayPtr()[iTaskIndex].getPeriod();
	while (dResponseTime != dResponseTime_RHS)
	{
		dResponseTime = dResponseTime_RHS;
		if (dResponseTime > fPeriod)
		{			
			break;
		}
		dResponseTime_RHS = HIResponseTime_RHS(iTaskIndex, dResponseTime);
	}
	return dResponseTime;
}

double ResponseTimeCalculator::MixResponseTime(int iTaskIndex, double fCritiChange, double dLORT)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task & rcTask = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = 0;
	double dResponseTime_RHS = dLORT;
	//double dResponseTime_RHS = rcTask.getCHI();	
	double fPeriod = m_pcTargetTaskSet->getTaskArrayPtr()[iTaskIndex].getPeriod();
	while (dResponseTime != dResponseTime_RHS)
	{
		dResponseTime = dResponseTime_RHS;
		if (dResponseTime > pcTaskArray[iTaskIndex].getDeadline())
		{			
			break;
		}
		dResponseTime_RHS = MixResponseTime_RHS(iTaskIndex, dResponseTime, fCritiChange);
	}
	return dResponseTime;
}

double ResponseTimeCalculator::MixResponseTimeHuang(int iTaskIndex, double fCritiChange)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task & rcTask = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = 0;
	//double dResponseTime_RHS = LOResponseTime(iTaskIndex);
	double dResponseTime_RHS = fCritiChange;
	double fPeriod = m_pcTargetTaskSet->getTaskArrayPtr()[iTaskIndex].getPeriod();
	while (dResponseTime != dResponseTime_RHS)
	{
		dResponseTime = dResponseTime_RHS;
		if (dResponseTime > pcTaskArray[iTaskIndex].getDeadline())
		{
			break;
		}
		dResponseTime_RHS = MixResponseTimeHuang_RHS(iTaskIndex, dResponseTime, fCritiChange);
	}
	return dResponseTime;
}

double ResponseTimeCalculator::WorstCasedMixResponseTime(int iTaskIndex, double * pfWCCCT)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task & rcTask = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	int iPriority = m_piPriorityTable[iTaskIndex];
	double dLOResponseTime = LOResponseTime(iTaskIndex);
	//double dResponseTime = max(HIResponseTime(iTaskIndex), dLOResponseTime);
	double dResponseTime = dLOResponseTime;
	double dResponseTimeTmp = 0;
	double dWCCCT = 0;
	double dDeadline = rcTask.getDeadline();
	set<double> setCritiChange = { 0 };	
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex) continue;
		if ((m_piPriorityTable[i] < iPriority) && (!pcTaskArray[i].getCriticality()))
		{
			double fCritiChange = 0;
			while (fCritiChange < dLOResponseTime)
			{								
				setCritiChange.insert(fCritiChange);
				fCritiChange += pcTaskArray[i].getPeriod();
			}
		}
	}
	
	for (set<double>::iterator iter = setCritiChange.begin(); iter != setCritiChange.end(); iter++)
	{
		double dResponseTimeTmp = MixResponseTime(iTaskIndex, *iter, dLOResponseTime);
		if (dResponseTimeTmp > dResponseTime)
		{
			dResponseTime = dResponseTimeTmp;
			dWCCCT = *iter;
		}

		if (dResponseTime > dDeadline)
		{
			break;
		}
	}

	if (pfWCCCT)
	{
		*pfWCCCT = dWCCCT;
	}
	return dResponseTime;
}

double ResponseTimeCalculator::WorstCasedMixResponseTimeHuang(int iTaskIndex, double * pfWCCCT)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task & rcTask = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	int iPriority = m_piPriorityTable[iTaskIndex];
	double dLOResponseTime = LOResponseTime(iTaskIndex);
	double dResponseTime = max(HIResponseTime(iTaskIndex), dLOResponseTime);
	double dResponseTimeTmp = 0;
	double dWCCCT = 0;

	for (int i = 0; i < iTaskNum; i++)
	{
		if ((m_piPriorityTable[i] <= iPriority))
		{
			double fCritiChange = pcTaskArray[i].getDeadline();
			while (fCritiChange <= dLOResponseTime)
			{
				dResponseTimeTmp = MixResponseTimeHuang(iTaskIndex, fCritiChange);
				if (dResponseTimeTmp > dResponseTime)
				{
					dResponseTime = dResponseTimeTmp;
					dWCCCT = fCritiChange;
				}
				fCritiChange += pcTaskArray[i].getPeriod();
			}
		}
	}
	if (pfWCCCT)
	{
		*pfWCCCT = dWCCCT;
	}
	return dResponseTime;
}

void ResponseTimeCalculator::CalcResponseTime(int iRTType)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (pcTaskArray[i].getCriticality())
		{
			double fResponseTime = 0, fWCCCT = 0;
			switch (iRTType)
			{
			case RTCALC_LO:
				fResponseTime = LOResponseTime(i);
				break;
			case RTCALC_HI:
				fResponseTime = HIResponseTime(i);
				break;
			case RTCALC_MIX:
				fResponseTime = WorstCasedMixResponseTime(i, &fWCCCT);
				break;
			case RTCALC_AMCRTB:
				fResponseTime = ResponseTimeAMCRTB(i);
				break;
			case RTCALC_VESTAL:
				fResponseTime = ResponseTimeVestal(i);
				break;
			default:
				my_assert(false, "Invalid RT Calculation Parameter.");
				break;
			}
			m_pdResponseTime[i] = fResponseTime;
			m_pdWorstCaseCritiChange[i] = fWCCCT;
		}
		else
		{
			double fResponseTime = 0;
			fResponseTime = LOResponseTime(i);
			m_pdResponseTime[i] = fResponseTime;
		}
	}
}

double ResponseTimeCalculator::getResponseTime(int iTaskIndex)
{
	my_assert(iTaskIndex < m_pcTargetTaskSet->getTaskNum(), "Task Index Out Bounded");
	return m_pdResponseTime[iTaskIndex];
}

double ResponseTimeCalculator::getWCCCT(int iTaskIndex)
{
	my_assert(iTaskIndex < m_pcTargetTaskSet->getTaskNum(), "Task Index Out Bounded");
	return m_pdWorstCaseCritiChange[iTaskIndex];
}

double ResponseTimeCalculator::ResponseTimeAMCRTB_RHS(int iTaskIndex, double fGuess,double fLOResponseTime)
{
	my_assert(iTaskIndex < m_pcTargetTaskSet->getTaskNum(), "Task Index Out Bounded");
	double dValue = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex].getCHI();
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	int iPriority = m_piPriorityTable[iTaskIndex];
	if (fLOResponseTime == -1)
		fLOResponseTime = LOResponseTime(iTaskIndex);
	for (int i = 0; i < iTaskNum; i++)
	{
		if (m_piPriorityTable[i] < iPriority)
		{
			if (pcTaskArray[i].getCriticality())
			{
				dValue += ceil(fGuess / pcTaskArray[i].getPeriod())*pcTaskArray[i].getCHI();
			}
			else
			{
				dValue += ceil(fLOResponseTime / pcTaskArray[i].getPeriod())*pcTaskArray[i].getCLO();
			}
		}
	}
	return dValue;
}

double ResponseTimeCalculator::ResponseTimeAMCRTB(int iTaskIndex)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task & rcTask = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = 0;
	double fLOResponseTime = LOResponseTime(iTaskIndex);
	double dResponseTime_RHS = fLOResponseTime;
	double fPeriod = m_pcTargetTaskSet->getTaskArrayPtr()[iTaskIndex].getPeriod();
	while (dResponseTime != dResponseTime_RHS)
	{
		dResponseTime = dResponseTime_RHS;
		if (dResponseTime > fPeriod)
		{
			break;
		}
		dResponseTime_RHS = ResponseTimeAMCRTB_RHS(iTaskIndex, dResponseTime,fLOResponseTime);
	}
	return dResponseTime;
}

double ResponseTimeCalculator::ResponseTimeVestal_RHS(int iTaskIndex, double fGuess)
{
	my_assert(iTaskIndex < m_pcTargetTaskSet->getTaskNum(), "Task Index Out Bounded");
	double dValue = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex].getCHI();
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	int iPriority = m_piPriorityTable[iTaskIndex];	
	for (int i = 0; i < iTaskNum; i++)
	{
		if (m_piPriorityTable[i] < iPriority)
		{
			if (pcTaskArray[iTaskIndex].getCriticality())
			{
				dValue += ceil(fGuess / pcTaskArray[i].getPeriod())*pcTaskArray[i].getCHI();
			}
			else
			{
				dValue += ceil(fGuess / pcTaskArray[i].getPeriod())*pcTaskArray[i].getCLO();
			}
		}
	}
	return dValue;
}

double ResponseTimeCalculator::ResponseTimeVestal(int iTaskIndex)
{
	int iTaskNum = m_pcTargetTaskSet->getTaskNum();
	Task & rcTask = m_pcTargetTaskSet->m_pcTaskArray[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->m_pcTaskArray;
	double dResponseTime = 0;
	double dResponseTime_RHS = pcTaskArray[iTaskIndex].getCHI();
	double dDeadline = m_pcTargetTaskSet->getTaskArrayPtr()[iTaskIndex].getDeadline();
	while (dResponseTime != dResponseTime_RHS)
	{
		dResponseTime = dResponseTime_RHS;
		if (dResponseTime > dDeadline)
		{
			break;
		}
		dResponseTime_RHS = ResponseTimeVestal_RHS(iTaskIndex, dResponseTime);
	}
	return dResponseTime;
}

double ResponseTimeCalculator::CalcResponseTimeTask(int iTaskIndex, int iRTType)
{
	Task & rcTask = m_pcTargetTaskSet->getTaskArrayPtr()[iTaskIndex];
	if (rcTask.getCriticality())
	{
		switch (iRTType)
		{
		case RTCALC_AMCMAX:
			return WorstCasedMixResponseTime(iTaskIndex);
			break;
		case RTCALC_AMCRTB:
			return ResponseTimeAMCRTB(iTaskIndex);
			break;
		case RTCALC_LO:
			return LOResponseTime(iTaskIndex);
			break;
		case RTCALC_VESTAL:
			return ResponseTimeVestal(iTaskIndex);
			break;
		default:
			my_assert(false, "Invalid RTCALC parameter");
			return 0;
			break;
		}
	}
	else
	{
		return LOResponseTime(iTaskIndex);
	}
}

//_LORTCalculator

_LORTCalculator::_LORTCalculator()
{
	m_bDeadlineMiss = false;
}

_LORTCalculator::_LORTCalculator(TaskSet & rcTaskSet)
{
	m_pcTaskSet = &rcTaskSet;
	m_vectorResponseTime.reserve(m_pcTaskSet->getTaskNum());
	m_bDeadlineMiss = false;
}

_LORTCalculator::~_LORTCalculator()
{

}

double _LORTCalculator::CalculateTaskFP(int iTaskIndex, double dBlocking, double dIniRT, double dLimit, TaskSetPriorityStruct & rcPriorityAssignment, void * pvExtraData)
{
	double dResponseTime_RHS = dIniRT;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	Task & rcTask = pcTaskArray[iTaskIndex];	
	double dResponseTime = 0;		
	while (dResponseTime != dResponseTime_RHS)
	{
		dResponseTime = dResponseTime_RHS;
		if (dResponseTime > dLimit)
		{
			break;
		}
		dResponseTime_RHS = RHS(iTaskIndex, dResponseTime, dBlocking, rcPriorityAssignment, pvExtraData);
	}
	return dResponseTime;
}

double _LORTCalculator::RHS(int iTaskIndex, double dRT, double dBlocking, TaskSetPriorityStruct & rcPriorityAssignment, void * pvExtraData)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dResponseTime = pcTaskArray[iTaskIndex].getCLO() + dBlocking;
	int iPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex) continue;
		int iInterPriority = rcPriorityAssignment.getPriorityByTask(i);
		assert(iInterPriority != iPriority);

		if ((iInterPriority < iPriority))
		{
			dResponseTime += ceil(dRT / pcTaskArray[i].getPeriod()) * pcTaskArray[i].getCLO();
		}
	}
	return dResponseTime;
}

double _LORTCalculator::CalculateRTTask(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, double dLB)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dResponseTime = CalculateTaskFP(iTaskIndex, 0,
		max(pcTaskArray[iTaskIndex].getCLO(), dLB), pcTaskArray[iTaskIndex].getDeadline(), rcPriorityAssignment, NULL);
	return dResponseTime;
}

double _LORTCalculator::CalculateRTTaskWithBlocking(int iTaskIndex, double dBlocking, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dResponseTime = CalculateTaskFP(iTaskIndex, dBlocking,
		pcTaskArray[iTaskIndex].getCLO(), pcTaskArray[iTaskIndex].getDeadline(), rcPriorityAssignment, NULL);
	return dResponseTime;
}

double _LORTCalculator::CalculateRTTaskGCD(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	double dRequest = pcTaskArray[iTaskIndex].getCLO();
	double dGCD = m_pcTaskSet->getPeriodGCD();	
	double dRT = ceil(dRequest / dGCD) * dGCD;		
	double dDeadline = pcTaskArray[iTaskIndex].getDeadline();	
	int iPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	double dFinalRT = 0;
	while (true)
	{				
		dRequest = pcTaskArray[iTaskIndex].getCLO();
		for (int i = 0; i < iTaskNum; i++)
		{
			if (i == iTaskIndex)	continue;
			if (rcPriorityAssignment.getPriorityByTask(i) < iPriority)
				dRequest += ceil(dRT / pcTaskArray[i].getPeriod()) * pcTaskArray[i].getCLO();
		}	

		if (dRequest > dDeadline)
		{
			dFinalRT = dRequest;
			break;
		}

		if (dRequest <= dRT)
		{
			dFinalRT = dRequest;
			break;			
		}		
		else
		{
			double dNewRT = ceil(dRequest / dGCD) * dGCD;
			if (DOUBLE_EQUAL(dRT, dNewRT, 1e-6))
			{
				dNewRT += dGCD;
			}
			dRT = dNewRT;
		}

		if (dRT > dDeadline)
		{
			dFinalRT = dRT;
			break;
		}
	}	
	return dFinalRT;
}

//_NonPreemptiveLORTCalculator
_NonPreemptiveLORTCalculator::_NonPreemptiveLORTCalculator(TaskSet & rcTaskSet)
{
	m_pcTaskSet = &rcTaskSet;
}

_NonPreemptiveLORTCalculator::_NonPreemptiveLORTCalculator()
{

}

double _NonPreemptiveLORTCalculator::CalculateRT(int iTaskIndex, TaskSetPriorityStruct & rcPA)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dRT_Prev = 0, dRT = pcTaskArray[iTaskIndex].getCLO();
	while (DOUBLE_EQUAL(dRT, dRT_Prev, 1e-6) == false)
	{
		dRT_Prev = dRT;
		dRT = NonpreemptiveRTRHS(dRT_Prev, iTaskIndex, rcPA);
		if (dRT > pcTaskArray[iTaskIndex].getDeadline())
		{
			break;
		}
	}
	return dRT;
}

double _NonPreemptiveLORTCalculator::NonpreemptiveRTRHS(double dTime, int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
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

double _NonPreemptiveLORTCalculator::ComputeBlocking(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
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

//_AMCMaxRTCalculator

_AMCMaxRTCalculator::_AMCMaxRTCalculator(TaskSet & rcTaskSet)
	:_LORTCalculator(rcTaskSet)
{

}

double _AMCMaxRTCalculator::RHS(int iTaskIndex, double dRT, double dBlocking, TaskSetPriorityStruct & rcPriorityAssignment, void * pvExtraData)
{
	double fCritiChange = *(double*)pvExtraData;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double fGuess = dRT;
	int iPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	double dRHS = pcTaskArray[iTaskIndex].getCHI() + dBlocking;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex) continue;
		int iInterPriority = rcPriorityAssignment.getPriorityByTask(i);
		if ((iInterPriority < iPriority))
		{
			if (pcTaskArray[i].getCriticality())
			{
				int iHIInstances = min(
					ceil((fGuess - fCritiChange - (pcTaskArray[i].getPeriod() - pcTaskArray[i].getDeadline())) / pcTaskArray[i].getPeriod()) + 1.0,
					ceil(fGuess / pcTaskArray[i].getPeriod())
					);
				int iLOInsatnces = ceil(fGuess / pcTaskArray[i].getPeriod()) - iHIInstances;
				dRHS += iLOInsatnces * pcTaskArray[i].getCLO() + iHIInstances * pcTaskArray[i].getCHI();
			}
			else
			{
				int iLOInstances = floor(fCritiChange / pcTaskArray[i].getPeriod()) + 1;
				//int iLOInstances = ceil(fCritiChange / pcTaskArray[i].getPeriod());
				dRHS += iLOInstances * pcTaskArray[i].getCLO();
			}
		}
	}
	return dRHS;
}

double _AMCMaxRTCalculator::ResponseTime(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, double dCritiChange, double dLORT)
{
	Task * pcTaskArray = m_pcTaskSet->getTaskArrayPtr();
	//return CalculateTaskFP(iTaskIndex, pcTaskArray[iTaskIndex].getCHI(), pcTaskArray[iTaskIndex].getDeadline(), rcPriorityAssignment, &dCritiChange);
	return CalculateTaskFP(iTaskIndex, 0.0, dLORT, pcTaskArray[iTaskIndex].getDeadline(), rcPriorityAssignment, &dCritiChange);
}

double _AMCMaxRTCalculator::WCRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	set<double> setCritiChange;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	Task ** pcTaskArrayByPtr = m_pcTaskSet->getTaskArrayByPeriosPtr();
	double dPeriod = pcTaskArray[iTaskIndex].getPeriod();
	int iPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	_LORTCalculator cLORTCalc(*m_pcTaskSet);
	double dLORT = cLORTCalc.CalculateRTTask(iTaskIndex, rcPriorityAssignment);
	double dDeadline = pcTaskArray[iTaskIndex].getDeadline();
	if (dLORT > dDeadline)
		return dLORT;
	if (!pcTaskArray[iTaskIndex].getCriticality())
		return dLORT;
	setCritiChange.insert(0);
	for (int i = 0; i < iTaskNum; i++)
	{
		double dThisPeriod = pcTaskArrayByPtr[i]->getPeriod();
		int iThisTask = pcTaskArrayByPtr[i]->getTaskIndex();
		int iThisPriority = rcPriorityAssignment.getPriorityByTask(iThisTask);
		if ((iThisPriority < iPriority) && !(pcTaskArrayByPtr[i]->getCriticality()))
		{
			double dCritiChange = pcTaskArray[iThisTask].getPeriod();
			while (dCritiChange < dLORT)
			{
				setCritiChange.insert(dCritiChange);
				dCritiChange += dThisPeriod;
			}
		}
	}

	double dMaxRT = 0;
	for (set<double>::iterator iter = setCritiChange.begin(); iter != setCritiChange.end(); iter++)
	{
		double dThisRT = ResponseTime(iTaskIndex, rcPriorityAssignment, *iter, dLORT);
		if (dThisRT > dMaxRT)
		{
			dMaxRT = dThisRT;
		}

		if (dMaxRT > dDeadline)
		{
			return dMaxRT;
		}
	}
	return dMaxRT;
}

double _AMCMaxRTCalculator::CalculateRTTask(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	return WCRT(iTaskIndex, rcPriorityAssignment);
}

//_AMCRtbRTCalculator

_AMCRtbRTCalculator::_AMCRtbRTCalculator(TaskSet & rcTaskSet)
	:_LORTCalculator(rcTaskSet)
{

}

double _AMCRtbRTCalculator::RHS(int iTaskIndex, double dRT, double dBlocking, TaskSetPriorityStruct & rcPriorityAssignment, void * pvExtraData)
{
	double dLORT = *(double*)pvExtraData;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	my_assert(iTaskIndex < m_pcTaskSet->getTaskNum(), "Task Index Out Bounded");
	double dValue = pcTaskArray[iTaskIndex].getCHI() + dBlocking;
	int iPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)continue;
		int iThisPriority = rcPriorityAssignment.getPriorityByTask(i);
		if (iThisPriority < iPriority)
		{
			if (pcTaskArray[i].getCriticality())
			{
				dValue += ceil(dRT / pcTaskArray[i].getPeriod())*pcTaskArray[i].getCHI();
			}
			else
			{
				dValue += ceil(dLORT / pcTaskArray[i].getPeriod())*pcTaskArray[i].getCLO();
			}
		}
	}
	return dValue;
}

double _AMCRtbRTCalculator::CalculateRTTask(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	_LORTCalculator cLORTCalc(*m_pcTaskSet);
	double dLORT = cLORTCalc.CalculateRTTask(iTaskIndex, rcPriorityAssignment);
	if (!pcTaskArray[iTaskIndex].getCriticality())
		return dLORT;
	return CalculateTaskFP(iTaskIndex, 0.0, dLORT, pcTaskArray[iTaskIndex].getDeadline(), rcPriorityAssignment, &dLORT);
}