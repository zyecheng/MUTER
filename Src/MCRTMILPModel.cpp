#include "stdafx.h"
#include "MCRTMILPModel.h"
#include "ResponseTimeCalculator.h"
#include <assert.h>
#include <iostream>
#include "Util.h"
using namespace std;
#define BIGM 1e7
extern void my_assert(bool bCondition, char axInfo[]);

extern IloEnv cEnv;
MCRTMILPModelTaskModel::MCRTMILPModelTaskModel()
	: m_cPriorityVarArray(cEnv),//binary variable
	m_cLORTExpr(cEnv),//continuous varialbe
	m_cMixRTExpr(cEnv),
	m_cLOIINVarArray(cEnv),// IIN short for LO Criticality Interference Instantces Number variable(cEnv), Integer type
	m_cMixIINVarArray(cEnv),// Similar as above but for Mix Criticality(cEnv), Integer Type
	m_cLOPIINVarArray(cEnv),//to model multiplication of a p * I (binary multiply integer), another set of integer variable need to be included(cEnv), Integer Type
	m_cMixPIINVarArray(cEnv),//Similar as above but for Mix Criticality(cEnv), Integer Type	
	m_cLORTConstraint(cEnv),//Constraint for Modeling LO RT
	m_cMixRTConstraint(cEnv),//Constraint for Modeling HI RT
	m_cSchedConstraint(cEnv),//Constraint for schedulability
	m_cLOIINResultArray(cEnv),
	m_cLOPIINResultArray(cEnv),
	m_cMixIINResultArray(cEnv),
	m_cMixPIINResultArray(cEnv),
	m_cLORTVar(cEnv, IloNumVar::Float),
	m_cMixRTVar(cEnv, IloNumVar::Float)
{

}

MCRTMILPModelTaskModel::~MCRTMILPModelTaskModel()
{
	
}

void MCRTMILPModelTaskModel::DisplayConstraint()
{
	int iSize = 0;
	cout << "LORTExpr: " << m_cLORTExpr << endl;
	cout << "MixRTExpr: " << m_cMixRTExpr << endl;
	iSize = m_cLORTConstraint.getSize();
	for (int i = 0; i < iSize; i++)
	{
		cout << m_cLORTConstraint[i] << endl;
	}

	iSize = m_cMixRTConstraint.getSize();
	for (int i = 0; i < iSize; i++)
	{
		cout << m_cMixRTConstraint[i] << endl;
	}

	iSize = m_cSchedConstraint.getSize();
	for (int i = 0; i < iSize; i++)
	{
		cout << m_cSchedConstraint[i] << endl;
	}
}

MCRTMILPModel::MCRTMILPModel()
	:m_cPriorityConsistencyConstraint(cEnv),
	m_cRTDetVariables(cEnv),
	m_cRTDetVarConstraint(cEnv)
{
	m_pcTaskModelObj = NULL;
}


MCRTMILPModel::~MCRTMILPModel()
{
}

void MCRTMILPModel::Initialize(TaskSet & rcTaskSet)
{
	m_pcTargetTaskSet = &rcTaskSet;
	m_iTaskNum = rcTaskSet.getTaskNum();
	m_pcTaskModelObj = new MCRTMILPModelTaskModel[m_iTaskNum];
	IniTaskModelObj();
	IniPriorityVariable();
	BuildTaskLORTConstraint();
	BuildTaskMixRTConstraint();
	PriorityEnforcement();
}

void MCRTMILPModel::DisplayConstraints()
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		cout << "Task " << i << endl;
		m_pcTaskModelObj[i].DisplayConstraint();
		cout << endl;
	}
}

bool MCRTMILPModel::SolveModel(double dTimeout, int iShowWhileDisplay)
{	
	IloModel cModel(cEnv);
	cModel.add(m_cPriorityConsistencyConstraint);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		cModel.add(m_pcTaskModelObj[i].m_cLORTConstraint);
		cModel.add(m_pcTaskModelObj[i].m_cMixRTConstraint);
		cModel.add(m_pcTaskModelObj[i].m_cSchedConstraint);
	}	
	cModel.add(MemoryConstraint());	
	cModel.add(m_cRTDetVarConstraint);
	cModel.add(Objective());

	IloCplex cSolver(cModel);
	cSolver.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, 1e-9);
	cSolver.setParam(IloCplex::Param::MIP::Tolerances::Integrality, 1e-9);
	cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
	//cSolver.setParam(IloCplex::Param::Emphasis::Memory, 1);
	if (iShowWhileDisplay == 0)
		cSolver.setOut(cEnv.getNullStream());
	m_iSolveTime = clock();
	bool bStatus = cSolver.solve();
	m_iSolveTime = clock() - m_iSolveTime;
	if (bStatus)
	{
		ExtractResult(cSolver);
		my_assert(VerifyPriorityConsistency(cSolver),"Priority Consistency Failed");
		my_assert(VerifySchedulability(cSolver), "Schedulability Failed");
		//DisplayResult();
		m_dObjectiveValue = cSolver.getObjValue();
	}
	else
	{

	}
	return bStatus;
}

void MCRTMILPModel::ExtractResult(IloCplex & rcSolver)
{	
	for (int i = 0; i < m_iTaskNum; i++)
	{		
		ExtractResultTask(rcSolver,i);		
	}
}

void MCRTMILPModel::ExtractResultTask(IloCplex & rcSolver,int iTaskIndex)
{

	IloNumArray cResult(cEnv);
	rcSolver.getValues(m_pcTaskModelObj[iTaskIndex].m_cPriorityVarArray, cResult);
	rcSolver.getValues(m_pcTaskModelObj[iTaskIndex].m_cLOPIINVarArray, m_pcTaskModelObj[iTaskIndex].m_cLOPIINResultArray);
	rcSolver.getValues(m_pcTaskModelObj[iTaskIndex].m_cLOIINVarArray, m_pcTaskModelObj[iTaskIndex].m_cLOIINResultArray);
	if (m_pcTargetTaskSet->getTaskArrayPtr()[iTaskIndex].getCriticality())
	{
		rcSolver.getValues(m_pcTaskModelObj[iTaskIndex].m_cMixPIINVarArray, m_pcTaskModelObj[iTaskIndex].m_cMixPIINResultArray);
		rcSolver.getValues(m_pcTaskModelObj[iTaskIndex].m_cMixIINVarArray, m_pcTaskModelObj[iTaskIndex].m_cMixIINResultArray);
	}

	int iSize = cResult.getSize();
	m_pcTaskModelObj[iTaskIndex].m_iPriority = 1;	
	for (int j = 0; j < iSize; j++)
	{
		if (round(cResult[j]) == 1)
		{
			m_pcTaskModelObj[iTaskIndex].m_iPriority++;
		}
		int iOrigIndex = j >= iTaskIndex ? j + 1 : j;		
	}	
	m_pcTaskModelObj[iTaskIndex].m_dRTLO = rcSolver.getValue(m_pcTaskModelObj[iTaskIndex].m_cLORTVar);
	m_pcTaskModelObj[iTaskIndex].m_dRTMix = rcSolver.getValue(m_pcTaskModelObj[iTaskIndex].m_cMixRTVar);
	GET_TASKSET_NUM_PTR(m_pcTargetTaskSet, iTaskNum, pcTaskArray);
}

void MCRTMILPModel::DisplayResult()
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		printf_s("Task %d / Priority: %d \n", i, m_pcTaskModelObj[i].m_iPriority);
	}
}

bool MCRTMILPModel::VerifyPriorityConsistency(IloCplex & rcSolver)
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		for (int j = 0; j < m_iTaskNum; j++)
		{
			if (i == j)
				continue;
			if (m_pcTaskModelObj[i].m_iPriority < m_pcTaskModelObj[j].m_iPriority)
			{
				if (rcSolver.getValue(getPriorityVariable(i, j)) == 0)
				{
					return false;
				}
			}
			else if (m_pcTaskModelObj[i].m_iPriority > m_pcTaskModelObj[j].m_iPriority)
			{
				if (rcSolver.getValue(getPriorityVariable(i, j)) == 1)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

bool MCRTMILPModel::VerifyInterResultConsistency(IloCplex & rcSolver)
{
	return true;
}

bool MCRTMILPModel::VerifySchedulability(IloCplex & rcSolver)
{
	ResponseTimeCalculator cRTCalc;
	cRTCalc.Initialize(*m_pcTargetTaskSet);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		cRTCalc.setPrioirty(i, m_pcTaskModelObj[i].m_iPriority);
	}
	cRTCalc.Calculate(RTCALC_AMCRTB);
	//DisplayConstraints();
#if 0
	DisplayResult();	
	for (int i = 0; i < m_iTaskNum; i++)
	{
		cout << "Task " << i << " RT: " << cRTCalc.getResponseTime(i) << endl;
	}
#endif
	return !cRTCalc.getDeadlineMiss();
}

IloNumVar & MCRTMILPModel::getPriorityVariable(int i, int j)
{
	my_assert(i != j, "Illegal Access to Priority Variable");
	i = (i > j) ? i - 1 : i;
	return m_pcTaskModelObj[j].m_cPriorityVarArray[i];
}

void MCRTMILPModel::IniPriorityVariable()
{
	int iTaskNum = m_iTaskNum;
	char axTemp[30] = { 0 };
	//create the priority variable for each task
	for (int i = 0; i < iTaskNum; i++)
	{
		IloNumVarArray & rcVarArray = m_pcTaskModelObj[i].m_cPriorityVarArray;
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i != j)
			{
				sprintf_s(axTemp, "p(%d,%d)", j, i);
				rcVarArray.add(IloNumVar(cEnv, 0, 1, IloNumVar::Int, axTemp));
			}
		}
	}

	//add the priority consistency constraint
	// p(i,k) +p (k,j)-1 <= p(i,j)
	int iIndex = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i != j)
			{
				for (int k = 0; k < iTaskNum; k++)
				{
					//I just found out that there can be n^3 constraint?
					if ((k != i) && (k != j))
					{
						m_cPriorityConsistencyConstraint.add(getPriorityVariable(i, k) + getPriorityVariable(k, j) - getPriorityVariable(i, j) - 1 <= 0);
					}
				}
				iIndex++;
			}
		}
	}
	//p(i,j) + p(j,i) = 1
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = i + 1; j < iTaskNum; j++)
		{
			m_cPriorityConsistencyConstraint.add(getPriorityVariable(i, j) + getPriorityVariable(j, i) - 1 == 0);
		}
	}
}

void MCRTMILPModel::BuildTaskLORTConstraintTask(int iTaskIndex)
{
	MCRTMILPModelTaskModel & rcTaskModelObj = m_pcTaskModelObj[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	char axName[128] = { 0 };

	//LO Response Time Expression
	rcTaskModelObj.m_cLORTExpr += pcTaskArray[iTaskIndex].getCLO();
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (i == iTaskIndex)
			continue;
		int iAccessIndex = (i > iTaskIndex) ? i - 1 : i;
		rcTaskModelObj.m_cLORTExpr += pcTaskArray[i].getCLO() * rcTaskModelObj.m_cLOPIINVarArray[iAccessIndex];
	}

	//LO Interfrence Instance Number;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (i == iTaskIndex)
			continue;
		int iAccessIndex = (i > iTaskIndex) ? i - 1 : i;
		IloExpr cLOIINExpr_A(cEnv);
		IloExpr cLOIINExpr_B(cEnv);
		cLOIINExpr_A = rcTaskModelObj.m_cLOIINVarArray[iAccessIndex] - (rcTaskModelObj.m_cLORTVar / pcTaskArray[i].getPeriod() + 1);
		cLOIINExpr_B = rcTaskModelObj.m_cLOIINVarArray[iAccessIndex] - rcTaskModelObj.m_cLORTVar / pcTaskArray[i].getPeriod();
		sprintf_s(axName, "LOIIN(%d,%d,%c)", i, iTaskIndex, 'A');
		rcTaskModelObj.m_cLORTConstraint.add(IloRange(cEnv, -IloInfinity, cLOIINExpr_A, 0 - 1e-7,axName));
		sprintf_s(axName, "LOIIN(%d,%d,%c)", i, iTaskIndex, 'B');
		rcTaskModelObj.m_cLORTConstraint.add(IloRange(cEnv, 0, cLOIINExpr_B, IloInfinity,axName));
	}

	//LO PIIN 
	double dBigM = DetermineBigMLO(iTaskIndex);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (i == iTaskIndex)
			continue;
		int iAccessIndex = (i > iTaskIndex) ? i - 1 : i;
		IloExpr cLOIINExpr_A(cEnv);
		IloExpr cLOIINExpr_B(cEnv);
		IloExpr cLOIINExpr_C(cEnv);
		dBigM = ceil(pcTaskArray[iTaskIndex].getDeadline() / pcTaskArray[i].getPeriod());
		cLOIINExpr_A = rcTaskModelObj.m_cLOPIINVarArray[iAccessIndex] - rcTaskModelObj.m_cLOIINVarArray[iAccessIndex];
		cLOIINExpr_B = rcTaskModelObj.m_cLOPIINVarArray[iAccessIndex] - rcTaskModelObj.m_cLOIINVarArray[iAccessIndex] +
			dBigM * (1 - getPriorityVariable(i, iTaskIndex));
		cLOIINExpr_C = rcTaskModelObj.m_cLOPIINVarArray[iAccessIndex] - dBigM * getPriorityVariable(i, iTaskIndex);
		sprintf_s(axName, "LOPIIN(%d,%d,%c)", i, iTaskIndex, 'A');
		rcTaskModelObj.m_cLORTConstraint.add(IloRange(cEnv, -IloInfinity, cLOIINExpr_A, 0, axName));
		sprintf_s(axName, "LOPIIN(%d,%d,%c)", i, iTaskIndex, 'B');
		rcTaskModelObj.m_cLORTConstraint.add(IloRange(cEnv, 0, cLOIINExpr_B, IloInfinity, axName));
		sprintf_s(axName, "LOPIIN(%d,%d,%c)", i, iTaskIndex, 'C');
//		if (i == 2 && iTaskIndex == 0)
			//{cout << IloRange(cEnv, -IloInfinity, cLOIINExpr_C, 0, axName) << endl; system("pause");}
	//	rcTaskModelObj.m_cLORTConstraint.add(IloRange(cEnv, -IloInfinity, cLOIINExpr_C, 0,axName));
	}

	//Schedulability Constraint
	sprintf_s(axName, "LOSched(%d)", iTaskIndex);
	rcTaskModelObj.m_cSchedConstraint.add(IloRange(cEnv, -IloInfinity, rcTaskModelObj.m_cLORTExpr, pcTaskArray[iTaskIndex].getDeadline(),axName));	
	rcTaskModelObj.m_cSchedConstraint.add(IloRange(cEnv, 0.0, rcTaskModelObj.m_cLORTExpr - rcTaskModelObj.m_cLORTVar, 0.0));
	if (!pcTaskArray[iTaskIndex].getCriticality())
		rcTaskModelObj.m_cSchedConstraint.add(IloRange(cEnv, 0.0, rcTaskModelObj.m_cLORTExpr - rcTaskModelObj.m_cMixRTVar, 0.0));
}

void MCRTMILPModel::BuildTaskMixRTConstraintTask(int iTaskIndex)
{
	MCRTMILPModelTaskModel & rcTaskModelObj = m_pcTaskModelObj[iTaskIndex];
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	char axName[128] = { 0 };
	my_assert(rcTaskModelObj.m_cMixIINVarArray.getSize() > 0, "Mix Response Time Variables Should not be Empty for HI Task");
	my_assert(rcTaskModelObj.m_cMixPIINVarArray.getSize() > 0, "Mix Response Time Variables Should not be Empty for HI Task");
	//Mix Response Time Expression
	rcTaskModelObj.m_cMixRTExpr += pcTaskArray[iTaskIndex].getCHI();

	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (i == iTaskIndex)
			continue;
		int iAccessIndex = (i > iTaskIndex) ? i - 1 : i;
		if (pcTaskArray[i].getCriticality())
		{
			rcTaskModelObj.m_cMixRTExpr += pcTaskArray[i].getCHI() * rcTaskModelObj.m_cMixPIINVarArray[iAccessIndex];
		}
		else
		{
			rcTaskModelObj.m_cMixRTExpr += pcTaskArray[i].getCLO() * rcTaskModelObj.m_cLOPIINVarArray[iAccessIndex];
		}		
	}

	//INN
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (i == iTaskIndex)
			continue;
		int iAccessIndex = (i>iTaskIndex) ? i - 1 : i;
		//HI Task
		if (pcTaskArray[i].getCriticality())
		{
			IloExpr cIINExpr_A(cEnv);
			IloExpr cIINExpr_B(cEnv);
			cIINExpr_B = rcTaskModelObj.m_cMixIINVarArray[iAccessIndex] - rcTaskModelObj.m_cMixRTExpr / pcTaskArray[i].getPeriod();
			cIINExpr_A = rcTaskModelObj.m_cMixIINVarArray[iAccessIndex] - (rcTaskModelObj.m_cMixRTExpr / pcTaskArray[i].getPeriod() + 1);
			sprintf_s(axName, "MixIIN(%d,%d,%c)", i, iTaskIndex, 'A');
			rcTaskModelObj.m_cMixRTConstraint.add(IloRange(cEnv, -IloInfinity, cIINExpr_A, 0 - 1e-7, axName));
			sprintf_s(axName, "MixIIN(%d,%d,%c)", i, iTaskIndex, 'B');
			rcTaskModelObj.m_cMixRTConstraint.add(IloRange(cEnv, 0, cIINExpr_B, IloInfinity, axName));
		}
	}

	//PINN
	double dBigM = DetermineBigMHI(iTaskIndex);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (i == iTaskIndex)
			continue;

	//	if (!pcTaskArray[i].getCriticality())
//			continue;

		int iAccessIndex = (i>iTaskIndex) ? i - 1 : i;
		IloExpr cMixIINExpr_A(cEnv);
		IloExpr cMixIINExpr_B(cEnv);
		IloExpr cMixIINExpr_C(cEnv);
		dBigM = ceil(pcTaskArray[iTaskIndex].getDeadline() / pcTaskArray[i].getPeriod());
		cMixIINExpr_A = rcTaskModelObj.m_cMixPIINVarArray[iAccessIndex] - rcTaskModelObj.m_cMixIINVarArray[iAccessIndex];
		cMixIINExpr_B = rcTaskModelObj.m_cMixPIINVarArray[iAccessIndex] - rcTaskModelObj.m_cMixIINVarArray[iAccessIndex] +
			dBigM * (1 - getPriorityVariable(i, iTaskIndex));
		cMixIINExpr_C = rcTaskModelObj.m_cMixPIINVarArray[iAccessIndex] - dBigM * getPriorityVariable(i, iTaskIndex);
		sprintf_s(axName, "MixPIIN(%d,%d,%c)", i, iTaskIndex, 'A');
		rcTaskModelObj.m_cMixRTConstraint.add(IloRange(cEnv, -IloInfinity, cMixIINExpr_A, 0, axName));
		sprintf_s(axName, "MixPIIN(%d,%d,%c)", i, iTaskIndex, 'B');
		rcTaskModelObj.m_cMixRTConstraint.add(IloRange(cEnv, 0, cMixIINExpr_B, IloInfinity, axName));
		sprintf_s(axName, "MixPIIN(%d,%d,%c)", i, iTaskIndex, 'C');
		//rcTaskModelObj.m_cMixRTConstraint.add(IloRange(cEnv, -IloInfinity, cMixIINExpr_C, 0, axName));
	}

	//Schedulability Constraint
	sprintf_s(axName, "MixSched(%d)", iTaskIndex);
	rcTaskModelObj.m_cSchedConstraint.add(IloRange(cEnv, -IloInfinity, rcTaskModelObj.m_cMixRTExpr, pcTaskArray[iTaskIndex].getDeadline(), axName));
	rcTaskModelObj.m_cSchedConstraint.add(IloRange(cEnv, 0.0, rcTaskModelObj.m_cMixRTExpr - rcTaskModelObj.m_cMixRTVar, 0.0));
}

void MCRTMILPModel::BuildTaskLORTConstraint()
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		BuildTaskLORTConstraintTask(i);
	}
}

void MCRTMILPModel::BuildTaskMixRTConstraint()
{
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (pcTaskArray[i].getCriticality())
		{
			BuildTaskMixRTConstraintTask(i);
		}		
	}
}

void MCRTMILPModel::IniTaskModelObj()
{
	char axName[128] = { 0 };
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();	
	for (int i = 0; i < m_iTaskNum; i++)
	{
		//Mainly Initialize The variable array
		sprintf_s(axName, "RTLO%d", i);
		m_pcTaskModelObj[i].m_cLORTVar.setName(axName);
		for (int j = 0; j < m_iTaskNum; j++)
		{
			if (j == i)
				continue;			
			sprintf_s(axName, "LOINN(%d,%d)", j, i);
			m_pcTaskModelObj[i].m_cLOIINVarArray.add(IloNumVar(cEnv, 0, IloInfinity, IloNumVar::Int, axName));
			sprintf_s(axName, "LOPINN(%d,%d)", j, i);
			m_pcTaskModelObj[i].m_cLOPIINVarArray.add(IloNumVar(cEnv, 0, IloInfinity, IloNumVar::Float, axName));			
		}
		if (pcTaskArray[i].getCriticality())
		{
			sprintf_s(axName, "RTMix%d", i);
			m_pcTaskModelObj[i].m_cMixRTVar.setName(axName);
			//Mainly Initialize The variable array
			for (int j = 0; j < m_iTaskNum; j++)
			{
				if (j == i)
					continue;
				sprintf_s(axName, "MixINN(%d,%d)", j, i);
				m_pcTaskModelObj[i].m_cMixIINVarArray.add(IloNumVar(cEnv, 0, IloInfinity, IloNumVar::Int, axName));
				sprintf_s(axName, "MixPINN(%d,%d)", j, i);
				m_pcTaskModelObj[i].m_cMixPIINVarArray.add(IloNumVar(cEnv, 0, IloInfinity, IloNumVar::Float, axName));
			}
		}
	}
}

void MCRTMILPModel::CalculateResponseTime()
{

}

IloRange MCRTMILPModel::MemoryConstraint()
{
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	char axName[128] = { 0 };
	IloExpr cConstExpression(cEnv);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		set<Task::CommunicationLink> & rsetSucessor = pcTaskArray[i].getSuccessorSet();
		for (set<Task::CommunicationLink>::iterator iter = rsetSucessor.begin();
			iter != rsetSucessor.end(); iter++)
		{
			int iSrcIndex = i;
			int iDstIndex = iter->m_iTargetTask;
			if (pcTaskArray[iSrcIndex].getPeriod() > pcTaskArray[iDstIndex].getPeriod())
			{
				//LH
				cConstExpression += (1 - getPriorityVariable(iSrcIndex, iDstIndex)) * iter->m_dMemoryCost;
			}
			else
			{
				sprintf_s(axName, "z(%d,%d)", iSrcIndex, iDstIndex);
				m_cRTDetVariables.add(IloNumVar(cEnv, 0, 1, IloNumVar::Int, axName));
				m_cRTDetVarConstraint.add(IloRange(cEnv, -IloInfinity, 
					m_pcTaskModelObj[iDstIndex].m_cMixRTVar -
					m_cRTDetVariables[m_cRTDetVariables.getSize() - 1] * BIGM - pcTaskArray[iSrcIndex].getDeadline(),
					0.0)
					);
				cConstExpression += m_cRTDetVariables[m_cRTDetVariables.getSize() - 1] * iter->m_dMemoryCost;
			}			
		}
	}
	cout << "Available Memory: " << m_pcTargetTaskSet->getAvailableMemory() << endl;	
	//cout << IloRange(cEnv, -IloInfinity, cConstExpression, m_pcTargetTaskSet->getAvailableMemory(), "MemoryConstraint") << endl;
	return IloRange(cEnv, -IloInfinity, cConstExpression, m_pcTargetTaskSet->getAvailableMemory(), "MemoryConstraint");
}

double MCRTMILPModel::ComputeMemoryCost()
{	
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	char axName[128] = { 0 };
	IloExpr cConstExpression(cEnv);
	double dMemoryCost = 0;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		set<Task::CommunicationLink> & rsetSucessor = pcTaskArray[i].getSuccessorSet();
		for (set<Task::CommunicationLink>::iterator iter = rsetSucessor.begin();
			iter != rsetSucessor.end(); iter++)
		{
			int iSrcIndex = i;
			int iDstIndex = iter->m_iTargetTask;
			if (pcTaskArray[iSrcIndex].getPeriod() > pcTaskArray[iDstIndex].getPeriod())
			{
				//LH
				//cConstExpression += (1 - getPriorityVariable(iSrcIndex, iDstIndex)) * iter->m_dMemoryCost;
				dMemoryCost += (1 - getPriorityResult(iSrcIndex, iDstIndex)) * iter->m_dMemoryCost;
			}
			else
			{				
				//cConstExpression += m_cRTDetVariables[m_cRTDetVariables.getSize() - 1] * iter->m_dMemoryCost;				
				if (!pcTaskArray[i].getCriticality())
				{
					//assert(0);
					assert(DOUBLE_EQUAL(m_pcTaskModelObj[iDstIndex].m_dRTLO, m_pcTaskModelObj[iDstIndex].m_dRTMix, 1e-6));
				}					
				dMemoryCost += (m_pcTaskModelObj[iDstIndex].m_dRTMix <= pcTaskArray[iSrcIndex].getDeadline()) ? 0.0 : iter->m_dMemoryCost;
			}
		}
	}
	return dMemoryCost;
}

void MCRTMILPModel::PriorityEnforcement()
{
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	IloExpr cConstExpression(cEnv);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		set<Task::CommunicationLink> & rsetSucessor = pcTaskArray[i].getSuccessorSet();
		for (set<Task::CommunicationLink>::iterator iter = rsetSucessor.begin();
			iter != rsetSucessor.end(); iter++)
		{
			int iSrcIndex = i;
			int iDstIndex = iter->m_iTargetTask;
			if (pcTaskArray[iSrcIndex].getPeriod() <= pcTaskArray[iDstIndex].getPeriod())
			{
				m_cPriorityConsistencyConstraint.add(getPriorityVariable(iSrcIndex, iDstIndex) == 1);
			}
		}
	}
}

IloObjective MCRTMILPModel::Objective()
{
	IloExpr cObjExpression(cEnv);
	Task * pcTaskArray = m_pcTargetTaskSet->getTaskArrayPtr();
	IloExpr cConstExpression(cEnv);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		set<Task::CommunicationLink> & rsetSucessor = pcTaskArray[i].getSuccessorSet();
		for (set<Task::CommunicationLink>::iterator iter = rsetSucessor.begin();
			iter != rsetSucessor.end(); iter++)
		{
			int iSrcIndex = i;
			int iDstIndex = iter->m_iTargetTask;
			if (pcTaskArray[iSrcIndex].getPeriod() > pcTaskArray[iDstIndex].getPeriod())
			{
				cObjExpression += (1 - getPriorityVariable(iSrcIndex, iDstIndex)) * iter->m_dDelayCost;
			}
		}
	}
	return IloObjective(cEnv, cObjExpression, IloObjective::Minimize, "Minimize Delay");
}

double MCRTMILPModel::DetermineBigMHI(int iTaskIndex)
{
	GET_TASKSET_NUM_PTR(m_pcTargetTaskSet, iTaskNum, pcTaskArray);
	assert(pcTaskArray[iTaskIndex].getCriticality());
	double dBigM = 0;
	dBigM += pcTaskArray[iTaskIndex].getCHI();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex) continue;
		int iIN = ceil(pcTaskArray[iTaskIndex].getDeadline() / pcTaskArray[i].getPeriod());
		dBigM += pcTaskArray[i].getCriticality() ? pcTaskArray[i].getCHI() * iIN : pcTaskArray[i].getCLO() * iIN;
	}
	return dBigM;
}

double MCRTMILPModel::DetermineBigMLO(int iTaskIndex)
{
	GET_TASKSET_NUM_PTR(m_pcTargetTaskSet, iTaskNum, pcTaskArray);
	//assert(pcTaskArray[iTaskIndex].getCriticality());
	double dBigM = 0;
	dBigM += pcTaskArray[iTaskIndex].getCLO();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex) continue;
		int iIN = ceil(pcTaskArray[iTaskIndex].getDeadline() / pcTaskArray[i].getPeriod());
		dBigM += pcTaskArray[i].getCLO() * iIN;
	}
	return dBigM;
}

double MCRTMILPModel::getPriorityResult(int iTaskA, int iTaskB)
{
	assert(iTaskA != iTaskB);
	if (m_pcTaskModelObj[iTaskA].m_iPriority < m_pcTaskModelObj[iTaskB].m_iPriority)
	{
		return 1.0;
	}
	else
		return 0.0;
}

//Minimize LORT Model

MinimizeLORTModel::MinimizeLORTModel(TaskSet & rcTaskSet)	
{

}

MinimizeLORTModel::MinimizeLORTModel()
{

}

bool MinimizeLORTModel::SolveModel(double dTimeout, int iShowWhileDisplay)
{
	GET_TASKSET_NUM_PTR(m_pcTargetTaskSet, iTaskNum, pcTaskArray);
	IloModel cModel(cEnv);
	cModel.add(m_cPriorityConsistencyConstraint);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		cModel.add(m_pcTaskModelObj[i].m_cLORTConstraint);
		//cModel.add(m_pcTaskModelObj[i].m_cMixRTConstraint);
		cModel.add(m_pcTaskModelObj[i].m_cSchedConstraint);		
	}	
	IloObjective cObjective = Objective();
	IloRangeArray cConst(cEnv);
#if 0	
	IloEnv & cSolveEnv = cEnv;
	IloNumVar cMinSlack(cSolveEnv, 0.0, IloInfinity, IloNumVar::Float);
	for (int i = 0; i < iTaskNum; i++)
	{
		cConst.add(IloRange(cSolveEnv, 0.0, (pcTaskArray[i].getDeadline() - m_pcTaskModelObj[i].m_cLORTVar) - cMinSlack, IloInfinity));
	}
	cObjective = IloMinimize(cSolveEnv, -1.0 * cMinSlack);
#endif

	//cModel.add(Objective());
	cModel.add(cConst);
	cModel.add(cObjective);	

	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			if (pcTaskArray[i].getPeriod() < pcTaskArray[j].getPeriod())
			{
				//cModel.add(IloRange(cEnv, 1.0, getPriorityVariable(i, j), 1.0));
			}			
		}		
	}

	IloCplex cSolver(cModel);
	cSolver.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, 1e-9);
	cSolver.setParam(IloCplex::Param::MIP::Tolerances::Integrality, 1e-9);
	cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
	//cSolver.setParam(IloCplex::Param::Emphasis::Memory, 1);
	if (iShowWhileDisplay == 0)
		cSolver.setOut(cEnv.getNullStream());
	m_iSolveTime = clock();
	bool bStatus = cSolver.solve();
	m_iSolveTime = clock() - m_iSolveTime;
	if (bStatus)
	{
		ExtractResult(cSolver);
		my_assert(VerifyPriorityConsistency(cSolver), "Priority Consistency Failed");		
		my_assert(VerifyObjective(), "Objective Failed");
		//DisplayResult();
		m_dObjectiveValue = cSolver.getObjValue();
	}
	else
	{
#if 0
		AudsleyEngine<SchedTest_AMCRTB> cSchedTest;
		cSchedTest.Initialize(*m_pcTargetTaskSet, NULL);
		assert(!cSchedTest.Run());
		ofstream ofstreamModel("ModelTemp.txt", ios::out);
		ofstreamModel << cModel << endl;
		ofstreamModel.close();
#endif
	}
	return bStatus;
}

IloObjective MinimizeLORTModel::Objective()
{
	int iConsiderationLimit = 200;
	GET_TASKSET_NUM_PTR(m_pcTargetTaskSet, iTaskNum, pcTaskArray);
	IloExpr cObjExpr(cEnv);
	iTaskNum = min(m_iTaskNum, iConsiderationLimit);
	for (int i = 0; i < iTaskNum; i++)	
	{
		cObjExpr += m_pcTaskModelObj[i].m_cLORTVar;
	}
	return IloMinimize(cEnv, cObjExpr);
}

bool MinimizeLORTModel::VerifyObjective()
{
	int iConsiderationLimit = 200;
	if (0)
	{
		ResponseTimeCalculator cRTCalc;
		cRTCalc.Initialize(*m_pcTargetTaskSet);
		for (int i = 0; i < m_iTaskNum; i++)
		{
			cRTCalc.setPrioirty(i, m_pcTaskModelObj[i].m_iPriority);
		}
		cRTCalc.Calculate(RTCALC_LO);
		//DisplayConstraints();
#if 1
		double dObjective = 0;
		int iTaskNum = min(m_iTaskNum, iConsiderationLimit);
		for (int i = 0; i < iTaskNum; i++)
		{
			cout << "Task " << i << " RT: " << cRTCalc.getResponseTime(i) << endl;
			dObjective += cRTCalc.getResponseTime(i);
		}
		cout << "Solution Objective: " << dObjective << endl;
		assert(!cRTCalc.getDeadlineMiss());
#endif
		return !cRTCalc.getDeadlineMiss();
	}

	{
		_LORTCalculator cCalc(*m_pcTargetTaskSet);
		TaskSetPriorityStruct cPriorityStruct(*m_pcTargetTaskSet);
		for (int i = 0; i < m_iTaskNum; i++)
		{
			cPriorityStruct.setPriority(i, m_pcTaskModelObj[i].m_iPriority - 1);
		}

		double dObjective = 0;
		//int iTaskNum = min(m_iTaskNum, iConsiderationLimit);
		int iTaskNum = m_iTaskNum;
		for (int i = 0; i < iTaskNum; i++)
		{
			double dRT = cCalc.CalculateRTTask(i, cPriorityStruct);
			if (i < iConsiderationLimit)
			{
				cout << "Task " << i << " RT: " <<  dRT << endl;
				dObjective += dRT;
			}
			assert(DOUBLE_GE(m_pcTargetTaskSet->getTaskArrayPtr()[i].getDeadline(), dRT, 1e-6));
		}
		cout << "Solution Objective: " << dObjective << endl;
		return true;
	}
}