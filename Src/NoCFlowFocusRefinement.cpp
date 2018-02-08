#include "stdafx.h"
#include "NoCFlowFocusRefinement.h"
#include "Util.h"
#include <direct.h>
#include <mutex>
#include <list>
//#include <windows.h>
#include <thread>
#include <mutex>
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
bool NoCFlowFocusRefinement::SchedCondComp::operator() (const SchedCondElement & rcLHS, const SchedCondElement & rcRHS)
{
	if (rcLHS.iCondType == rcRHS.iCondType)
	{
		if (rcLHS.iTaskIndex == rcRHS.iTaskIndex)
		{
			return rcLHS.iValue < rcRHS.iValue;
		}
		else
			return rcLHS.iTaskIndex < rcRHS.iTaskIndex;
	}
	else
		return rcLHS.iCondType < rcRHS.iCondType;
}

NoCFlowFocusRefinement::NoCFlowFocusRefinement()
{
}

NoCFlowFocusRefinement::NoCFlowFocusRefinement(FlowsSet_2DNetwork & rcFlowsSet)
	:GeneralFocusRefinement(rcFlowsSet)
{

}


NoCFlowFocusRefinement::~NoCFlowFocusRefinement()
{
}

void NoCFlowFocusRefinement::CreateJitterRTVariable(IloNumVarArray & rcJitterLOVars, 
	IloNumVarArray & rcJitterHIVars, IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcJitterHIVars.getEnv();
	char axBuffer[128] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		sprintf_s(axBuffer, "JHI(%d)", i);
		rcJitterHIVars.add(IloNumVar(rcEnv, 0.0, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCHI(), IloNumVar::Float, axBuffer));

		sprintf_s(axBuffer, "JLO(%d)", i);
		rcJitterLOVars.add(IloNumVar(rcEnv, 0.0, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO(), IloNumVar::Float, axBuffer));

		sprintf_s(axBuffer, "RTLO(%d)", i);
		rcRTLOVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));

		sprintf_s(axBuffer, "RTHI(%d)", i);
		rcRTHIVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCHI(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));
	}
}

IloNumVar & NoCFlowFocusRefinement::getJitterHI(int iTaskIndex, IloNumVarArray & rcJitterHIVars)
{
	assert(iTaskIndex < rcJitterHIVars.getSize());
	return rcJitterHIVars[iTaskIndex];
}

IloNumVar & NoCFlowFocusRefinement::getJitterLO(int iTaskIndex, IloNumVarArray & rcJitterLOVars)
{
	assert(iTaskIndex < rcJitterLOVars.getSize());
	return rcJitterLOVars[iTaskIndex];
}

IloNumVar & NoCFlowFocusRefinement::getRTLO(int iTaskIndex, IloNumVarArray & rcRTLOVars)
{
	assert(iTaskIndex < rcRTLOVars.getSize());
	return rcRTLOVars[iTaskIndex];
}

IloNumVar & NoCFlowFocusRefinement::getRTHI(int iTaskIndex, IloNumVarArray & rcRTHIVars)
{
	assert(iTaskIndex < rcRTHIVars.getSize());
	return rcRTHIVars[iTaskIndex];
}

void NoCFlowFocusRefinement::CreateSchedCondVars(const IloEnv & rcEnv, SchedCondVars & rcSchedCondVars, UnschedCores & rcUnschedCores)
{
	char axBuffer[128] = { 0 };
	for (UnschedCores::iterator iterCore = rcUnschedCores.begin(); iterCore != rcUnschedCores.end(); iterCore++)
	{
		for (UnschedCore::iterator iter = iterCore->begin(); iter != iterCore->end(); iter++)
		{			
			if (rcSchedCondVars.count(*iter) == 1)	continue;
			sprintf_s(axBuffer, "SC(%d, %d, %d)", iter->iCondType, iter->iTaskIndex, iter->iValue);
			rcSchedCondVars[*iter] = IloNumVar(rcEnv, 0.0, 1.0, IloNumVar::Int, axBuffer);
		}
	}
}

IloNumVar & NoCFlowFocusRefinement::getSchedCondVars(const SchedCondElement & rcElement, SchedCondVars & rcSchedCondVars)
{
	SchedCondVars::iterator iterFind = rcSchedCondVars.find(rcElement);
	assert(iterFind != rcSchedCondVars.end());
	return iterFind->second;
}

void NoCFlowFocusRefinement::GenJitterRTConst(IloNumVarArray & rcJitterLOVars, 
	IloNumVarArray & rcJitterHIVars, IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcJitterHIVars.getEnv();
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		rcConst.add(IloRange(rcEnv, 0.0, rcJitterLOVars[i] - (rcRTLOVars[i] - pcTaskArray[i].getCLO()), 0.0));
		if (pcTaskArray[i].getCriticality())
		{
			rcConst.add(IloRange(rcEnv, 0.0, rcJitterHIVars[i] - (rcRTHIVars[i] - pcTaskArray[i].getCLO()), 0.0));
		}		
	}
}

void NoCFlowFocusRefinement::GenJitterRTSchedCondVarConst(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars, 
	IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, SchedCondVars & rcSchedCondVars, UnschedCores & rcUnschedCores, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcJitterHIVars.getEnv();
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (SchedCondVars::iterator iter = rcSchedCondVars.begin(); iter != rcSchedCondVars.end(); iter++)
	{
		int iTaskIndex = iter->first.iTaskIndex;
		IloNumVar & rcJitterHI = getJitterHI(iTaskIndex, rcJitterHIVars);
		IloNumVar & rcJitterLO = getJitterLO(iTaskIndex, rcJitterLOVars);
		IloNumVar & rcRTHI = getRTHI(iTaskIndex, rcRTHIVars);
		IloNumVar & rcRTLO = getRTLO(iTaskIndex, rcRTLOVars);
		IloNumVar & rcSchedCondVar = getSchedCondVars(iter->first, rcSchedCondVars);
		switch (iter->first.iCondType)
		{
		case  SchedCondElement::JitterHI:
			if (pcTaskArray[iTaskIndex].getCriticality())
			{
				rcConst.add(IloRange(rcEnv, -IloInfinity,
					rcJitterHI - (iter->first.iValue - 1) * rcSchedCondVar - (1.0 - rcSchedCondVar) * rcJitterHI.getUB(),
					0.0));
			}				
			break;
		case  SchedCondElement::JitterLO:
			rcConst.add(IloRange(rcEnv, -IloInfinity,
				rcJitterLO - (iter->first.iValue - 1) * rcSchedCondVar - (1.0 - rcSchedCondVar) * rcJitterLO.getUB(),
				0.0));
			break;
		case  SchedCondElement::ResponseTimeLO:
			rcConst.add(IloRange(rcEnv, 0.0, rcRTLO - (iter->first.iValue + 1) * rcSchedCondVar, IloInfinity));
			break;
		case  SchedCondElement::ResponseTimeHI:
			if (pcTaskArray[iTaskIndex].getCriticality())
			{
				rcConst.add(IloRange(rcEnv, 0.0, rcRTHI - (iter->first.iValue + 1) * rcSchedCondVar, IloInfinity));
			}				
			break;
		default:
			assert(0);
			break;
		}
	}

	for (UnschedCores::iterator iter = rcUnschedCores.begin(); iter != rcUnschedCores.end(); iter++)
	{
		int iSize = iter->size();
		IloExpr cExpr(rcEnv);
		for (UnschedCore::iterator iterElement = iter->begin(); iterElement != iter->end(); iterElement++)
		{
			cExpr += getSchedCondVars(*iterElement, rcSchedCondVars);
		}
		rcConst.add(IloRange(rcEnv, 1.0, cExpr, iSize));
	}
}

int NoCFlowFocusRefinement::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout)
{
	try
	{
		IloEnv cSolveEnv;
		SchedCondVars cSchedCondVars;
		IloNumVarArray cJitterHI(cSolveEnv), cJitterLO(cSolveEnv), cRTLO(cSolveEnv), cRTHI(cSolveEnv);
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);
		CreateJitterRTVariable(cJitterLO, cJitterHI, cRTLO, cRTHI);		
		CreateSchedCondVars(cSolveEnv, cSchedCondVars, rcGenUC);

		GenJitterRTConst(cJitterLO, cJitterHI, cRTLO, cRTHI, cConst);
		GenJitterRTSchedCondVarConst(cJitterLO, cJitterHI, cRTLO, cRTHI, cSchedCondVars, rcGenUC, cConst);
		//GenSchedCondVarDominanceConst(cSchedCondVars, cConst);

		IloExpr cObjExpr(cSolveEnv);
		IloNumVar cMinSlack(cSolveEnv, 0.0, IloInfinity, IloNumVar::Float);
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);

		if (1)
		{
			for (int i = 1; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
					cConst.add(IloRange(cSolveEnv, 0.0, (pcTaskArray[i].getDeadline() - cRTHI[i]) - cMinSlack, IloInfinity));
				else
					cConst.add(IloRange(cSolveEnv, 0.0, (pcTaskArray[i].getDeadline() - cRTLO[i]) - cMinSlack, IloInfinity));
			}
			cObjExpr = -cMinSlack;
		}

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);		
		//cModel.add(IloMinimize(cSolveEnv, cObjExpr));		
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
			
			ExtractSolutionAll(cJitterLO, cJitterHI, cRTLO, cRTHI, cSolver, m_dequeSolutionSchedConds);
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

void NoCFlowFocusRefinement::ExtractSolution(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
	IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloCplex & rcSolver, int iSolIndex, SolutionSchedCondsDeque & rdequeSolutionSchedConds)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	SchedConds cSchedCondsConfig;
	for (int i = 0; i < iTaskNum; i++)
	{
		int iTaskIndex = i;
		IloNumVar & rcJitterHI = getJitterHI(iTaskIndex, rcJitterHIVars);
		IloNumVar & rcJitterLO = getJitterLO(iTaskIndex, rcJitterLOVars);
		IloNumVar & rcRTHI = getRTHI(iTaskIndex, rcRTHIVars);
		IloNumVar & rcRTLO = getRTLO(iTaskIndex, rcRTLOVars);

		cSchedCondsConfig.insert({ SchedCondElement::JitterLO, iTaskIndex, round(rcSolver.getValue(rcJitterLO, iSolIndex)) });
		cSchedCondsConfig.insert({ SchedCondElement::ResponseTimeLO, iTaskIndex, round(rcSolver.getValue(rcRTLO, iSolIndex)) });

		if (pcTaskArray[i].getCriticality())
		{
			cSchedCondsConfig.insert({ SchedCondElement::JitterHI, iTaskIndex, round(rcSolver.getValue(rcJitterHI, iSolIndex)) });
			cSchedCondsConfig.insert({ SchedCondElement::ResponseTimeHI, iTaskIndex, round(rcSolver.getValue(rcRTHI, iSolIndex)) });
		}
	}	
	rdequeSolutionSchedConds.push_back({ rcSolver.getObjValue(iSolIndex), cSchedCondsConfig });
}

void NoCFlowFocusRefinement::ExtractSolutionAll(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
	IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloCplex & rcSolver, SolutionSchedCondsDeque & rdequeSolutionSchedConds)
{
	int iNSol = rcSolver.getSolnPoolNsolns();
	for (int i = 0; i < iNSol; i++)
	{
		ExtractSolution(rcJitterLOVars, rcJitterHIVars, rcRTLOVars, rcRTHIVars, rcSolver, i, rdequeSolutionSchedConds);
	}
}

void NoCFlowFocusRefinement::OptimalHook()
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	NoCFlowAMCRTCalculator_DerivedJitter cExactRTCalc(*m_pcTaskSet, m_cPriorityAssignmentSol, m_pcTaskSet->getDirectJitterTable());	
	cExactRTCalc.ComputeAllResponseTime();
	if (!cExactRTCalc.IsSchedulable())
	{
		for (int i = 0; i < iTaskNum; i++)
		{
			printf_s("Task %d \t CL: %d, P: %d,  JitterLO: %.2f, JitterHI: %.2f, RTLO: %.2f, RTHI: %.2f, Deadline: %.2f\n",
				i, pcTaskArray[i].getCriticality(), m_cPriorityAssignmentSol.getPriorityByTask(i), cExactRTCalc.getJitterLO(i), cExactRTCalc.getJitterHI(i), cExactRTCalc.getRTLO(i), cExactRTCalc.getRTHI(i), pcTaskArray[i].getDeadline());
		}
		cout << "Schedulability Failed" << endl;
		while (1);
	}	
}

void NoCFlowFocusRefinement::PrintExtraInfoToLog(ofstream & ofstreamLogFile)
{
	if (m_dequeIterationLogs.back().enumState != GeneralFocusRefinement<NoCFlowRTJUnschedCoreComputer>::Optimal)
		return;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	ofstreamLogFile << endl << "Exact Response Time and Jitter" << endl;
	NoCFlowAMCRTCalculator_DerivedJitter cExactRTCalc(*m_pcTaskSet, m_cPriorityAssignmentSol, m_pcTaskSet->getDirectJitterTable());
	cExactRTCalc.ComputeAllResponseTime();
	for (int i = 0; i < iTaskNum; i++)
	{
		ofstreamLogFile << "**********************************" << endl;
		ofstreamLogFile << "Task " << i << endl;
		ofstreamLogFile << "Priority: " << m_cPriorityAssignmentSol.getPriorityByTask(i) << endl;
		ofstreamLogFile << "Criticality: " << pcTaskArray[i].getCriticality() << endl;
		ofstreamLogFile << "JitterLO: " << cExactRTCalc.getJitterLO(i) << endl;
		ofstreamLogFile << "JitterHI: " << cExactRTCalc.getJitterHI(i) << endl;
		ofstreamLogFile << "RTLO: " << cExactRTCalc.getRTLO(i) << endl;
		ofstreamLogFile << "RTHI: " << cExactRTCalc.getRTHI(i) << endl;
		ofstreamLogFile << "Deadline: " << pcTaskArray[i].getDeadline() << endl;
	}
}

void NoCFlowFocusRefinement::GenSchedCondVarDominanceConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst)
{
	const IloEnv & rcEnv = rcConst.getEnv();
	for (SchedCondVars::iterator iter = rcSchedCondVars.begin(); iter != rcSchedCondVars.end(); iter++)
	{
		SchedCondVars::iterator iterPost = iter;
		iterPost++;
		if (iterPost == rcSchedCondVars.end()) break;
		if ((iterPost->first.iTaskIndex == iter->first.iTaskIndex) && (iterPost->first.iCondType == iter->first.iCondType))
		{
			if ((iterPost->first.iCondType == NoCFlowRTJSchedCond::ResponseTimeHI) || (iterPost->first.iCondType == NoCFlowRTJSchedCond::ResponseTimeLO))
			{
				rcConst.add(IloRange(rcEnv, 0.0, iter->second - iterPost->second, IloInfinity));
			}

			if ((iterPost->first.iCondType == NoCFlowRTJSchedCond::JitterHI) || (iterPost->first.iCondType == NoCFlowRTJSchedCond::JitterLO))
			{
				rcConst.add(IloRange(rcEnv, 0.0, iterPost->second - iter->second, IloInfinity));
			}
			
		}
	}
}

//Full MILP model

NoCFlowRTMILPModel::NoCFlowRTMILPModel(FlowsSet_2DNetwork & rcFlowSet)
{
	m_pcFlowSet = &rcFlowSet;
	m_pcPredefinePriorityAssignment = NULL;
	m_enumObjType = None;
}

NoCFlowRTMILPModel::NoCFlowRTMILPModel()
{
	m_pcPredefinePriorityAssignment = NULL;
	m_enumObjType = None;
}

NoCFlowRTMILPModel::~NoCFlowRTMILPModel()
{

}

void NoCFlowRTMILPModel::CreateJitterRTVariable(IloNumVarArray & rcJitterLOVars,
	IloNumVarArray & rcJitterHIVars, IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcJitterHIVars.getEnv();
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		sprintf_s(axBuffer, "JLO(%d)", i);
		rcJitterLOVars.add(IloNumVar(rcEnv, 0.0, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO(), axBuffer));

		sprintf_s(axBuffer, "JHI(%d)", i);
		rcJitterHIVars.add(IloNumVar(rcEnv, 0.0, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO(), axBuffer));

		sprintf_s(axBuffer, "RTLO(%d)", i);
		rcRTLOVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), axBuffer));

		sprintf_s(axBuffer, "RTHI(%d)", i);		
		rcRTHIVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCHI(), pcTaskArray[i].getDeadline(), axBuffer));
	}
}

void NoCFlowRTMILPModel::CreatePriorityVariable(IloNumVarArray & rcPVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcPVars.getEnv();
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			sprintf_s(axBuffer, "p(%d, %d)", i, j);
			rcPVars.add(IloNumVar(rcEnv, 0.0, 1.0, IloNumVar::Int, axBuffer));
		}
	}
}

void NoCFlowRTMILPModel::CreateRTHIVariable(IloNumVarArray & rcRTHI_aVars, IloNumVarArray & rcRTHI_bVars, IloNumVarArray & rcRTHI_cVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcRTHI_aVars.getEnv();
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		sprintf_s(axBuffer, "RTHI_a(%d)", i);
		rcRTHI_aVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));

		sprintf_s(axBuffer, "RTHI_b(%d)", i);
		rcRTHI_bVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));

		sprintf_s(axBuffer, "RTHI_c(%d)", i);
		rcRTHI_cVars.add(IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline(), IloNumVar::Float, axBuffer));
	}
}

void NoCFlowRTMILPModel::CreateInterfereVariable(IloNumVarArray & rcRTLOInterVars, IloNumVarArray & rcRT_aInterVars, IloNumVarArray & rcRT_bInterVars, IloNumVarArray & rcRT_cInterVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcRT_aInterVars.getEnv();
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			sprintf_s(axBuffer, "RTLOInter(%d, %d)", i, j);
			rcRTLOInterVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Int, axBuffer));

			sprintf_s(axBuffer, "RT_aInter(%d, %d)", i, j);
			rcRT_aInterVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Int, axBuffer));

			sprintf_s(axBuffer, "RT_bInter(%d, %d)", i, j);
			rcRT_bInterVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Int, axBuffer));

			sprintf_s(axBuffer, "RT_cInter(%d, %d)", i, j);
			rcRT_cInterVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Int, axBuffer));
		}
	}
}

void NoCFlowRTMILPModel::CreatePInterfereVariable(IloNumVarArray & rcRTLOPInterVars, IloNumVarArray & rcRT_aPInterVars, IloNumVarArray & rcRT_bPInterVars, IloNumVarArray & rcRT_cPInterVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcRT_aPInterVars.getEnv();
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			sprintf_s(axBuffer, "RTLOPInter(%d, %d)", i, j);
			rcRTLOPInterVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Float, axBuffer));

			sprintf_s(axBuffer, "RT_aPInter(%d, %d)", i, j);
			rcRT_aPInterVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Float, axBuffer));

			sprintf_s(axBuffer, "RT_bPInter(%d, %d)", i, j);
			rcRT_bPInterVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Float, axBuffer));

			sprintf_s(axBuffer, "RT_cPInter(%d, %d)", i, j);
			rcRT_cPInterVars.add(IloNumVar(rcEnv, 0.0, IloInfinity, IloNumVar::Int, axBuffer));
		}
	}
}

IloNumVar & NoCFlowRTMILPModel::GetPriorityVariable(int iTaskA, int iTaskB, IloNumVarArray & rcPVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	assert(iTaskA != iTaskB);
	assert(iTaskA < iTaskNum);
	assert(iTaskB < iTaskNum);
	int iAccessIndex = iTaskA * iTaskNum + iTaskB;
	return rcPVars[iAccessIndex];
}

IloNumVar & NoCFlowRTMILPModel::GetRTVariable(int iTaskIndex, IloNumVarArray & rcRTVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	assert(iTaskIndex < iTaskNum);
	return rcRTVars[iTaskIndex];
}

IloNumVar & NoCFlowRTMILPModel::GetJitterVariable(int iTaskIndex, IloNumVarArray & rcJitterVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	assert(iTaskIndex < iTaskNum);
	return rcJitterVars[iTaskIndex];
}

IloNumVar & NoCFlowRTMILPModel::GetRTInterfereVariable(int iTaskA, int iTaskB, IloNumVarArray & rcRTInterVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	assert(iTaskA != iTaskB);
	assert(iTaskA < iTaskNum);
	assert(iTaskB < iTaskNum);
	int iAccessIndex = iTaskA * iTaskNum + iTaskB;
	return rcRTInterVars[iAccessIndex];
}

IloNumVar & NoCFlowRTMILPModel::GetRTPInterfereVariable(int iTaskA, int iTaskB, IloNumVarArray & rcRTPInterVars)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	assert(iTaskA != iTaskB);
	assert(iTaskA < iTaskNum);
	assert(iTaskB < iTaskNum);
	int iAccessIndex = iTaskA * iTaskNum + iTaskB;
	return rcRTPInterVars[iAccessIndex];
}

#define PINTERUB	1
void NoCFlowRTMILPModel::GenRTLOInterVarConst(
	IloNumVarArray & rcPVars,
	IloNumVarArray & rcJitterLOVars,
	IloNumVarArray & rcRTLOVars,
	IloNumVarArray & rcRTLOInterVars, IloNumVarArray & rcRTLOPInterVars,
	IloRangeArray & rcConst
	)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcPVars.getEnv();
	vector<double> & rvectorDJItter = m_pcFlowSet->getDirectJitterTable();
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			if (!m_pcFlowSet->IsSharingRouter(i, j))
			{
				rcConst.add(IloRange(rcEnv, 0.0, GetRTPInterfereVariable(i, j, rcRTLOPInterVars), 0.0));
				rcConst.add(IloRange(rcEnv, 0.0, GetRTInterfereVariable(i, j, rcRTLOInterVars), 0.0));
				continue;
			}

			rcConst.add(IloRange(rcEnv, 0.0,
				GetRTInterfereVariable(i, j, rcRTLOInterVars) - 
				(GetRTVariable(i, rcRTLOVars) + rvectorDJItter[j] + GetJitterVariable(j, rcJitterLOVars)) / m_pcFlowSet->getLOPeriod(j),
#if PINTERUB
				1.0 - 1e-7
#else	
				IloInfinity
#endif
				));

			double dBigM = 20 * 
				ceil((pcTaskArray[i].getDeadline() + rvectorDJItter[j] + pcTaskArray[j].getDeadline()) / m_pcFlowSet->getLOPeriod(j));

			rcConst.add(IloRange(rcEnv, 0.0, 
				GetRTPInterfereVariable(i, j, rcRTLOPInterVars) - 
				GetRTInterfereVariable(i, j, rcRTLOInterVars) + GetPriorityVariable(i, j, rcPVars) * dBigM,
				IloInfinity
				));
		}
	}
}

void NoCFlowRTMILPModel::GenRTHI_aInterVarConst(
	IloNumVarArray & rcPVars,
	IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
	IloNumVarArray & rcRTHI_aVar,
	IloNumVarArray & rcRTHI_aInterVars, IloNumVarArray & rcRTHI_aPInterVars,
	IloRangeArray & rcConst
	)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcPVars.getEnv();
	vector<double> & rvectorDJItter = m_pcFlowSet->getDirectJitterTable();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (!pcTaskArray[i].getCriticality())	continue;
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			if ((!m_pcFlowSet->IsSharingRouter(i, j)) || (!pcTaskArray[j].getCriticality()))
			{
				rcConst.add(IloRange(rcEnv, 0.0, GetRTPInterfereVariable(i, j, rcRTHI_aPInterVars), 0.0));
				rcConst.add(IloRange(rcEnv, 0.0, GetRTInterfereVariable(i, j, rcRTHI_aInterVars), 0.0));
				continue;
			}			

			rcConst.add(IloRange(rcEnv, 0.0,
				GetRTInterfereVariable(i, j, rcRTHI_aInterVars) -
				(GetRTVariable(i, rcRTHI_aVar) + GetJitterVariable(j, rcJitterHIVars) + rvectorDJItter[j]) / m_pcFlowSet->getHIPeriod(j),
#if PINTERUB
				1.0 - 1e-7
#else	
				IloInfinity
#endif
				));


			double dBigM = 20 * ceil(
				(pcTaskArray[i].getDeadline() + rvectorDJItter[j] + pcTaskArray[j].getDeadline()) /
				m_pcFlowSet->getHIPeriod(j)
				);

			rcConst.add(IloRange(rcEnv, 0.0,
				GetRTPInterfereVariable(i, j, rcRTHI_aPInterVars) - 
				GetRTInterfereVariable(i, j, rcRTHI_aInterVars) + GetPriorityVariable(i, j, rcPVars) * dBigM,
				IloInfinity
				));
		}
	}
}

void NoCFlowRTMILPModel::GenRTHI_bInterVarConst(
	IloNumVarArray & rcPVars,
	IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
	IloNumVarArray & rcRTHI_bVar,
	IloNumVarArray & rcRTHI_bInterVars, IloNumVarArray & rcRTHI_bPInterVars,
	IloRangeArray & rcConst
	)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcPVars.getEnv();
	vector<double> & rvectorDJItter = m_pcFlowSet->getDirectJitterTable();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (!pcTaskArray[i].getCriticality())	continue;
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			if (!m_pcFlowSet->IsSharingRouter(i, j))
			{
				rcConst.add(IloRange(rcEnv, 0.0, GetRTPInterfereVariable(i, j, rcRTHI_bPInterVars), 0.0));
				rcConst.add(IloRange(rcEnv, 0.0, GetRTInterfereVariable(i, j, rcRTHI_bInterVars), 0.0));
				continue;
			}

			if (pcTaskArray[j].getCriticality())
			{

				rcConst.add(IloRange(rcEnv, 0.0,
					GetRTInterfereVariable(i, j, rcRTHI_bInterVars) -
					(GetRTVariable(i, rcRTHI_bVar) + GetJitterVariable(j, rcJitterHIVars) + rvectorDJItter[j]) / m_pcFlowSet->getLOPeriod(j) ,
#if PINTERUB
					1.0 - 1e-7
#else	
					IloInfinity
#endif
					));

				double dBigM = 20 * ceil(
					(pcTaskArray[i].getDeadline() + rvectorDJItter[j] + pcTaskArray[j].getDeadline() ) /
					m_pcFlowSet->getLOPeriod(j)
					);

				rcConst.add(IloRange(rcEnv, 0.0,
					GetRTPInterfereVariable(i, j, rcRTHI_bPInterVars) - 
					GetRTInterfereVariable(i, j, rcRTHI_bInterVars) + GetPriorityVariable(i, j, rcPVars) * dBigM,
					IloInfinity
					));
			}
			else
			{

				rcConst.add(IloRange(rcEnv, 0.0,
					GetRTInterfereVariable(i, j, rcRTHI_bInterVars) -
					(GetRTVariable(i, rcRTHI_bVar) + GetJitterVariable(j, rcJitterLOVars) + rvectorDJItter[j]) / m_pcFlowSet->getLOPeriod(j),
#if PINTERUB
					1.0 - 1e-7
#else	
					IloInfinity
#endif
					));

				double dBigM = 20 * ceil(
					(pcTaskArray[i].getDeadline() + rvectorDJItter[j] + pcTaskArray[j].getDeadline()) /
					m_pcFlowSet->getLOPeriod(j)
					);

				rcConst.add(IloRange(rcEnv, 0.0,
					GetRTPInterfereVariable(i, j, rcRTHI_bPInterVars) -
					GetRTInterfereVariable(i, j, rcRTHI_bInterVars) + GetPriorityVariable(i, j, rcPVars) * dBigM,
					IloInfinity
					));
			}		
		}
	}
}

void NoCFlowRTMILPModel::GenRTHI_cInterVarConst(
	IloNumVarArray & rcPVars,
	IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
	IloNumVarArray & rcRTHI_cVar, IloNumVarArray & rcRTHI_bVar,
	IloNumVarArray & rcRTHI_cInterVars, IloNumVarArray & rcRTHI_cPInterVars,
	IloRangeArray & rcConst
	)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcPVars.getEnv();
	vector<double> & rvectorDJItter = m_pcFlowSet->getDirectJitterTable();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (!pcTaskArray[i].getCriticality())	continue;
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			if (!m_pcFlowSet->IsSharingRouter(i, j))
			{
				rcConst.add(IloRange(rcEnv, 0.0, GetRTPInterfereVariable(i, j, rcRTHI_cPInterVars), 0.0));
				rcConst.add(IloRange(rcEnv, 0.0, GetRTInterfereVariable(i, j, rcRTHI_cInterVars), 0.0));
				continue;
			}

			if (pcTaskArray[j].getCriticality())
			{

				rcConst.add(IloRange(rcEnv, 0.0,
					GetRTInterfereVariable(i, j, rcRTHI_cInterVars) -
					(GetRTVariable(i, rcRTHI_cVar) + GetJitterVariable(j, rcJitterHIVars) + rvectorDJItter[j]) / m_pcFlowSet->getHIPeriod(j),
#if PINTERUB
					1.0 - 1e-7
#else	
					IloInfinity
#endif
					));

				double dBigM = 20 * ceil(
					(pcTaskArray[i].getDeadline() + rvectorDJItter[j] + pcTaskArray[j].getDeadline()) /
					m_pcFlowSet->getHIPeriod(j)
					);

				rcConst.add(IloRange(rcEnv, 0.0,
					GetRTPInterfereVariable(i, j, rcRTHI_cPInterVars) -
					GetRTInterfereVariable(i, j, rcRTHI_cInterVars) + GetPriorityVariable(i, j, rcPVars) * dBigM,
					IloInfinity
					));
			}
			else
			{
				if (m_pcFlowSet->getUpDownStreamLOType(i, j) == 1)
				{

					rcConst.add(IloRange(rcEnv, 0.0,
						GetRTInterfereVariable(i, j, rcRTHI_cInterVars) -
						(GetRTVariable(i, rcRTHI_cVar) + GetJitterVariable(j, rcJitterLOVars) + rvectorDJItter[j]) / m_pcFlowSet->getLOPeriod(j),
#if PINTERUB
						1.0 - 1e-7
#else	
						IloInfinity
#endif
						));


					double dBigM = 20 * ceil(
						(pcTaskArray[i].getDeadline() + rvectorDJItter[j] + pcTaskArray[j].getDeadline()) /
						m_pcFlowSet->getLOPeriod(j)
						);

					rcConst.add(IloRange(rcEnv, 0.0,
						GetRTPInterfereVariable(i, j, rcRTHI_cPInterVars) -
						GetRTInterfereVariable(i, j, rcRTHI_cInterVars) + GetPriorityVariable(i, j, rcPVars) * dBigM,
						IloInfinity
						));
				}
				else
				{

					rcConst.add(IloRange(rcEnv, 0.0,
						GetRTInterfereVariable(i, j, rcRTHI_cInterVars) -
						(GetRTVariable(i, rcRTHI_bVar) + GetJitterVariable(j, rcJitterLOVars) + rvectorDJItter[j]) / m_pcFlowSet->getLOPeriod(j),
#if PINTERUB
						1.0 - 1e-7
#else	
						IloInfinity
#endif
						));

					double dBigM = 20 * ceil(
						(pcTaskArray[i].getDeadline() + rvectorDJItter[j] + pcTaskArray[j].getDeadline()) /
						m_pcFlowSet->getLOPeriod(j)
						);

					rcConst.add(IloRange(rcEnv, 0.0,
						GetRTPInterfereVariable(i, j, rcRTHI_cPInterVars) -
						GetRTInterfereVariable(i, j, rcRTHI_cInterVars) + GetPriorityVariable(i, j, rcPVars) * dBigM,
						IloInfinity
						));
				}				
			}
		}
	}
}

void NoCFlowRTMILPModel::GenSchedConst(
	IloNumVarArray & rcPVars,
	IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloNumVarArray & rcRTHI_aVars, IloNumVarArray & rcRTHI_bVars, IloNumVarArray & rcRTHI_cVars,
	IloNumVarArray & rcRTLOPInterVars, IloNumVarArray & rcRTHI_aPInterVars, IloNumVarArray & rcRTHI_bPInterVars, IloNumVarArray & rcRTHI_cPInterVars,
	IloRangeArray & rcConst
	)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcPVars.getEnv();
	for (int i = 0; i < iTaskNum; i++)
	{

		IloExpr cRTLORHS(rcEnv);
		cRTLORHS += pcTaskArray[i].getCLO();
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			cRTLORHS += GetRTPInterfereVariable(i, j, rcRTLOPInterVars) * pcTaskArray[j].getCLO();
		}
		rcConst.add(IloRange(rcEnv, 0.0, GetRTVariable(i, rcRTLOVars) - cRTLORHS, 0.0));


		if (pcTaskArray[i].getCriticality())
		{			
			IloExpr cRTHI_aRHS(rcEnv), cRTHI_bRHS(rcEnv), cRTHI_cRHS(rcEnv);
			cRTHI_aRHS += pcTaskArray[i].getCHI();
			cRTHI_bRHS += pcTaskArray[i].getCLO();
			cRTHI_cRHS += pcTaskArray[i].getCLO();
			for (int j = 0; j < iTaskNum; j++)
			{
				if (i == j)	continue;
				cRTHI_aRHS += GetRTPInterfereVariable(i, j, rcRTHI_aPInterVars) * pcTaskArray[j].getCHI();
				cRTHI_bRHS += GetRTPInterfereVariable(i, j, rcRTHI_bPInterVars) * pcTaskArray[j].getCLO();
				if (pcTaskArray[j].getCriticality())
				{
					cRTHI_cRHS += GetRTPInterfereVariable(i, j, rcRTHI_cPInterVars) * pcTaskArray[j].getCHI();
				}
				else
				{
					cRTHI_cRHS += GetRTPInterfereVariable(i, j, rcRTHI_cPInterVars) * pcTaskArray[j].getCLO();
				}				
			}
			rcConst.add(IloRange(rcEnv, 0.0, GetRTVariable(i, rcRTHI_aVars) - cRTHI_aRHS, 0.0));
			rcConst.add(IloRange(rcEnv, 0.0, GetRTVariable(i, rcRTHI_bVars) - cRTHI_bRHS, 0.0));
			rcConst.add(IloRange(rcEnv, 0.0, GetRTVariable(i, rcRTHI_cVars) - cRTHI_cRHS, 0.0));
			rcConst.add(IloRange(rcEnv, 0.0, GetRTVariable(i, rcRTHIVars) - cRTHI_aRHS, IloInfinity));
			rcConst.add(IloRange(rcEnv, 0.0, GetRTVariable(i, rcRTHIVars) - cRTHI_cRHS, IloInfinity));
			m_mapTempExpr[i] = cRTHI_cRHS;
		}	
	}
}

void NoCFlowRTMILPModel::GenRTJitterConst(
	IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars,
	IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
	IloRangeArray & rcConst
	)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcRTLOVars.getEnv();
	for (int i = 0; i < iTaskNum; i++)
	{
		rcConst.add(IloRange(rcEnv, 0.0, GetJitterVariable(i, rcJitterLOVars) - (GetRTVariable(i, rcRTLOVars) - pcTaskArray[i].getCLO()), 0.0));
		rcConst.add(IloRange(rcEnv, 0.0, GetJitterVariable(i, rcJitterHIVars) - (GetRTVariable(i, rcRTHIVars) - pcTaskArray[i].getCLO()), 0.0));
	}
}

void NoCFlowRTMILPModel::GenPriorityConst(IloNumVarArray & rcPVars, IloRangeArray & rcRangeArray)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcPVars.getEnv();
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (j == i)
			{
				rcRangeArray.add(IloRange(rcEnv, 0.0, rcPVars[i * iTaskNum + j], 0.0));
				continue;
			}

			GenPriorityConst(i, j, rcPVars, rcRangeArray);

			for (int k = 0; k < iTaskNum; k++)
			{
				if ((i == k) || (j == k))
				{
					continue;
				}
				GenPriorityConst(i, j, k, rcPVars, rcRangeArray);
			}
		}
	}
}

void NoCFlowRTMILPModel::GenPriorityConst(int i, int j, IloNumVarArray & rcPVars, IloRangeArray & rcRangeArray)
{
	const IloEnv & rcEnv = rcPVars.getEnv();
	rcRangeArray.add(IloRange(rcEnv, 0.0,
		GetPriorityVariable(i, j, rcPVars) + GetPriorityVariable(j, i, rcPVars) - 1.0,
		0.0));
	return;
}

void NoCFlowRTMILPModel::GenPriorityConst(int i, int j, int k, IloNumVarArray & rcPVars, IloRangeArray & rcRangeArray)
{
	const IloEnv & rcEnv = rcPVars.getEnv();
	rcRangeArray.add(IloRange(rcEnv, -IloInfinity,
		GetPriorityVariable(i, k, rcPVars) + GetPriorityVariable(k, j, rcPVars) - 1.0 - GetPriorityVariable(i, j, rcPVars),
		0.0));
}

void NoCFlowRTMILPModel::ExtractPriorityResult(IloNumVarArray & rcPVars, IloCplex & rcSolver)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	m_cPrioritySolution = TaskSetPriorityStruct(*m_pcFlowSet);
	for (int i = 0; i < iTaskNum; i++)
	{
		int iPriority = 0;
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j) continue;
			iPriority += round(rcSolver.getValue(GetPriorityVariable(j, i, rcPVars)));
		}
		m_cPrioritySolution.setPriority(i, iPriority);
	}	
}

void NoCFlowRTMILPModel::VerifyResult(
	IloCplex & rcSolver,
	IloNumVarArray & rcPVars,
	IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
	IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloNumVarArray & rcRTHI_aVars, IloNumVarArray & rcRTHI_bVars, IloNumVarArray & rcRTHI_cVars,
	IloNumVarArray & rcRTLOInterVars, IloNumVarArray & rcRTHI_aInterVars, IloNumVarArray & rcRTHI_bInterVars, IloNumVarArray & rcRTHI_cInterVars,
	IloNumVarArray & rcRTLOPInterVars, IloNumVarArray & rcRTHI_aPInterVars, IloNumVarArray & rcRTHI_bPInterVars, IloNumVarArray & rcRTHI_cPInterVars	
	)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	vector<double> & rvectorDJitter = m_pcFlowSet->getDirectJitterTable();

	for (int i = 0; i < iTaskNum; i++)
	{
		double dRTLO = rcSolver.getValue(GetRTVariable(i, rcRTLOVars));
		double dJitterLO = rcSolver.getValue(GetJitterVariable(i, rcJitterLOVars));
		assert(DOUBLE_EQUAL(dJitterLO, dRTLO - pcTaskArray[i].getCLO(), 1e-6));

		double dRTLORHS = pcTaskArray[i].getCLO();
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			dRTLORHS += rcSolver.getValue(GetRTPInterfereVariable(i, j, rcRTLOPInterVars)) * pcTaskArray[j].getCLO();
		}
		if (!DOUBLE_EQUAL(dRTLORHS, dRTLO, 1e-6))
		{
			cout << dRTLORHS << " " << dRTLO << endl;
		}
		assert(DOUBLE_EQUAL(dRTLORHS, dRTLO, 1e-6));
	
		if (pcTaskArray[i].getCriticality())
		{
			double dRTHI = rcSolver.getValue(GetRTVariable(i, rcRTHIVars));
			double dRTHI_a = rcSolver.getValue(GetRTVariable(i, rcRTHI_aVars));
			double dRTHI_b = rcSolver.getValue(GetRTVariable(i, rcRTHI_bVars));
			double dRTHI_c = rcSolver.getValue(GetRTVariable(i, rcRTHI_cVars));
			double dJitterHI = rcSolver.getValue(GetJitterVariable(i, rcJitterHIVars));
			assert(DOUBLE_EQUAL(dJitterHI, dRTHI - pcTaskArray[i].getCLO(), 1e-6));
			assert(DOUBLE_GE(dRTHI, dRTHI_a, 1e-6) && DOUBLE_GE(dRTHI, dRTHI_c, 1e-6));
			double dRTHI_aRHS = pcTaskArray[i].getCHI();
			double dRTHI_bRHS = pcTaskArray[i].getCLO();
			double dRTHI_cRHS = pcTaskArray[i].getCLO();
			for (int j = 0; j < iTaskNum; j++)
			{
				if (i == j)	continue;
				dRTHI_aRHS += rcSolver.getValue(GetRTPInterfereVariable(i, j, rcRTHI_aPInterVars)) * pcTaskArray[j].getCHI();
				dRTHI_bRHS += rcSolver.getValue(GetRTPInterfereVariable(i, j, rcRTHI_bPInterVars)) * pcTaskArray[j].getCLO();
				if (pcTaskArray[j].getCriticality())
					dRTHI_cRHS += rcSolver.getValue(GetRTPInterfereVariable(i, j, rcRTHI_cPInterVars)) * pcTaskArray[j].getCHI();
				else
					dRTHI_cRHS += rcSolver.getValue(GetRTPInterfereVariable(i, j, rcRTHI_cPInterVars)) * pcTaskArray[j].getCLO();
			}
			assert(DOUBLE_EQUAL(dRTHI_a, dRTHI_aRHS, 1e-6));
			assert(DOUBLE_EQUAL(dRTHI_b, dRTHI_bRHS, 1e-6));
			assert(DOUBLE_EQUAL(dRTHI_c, dRTHI_cRHS, 1e-6));
		}
	}
	//Verify Interference
	for (int i = 0; i < iTaskNum; i++)
	{
		double dRTLO = rcSolver.getValue(GetRTVariable(i, rcRTLOVars));		
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			double dJitterLOJ = rcSolver.getValue(GetJitterVariable(j, rcJitterLOVars));
			double dRTLOInter = rcSolver.getValue(GetRTInterfereVariable(i, j, rcRTLOInterVars));
			double dRTLOPInter = rcSolver.getValue(GetRTInterfereVariable(i, j, rcRTLOPInterVars));
			if (m_pcFlowSet->IsSharingRouter(i, j))
			{				
				int iPri = round(rcSolver.getValue(GetPriorityVariable(i, j, rcPVars)));
				//assert(dRTLOInter >= ceil((dRTLO + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)));
				if (!(DOUBLE_GE(dRTLOInter, ceil((dRTLO + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)), 1e-6)))
				{
					cout << dRTLOInter << " " << ceil((dRTLO + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)) << endl;
				}
				assert(DOUBLE_GE(dRTLOInter, ceil((dRTLO + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)), 1e-6));
				if ((iPri == 0))
				{					
					//assert(dRTLOPInter - dRTLOInter >= -1e-6);
					assert(DOUBLE_GE(dRTLOPInter, dRTLOInter, 1e-6));
				}			
			}
			else
			{
				assert(DOUBLE_EQUAL(dRTLOPInter, 0.0, 1e-6));
			}
		
		}

		if (pcTaskArray[i].getCriticality())
		{
		
			double dRTHI = rcSolver.getValue(GetRTVariable(i, rcRTHIVars));
			double dRTHI_a = rcSolver.getValue(GetRTVariable(i, rcRTHI_aVars));
			double dRTHI_b = rcSolver.getValue(GetRTVariable(i, rcRTHI_bVars));
			double dRTHI_c = rcSolver.getValue(GetRTVariable(i, rcRTHI_cVars));						

			for (int j = 0; j < iTaskNum; j++)
			{
				if (i == j) continue;
				double dRTHI_aInter = rcSolver.getValue(GetRTInterfereVariable(i, j, rcRTHI_aInterVars));
				double dRTHI_aPInter = rcSolver.getValue(GetRTInterfereVariable(i, j, rcRTHI_aPInterVars));
				double dRTHI_bInter = rcSolver.getValue(GetRTInterfereVariable(i, j, rcRTHI_bInterVars));
				double dRTHI_bPInter = rcSolver.getValue(GetRTInterfereVariable(i, j, rcRTHI_bPInterVars));
				double dRTHI_cInter = rcSolver.getValue(GetRTInterfereVariable(i, j, rcRTHI_cInterVars));
				double dRTHI_cPInter = rcSolver.getValue(GetRTInterfereVariable(i, j, rcRTHI_cPInterVars));
				int iPri = round(rcSolver.getValue(GetPriorityVariable(i, j, rcPVars)));

				//RTHI_a
				if( (m_pcFlowSet->IsSharingRouter(i, j)) && (pcTaskArray[j].getCriticality()))
				{
					double dJitterLOJ = rcSolver.getValue(GetJitterVariable(j, rcJitterLOVars));
					double dJitterHIJ = rcSolver.getValue(GetJitterVariable(j, rcJitterHIVars));
					if (!(DOUBLE_GE(dRTHI_aInter, ceil((dRTHI_a + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getHIPeriod(j)), 1e-6)))
					{
						cout << dRTHI_aInter << " " << (dRTHI_a + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getHIPeriod(j) << " "  <<
							rcSolver.getValue(
							GetRTInterfereVariable(i, j, rcRTHI_aInterVars) -
							(GetRTVariable(i, rcRTHI_aVars) + GetJitterVariable(j, rcJitterHIVars) + rvectorDJitter[j]) / m_pcFlowSet->getHIPeriod(j)
							) <<
							endl;
					}
					assert(DOUBLE_GE(dRTHI_aInter, ceil((dRTHI_a + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getHIPeriod(j)), 1e-6));
					if (iPri == 0)
					{
						//assert(dRTHI_aPInter >= dRTHI_aInter);
						assert(DOUBLE_GE(dRTHI_aPInter, dRTHI_aInter, 1e-6));
					}
				}
				else
				{
					assert(DOUBLE_EQUAL(dRTHI_aPInter, 0.0, 1e-6));
				}
			

				//RTHI_b
				if (m_pcFlowSet->IsSharingRouter(i, j))
				{
					if (pcTaskArray[j].getCriticality())
					{
						double dJitterLOJ = rcSolver.getValue(GetJitterVariable(j, rcJitterLOVars));
						double dJitterHIJ = rcSolver.getValue(GetJitterVariable(j, rcJitterHIVars));
						if (!(dRTHI_bInter >= ceil((dRTHI_b + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getLOPeriod(j))))
						{
							cout << dRTHI_bInter << " " << ceil((dRTHI_b + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getLOPeriod(j)) << " " << 
								(dRTHI_b + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getLOPeriod(j) << endl;
						}
						assert(DOUBLE_GE(dRTHI_bInter, ceil((dRTHI_b + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getLOPeriod(j)), 1e-6));
						//assert(dRTHI_bInter >= ceil((dRTHI_b + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getLOPeriod(j)));
						if (iPri == 0)
						{
							//assert(dRTHI_bPInter >= dRTHI_bInter);
							assert(DOUBLE_GE(dRTHI_bPInter, dRTHI_bInter, 1e-6));
						}
					}
					else
					{
						double dJitterLOJ = rcSolver.getValue(GetJitterVariable(j, rcJitterLOVars));
						if (!(dRTHI_bInter >= ceil((dRTHI_b + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j))))
						{
							cout << dRTHI_bInter << " " << ceil((dRTHI_b + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)) << " " <<
								(dRTHI_b + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j) << endl;
							cout << rcSolver.getValue(
								GetRTInterfereVariable(i, j, rcRTHI_bInterVars) -
								(GetRTVariable(i, rcRTHI_bVars) + GetJitterVariable(j, rcJitterLOVars) + rvectorDJitter[j]) / m_pcFlowSet->getLOPeriod(j)
								) << endl;;
						}
						assert(DOUBLE_GE(dRTHI_bInter, ceil((dRTHI_b + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)), 1e-6));
						//assert(dRTHI_bInter >= ceil((dRTHI_b + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)));
						if (iPri == 0)
						{
							//assert(dRTHI_bPInter >= dRTHI_bInter);
							assert(DOUBLE_GE(dRTHI_cPInter, dRTHI_cInter, 1e-6));
						}
					}
				}
				else
				{
					assert(DOUBLE_EQUAL(dRTHI_bPInter, 0.0, 1e-6));
				}
			

				//RTHI_c
				if (m_pcFlowSet->IsSharingRouter(i, j))
				{
					if (pcTaskArray[j].getCriticality())
					{
						double dJitterLOJ = rcSolver.getValue(GetJitterVariable(j, rcJitterLOVars));
						double dJitterHIJ = rcSolver.getValue(GetJitterVariable(j, rcJitterHIVars));
						assert(dRTHI_cInter >= ceil((dRTHI_c + rvectorDJitter[j] + dJitterHIJ) / m_pcFlowSet->getHIPeriod(j)));
						if (iPri == 0)
						{
							assert(DOUBLE_GE(dRTHI_cPInter, dRTHI_cInter, 1e-6));
							//assert(dRTHI_cPInter >= dRTHI_cInter);
						}
					}
					else
					{
						double dJitterLOJ = rcSolver.getValue(GetJitterVariable(j, rcJitterLOVars));
						if (m_pcFlowSet->getUpDownStreamLOType(i, j) == 1)
						{
							assert(DOUBLE_GE(dRTHI_cInter, ceil((dRTHI_c + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)), 1e-6));
							//assert(dRTHI_cInter >= ceil((dRTHI_b + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)));
						}
						else
						{
							assert(DOUBLE_GE(dRTHI_cInter, ceil((dRTHI_b + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)), 1e-6));
							//assert(dRTHI_cInter >= ceil((dRTHI_c + rvectorDJitter[j] + dJitterLOJ) / m_pcFlowSet->getLOPeriod(j)));
						}

						if (iPri == 0)
						{
							assert(DOUBLE_GE(dRTHI_cPInter, dRTHI_cInter, 1e-6));							
						}
					}
				}				
				else
				{
					assert(DOUBLE_EQUAL(dRTHI_cPInter, 0.0, 1e-6));
				}							
			}
		}		
	}
}

void NoCFlowRTMILPModel::VerifyResult2(
	IloCplex & rcSolver,
	IloNumVarArray & rcPVars,
	IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
	IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloNumVarArray & rcRTHI_aVars, IloNumVarArray & rcRTHI_bVars, IloNumVarArray & rcRTHI_cVars,
	IloNumVarArray & rcRTLOInterVars, IloNumVarArray & rcRTHI_aInterVars, IloNumVarArray & rcRTHI_bInterVars, IloNumVarArray & rcRTHI_cInterVars,
	IloNumVarArray & rcRTLOPInterVars, IloNumVarArray & rcRTHI_aPInterVars, IloNumVarArray & rcRTHI_bPInterVars, IloNumVarArray & rcRTHI_cPInterVars
	)
{
	TaskSetPriorityStruct cPA(*m_pcFlowSet);
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	cPA = getPrioritySol();
	assert(cPA.getSize() == iTaskNum);
	cPA.PrintByPriority();
	NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(*m_pcFlowSet, cPA, m_pcFlowSet->getDirectJitterTable());
	cRTCalc.ComputeAllResponseTime();
	cRTCalc.PrintSchedulabilityInfo();
	double dMinLaxity = pcTaskArray[0].getDeadline();
	for (int i = 0; i < iTaskNum; i++)
	{
		double dThisMinLaxity = pcTaskArray[i].getDeadline();
		cout << "Verifying Task " << i << endl;
		double dRTLO_Verify = cRTCalc.getRTLO(i);
		double dRTLO_MILP = rcSolver.getValue(rcRTLOVars[i]);
		if (DOUBLE_EQUAL(dRTLO_MILP, dRTLO_Verify, 1e-5) == false)
		{
			cout << "Task " << i << " RT computed wrongly" << endl;
			cout << dRTLO_Verify << " " << dRTLO_MILP << endl;
			while (1);
		}
		dThisMinLaxity = pcTaskArray[i].getDeadline() - dRTLO_MILP;
		if (pcTaskArray[i].getCriticality() == false)
		{
			if (dThisMinLaxity < dMinLaxity)
			{
				cout << "Task " << i << " " << pcTaskArray[i].getDeadline() << " " << dRTLO_MILP << endl;
			}
			dMinLaxity = min(dMinLaxity, dThisMinLaxity);

			continue;
		}
		
		double dRTHI_Verify = cRTCalc.getRTHI(i);
		double dRTHI_MILP = rcSolver.getValue(rcRTHIVars[i]);
		if (DOUBLE_GE(dRTHI_MILP, dRTHI_Verify, 1e-5) == false)
		{
			cout << "Task " << i << " RT computed wrongly" << endl;
			cout << dRTHI_Verify << " " << dRTHI_MILP << endl;
			cout << "RT_a: " << cRTCalc.getRTHI_a(i) << " " << rcSolver.getValue(rcRTHI_aVars[i]) << endl;
			cout << "RT_b: " << cRTCalc.getRTHI_b(i) << " " << rcSolver.getValue(rcRTHI_bVars[i]) << endl;
			cout << "RT_c: " << cRTCalc.getRTHI_c(i) << " " << rcSolver.getValue(rcRTHI_cVars[i]) << endl;
			cout << rcSolver.getValue(m_mapTempExpr[i]) << endl;
			while (1);
		}
		dThisMinLaxity = pcTaskArray[i].getDeadline() - dRTHI_MILP;
		if (dThisMinLaxity < dMinLaxity)
		{
			cout << "Task " << i << " " << pcTaskArray[i].getDeadline() << " " << dRTHI_MILP << endl;
		}
		dMinLaxity = min(dMinLaxity, dThisMinLaxity);
	}	
	cout << "Min Laxity: " << dMinLaxity << endl;
	cout << rcSolver.getValue(m_mapTempExpr[-1]) << endl;
}

void NoCFlowRTMILPModel::setSolverParam(IloCplex & rcSolver)
{
	rcSolver.setParam(IloCplex::Param::Emphasis::MIP, 0);
	rcSolver.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, 1e-9);
	rcSolver.setParam(IloCplex::Param::MIP::Tolerances::Integrality, 0);
}

void NoCFlowRTMILPModel::EnableSolverDisp(int iLevel, IloCplex & rcSolver)
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

int NoCFlowRTMILPModel::Solve(int iDisplay /* = 0 */, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		IloNumVarArray cPVars(cSolveEnv);
		IloNumVarArray cJitterHIVars(cSolveEnv), cJitterLOVars(cSolveEnv), cRTLOVars(cSolveEnv), cRTHIVars(cSolveEnv);
		IloNumVarArray cRTHI_aVars(cSolveEnv), cRTHI_bVars(cSolveEnv), cRTHI_cVars(cSolveEnv);
		IloNumVarArray cRTLOInterVars(cSolveEnv), cRTHI_aInterVars(cSolveEnv), cRTHI_bInterVars(cSolveEnv), cRTHI_cInterVars(cSolveEnv);
		IloNumVarArray cRTLOPInterVars(cSolveEnv), cRTHI_aPInterVars(cSolveEnv), cRTHI_bPInterVars(cSolveEnv), cRTHI_cPInterVars(cSolveEnv);
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);
		
		CreateJitterRTVariable(cJitterLOVars, cJitterHIVars, cRTLOVars, cRTHIVars);
		CreatePriorityVariable(cPVars);
		CreateRTHIVariable(cRTHI_aVars, cRTHI_bVars, cRTHI_cVars);
		CreateInterfereVariable(cRTLOInterVars, cRTHI_aInterVars, cRTHI_bInterVars, cRTHI_cInterVars);
		CreatePInterfereVariable(cRTLOPInterVars, cRTHI_aPInterVars, cRTHI_bPInterVars, cRTHI_cPInterVars);

		GenRTLOInterVarConst(
			cPVars,
			cJitterLOVars,
			cRTLOVars,
			cRTLOInterVars, cRTLOPInterVars,
			cConst
			);

		GenRTHI_aInterVarConst(
			cPVars,
			cJitterLOVars, cJitterHIVars,
			cRTHI_aVars,
			cRTHI_aInterVars, cRTHI_aPInterVars,
			cConst
			);

		GenRTHI_bInterVarConst(
			cPVars,
			cJitterLOVars, cJitterHIVars,
			cRTHI_bVars,
			cRTHI_bInterVars, cRTHI_bPInterVars,
			cConst
			);

		GenRTHI_cInterVarConst(
			cPVars,
			cJitterLOVars, cJitterHIVars,
			cRTHI_cVars, cRTHI_bVars,
			cRTHI_cInterVars, cRTHI_cPInterVars,
			cConst
			);

		GenRTJitterConst(
			cRTLOVars, cRTHIVars,
			cJitterLOVars, cJitterHIVars,
			cConst
			);

		GenSchedConst(
			cPVars,
			cRTLOVars, cRTHIVars, cRTHI_aVars, cRTHI_bVars, cRTHI_cVars,
			cRTLOPInterVars, cRTHI_aPInterVars, cRTHI_bPInterVars, cRTHI_cPInterVars,
			cConst
			);

		GenPriorityConst(cPVars, cConst);

		if (m_pcPredefinePriorityAssignment)
		{
			GenPredefinePriorityConst(*m_pcPredefinePriorityAssignment, cPVars, cConst);
		}

		IloExpr cObjExpr(cSolveEnv);
		IloNumVar cMinSlack(cSolveEnv, 0.0, IloInfinity, IloNumVar::Float);
		if (1)
		{			
			for (int i = 0; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
				{
					cConst.add(IloRange(cSolveEnv, 0.0, (pcTaskArray[i].getDeadline() - cRTHIVars[i]) - cMinSlack, IloInfinity));					
				}
				else
				{
					cConst.add(IloRange(cSolveEnv, 0.0, (pcTaskArray[i].getDeadline() - cRTLOVars[i]) - cMinSlack, IloInfinity));
				}
			}
			cObjExpr = -cMinSlack;
		}
		m_mapTempExpr[-1] = cMinSlack;
		IloModel cModel(cSolveEnv);
		cModel.add(cConst);		
		switch (m_enumObjType)
		{
		case NoCFlowRTMILPModel::None:
			break;
		case NoCFlowRTMILPModel::MaxMinLaxity:
			cObjExpr = -cMinSlack;
			cModel.add(IloMinimize(cSolveEnv, cObjExpr));
			break;
		default:
			break;
		}
		IloCplex cSolver(cModel);

		//cSolver.exportModel("Temp.lp");
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);
		m_dObjective = -1;
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;		
		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;		
		if (bStatus)
		{
			m_dObjective = cSolver.getObjValue();
			if (0)
			{
				cout << "Verifying...." << endl;
				VerifyResult(
					cSolver,
					cPVars,
					cJitterLOVars, cJitterHIVars,
					cRTLOVars, cRTHIVars, cRTHI_aVars, cRTHI_bVars, cRTHI_cVars,
					cRTLOInterVars, cRTHI_aInterVars, cRTHI_bInterVars, cRTHI_cInterVars,
					cRTLOPInterVars, cRTHI_aPInterVars, cRTHI_bPInterVars, cRTHI_cPInterVars
					);
			}			
			ExtractPriorityResult(cPVars, cSolver);
			//DisplayExactResult();
			if (0)
			{
				cout << "Verifying...." << endl;
				VerifyResult2(
					cSolver,
					cPVars,
					cJitterLOVars, cJitterHIVars,
					cRTLOVars, cRTHIVars, cRTHI_aVars, cRTHI_bVars, cRTHI_cVars,
					cRTLOInterVars, cRTHI_aInterVars, cRTHI_bInterVars, cRTHI_cInterVars,
					cRTLOPInterVars, cRTHI_aPInterVars, cRTHI_bPInterVars, cRTHI_cPInterVars
					);
			}
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1; 
		else if ((cSolver.getCplexStatus() == cSolver.AbortTimeLim) && (bStatus))
			iRetStatus = -3;
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

int NoCFlowRTMILPModel::Solve_MinJitterRTDifference(int iDisplay /* = 0 */, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		IloNumVarArray cPVars(cSolveEnv);
		IloNumVarArray cJitterHIVars(cSolveEnv), cJitterLOVars(cSolveEnv), cRTLOVars(cSolveEnv), cRTHIVars(cSolveEnv);
		IloNumVarArray cRTHI_aVars(cSolveEnv), cRTHI_bVars(cSolveEnv), cRTHI_cVars(cSolveEnv);
		IloNumVarArray cRTLOInterVars(cSolveEnv), cRTHI_aInterVars(cSolveEnv), cRTHI_bInterVars(cSolveEnv), cRTHI_cInterVars(cSolveEnv);
		IloNumVarArray cRTLOPInterVars(cSolveEnv), cRTHI_aPInterVars(cSolveEnv), cRTHI_bPInterVars(cSolveEnv), cRTHI_cPInterVars(cSolveEnv);
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);

		CreateJitterRTVariable(cJitterLOVars, cJitterHIVars, cRTLOVars, cRTHIVars);
		CreatePriorityVariable(cPVars);
		CreateRTHIVariable(cRTHI_aVars, cRTHI_bVars, cRTHI_cVars);
		CreateInterfereVariable(cRTLOInterVars, cRTHI_aInterVars, cRTHI_bInterVars, cRTHI_cInterVars);
		CreatePInterfereVariable(cRTLOPInterVars, cRTHI_aPInterVars, cRTHI_bPInterVars, cRTHI_cPInterVars);

		GenRTLOInterVarConst(
			cPVars,
			cJitterLOVars,
			cRTLOVars,
			cRTLOInterVars, cRTLOPInterVars,
			cConst
			);

		GenRTHI_aInterVarConst(
			cPVars,
			cJitterLOVars, cJitterHIVars,
			cRTHI_aVars,
			cRTHI_aInterVars, cRTHI_aPInterVars,
			cConst
			);

		GenRTHI_bInterVarConst(
			cPVars,
			cJitterLOVars, cJitterHIVars,
			cRTHI_bVars,
			cRTHI_bInterVars, cRTHI_bPInterVars,
			cConst
			);

		GenRTHI_cInterVarConst(
			cPVars,
			cJitterLOVars, cJitterHIVars,
			cRTHI_cVars, cRTHI_bVars,
			cRTHI_cInterVars, cRTHI_cPInterVars,
			cConst
			);
#if 0
		GenRTJitterConst(
			cRTLOVars, cRTHIVars,
			cJitterLOVars, cJitterHIVars,
			cConst
			);
#endif
		GenSchedConst(
			cPVars,
			cRTLOVars, cRTHIVars, cRTHI_aVars, cRTHI_bVars, cRTHI_cVars,
			cRTLOPInterVars, cRTHI_aPInterVars, cRTHI_bPInterVars, cRTHI_cPInterVars,
			cConst
			);

		GenPriorityConst(cPVars, cConst);

		if (m_pcPredefinePriorityAssignment)
		{
			GenPredefinePriorityConst(*m_pcPredefinePriorityAssignment, cPVars, cConst);
		}

		IloExpr cObjExpr(cSolveEnv);
		IloNumVar cMinSlack(cSolveEnv, 0.0, IloInfinity, IloNumVar::Float);
		IloExpr cTotalJitterRTDiff(cSolveEnv);
		for (int i = 1; i < iTaskNum; i++)
		{
			cConst.add(IloRange(cSolveEnv,
				-IloInfinity,
				GetJitterVariable(i, cJitterLOVars) - (GetRTVariable(i, cRTLOVars) - pcTaskArray[i].getCLO()),
				0.0
				));
			cTotalJitterRTDiff += (GetRTVariable(i, cRTLOVars) - pcTaskArray[i].getCLO()) - GetJitterVariable(i, cJitterLOVars);

			if (pcTaskArray[i].getCriticality())
			{
				cConst.add(IloRange(cSolveEnv,
					-IloInfinity,
					GetJitterVariable(i, cJitterHIVars) - (GetRTVariable(i, cRTHIVars) - pcTaskArray[i].getCLO()),
					0.0
					));
				cTotalJitterRTDiff += (GetRTVariable(i, cRTHIVars) - pcTaskArray[i].getCLO()) - GetJitterVariable(i, cJitterHIVars);
			}
		}
		IloObjective cObj = IloMinimize(cSolveEnv, cTotalJitterRTDiff);

		IloModel cModel(cSolveEnv);
		cModel.add(cConst);		
		cModel.add(cObj);
		IloCplex cSolver(cModel);		

		//cSolver.exportModel("Temp.lp");
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;
		m_dSolveCPUTime = getCPUTimeStamp();
		m_dSolveWallTime = cSolver.getCplexTime();
		bool bStatus = cSolver.solve();
		m_dSolveCPUTime = getCPUTimeStamp() - m_dSolveCPUTime;
		m_dSolveWallTime = cSolver.getCplexTime() - m_dSolveWallTime;

		if (bStatus)
		{		
			ExtractPriorityResult(cPVars, cSolver);			
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			iRetStatus = 1;
		else if ((cSolver.getCplexStatus() == cSolver.AbortTimeLim) && (bStatus))
			iRetStatus = -3;
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

void NoCFlowRTMILPModel::DisplayExactResult()
{	
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	char axBuffer[512] = { 0 };
	cout << endl << "Exact Response Time and Jitter" << endl;
	NoCFlowAMCRTCalculator_DerivedJitter cExactRTCalc(*m_pcFlowSet, m_cPrioritySolution, m_pcFlowSet->getDirectJitterTable());
	cExactRTCalc.ComputeAllResponseTime();
	assert(cExactRTCalc.IsSchedulable());
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "**********************************" << endl;
		cout << "Task " << i << endl;
		cout << "Priority: " << m_cPrioritySolution.getPriorityByTask(i) << endl;
		cout << "Criticality: " << pcTaskArray[i].getCriticality() << endl;
		cout << "JitterLO: " << cExactRTCalc.getJitterLO(i) << endl;
		cout << "JitterHI: " << cExactRTCalc.getJitterHI(i) << endl;
		cout << "RTLO: " << cExactRTCalc.getRTLO(i) << endl;
		cout << "RTHI: " << cExactRTCalc.getRTHI(i) << endl;
		cout << "Deadline: " << pcTaskArray[i].getDeadline() << endl;
	}
}

void NoCFlowRTMILPModel::GenPredefinePriorityConst(TaskSetPriorityStruct & rcPriorityAssignment, IloNumVarArray & rcPVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcPVars.getEnv();
	for (int i = 0; i < iTaskNum; i++)
	{
		for (int j = 0; j < iTaskNum; j++)
		{
			if (i == j)	continue;
			int iPriority_i = rcPriorityAssignment.getPriorityByTask(i);
			int iPriority_j = rcPriorityAssignment.getPriorityByTask(j);
			if (iPriority_j == -1 || iPriority_i == -1)	continue;
			if (iPriority_i < iPriority_j)
			{
				rcConst.add(IloRange(rcEnv, 1.0, GetPriorityVariable(i, j, rcPVars), 1.0));
			}
			else
			{
				rcConst.add(IloRange(rcEnv, 0.0, GetPriorityVariable(i, j, rcPVars), 0.0));
			}
		}
	}
}

NoCFlowDLUFocusRefinement::NoCFlowDLUFocusRefinement()
{
	m_enumObjType = None;
}

NoCFlowDLUFocusRefinement::NoCFlowDLUFocusRefinement(SystemType & rcSystem)
	:GeneralFocusRefinement_MonoInt(rcSystem)
{
	m_enumObjType = None;
}

void NoCFlowDLUFocusRefinement::GenLUBEqualConst(SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	const IloEnv & rcEnv = rcConst.getEnv();
	for (int i = 0; i < iTaskNum; i++)
	{				
		double dTolerableDifference = 0;
		if (m_cCustomParam.count("LUBTolDiff"))
		{
			double dMaxDifference = m_cUnschedCoreComputer.SchedCondUpperBound(NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBLO)) -
				m_cUnschedCoreComputer.SchedCondLowerBound(NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBLO)) + 0.0;
			dTolerableDifference = m_cCustomParam["LUBTolDiff"] * dMaxDifference;
		}
#if 1
		rcConst.add(IloRange(rcEnv, 0.0,
			rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBLO)] - rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBLO)] -
			dTolerableDifference,
			0.0
			));
#else
		rcConst.add(IloRange(rcEnv, 0.0,
			m_cCustomParam["LUBTolDiff"] * rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBLO)] - rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBLO)],
			0.0
			));
#endif
		if (pcTaskArray[i].getCriticality())
		{
			dTolerableDifference = 0;
			if (m_cCustomParam.count("LUBTolDiff"))
			{
				double dMaxDifference = m_cUnschedCoreComputer.SchedCondUpperBound(NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBLO)) -
					m_cUnschedCoreComputer.SchedCondLowerBound(NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBLO)) + 0.0;
				dTolerableDifference = m_cCustomParam["LUBTolDiff"] * dMaxDifference;
			}
#if 1
			rcConst.add(IloRange(rcEnv, 0.0,
				rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBHI)] - rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBHI)] -
				dTolerableDifference,
				0.0
				));
#else
			rcConst.add(IloRange(rcEnv, 0.0,
				m_cCustomParam["LUBTolDiff"] * rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBHI)] - rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBHI)],
				0.0
				));
#endif
			
		}
	}
}

void NoCFlowDLUFocusRefinement::GenMaxMinLaxityObjectiveOpt(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypes, IloModel & rcModel, double dLB)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IloNumVar cMinLaxity(rcEnv, 0.0, IloInfinity, "MinLaxity");
	IloRangeArray cConst(rcEnv);
	for (int i = 0; i < iTaskNum; i++)
	{
		if (pcTaskArray[i].getCriticality())
		{
			cConst.add(IloRange(rcEnv, -IloInfinity,
				cMinLaxity - (pcTaskArray[i].getDeadline() - rcSchedCondTypes[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBHI)]),
				0.0
				));
		}
		else
		{
			cConst.add(IloRange(rcEnv, -IloInfinity,
				cMinLaxity - (pcTaskArray[i].getDeadline() - rcSchedCondTypes[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBLO)]),
				0.0
				));
		}
	}	
	IloExpr cObjExpr = -cMinLaxity;
	IloObjective cObjective = IloMinimize(rcEnv, cObjExpr);
	cConst.add(cObjExpr >= dLB);
	rcModel.add(cConst);
	rcModel.add(cObjective);
}

void NoCFlowDLUFocusRefinement::GenMinMaxLaxityObjectiveOpt(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypes, IloModel & rcModel, double dLB)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IloNumVar cMaxLaxity(rcEnv, 0.0, IloInfinity, "MinLaxity");
	IloRangeArray cConst(rcEnv);
	for (int i = 0; i < iTaskNum; i++)
	{
		if (pcTaskArray[i].getCriticality())
		{
			cConst.add(IloRange(rcEnv, 0.0,
				cMaxLaxity - (pcTaskArray[i].getDeadline() - rcSchedCondTypes[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBHI)]),
				IloInfinity
				));
		}
	//	else
		{
			cConst.add(IloRange(rcEnv, 0.0,
				cMaxLaxity - (pcTaskArray[i].getDeadline() - rcSchedCondTypes[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBLO)]),
				IloInfinity
				));
		}
	}
	IloExpr cObjExpr = cMaxLaxity;
	IloObjective cObjective = IloMinimize(rcEnv, cObjExpr);
	cConst.add(cObjExpr >= dLB);
	rcModel.add(cConst);
	rcModel.add(cObjective);
}

ILOMIPINFOCALLBACK2(timeLimitCallbackNoC,
	IloCplex, cplex,
	IloBool, aborted)
{
	if (!aborted  &&  hasIncumbent()) {
		cout << "************************" << endl;
		cout << "Incumbent found. Abort" << endl;
		cout << "************************" << endl;
		abort();
#if 0
		IloNum gap = 100.0 * getMIPRelativeGap();
		IloNum timeUsed = cplex.getCplexTime() - timeStart;
		if (timeUsed > timeLimit && gap < acceptableGap) {
			aborted = IloTrue;
			abort();
		}
#endif
	}
}

int NoCFlowDLUFocusRefinement::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		SchedCondTypeVars cSchedCondTypeVars;
		SchedCondVars cSchedCondVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);
		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);

		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenLUBEqualConst(cSchedCondTypeVars, cConst);
		GenSchedCondTypeConst(cSolveEnv, cSchedCondTypeVars, cSchedCondVars, cConst);
			
		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cDummyInteger);	
		switch (m_enumObjType)
		{
		case NoCFlowDLUFocusRefinement::None:
			break;
		case NoCFlowDLUFocusRefinement::MaxMinLaxity:
			GenMaxMinLaxityObjectiveOpt(cSolveEnv, cSchedCondTypeVars, cModel, dObjLB);
			break;
		case NoCFlowDLUFocusRefinement::MinMaxLaxity:
			GenMinMaxLaxityObjectiveOpt(cSolveEnv, cSchedCondTypeVars, cModel, dObjLB);				
			break;
		case ReferenceGuided:
		case SuccessiveReferenceGuided:			
		case IntervalGuided:
			GenRefGuidedSearchModel(cSolveEnv, m_cReferenceSchedConds, cSchedCondTypeVars, cModel);
			break;
		default:
			break;
		}		
		IloCplex cSolver(cModel);		
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;
		//cSolver.use(timeLimitCallbackNoC(cSolveEnv, cSolver, false));
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

		switch (m_enumObjType)
		{
		case NoCFlowDLUFocusRefinement::None:
			break;
		case NoCFlowDLUFocusRefinement::MaxMinLaxity:
			break;
		case NoCFlowDLUFocusRefinement::MinMaxLaxity:
			break;
		case NoCFlowDLUFocusRefinement::ReferenceGuided:
			break;
		case NoCFlowDLUFocusRefinement::SuccessiveReferenceGuided:
			m_cReferenceSchedConds = m_cSchedCondsConfig;
			break;
		case IntervalGuided:
			if (m_cUnschedCores.size() >= 1000)
			{
				m_cReferenceSchedConds = m_cSchedCondsConfig;
				m_cUnschedCores.clear();
			}
			break;
		default:
			break;
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

void NoCFlowDLUFocusRefinement::CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	UnschedCoreComputer_MonoInt & rcUCComp = getUnschedCoreComputer();
	for (int i = 0; i < iTaskNum; i++)
	{
		rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBLO)] =
			IloNumVar(rcEnv, rcUCComp.SchedCondLowerBound(NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBLO)), pcTaskArray[i].getDeadline());
		rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBLO)] =
			IloNumVar(rcEnv, pcTaskArray[i].getCLO(), pcTaskArray[i].getDeadline());
		if (pcTaskArray[i].getCriticality())
		{
			rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBHI)] =
				IloNumVar(rcEnv, rcUCComp.SchedCondLowerBound(NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineLBHI)), pcTaskArray[i].getDeadline());
			rcSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBHI)] =
				IloNumVar(rcEnv, pcTaskArray[i].getCHI(), pcTaskArray[i].getDeadline());
		}
	}
}

double NoCFlowDLUFocusRefinement::ComputeLORTByMILP(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, UnschedCores & rcGenUC)
{	
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		vector<double> & rvectorDirectJitter = m_pcTaskSet->getDirectJitterTable();
		double dRet = -1;
		IloEnv cSolveEnv;
		SchedCondTypeVars cSchedCondTypeVars;
		SchedCondVars cSchedCondVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);
		IloNumVar cRTVar(cSolveEnv, pcTaskArray[iTaskIndex].getCLO(), pcTaskArray[iTaskIndex].getDeadline());
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);
		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);

		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenLUBEqualConst(cSchedCondTypeVars, cConst);
		GenSchedCondTypeConst(cSolveEnv, cSchedCondTypeVars, cSchedCondVars, cConst);		
		int iTaskPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
		IloNumVarArray cInterInstVars(cSolveEnv);
		IloExpr cRTRHS(cSolveEnv);
		cRTRHS += pcTaskArray[iTaskIndex].getCLO();
		for (int i = 0; i < iTaskNum; i++)
		{
			if (i == iTaskIndex)	continue;
			if ((m_pcTaskSet->IsSharingRouter(i, iTaskIndex)) && (rcPriorityAssignment.getPriorityByTask(i) < iTaskPriority))
			{
				cInterInstVars.add(IloNumVar(cSolveEnv, 0.0, IloInfinity, IloNumVar::Int));
				IloNumVar & rcThisInterInstVar = cInterInstVars[cInterInstVars.getSize() - 1];
				SchedCondType cHPKey(i, SchedCondType::DeadlineUBLO);
				cConst.add(IloRange(cSolveEnv,
					0.0, 
					rcThisInterInstVar - (cRTVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCLO()) / m_pcTaskSet->getLOPeriod(i),
					IloInfinity
					));
				cRTRHS += rcThisInterInstVar * pcTaskArray[i].getCLO();
			}
		}
		cConst.add(IloRange(cSolveEnv,
			0.0, 
			cRTRHS - cRTVar, 
			0.0
			));

		IloObjective cObjective = IloMinimize(cSolveEnv, cRTVar);
		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(0, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::TimeLimit, 1e74);
		int iRetStatus = 0;

		bool bStatus = cSolver.solve();				
		if (bStatus)
		{
				
		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			dRet = cSolver.getValue(cRTVar);
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			dRet = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			dRet = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			dRet = -3;
		else
			dRet = -4;
		cSolveEnv.end();
		return dRet;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);		
	}	
}

double NoCFlowDLUFocusRefinement::ComputeHIRTByMILP(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, UnschedCores & rcGenUC)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		vector<double> & rvectorDirectJitter = m_pcTaskSet->getDirectJitterTable();
		double dRet = -1;
		IloEnv cSolveEnv;
		SchedCondTypeVars cSchedCondTypeVars;
		SchedCondVars cSchedCondVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);
		IloNumVar cRTHIVar(cSolveEnv, pcTaskArray[iTaskIndex].getCLO(), pcTaskArray[iTaskIndex].getDeadline());						
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);
		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);

		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
		GenLUBEqualConst(cSchedCondTypeVars, cConst);
		GenSchedCondTypeConst(cSolveEnv, cSchedCondTypeVars, cSchedCondVars, cConst);
		int iTaskPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);

		//RTHI_a
		IloNumVar cRTHI_aVar(cSolveEnv, pcTaskArray[iTaskIndex].getCHI(), pcTaskArray[iTaskIndex].getDeadline());
		IloNumVarArray cRTHI_aInterInstVars(cSolveEnv);
		IloExpr cRTHI_aRHS(cSolveEnv);
		cRTHI_aRHS += pcTaskArray[iTaskIndex].getCHI();
		for (int i = 0; i < iTaskNum; i++)
		{
			if (i == iTaskIndex)	continue;
			if ((pcTaskArray[i].getCriticality()) 
				&& (m_pcTaskSet->IsSharingRouter(i, iTaskIndex))
				&& (rcPriorityAssignment.getPriorityByTask(i) < iTaskPriority))
			{
				cRTHI_aInterInstVars.add(IloNumVar(cSolveEnv, 0.0, IloInfinity, IloNumVar::Int));
				IloNumVar & rcThisInterInstVar = cRTHI_aInterInstVars[cRTHI_aInterInstVars.getSize() - 1];
				SchedCondType cHPKey(i, SchedCondType::DeadlineUBHI);
				cConst.add(IloRange(cSolveEnv,
					0.0,
					rcThisInterInstVar - (cRTHI_aVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCHI()) / m_pcTaskSet->getHIPeriod(i),
					IloInfinity
					));
				cRTHI_aRHS += rcThisInterInstVar * pcTaskArray[i].getCHI();
			}
		}
		cConst.add(IloRange(cSolveEnv,
			0.0, 
			cRTHI_aRHS - cRTHI_aVar,
			0.0
			));

		//RTHI_b
		IloNumVarArray cRTHI_bInterInstVars(cSolveEnv);
		IloNumVar cRTHI_bVar(cSolveEnv, pcTaskArray[iTaskIndex].getCLO(), pcTaskArray[iTaskIndex].getDeadline());
		IloExpr cRTHI_bRHS(cSolveEnv);		
		cRTHI_bRHS += pcTaskArray[iTaskIndex].getCLO();
		for (int i = 0; i < iTaskNum; i++)
		{
			if (i == iTaskIndex)	continue;
			if ((m_pcTaskSet->IsSharingRouter(i, iTaskIndex))
				&& (rcPriorityAssignment.getPriorityByTask(i) < iTaskPriority))
			{
				cRTHI_bInterInstVars.add(IloNumVar(cSolveEnv, 0.0, IloInfinity, IloNumVar::Int));
				IloNumVar & rcThisInterInstVar = cRTHI_bInterInstVars[cRTHI_bInterInstVars.getSize() - 1];
				if (pcTaskArray[i].getCriticality())
				{
					SchedCondType cHPKey(i, SchedCondType::DeadlineUBHI);
					cConst.add(IloRange(cSolveEnv,
						0.0,
						rcThisInterInstVar - (cRTHI_bVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCHI()) / m_pcTaskSet->getLOPeriod(i),
						IloInfinity
						));
				}
				else
				{
					SchedCondType cHPKey(i, SchedCondType::DeadlineUBLO);
					cConst.add(IloRange(cSolveEnv,
						0.0,
						rcThisInterInstVar - (cRTHI_bVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCLO()) / m_pcTaskSet->getLOPeriod(i),
						IloInfinity
						));
				}		
				cRTHI_bRHS += rcThisInterInstVar * pcTaskArray[i].getCLO();
			}
		}
		cConst.add(IloRange(cSolveEnv,
			0.0,
			cRTHI_bRHS - cRTHI_bVar,
			0.0
			));

		//RTHI_c
		IloNumVarArray cRTHI_cInterInstVars(cSolveEnv);
		IloNumVar cRTHI_cVar(cSolveEnv, pcTaskArray[iTaskIndex].getCLO(), pcTaskArray[iTaskIndex].getDeadline());
		IloExpr cRTHI_cRHS(cSolveEnv);
		cRTHI_cRHS += pcTaskArray[iTaskIndex].getCLO();
		for (int i = 0; i < iTaskNum; i++)
		{
			if (i == iTaskIndex)	continue;
			if ((m_pcTaskSet->IsSharingRouter(i, iTaskIndex))
				&& (rcPriorityAssignment.getPriorityByTask(i) < iTaskPriority))
			{
				cRTHI_cInterInstVars.add(IloNumVar(cSolveEnv, 0.0, IloInfinity, IloNumVar::Int));
				IloNumVar & rcThisInterInstVar = cRTHI_cInterInstVars[cRTHI_cInterInstVars.getSize() - 1];
				if (pcTaskArray[i].getCriticality())
				{
					SchedCondType cHPKey(i, SchedCondType::DeadlineUBHI);
					cConst.add(IloRange(cSolveEnv,
						0.0,
						rcThisInterInstVar - (cRTHI_cVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCHI()) / m_pcTaskSet->getHIPeriod(i),
						IloInfinity
						));
					cRTHI_cRHS += rcThisInterInstVar * pcTaskArray[i].getCHI();
				}
				else
				{
					if (m_pcTaskSet->getUpDownStreamLOType(iTaskIndex, i))
					{
						SchedCondType cHPKey(i, SchedCondType::DeadlineUBLO);
						cConst.add(IloRange(cSolveEnv,
							0.0,
							rcThisInterInstVar - (cRTHI_cVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCLO()) / m_pcTaskSet->getLOPeriod(i),
							IloInfinity
							));
						cRTHI_cRHS += rcThisInterInstVar * pcTaskArray[i].getCLO();
					}
					else
					{
						SchedCondType cHPKey(i, SchedCondType::DeadlineUBLO);
						cConst.add(IloRange(cSolveEnv,
							0.0,
							rcThisInterInstVar - (cRTHI_bVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCLO()) / m_pcTaskSet->getLOPeriod(i),
							IloInfinity
							));
						cRTHI_cRHS += rcThisInterInstVar * pcTaskArray[i].getCLO();
					}				
				}
				
			}
		}
		cConst.add(IloRange(cSolveEnv,
			0.0,
			cRTHI_cRHS - cRTHI_cVar,
			0.0
			));

		cConst.add(IloRange(cSolveEnv,
			0.0, 
			cRTHIVar - cRTHI_aVar,
			IloInfinity
			));

		cConst.add(IloRange(cSolveEnv,
			0.0,
			cRTHIVar - cRTHI_cVar,
			IloInfinity
			));


		IloObjective cObjective = IloMinimize(cSolveEnv, cRTHIVar);
		IloModel cModel(cSolveEnv);
		cModel.add(cConst);
		cModel.add(cObjective);
		IloCplex cSolver(cModel);
		EnableSolverDisp(0, cSolver);
		setSolverParam(cSolver);

		cSolver.setParam(IloCplex::Param::TimeLimit, 1e74);
		int iRetStatus = 0;

		bool bStatus = cSolver.solve();
		if (bStatus)
		{

		}

		if ((cSolver.getCplexStatus() == cSolver.Optimal) || (cSolver.getCplexStatus() == cSolver.OptimalTol))
			dRet = cSolver.getValue(cRTHIVar);
		else if (cSolver.getCplexStatus() == cSolver.AbortTimeLim)
			dRet = -1;
		else if (cSolver.getCplexStatus() == cSolver.AbortUser)
			dRet = -2;
		else if (cSolver.getCplexStatus() == cSolver.Infeasible)
			dRet = -3;
		else
			dRet = -4;
		cSolveEnv.end();
		return dRet;

	}
	catch (IloException& e)
	{
		cerr << "Concert exception caught: " << e << endl;
		while (1);
	}
}


//NoCFlowDLU_OverApproxAudsley_FocusRefinement
NoCFlowDLU_SingleTaskSched_FocusRefinement::NoCFlowDLU_SingleTaskSched_FocusRefinement()
{
	m_iTargetTask = -1;
	m_cTargetPriorityAssignment = TaskSetPriorityStruct(*m_pcTaskSet);
}

NoCFlowDLU_SingleTaskSched_FocusRefinement::NoCFlowDLU_SingleTaskSched_FocusRefinement(SystemType & rcSystem)
	:NoCFlowDLUFocusRefinement(rcSystem)
{
	m_iTargetTask = -1;
	m_cTargetPriorityAssignment = TaskSetPriorityStruct(*m_pcTaskSet);
}

void NoCFlowDLU_SingleTaskSched_FocusRefinement::setTarget(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcConstSchedConds)
{
	m_iTargetTask = iTaskIndex;
	m_cTargetPriorityAssignment = rcPriorityAssignment;
	m_cConstSchedConds = rcConstSchedConds;
}

void NoCFlowDLU_SingleTaskSched_FocusRefinement::CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);	
	char axBuffer[512] = { 0 };
	for (int i = 0; i < iTaskNum; i++)
	{
		SchedCondType cLOUBKey(i, SchedCondType::DeadlineUBLO);
		SchedCondType cLOLBKey(i, SchedCondType::DeadlineLBLO);
		double dLB = m_cUnschedCoreComputer.SchedCondLowerBound(cLOUBKey);
		if (m_cConstSchedConds.count(cLOLBKey))
			dLB = max(dLB, m_cConstSchedConds[cLOLBKey]);
		double dUB = pcTaskArray[i].getDeadline();
		if (m_cConstSchedConds.count(cLOUBKey))
			dUB = min(dUB, m_cConstSchedConds[cLOUBKey]);		
		sprintf_s(axBuffer, "RLO(%d)", i);
		rcSchedCondTypeVars[cLOUBKey] = IloNumVar(rcEnv, dLB, dUB, IloNumVar::Float, axBuffer);

		if (pcTaskArray[i].getCriticality())
		{
			SchedCondType cHIUBKey(i, SchedCondType::DeadlineUBHI);
			SchedCondType cHILBKey(i, SchedCondType::DeadlineLBHI);
			double dLB = pcTaskArray[i].getCHI();
			if (m_cConstSchedConds.count(cHILBKey))
				dLB = max(dLB, m_cConstSchedConds[cHILBKey]);
			double dUB = pcTaskArray[i].getDeadline();
			if (m_cConstSchedConds.count(cHIUBKey))
				dUB = min(dUB, m_cConstSchedConds[cHIUBKey]);
			sprintf_s(axBuffer, "RHI(%d)", i);
			rcSchedCondTypeVars[cHIUBKey] = IloNumVar(rcEnv, dLB, dUB, IloNumVar::Float, axBuffer);

		}
	}
	
}

void NoCFlowDLUFocusRefinement::GenRTLOModel(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	vector<double> & rvectorDirectJitter = m_pcTaskSet->getDirectJitterTable();
	double dRet = -1;
	int iTaskPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	const IloEnv & cSolveEnv = rcModel.getEnv();
	SchedCondType cThisKey(iTaskIndex, SchedCondType::DeadlineUBLO);
	IloRangeArray cConst(cSolveEnv);			
	IloNumVarArray cInterInstVars(cSolveEnv);
	IloExpr cRTRHS(cSolveEnv);
	cRTRHS += pcTaskArray[iTaskIndex].getCLO();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		if ((m_pcTaskSet->IsSharingRouter(i, iTaskIndex)) && (rcPriorityAssignment.getPriorityByTask(i) < iTaskPriority))
		{
			cInterInstVars.add(IloNumVar(cSolveEnv, 0.0, IloInfinity, IloNumVar::Int));
			IloNumVar & rcThisInterInstVar = cInterInstVars[cInterInstVars.getSize() - 1];
			SchedCondType cHPKey(i, SchedCondType::DeadlineUBLO);
			cConst.add(IloRange(cSolveEnv,
				0.0,
				rcThisInterInstVar - (rcSchedCondTypeVars[cThisKey] + rvectorDirectJitter[i] + rcSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCLO()) / m_pcTaskSet->getLOPeriod(i),
				IloInfinity
				));
			cRTRHS += rcThisInterInstVar * pcTaskArray[i].getCLO();
		}
	}
	cConst.add(IloRange(cSolveEnv,
		0.0,
		cRTRHS - rcSchedCondTypeVars[cThisKey],
		0.0
		));
	rcModel.add(cConst);
}

void NoCFlowDLUFocusRefinement::GenRTHIModel(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	vector<double> & rvectorDirectJitter = m_pcTaskSet->getDirectJitterTable();	
	SchedCondType cThisKey(iTaskIndex, SchedCondType::DeadlineUBHI);
	const IloEnv & cSolveEnv = rcModel.getEnv();
	SchedCondTypeVars & cSchedCondTypeVars = rcSchedCondTypeVars;	
	IloRangeArray cConst(cSolveEnv);	
	int iTaskPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);

	//RTHI_a
	IloNumVar cRTHI_aVar(cSolveEnv, pcTaskArray[iTaskIndex].getCHI(), pcTaskArray[iTaskIndex].getDeadline());
	IloNumVarArray cRTHI_aInterInstVars(cSolveEnv);
	IloExpr cRTHI_aRHS(cSolveEnv);
	cRTHI_aRHS += pcTaskArray[iTaskIndex].getCHI();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		if ((pcTaskArray[i].getCriticality())
			&& (m_pcTaskSet->IsSharingRouter(i, iTaskIndex))
			&& (rcPriorityAssignment.getPriorityByTask(i) < iTaskPriority))
		{
			cRTHI_aInterInstVars.add(IloNumVar(cSolveEnv, 0.0, IloInfinity, IloNumVar::Int));
			IloNumVar & rcThisInterInstVar = cRTHI_aInterInstVars[cRTHI_aInterInstVars.getSize() - 1];
			SchedCondType cHPKey(i, SchedCondType::DeadlineUBHI);
			cConst.add(IloRange(cSolveEnv,
				0.0,
				rcThisInterInstVar - (cRTHI_aVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCHI()) / m_pcTaskSet->getHIPeriod(i),
				IloInfinity
				));
			cRTHI_aRHS += rcThisInterInstVar * pcTaskArray[i].getCHI();
		}
	}
	cConst.add(IloRange(cSolveEnv,
		0.0,
		cRTHI_aRHS - cRTHI_aVar,
		0.0
		));

	//RTHI_b
	IloNumVarArray cRTHI_bInterInstVars(cSolveEnv);
	IloNumVar cRTHI_bVar(cSolveEnv, pcTaskArray[iTaskIndex].getCLO(), pcTaskArray[iTaskIndex].getDeadline());
	IloExpr cRTHI_bRHS(cSolveEnv);
	cRTHI_bRHS += pcTaskArray[iTaskIndex].getCLO();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		if ((m_pcTaskSet->IsSharingRouter(i, iTaskIndex))
			&& (rcPriorityAssignment.getPriorityByTask(i) < iTaskPriority))
		{
			cRTHI_bInterInstVars.add(IloNumVar(cSolveEnv, 0.0, IloInfinity, IloNumVar::Int));
			IloNumVar & rcThisInterInstVar = cRTHI_bInterInstVars[cRTHI_bInterInstVars.getSize() - 1];
			if (pcTaskArray[i].getCriticality())
			{
				SchedCondType cHPKey(i, SchedCondType::DeadlineUBHI);
				cConst.add(IloRange(cSolveEnv,
					0.0,
					rcThisInterInstVar - (cRTHI_bVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCHI()) / m_pcTaskSet->getLOPeriod(i),
					IloInfinity
					));
			}
			else
			{
				SchedCondType cHPKey(i, SchedCondType::DeadlineUBLO);
				cConst.add(IloRange(cSolveEnv,
					0.0,
					rcThisInterInstVar - (cRTHI_bVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCLO()) / m_pcTaskSet->getLOPeriod(i),
					IloInfinity
					));
			}
			cRTHI_bRHS += rcThisInterInstVar * pcTaskArray[i].getCLO();
		}
	}
	cConst.add(IloRange(cSolveEnv,
		0.0,
		cRTHI_bRHS - cRTHI_bVar,
		0.0
		));

	//RTHI_c
	IloNumVarArray cRTHI_cInterInstVars(cSolveEnv);
	IloNumVar cRTHI_cVar(cSolveEnv, pcTaskArray[iTaskIndex].getCLO(), pcTaskArray[iTaskIndex].getDeadline());
	IloExpr cRTHI_cRHS(cSolveEnv);
	cRTHI_cRHS += pcTaskArray[iTaskIndex].getCLO();
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		if ((m_pcTaskSet->IsSharingRouter(i, iTaskIndex))
			&& (rcPriorityAssignment.getPriorityByTask(i) < iTaskPriority))
		{
			cRTHI_cInterInstVars.add(IloNumVar(cSolveEnv, 0.0, IloInfinity, IloNumVar::Int));
			IloNumVar & rcThisInterInstVar = cRTHI_cInterInstVars[cRTHI_cInterInstVars.getSize() - 1];
			if (pcTaskArray[i].getCriticality())
			{
				SchedCondType cHPKey(i, SchedCondType::DeadlineUBHI);
				cConst.add(IloRange(cSolveEnv,
					0.0,
					rcThisInterInstVar - (cRTHI_cVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCHI()) / m_pcTaskSet->getHIPeriod(i),
					IloInfinity
					));
				cRTHI_cRHS += rcThisInterInstVar * pcTaskArray[i].getCHI();
			}
			else
			{
				if (m_pcTaskSet->getUpDownStreamLOType(iTaskIndex, i))
				{
					SchedCondType cHPKey(i, SchedCondType::DeadlineUBLO);
					cConst.add(IloRange(cSolveEnv,
						0.0,
						rcThisInterInstVar - (cRTHI_cVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCLO()) / m_pcTaskSet->getLOPeriod(i),
						IloInfinity
						));
					cRTHI_cRHS += rcThisInterInstVar * pcTaskArray[i].getCLO();
				}
				else
				{
					SchedCondType cHPKey(i, SchedCondType::DeadlineUBLO);
					cConst.add(IloRange(cSolveEnv,
						0.0,
						rcThisInterInstVar - (cRTHI_bVar + rvectorDirectJitter[i] + cSchedCondTypeVars[cHPKey] - pcTaskArray[i].getCLO()) / m_pcTaskSet->getLOPeriod(i),
						IloInfinity
						));
					cRTHI_cRHS += rcThisInterInstVar * pcTaskArray[i].getCLO();
				}
			}

		}
	}
	cConst.add(IloRange(cSolveEnv,
		0.0,
		cRTHI_cRHS - cRTHI_cVar,
		0.0
		));

	cConst.add(IloRange(cSolveEnv,
		0.0,
		cSchedCondTypeVars[cThisKey] - cRTHI_aVar,
		IloInfinity
		));

	cConst.add(IloRange(cSolveEnv,
		0.0,
		cSchedCondTypeVars[cThisKey] - cRTHI_cVar,
		IloInfinity
		));
	rcModel.add(cConst);
}

void NoCFlowDLUFocusRefinement::GenRTConst(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	assert(iTaskIndex < iTaskNum);
	GenRTLOModel(iTaskIndex, rcPriorityAssignment, rcSchedCondTypeVars, rcModel);
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		GenRTHIModel(iTaskIndex, rcPriorityAssignment, rcSchedCondTypeVars, rcModel);
	}	
}

int NoCFlowDLU_SingleTaskSched_FocusRefinement::SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */)
{
	try
	{
		GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
		IloEnv cSolveEnv;
		SchedCondTypeVars cSchedCondTypeVars;
		SchedCondVars cSchedCondVars;
		IloRangeArray cConst(cSolveEnv);
		IloNumVar cDummyInteger(cSolveEnv, 0.0, 1.0, IloNumVar::Int);
		CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);		
		CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);
		GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);		
		GenSchedCondTypeConst(cSolveEnv, cSchedCondTypeVars, cSchedCondVars, cConst);		
		IloExpr cObjExpr(cSolveEnv);				
		for (int i = 0; i < iTaskNum; i++)
		{
			if (i == m_iTargetTask)	continue;
			cObjExpr += -cSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBLO)];			
			if (pcTaskArray[i].getCriticality())
			{
				cObjExpr += -cSchedCondTypeVars[NoCFlowDLUSchedCond(i, NoCFlowDLUSchedCond::DeadlineUBHI)];				
			}
		}
				
		IloObjective cObjective = IloMinimize(cSolveEnv, cObjExpr);		
		IloModel cModel(cSolveEnv);		
		cModel.add(cObjExpr <= 0);
		cModel.add(cConst);
		cModel.add(cDummyInteger);
	//	cModel.add(cObjective);

		if (m_iTargetTask != -1)
		{
			GenRTConst(m_iTargetTask, m_cTargetPriorityAssignment, cSchedCondTypeVars, cModel);
		}
#if 1
		//Lower Priority Burden
		int iTargetPriority = m_cTargetPriorityAssignment.getPriorityByTask(m_iTargetTask);
		for (int iLP = iTaskNum - 1; iLP > iTargetPriority; iLP--)
		{
			int iLPTask = m_cTargetPriorityAssignment.getTaskByPriority(iLP);
			GenRTConst(iLPTask, m_cTargetPriorityAssignment, cSchedCondTypeVars, cModel);
		}
#endif

		GenMustHPSetsConst(m_mapMustHPSets, cSchedCondTypeVars, cModel);

		IloCplex cSolver(cModel);		
		EnableSolverDisp(iDisplay, cSolver);
		setSolverParam(cSolver);
		cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
		int iRetStatus = 0;
		cSolver.exportModel("Temp.lp");
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
			//Adjust Result.
			IloModel cAdjModel(cSolveEnv);
			IloExpr cAdjObjExpr(cSolveEnv);
			for (SchedCondTypeVars::iterator iter = cSchedCondTypeVars.begin(); iter != cSchedCondTypeVars.end(); iter++)
			{
				cAdjModel.add(iter->second >= round(cSolver.getValue(iter->second)));
				cAdjObjExpr += -iter->second;
			}
			GenRTConst(m_iTargetTask, m_cTargetPriorityAssignment, cSchedCondTypeVars, cAdjModel);
			cAdjModel.add(IloMinimize(cSolveEnv, cAdjObjExpr));
			cAdjModel.add(cDummyInteger);
			IloCplex cAdjSolver(cAdjModel);
			EnableSolverDisp(iDisplay, cAdjSolver);
			setSolverParam(cAdjSolver);
			if (iDisplay >= 1)
			{
				cout << "Adjustment Stage" << endl;
			}			
			double dAdjWallTime = cAdjSolver.getCplexTime();
			double dAdjCPUTime = getCPUTimeStamp();
			bool bAdjStatus = cAdjSolver.solve();
			dAdjCPUTime = getCPUTimeStamp() - dAdjCPUTime;
			dAdjWallTime = cAdjSolver.getCplexTime() - dAdjWallTime;
			m_dSolveWallTime += dAdjWallTime;
			m_dSolveCPUTime += dAdjCPUTime;
			if (bAdjStatus)
				cSolver = cAdjSolver;						
			ExtractAllSolution(cSolver, cSchedCondTypeVars);
			MarshalAllSolution(m_dequeSolutionSchedConds);		
			if (!bAdjStatus)
			{
				cout << "Adjustment Failed ?" << endl;
				m_cUnschedCoreComputer.PrintSchedConds(m_cSchedCondsConfig, cout); cout << endl;
			}
			bool bVerifyStatus = VerifyResult(m_iTargetTask, m_cTargetPriorityAssignment, m_cSchedCondsConfig);
			if (!bVerifyStatus)
			{
				cout << "Verified Failed" << endl;
				while (1);
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

bool NoCFlowDLU_SingleTaskSched_FocusRefinement::VerifyResult(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsConfig)
{
	vector<double> vectorJitterLO, vectorJitterHI;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorJitterLO.push_back(pcTaskArray[i].getDeadline());
		vectorJitterHI.push_back(pcTaskArray[i].getDeadline());
	}

	for (SchedConds::iterator iter = rcSchedCondsConfig.begin(); iter != rcSchedCondsConfig.end(); iter++)
	{		
		int iTaskIndex = iter->first.iTaskIndex;
		if (iter->first.enumCondType == SchedCondType::DeadlineUBLO)
		{
			vectorJitterLO[iTaskIndex] = min(vectorJitterLO[iTaskIndex], iter->second);
		}
		else if (iter->first.enumCondType == SchedCondType::DeadlineUBHI)
		{
			vectorJitterHI[iTaskIndex] = min(vectorJitterHI[iTaskIndex], iter->second);
		}
		else
		{
			cout << "Unexpected SchedCondType" << endl;
			while (1);
		}
	}

	for (int i = 0; i < iTaskNum; i++)
	{		
		//cout << "Task " << i << ": " << vectorJitterLO[i] << " " << vectorJitterHI[i] << endl;
		vectorJitterHI[i] = vectorJitterHI[i] - pcTaskArray[i].getCHI();
		vectorJitterLO[i] = vectorJitterLO[i] - pcTaskArray[i].getCLO();
	}

	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(*m_pcTaskSet, m_pcTaskSet->getDirectJitterTable(), vectorJitterLO, vectorJitterHI, rcPriorityAssignment);
	double dRTLO = cRTCalc.RTLO(iTaskIndex);
	if (dRTLO > pcTaskArray[iTaskIndex].getDeadline())
	{
		cout << "LO RT: " << dRTLO << endl;
		return false;
	}
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRTHI = cRTCalc.WCRTHI(iTaskIndex);
		if (dRTHI > pcTaskArray[iTaskIndex].getDeadline())
		{
			cout << "HI RT: " << dRTHI << endl;
			return false;
		}
	}
	return true;
}

void NoCFlowDLUFocusRefinement::GenMustHPSetsConst(map<int, TaskSetPriorityStruct> & rmapMustHPSets, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel)
{
	for (map<int, TaskSetPriorityStruct>::iterator iter = rmapMustHPSets.begin(); iter != rmapMustHPSets.end(); iter++)
	{
		int iTaskIndex = iter->first;
		GenRTConst(iTaskIndex, iter->second, rcSchedCondTypeVars, rcModel);
	}
}



void TestNoCFR()
{
	if (0)
	{
		struct ExampleTaskSetConfig
		{
			double dPeriod;
			double dUtil;
			bool bCriticality;
		}atagConfig[] = {
			{ 100, 40 / 200.0, false },
			{ 200, 49 / 200.0, false },
			{ 400, 80 / 200.0, false },
			//{ 50, 15 / 50.0, false },
			//{ 100, 10 / 100.0, false },
		};
		FlowsSet_2DNetwork cTaskSet;
		int iNTask = sizeof(atagConfig) / sizeof(struct ExampleTaskSetConfig);
		for (int i = 0; i < iNTask; i++)
		{
			cTaskSet.AddTask(atagConfig[i].dPeriod, atagConfig[i].dPeriod, atagConfig[i].dUtil, 2.0, atagConfig[i].bCriticality);
		}
		cTaskSet.ConstructTaskArray();
		cTaskSet.GenerateRandomFlowRoute(1, 1);

		NoCFlowFocusRefinement cFRModel(cTaskSet);
		cFRModel.setCorePerIter(5);
		cFRModel.setSubILPDisplay(0);
		cFRModel.setLogFile("Temp_log_NoC.txt");
		cout << "Begin FR..." << endl;
		cFRModel.FocusRefinement(1);

		system("pause");
	}

	FlowsSet_2DNetwork cFlowSet;
#if 1
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
#else
	if (1)
	{
		cFlowSet.GenRandomTaskSetInteger(40, 0.9997, 0.5, 2.0);
		cFlowSet.GenerateRandomFlowRoute(3, 3);
	}
	else
		cFlowSet.GenRandomFlow_2014Burns(50, 1000, 0.5, 0.03, 2.0, 4, 4);
	cFlowSet.WriteImageFile("NoCFlowSetForFR.tskst");
	cFlowSet.WriteTextFile("NoCFlowSetForFR.tskst.txt");
#endif

	TaskSetPriorityStruct cDMPA = cFlowSet.GetDMPriorityAssignment();
	NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cDMPA, cFlowSet.getDirectJitterTable());
	cRTCalc.ComputeAllResponseTime();
	cout << "DMPA Schedulability: " << cRTCalc.IsSchedulable() << endl;

	if (1)
	{
	FOCUS_REFINEMENT:
		NoCFlowFocusRefinement cFRModel(cFlowSet);
		cFRModel.setCorePerIter(5);
		cFRModel.setLogFile("Temp_log_NoC.txt");
		cout << "Begin FR..." << endl;
		cFRModel.FocusRefinement(1);
		TaskSetPriorityStruct & rcPriorityAssignment = cFRModel.getPriorityAssignmentSol();
		rcPriorityAssignment.WriteTextFile("NoCPASol.txt");
		cout << "Verifying by MILP.... " << endl;
		NoCFlowRTMILPModel cModel(cFlowSet);
		cModel.setPredefinePA(rcPriorityAssignment);
		int iStatus = cModel.Solve(2);
		cout << "MILP Status: " << iStatus << endl;
	}

	if (1)
	{
	FULLMILP:
		NoCFlowRTMILPModel cModel(cFlowSet);
		cModel.Solve(3);
	}
}

void TestNoCFR_MonoInt()
{
	FlowsSet_2DNetwork cFlowSet;
#if 1
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
#else
	if (1)
	{
		cFlowSet.GenRandomTaskSetInteger(60, 1.187, 0.5, 2.0);
		cFlowSet.GenerateRandomFlowRoute(4, 4);
	}
	else
		cFlowSet.GenRandomFlow_2014Burns(50, 1000, 0.5, 0.03, 2.0, 4, 4);
	cFlowSet.WriteImageFile("NoCFlowSetForFR.tskst");
	cFlowSet.WriteTextFile("NoCFlowSetForFR.tskst.txt");
#endif

	TaskSetPriorityStruct cDMPA = cFlowSet.GetDMPriorityAssignment();
	cDMPA.ReadTextFile("HardNoCCasePA.txt");
	NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cDMPA, cFlowSet.getDirectJitterTable());
	cRTCalc.ComputeAllResponseTime();
	cout << "DMPA Schedulability: " << cRTCalc.IsSchedulable() << endl;
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	typedef NoCFlowDLUFocusRefinement::SchedConds SchedConds;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	SchedConds cRefSchedConds;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (cRTCalc.getRTLO(i) > pcTaskArray[i].getDeadline()) continue;
		cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineLBLO)] = round(cRTCalc.getRTLO(i));
		cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineUBLO)] = round(cRTCalc.getRTLO(i));

		if (pcTaskArray[i].getCriticality())
		{
			if (cRTCalc.getRTHI(i) > pcTaskArray[i].getDeadline()) continue;
			cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineLBHI)] = round(cRTCalc.getRTHI(i));
			cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineUBHI)] = round(cRTCalc.getRTHI(i));
		}
	}


	if (0)
	{
	FOCUS_REFINEMENT:
		cout << "FocusRefinement" << endl;
		NoCFlowFocusRefinement cFRModel(cFlowSet);
		cFRModel.setCorePerIter(5);
		cFRModel.setLogFile("Temp_log_NoC.txt");
		cout << "Begin FR..." << endl;
		int iStatus = cFRModel.FocusRefinement(1);
		if (iStatus == 1)
		{
			TaskSetPriorityStruct & rcPriorityAssignment = cFRModel.getPriorityAssignmentSol();
			rcPriorityAssignment.WriteTextFile("NoCPASol.txt");
			cout << "Verifying by MILP.... " << endl;
			NoCFlowRTMILPModel cModel(cFlowSet);
			cModel.setPredefinePA(rcPriorityAssignment);
			iStatus = cModel.Solve(2);
			cout << "MILP Status: " << iStatus << endl;
		}

	}

	if (1)
	{
	FOCUSREFINEMENT_MONOINT:
		NoCFlowDLUUnschedCoreComputer cUCCalc(cFlowSet);
		SchedConds cDummy;
		map<int, TaskSetPriorityStruct> mapMustHPSets;
		for (int i = 0; i < iTaskNum; i++)
		{
			TaskSetPriorityStruct cPA(cFlowSet);
			cUCCalc.HighestPriorityAssignment(i, cDummy, cDummy, cPA);
			if (cPA.getPriorityByTask(i) > 0)
				mapMustHPSets[i] = cPA;
		}
		cout << "MonoInt FocusRefinement: " << endl;
		NoCFlowDLUFocusRefinement cFRModel(cFlowSet);
		//cFRModel.setMustHPSets(mapMustHPSets);
		cout << "Reference SchedConds: " << endl;
		cFRModel.getUnschedCoreComputer().PrintSchedConds(cRefSchedConds, cout);
		cout << endl;
		cFRModel.setReferenceSchedConds(cRefSchedConds);
		cFRModel.setCorePerIter(5);
		cFRModel.setLogFile("Temp_log_NoC_MonoInt.txt");
		cout << "Begin FR..." << endl;
		int iStatus = cFRModel.FocusRefinement(1);
		if (iStatus == 1)
		{
			TaskSetPriorityStruct & rcPriorityAssignment = cFRModel.getPriorityAssignmentSol();
			rcPriorityAssignment.WriteTextFile("NoCPASol_MonoInt.txt");
			cout << "Verifying by MILP.... " << endl;
			NoCFlowRTMILPModel cModel(cFlowSet);
			cModel.setPredefinePA(rcPriorityAssignment);
			iStatus = cModel.Solve(2);
			cout << "MILP Status: " << iStatus << endl;
		}
	}

	if (1)
	{
	FULLMILP:
		NoCFlowRTMILPModel cModel(cFlowSet);
		cModel.Solve(3);
	}
}

void TestNoCMILPRT()
{
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	NoCFlowDLUFocusRefinement cFRModel(cFlowSet);
	typedef NoCFlowDLUFocusRefinement::UnschedCores UnschedCores;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	NoCFlowDLUFocusRefinement::UnschedCoreComputer_MonoInt &  rcUnschedCoreComp = cFRModel.getUnschedCoreComputer();
	UnschedCores cUCs;
	cFRModel.ReadUnschedCoreFromFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\Temp_log_NoC_MonoInt.txt", cUCs);
	cout << "UC size read: " << cUCs.size() << endl;
	system("pause");
	TaskSetPriorityStruct cPA(cFlowSet);
	vector<double> vectorJitterLO, vectorJitterHI;
	vectorJitterLO.reserve(iTaskNum);
	vectorJitterHI.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorJitterHI.push_back(rcUnschedCoreComp.SchedCondLowerBound(SchedCondType(i, SchedCondType::DeadlineLBHI)) - pcTaskArray[i].getCHI());
		vectorJitterLO.push_back(rcUnschedCoreComp.SchedCondLowerBound(SchedCondType(i, SchedCondType::DeadlineLBLO)) - pcTaskArray[i].getCLO());
	}
	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(cFlowSet, cFlowSet.getDirectJitterTable(), vectorJitterLO, vectorJitterHI, cPA);
	for (int iPriority = iTaskNum - 1; iPriority--; iPriority >= 0)
	{
		bool bAssigned = false;
		cout << "Priority Level: " << iPriority << endl;
		for (int i = 0; i < iTaskNum; i++)
		{
			if (cPA.getPriorityByTask(i) != -1)	continue;
			cPA.setPriority(i, iPriority);
			cout << "Try task " << i << " Deadline: " << pcTaskArray[i].getDeadline() << endl;
			double dRTLO = cRTCalc.RTLO(i);
			if (dRTLO > pcTaskArray[i].getDeadline())
			{
				cout << "Failed" << endl;
				cPA.unset(i);
				continue;
			}
			double dRTLO_MILP = cFRModel.ComputeLORTByMILP(i, cPA, cUCs);
			printf_s("RTLO: %.2f(Iterative), %.2f(MILP); \n", dRTLO, dRTLO_MILP);
			double dRTHI = 0;
			double dRTHI_MILP = 0;
			if (pcTaskArray[i].getCriticality())
			{
				dRTHI = cRTCalc.WCRTHI(i);
				if (dRTHI > pcTaskArray[i].getDeadline())
				{
					cout << "Failed" << endl;
					cPA.unset(i);
					continue;
				}
				dRTHI_MILP = cFRModel.ComputeHIRTByMILP(i, cPA, cUCs);
				printf_s("RTHI: %.2f(Iterative), %.2f(MILP); \n", dRTHI, dRTHI_MILP);
			}
			bAssigned = true;
			cout << "-----------------------------------------" << endl;
			cout << "Priority: " << iPriority << endl;
			cout << "Task " << i << endl;
			cout << "LORT: " << dRTLO << " " << dRTLO_MILP << endl;
			if (pcTaskArray[i].getCriticality())
				cout << "HIRT: " << dRTHI << " " << dRTHI_MILP << endl;
			system("pause");
			break;
		}
	}

}

void TestNoCMILPUCRTComputation()
{
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\NoCFlowSetForFR.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	NoCFlowDLUUnschedCoreComputer cUCCalc(cFlowSet);
	typedef NoCFlowDLUUnschedCoreComputer::SchedCondType SchedCondType;
	typedef NoCFlowDLUUnschedCoreComputer::SchedConds SchedConds;
	typedef NoCFlowDLUUnschedCoreComputer::SchedCond SchedCond;
	NoCFlowDLUUnschedCoreComputer::UnschedCores cUCs;
	NoCFlowDLUFocusRefinement cFRModel(cFlowSet);
	TaskSetPriorityStruct cPA(cFlowSet);
}

void TestNoCFlowDLU_SingleTaskSched_FocusRefinement()
{
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	NoCFlowDLUFocusRefinement cFRModel(cFlowSet);
	typedef NoCFlowDLUFocusRefinement::UnschedCores UnschedCores;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	NoCFlowDLUFocusRefinement::UnschedCoreComputer_MonoInt &  rcUnschedCoreComp = cFRModel.getUnschedCoreComputer();
	UnschedCores cUCs;	
	TaskSetPriorityStruct cPA(cFlowSet);
	vector<double> vectorJitterLO, vectorJitterHI;
	vectorJitterLO.reserve(iTaskNum);
	vectorJitterHI.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorJitterHI.push_back(rcUnschedCoreComp.SchedCondLowerBound(SchedCondType(i, SchedCondType::DeadlineLBHI)) - pcTaskArray[i].getCHI());
		vectorJitterLO.push_back(rcUnschedCoreComp.SchedCondLowerBound(SchedCondType(i, SchedCondType::DeadlineLBLO)) - pcTaskArray[i].getCLO());
	}
	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(cFlowSet, cFlowSet.getDirectJitterTable(), vectorJitterLO, vectorJitterHI, cPA);
	NoCFlowDLU_SingleTaskSched_FocusRefinement::SchedConds cDummy;
	map<int, UnschedCores> cTask2Cores;
	for (int iPriority = iTaskNum - 1; iPriority--; iPriority >= 0)
	{
		bool bAssigned = false;		
		for (int i = 0; i < iTaskNum; i++)
		{
			if (cPA.getPriorityByTask(i) != -1)	continue;
			cPA.setPriority(i, iPriority);			
			double dRTLO = cRTCalc.RTLO(i);
			if (dRTLO > pcTaskArray[i].getDeadline())
			{				
				cPA.unset(i);
				continue;
			}			
			double dRTHI = 0;
			double dRTHI_MILP = 0;
			if (pcTaskArray[i].getCriticality())
			{
				dRTHI = cRTCalc.WCRTHI(i);
				if (dRTHI > pcTaskArray[i].getDeadline())
				{
					cPA.unset(i);
					continue;
				}								
			}
			bAssigned = true;
			cout << "-----------------------------------------" << endl;
			cout << "Priority: " << iPriority << endl;
			cout << "Task " << i << endl;
			cout << "LORT: " << dRTLO << endl;
			if (pcTaskArray[i].getCriticality())
				cout << "HIRT: " << dRTHI << endl;			
			system("pause");

			cout << "Verify Using SingleTaskSched FocusRefinement" << endl;
			NoCFlowDLU_SingleTaskSched_FocusRefinement cSingleSchedFR(cFlowSet);
			cSingleSchedFR.setTarget(i, cPA, cDummy);
			cSingleSchedFR.setCorePerIter(5);
			//cSingleSchedFR.setSubILPDisplay(2);
			cSingleSchedFR.setLogFile("SingleSchedVerifyFRLog.txt");
			if (cTask2Cores.count(i))
			{
				cSingleSchedFR.LoadUnschedCores(cTask2Cores[i]);
			}
			int iStatus = cSingleSchedFR.FocusRefinement(1);
			if (iStatus == 0)
			{
				cTask2Cores[i] = cSingleSchedFR.getUnschedCores();
				cPA.unset(i);
				continue;
			}
			break;
		}
		if (!bAssigned)
		{
			cout << "Infeasible " << endl;
			while (1);
		}
	}

	NoCFlowAMCRTCalculator_DerivedJitter cDerivedRTCalc(cFlowSet, cPA, cFlowSet.getDirectJitterTable());
	cDerivedRTCalc.ComputeAllResponseTime();
	cout << "Real Schedulability: " << endl;
	cout << cDerivedRTCalc.IsSchedulable() << endl;
}

bool IsSchedulableFR(FlowsSet_2DNetwork & rcFlowSet, NoCFlowDLUFocusRefinement::SchedConds & rcSchedConds, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(&rcFlowSet, iTaskNum, pcTaskArray);
	typedef NoCFlowDLUFocusRefinement::SchedConds SchedConds;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	typedef NoCFlowDLUFocusRefinement::UnschedCores UnschedCores;
	map<int, UnschedCores> cTask2Cores;
	TaskSetPriorityStruct & cPA = rcPriorityAssignment;
	NoCFlowDLUUnschedCoreComputer cUCCalc(rcFlowSet);	
	for (int iPriority = iTaskNum - 1; iPriority >= 0; iPriority--)
	{
		cout << "-----------------------------------------" << endl;
		cout << "Priority: " << iPriority << endl;
		bool bAssigned = false;
		deque<int> dequeCandidate;
		//Find must-schedulable candidate;
		for (int i = 0; i < iTaskNum; i++)
		{
			if (cPA.getPriorityByTask(i) != -1)	continue;
			cPA.setPriority(i, iPriority);
			//double dRTPes = cUCCalc.ComputePessimisticRT(i, cCarryIn, cPA);
			//double dRTOpt = cUCCalc.ComputeOptimisticRT(i, cCarryIn, cPA);
			if (cUCCalc.SchedTestOptimistic(i, rcSchedConds, cPA) == false)
			{
				cPA.unset(i);
				continue;
			}

			if (cUCCalc.SchedTestPessimistic(i, rcSchedConds, cPA) == false)
			{
				dequeCandidate.push_back(i);
				cPA.unset(i);
				continue;
			}
			bAssigned = true;
			cout << "Task " << i << " Succeeds" << endl;
			cout << "Pessimistic Schedulable" << endl;
			system("pause");
			break;
		}

		if (bAssigned)	continue;

		//if not test possible candidates
		cout << "FR Test Candidates: ";
		for (deque<int>::iterator iterCand = dequeCandidate.begin(); iterCand != dequeCandidate.end(); iterCand++)
		{
			cout << *iterCand << ", ";
		}
		cout << endl;
		for (deque<int>::iterator iterCand = dequeCandidate.begin(); iterCand != dequeCandidate.end(); iterCand++)
		{
			int iCand = *iterCand;
			cPA.setPriority(iCand, iPriority);
			cout << "SingleTaskSched FocusRefinement for Task " << iCand << endl;
			NoCFlowDLU_SingleTaskSched_FocusRefinement cSingleSchedFR(rcFlowSet);
			cSingleSchedFR.setTarget(iCand, cPA, rcSchedConds);
			cSingleSchedFR.setCorePerIter(5);
			//cSingleSchedFR.setSubILPDisplay(2);
			cSingleSchedFR.setLogFile("SingleSchedVerifyFRLog.txt");
			if (cTask2Cores.count(iCand))
			{
				cSingleSchedFR.LoadUnschedCores(cTask2Cores[iCand]);
			}
			int iStatus = cSingleSchedFR.FocusRefinement(1);
			if (iStatus == 0)
			{
				cTask2Cores[iCand] = cSingleSchedFR.getUnschedCores();
				cPA.unset(iCand);
				continue;
			}			
			bAssigned = true;
			cout << "Task " << iCand << " Succeeds" << endl;
			cout << "FR Schedulable" << endl;						
			system("pause");
			break;
		}
		if (!bAssigned)
		{
			cout << "Infeasible" << endl;
			return false;
		}

	}
	return true;
}

void NoCAudsleyFR()
{
	typedef NoCFlowDLU_SingleTaskSched_FocusRefinement::SchedConds SchedConds;
	typedef NoCFlowDLUFocusRefinement::UnschedCores UnschedCores;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	UnschedCores cUCs;
	TaskSetPriorityStruct cPA(cFlowSet);
	NoCFlowDLUUnschedCoreComputer cUCCalc(cFlowSet);
	cUCCalc.InitializeDeadlineLBLB();
	map<int, UnschedCores> cTask2Cores;
	SchedConds cDummy;
	//Must Schedulable
	int iCurrentPriority = iTaskNum - 1;
	for (; iCurrentPriority >= 0; iCurrentPriority--)
	{
		bool bAssigned = false;
		for (int i = 0; i < iTaskNum; i++)
		{
			if (cPA.getPriorityByTask(i) != -1)	continue;
			cPA.setPriority(i, iCurrentPriority);
			if (cUCCalc.SchedTestPessimistic(i, cDummy, cPA) == false)
			{
				cPA.unset(i);
				continue;
			}
			bAssigned = true;
			break;
		}
		if (bAssigned == false)
			break;
	}
	map<int, UnschedCores> mapTask2Cores;
	cout << "Start Audsley BnB" << endl;
	for (; iCurrentPriority >= 0; iCurrentPriority--)
	{
		bool bAssigned = false;
		for (int i = 0; i < iTaskNum; i++)
		{
			int iCand = i;
			assert(cPA.getTaskByPriority(iCurrentPriority) == -1);
			if (cPA.getPriorityByTask(iCand) != -1)	continue;
			cPA.setPriority(iCand, iCurrentPriority);
			if (cUCCalc.SchedTestOptimistic(iCand, cDummy, cPA) == false)
			{
				cPA.unset(iCand);
				continue;
			}

			NoCFlowDLU_SingleTaskSched_FocusRefinement cSingleSchedFR(cFlowSet);
			cSingleSchedFR.getUnschedCoreComputer() = cUCCalc;
			if (mapTask2Cores.count(iCand))	cSingleSchedFR.LoadUnschedCores(mapTask2Cores[iCand]);			
			cout << "FR Sched Test on Task " << iCand << "..." << endl;
			cSingleSchedFR.setTarget(iCand, cPA, cDummy);
			cSingleSchedFR.setCorePerIter(5);
			cSingleSchedFR.setLogFile("SingleSchedVerifyFRLog.txt");
			int iSchedStatus = cSingleSchedFR.FocusRefinement(1);
			mapTask2Cores[iCand] = cSingleSchedFR.getUnschedCores();
			if (iSchedStatus != 1)
			{
				cPA.unset(iCand);
				continue;
			}
			bAssigned = true;
			break;
		}
		if (bAssigned == false)
		{
			cout << "Infeasible" << endl;
			while (1);
		}
		cout << "--------------------------------------------------" << endl;
		cout << "Priority Level: " << iCurrentPriority << endl;
		cout << "Assignment: ";
		for (int iPriority = iTaskNum - 1; iPriority >= 0; iPriority--)
		{
			int iAssignedTask = cPA.getTaskByPriority(iPriority);
			if (iAssignedTask == -1)	break;
			cout << iAssignedTask << ",";
		}
		cout << endl;
		system("pause");
	}
#if 1
	cout << cPA.getSize() << endl;
	//cPA.Print();
	NoCFlowAMCRTCalculator_DerivedJitter cDerivedRTCalc(cFlowSet, cPA, cFlowSet.getDirectJitterTable());
	cDerivedRTCalc.ComputeAllResponseTime();
	cout << "Real Schedulability: " << endl;
	cDerivedRTCalc.PrintSchedulabilityInfo();
	cPA.WriteTextFile("HardNoCCasePA.txt");
#endif
}

void NoCAudsleyBnBFR()
{
	typedef NoCFlowDLU_SingleTaskSched_FocusRefinement::SchedConds SchedConds;
	typedef NoCFlowDLUFocusRefinement::UnschedCores UnschedCores;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);			
	UnschedCores cUCs;
	TaskSetPriorityStruct cPA(cFlowSet);		
	NoCFlowDLUUnschedCoreComputer cUCCalc(cFlowSet);
	cUCCalc.InitializeDeadlineLBLB();
	map<int, UnschedCores> cTask2Cores;
	SchedConds cDummy;
	//Must Schedulable
	int iCurrentPriority = iTaskNum - 1;
	for (; iCurrentPriority >= 0; iCurrentPriority--)
	{		
		bool bAssigned = false;
		for (int i = 0; i < iTaskNum; i++)
		{
			if (cPA.getPriorityByTask(i) != -1)	continue;
			cPA.setPriority(i, iCurrentPriority);
			if (cUCCalc.SchedTestPessimistic(i, cDummy, cPA) == false)
			{
				cPA.unset(i);
				continue;
			}			
			bAssigned = true;
			break;
		}
		if (bAssigned == false)
			break;
	}
	cout << "Start Audsley BnB" << endl;
	cout << "Result: " << NoCAudsleyBnBFR_Recur(iCurrentPriority, cFlowSet, cPA, cUCCalc) << endl;
	cPA.WriteTextFile("AudsleyBnB.txt");
}

int NoCAudsleyBnBFR_Recur(int iCurrentPriority, FlowsSet_2DNetwork & rcFlowSet, TaskSetPriorityStruct & rcPriorityAssignment, 
	NoCFlowDLUUnschedCoreComputer & rcUCCalc)
{
	GET_TASKSET_NUM_PTR(&rcFlowSet, iTaskNum, pcTaskArray);
	deque<int> dequeCandidate;	
	typedef NoCFlowDLUUnschedCoreComputer::SchedCond	SchedCond;
	typedef NoCFlowDLUUnschedCoreComputer::SchedConds	SchedConds;
	SchedConds cDummy;

	if (rcPriorityAssignment.getSize() == iTaskNum)	return 1;

	//Gather Candidate
	for (int i = 0; i < iTaskNum; i++)
	{
		if (rcPriorityAssignment.getPriorityByTask(i) != -1)	continue;
		rcPriorityAssignment.setPriority(i, iCurrentPriority);
		if (rcUCCalc.SchedTestOptimistic(i, cDummy, rcPriorityAssignment) == true)
		{
			dequeCandidate.push_back(i);
		}		
		rcPriorityAssignment.unset(i);
	}
	cout << "--------------------------------------------------" << endl;
	cout << "Priority Level: " << iCurrentPriority << endl;
	cout << "Assignment: ";
	for (int iPriority = iTaskNum - 1; iPriority >= 0; iPriority--)
	{
		int iAssignedTask = rcPriorityAssignment.getTaskByPriority(iPriority);
		if (iAssignedTask == -1)	break;
		cout << iAssignedTask << ",";
	}
	cout << endl;
	cout << "Candidate: ";
	for (deque<int>::iterator iter = dequeCandidate.begin(); iter != dequeCandidate.end(); iter++)
	{
		cout << *iter << ", ";
	}
	cout << endl;
	system("pause");
	//Branch on the decision
#define PRETEST	0
	deque<int> dequeFeasibleCandidate;
	for (deque<int>::iterator iter = dequeCandidate.begin(); iter != dequeCandidate.end(); iter++)
	{
		int iCand = *iter;
		NoCFlowDLU_SingleTaskSched_FocusRefinement cSingleSchedFR(rcFlowSet);
		cSingleSchedFR.getUnschedCoreComputer() = rcUCCalc;
		rcPriorityAssignment.setPriority(iCand, iCurrentPriority);
		cout << "FR Sched Test on Task " << iCand << "..." << endl;
		cSingleSchedFR.setTarget(iCand, rcPriorityAssignment, cDummy);
		cSingleSchedFR.setCorePerIter(5);		
		cSingleSchedFR.setLogFile("SingleSchedVerifyFRLog.txt");
		int iSchedStatus = cSingleSchedFR.FocusRefinement(1);
		if (iSchedStatus == 1)
		{
			dequeFeasibleCandidate.push_back(iCand);
#if !PRETEST
			int iStatus = NoCAudsleyBnBFR_Recur(iCurrentPriority - 1, rcFlowSet, rcPriorityAssignment, rcUCCalc);
			if (iStatus == 1)
			{
				return 1;
			}
#endif
		}
		rcPriorityAssignment.unset(iCand);
	}
#if PRETEST
	cout << "Feasible Candidate: ";
	for (deque<int>::iterator iter = dequeFeasibleCandidate.begin(); iter != dequeFeasibleCandidate.end(); iter++)
	{
		cout << *iter << ", ";
	}
	cout << endl;
#endif

#if PRETEST
	for (deque<int>::iterator iter = dequeFeasibleCandidate.begin(); iter != dequeFeasibleCandidate.end(); iter++)
	{
		int iCand = *iter;
		rcPriorityAssignment.setPriority(iCand, iCurrentPriority);
		int iStatus = NoCAudsleyBnBFR_Recur(iCurrentPriority - 1, rcFlowSet, rcPriorityAssignment, rcUCCalc);
		if (iStatus == 1)
		{
			return 1;
		}
		rcPriorityAssignment.unset(iCand);
	}
#endif
	return 0;
}

void TestNoCFRBasedSchedTest()
{
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	NoCFlowDLUFocusRefinement cFRModel(cFlowSet);
	typedef NoCFlowDLUFocusRefinement::UnschedCores UnschedCores;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	NoCFlowDLUFocusRefinement::UnschedCoreComputer_MonoInt &  rcUnschedCoreComp = cFRModel.getUnschedCoreComputer();
	NoCFlowDLU_SingleTaskSched_FocusRefinement::SchedConds cCarryIn, cDummy;
	typedef NoCFlowDLU_SingleTaskSched_FocusRefinement::SchedConds SchedConds;
	NoCFlowDLUUnschedCoreComputer cUCCalc(cFlowSet);
	SchedConds cOrigUC, cNewSchedConds;
	cUCCalc.ReadSchedCondsFromStr(cOrigUC, "({49, DUBLO} : 416), ({56, DUBLO} : 416), ({58, DUBLO} : 416), ({59, DUBLO} : 416), ");
	cUCCalc.ReadSchedCondsFromStr(cNewSchedConds, "({49, DUBLO} : 416), ({56, DUBLO} : 417), ({58, DUBLO} : 416), ({59, DUBLO} : 416), ");
	cUCCalc.PrintSchedConds(cNewSchedConds, cout); cout << endl;
	{
		TaskSetPriorityStruct cPA(cFlowSet);		
		cout << cUCCalc.IsSchedulable(cNewSchedConds, cDummy, cPA) << endl;
	}

	{
		TaskSetPriorityStruct cPA(cFlowSet);
		cout << IsSchedulableFR(cFlowSet, cNewSchedConds, cPA) << endl;
	}

}

//NoCDLUSingleSchedFRUnschedCoreComputer

NoCDLUSingleSchedFRUnschedCoreComputer::NoCDLUSingleSchedFRUnschedCoreComputer()
{

}

NoCDLUSingleSchedFRUnschedCoreComputer::NoCDLUSingleSchedFRUnschedCoreComputer(SystemType & rcSystem)
	:NoCFlowDLUUnschedCoreComputer(rcSystem),
	m_cFRTester(rcSystem)
{
	m_cFRTester.setCorePerIter(5);
	m_cFRTester.getUnschedCoreComputer().InitializeDeadlineLBLB();
}

bool NoCDLUSingleSchedFRUnschedCoreComputer::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	FlowsSet_2DNetwork & rcFlowsSet = *(FlowsSet_2DNetwork*)m_pcTaskSet;
	SchedConds cCombined = ConjunctSchedConds(rcSchedCondsFlex, rcSchedCondsFixed);
	TaskSetPriorityStruct cPA(rcFlowsSet);	
	cPA = rcPriorityAssignment;
	//Perform the FR test
	//cout << "FR test on task " << iTaskIndex << endl;
	m_cFRTester.setTarget(iTaskIndex, cPA, cCombined);	
	m_cFRTester.ClearFR();
	m_cFRTester.setCorePerIter(5);
	m_cFRTester.LoadUnschedCores(m_mapTask2Cores[iTaskIndex]);
	int iFRTestStatus = m_cFRTester.FocusRefinement(0);
	//if (iFRTestStatus == 0)
	{
		m_mapTask2Cores[iTaskIndex] = m_cFRTester.getUnschedCores();
	}
	return (iFRTestStatus == 1);
}

void NoCDLUSingleSchedFRUnschedCoreComputer::IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	NoCFlowDLUUnschedCoreComputer::IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	m_mapTask2Cores.clear();
}

bool NoCDLUSingleSchedFRUnschedCoreComputer::IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	TaskSetPriorityStruct & cPriorityAssignment = rcPriorityAssignment;
	int iPriority = iTaskNum - 1;
	int iPreAssignedSize = cPriorityAssignment.getSize();
	SchedConds cCombined = ConjunctSchedConds(rcSchedCondsFixed, rcSchedCondsFlex);
	for (; iPriority >= 0; iPriority--)
	{
		bool bAssigned = false;
		if (rcPriorityAssignment.getTaskByPriority(iPriority) != -1)
		{
			continue;
		}

		//Pessimistic and Optimistic Pretest
		deque<int> dequeCandidate;
		for (int i = 0; i < iTaskNum; i++)
		{
			int iCand = pcTaskArray[i].getTaskIndex();
			if ((cPriorityAssignment.getPriorityByTask(iCand) != -1))
				continue;
			cPriorityAssignment.setPriority(iCand, iPriority);
			if (SchedTestOptimistic(iCand, cCombined, rcPriorityAssignment) == false)
			{
				cPriorityAssignment.unset(iCand);
				continue;
			}

			if (SchedTestPessimistic(iCand, cCombined, rcPriorityAssignment) == false)
			{
				dequeCandidate.push_back(iCand);
				cPriorityAssignment.unset(iCand);
				continue;
			}			
			bAssigned = true;
			break;
		}
		if (bAssigned)	continue;
		//for (int i = 0; i < iTaskNum; i++)
		for (deque<int>::iterator iter = dequeCandidate.begin(); iter != dequeCandidate.end(); iter++)
		{
			//int iCand = pcTaskArray[i].getTaskIndex();
			int iCand = *iter;
			if ((cPriorityAssignment.getPriorityByTask(iCand) != -1))
				continue;
			cPriorityAssignment.setPriority(iCand, iPriority);
			bool bSchedStatus = false;
			int iPreSchedTest = PreSchedTest(iCand, rcPriorityAssignment, bSchedStatus, rcSchedCondsFlex, rcSchedCondsFixed);
			if (iPreSchedTest == 1)
				bSchedStatus = SchedTest(iCand, cPriorityAssignment, rcSchedCondsFlex, rcSchedCondsFixed);
			int iPostSchedTest = PostSchedTest(iCand, cPriorityAssignment, bSchedStatus, rcSchedCondsFlex, rcSchedCondsFixed);
			if (bSchedStatus)
			{
				bAssigned = true;
				break;
			}
			else
			{
				bAssigned = false;
				cPriorityAssignment.unset(iCand);
			}
		}

		if (!bAssigned)
		{
			return false;
		}
	}
	return true;
}

bool NoCDLUSingleSchedFRUnschedCoreComputer::HighestPriorityAssignment(int iTaskIndex, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	HighestPriorityAssignment_Prework(iTaskIndex, rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	TaskSetPriorityStruct & cPriorityAssignment = rcPriorityAssignment;
	int iPriority = iTaskNum - 1;
	int iPreAssignedSize = cPriorityAssignment.getSize();
	SchedConds cCombined = ConjunctSchedConds(rcSchedCondsFixed, rcSchedCondsFlex);
	for (; iPriority >= 0; iPriority--)
	{
		bool bAssigned = false;
		if (rcPriorityAssignment.getTaskByPriority(iPriority) != -1)
		{
			continue;
		}

		//Pessimistic and Optimistic Pretest
		deque<int> dequeCandidate;
#if 1
		for (int i = 0; i < iTaskNum; i++)
		{
			int iCand = pcTaskArray[i].getTaskIndex();
			if (iCand == iTaskIndex) continue;
			if ((cPriorityAssignment.getPriorityByTask(iCand) != -1))
				continue;
			cPriorityAssignment.setPriority(iCand, iPriority);
			if (SchedTestOptimistic(iCand, cCombined, rcPriorityAssignment) == false)
			{
				cPriorityAssignment.unset(iCand);
				continue;
			}

			if (SchedTestPessimistic(iCand, cCombined, rcPriorityAssignment) == false)
			{
				dequeCandidate.push_back(iCand);
				cPriorityAssignment.unset(iCand);
				continue;
			}
			bAssigned = true;
			break;
		}
#else
		for (int i = 0; i < iTaskNum; i++) dequeCandidate.push_back(i);
#endif
		if (bAssigned)	continue;

		for (deque<int>::iterator iter = dequeCandidate.begin(); iter != dequeCandidate.end(); iter++)
		{
			int iCand = *iter;
			if (iCand == iTaskIndex)	continue;
			if ((cPriorityAssignment.getPriorityByTask(iCand) != -1))
				continue;
			cPriorityAssignment.setPriority(iCand, iPriority);
			bool bSchedStatus = false;
			int iPreSchedTest = PreSchedTest(iCand, rcPriorityAssignment, bSchedStatus, rcSchedCondsFlex, rcSchedCondsFixed);
			if (iPreSchedTest == 1) bSchedStatus = SchedTest(iCand, cPriorityAssignment, rcSchedCondsFlex, rcSchedCondsFixed);
			int iPostSchedTest = PostSchedTest(iCand, cPriorityAssignment, bSchedStatus, rcSchedCondsFlex, rcSchedCondsFixed);
			if (bSchedStatus)
			{
				bAssigned = true;
				break;
			}
			else
			{
				bAssigned = false;
				cPriorityAssignment.unset(iCand);
			}
		}

		if (!bAssigned)
		{
			if (cPriorityAssignment.getPriorityByTask(iTaskIndex) == -1)
			{
				int iCand = iTaskIndex;
				cPriorityAssignment.setPriority(iCand, iPriority);
				bool bSchedStatus = false;
				int iPreSchedTest = PreSchedTest(iCand, rcPriorityAssignment, bSchedStatus, rcSchedCondsFlex, rcSchedCondsFixed);
				if (iPreSchedTest == 1) bSchedStatus = SchedTest(iCand, cPriorityAssignment, rcSchedCondsFlex, rcSchedCondsFixed);
				int iPostSchedTest = PostSchedTest(iCand, cPriorityAssignment, bSchedStatus, rcSchedCondsFlex, rcSchedCondsFixed);
				if (!bSchedStatus)
					return false;
			}
			else
				return false;
		}
	}
	return true;
}

void TestSingleSchedFRAnalysis()
{
	typedef NoCFlowDLU_SingleTaskSched_FocusRefinement::SchedConds SchedConds;
	typedef NoCFlowDLUFocusRefinement::UnschedCores UnschedCores;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	UnschedCores cUCs;
	TaskSetPriorityStruct cPA(cFlowSet);
	NoCFlowDLUUnschedCoreComputer cUCCalc(cFlowSet);
	NoCDLUSingleSchedFRUnschedCoreComputer cFRUCCalc(cFlowSet);
	SchedConds cDummy;
	map<int, TaskSetPriorityStruct> mapMustHPSets;
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "------------------------------------------------" << endl;
		cout << "Task " << i << endl;
		cout << "Original: " << endl;		
		cPA.Clear();
		cUCCalc.HighestPriorityAssignment(i, cDummy, cDummy, cPA);
		int iThisTaskPriority = cPA.getPriorityByTask(i);		
		cout << "Must HP: ";
		for (int j = iThisTaskPriority - 1; j >= 0; j--)
		{
			cout << cPA.getTaskByPriority(j) << ", ";
		}
		cout << endl;
		cout << "cUCCalc Verify: " << cUCCalc.SchedTest_Public(i, cPA, cDummy, cDummy) << endl;		
		if (iThisTaskPriority > 0)
		{
			mapMustHPSets[i] = cPA;
		}
		system("pause");		
	}
	cFRUCCalc.getFRTestObj().setMustHPSets(mapMustHPSets);
	cout << "FRUCCalc" << endl;
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "------------------------------------------------" << endl;
		cout << "Task " << i << endl;
		cout << "FR: " << endl;
		cPA.Clear();
		cFRUCCalc.HighestPriorityAssignment(i, cDummy, cDummy, cPA);
		int iThisTaskPriority = cPA.getPriorityByTask(i);
		cPA.WriteTextFile("FRUCCalcHPA.txt");
		cout << "Must HP: ";
		for (int j = iThisTaskPriority - 1; j >= 0; j--)
		{
			cout << cPA.getTaskByPriority(j) << ", ";
		}
		cout << endl;
		cout << "cUCCalc Verify: " << cUCCalc.SchedTest_Public(i, cPA, cDummy, cDummy) << endl;
		
	}
}

void NoCFlowSubSetPriorityAssignment(MySet<int> setSubSet, TaskSetPriorityStruct & rcPriorityAssignment)
{

}

void TestHPPBasedAnalysis()
{
	typedef NoCFlowDLU_SingleTaskSched_FocusRefinement::SchedConds SchedConds;
	typedef NoCFlowDLUFocusRefinement::UnschedCores UnschedCores;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	FlowsSet_2DNetwork cFlowSet;
	//cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
//	cFlowSet.ReadImageFile("NocHardCases\\NoCFlowSetForFR_1.tskst"); 
	cFlowSet.ReadImageFile("NocHardCases\\FlowSet43.tskst");
	cFlowSet.DisplayTaskInfo();
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	UnschedCores cUCs;
	TaskSetPriorityStruct cPA(cFlowSet);
	NoCFlowDLUUnschedCoreComputer cUCCalc(cFlowSet);
	SchedConds cDummy;
	//Print all MustHP sets
	for (int i = 0; i < iTaskNum; i++)
	{
		MySet<int> cMustHPSet;
		cUCCalc.HighestPriorityAssignment(i, cDummy, cDummy, cPA);
		cPA.getHPSet(i, cMustHPSet);
		cout << "------------------------------------------------" << endl;
		cout << "Task " << i << " must HP set: " << endl;
		PrintSetContent(cMustHPSet.getSetContent()); 
		cPA.Clear();
	}
	system("pause");
	int iTargetTask = 28;
	cin >> iTargetTask;
	TaskSetPriorityStruct cHPPA(cFlowSet);
	cUCCalc.HighestPriorityAssignment(iTargetTask, cDummy, cDummy, cHPPA);
	MySet<int> cHPESet;
	cout << "HPP Derived" << endl;
	cHPPA.getHPESet(iTargetTask, cHPESet);
	cout << "HPPSet: " << endl;
	PrintSetContent(cHPESet.getSetContent());
	cout << endl;
	FlowsSet_2DNetwork cSubFlowSet;
	map<int, int> mapIndexMapping, mapSub2Full;
	cFlowSet.GenSubFlowSet(cHPESet, cSubFlowSet, mapIndexMapping, mapSub2Full);
	for (map<int, int>::iterator iter = mapIndexMapping.begin(); iter != mapIndexMapping.end(); iter++)
	{
		cout << iter->first << " " << iter->second << endl;
	}
	//Test Whether The HPE sub set is schedulable;
	cout << "FR: " << endl;
	NoCFlowDLUFocusRefinement cNocFR(cSubFlowSet);
	cNocFR.setCorePerIter(5);
	cNocFR.setLogFile("NoCSubsetFRLog.txt");
	cNocFR.FocusRefinement(1);
	TaskSetPriorityStruct cResultPA = cNocFR.getSolPA();
	NoCFlowAMCRTCalculator_DerivedJitter cDerivedSubRTCalc(cSubFlowSet, cResultPA, cSubFlowSet.getDirectJitterTable());
	cout << "Result Response Time: " << endl;
	cDerivedSubRTCalc.ComputeAllResponseTime();
	cout << cDerivedSubRTCalc.getRTLO(19) << endl;
	SchedConds cSolutionSchedConds = cNocFR.getOptimalSchedCondConfig();
	NoCFlowDLUUnschedCoreComputer cSubsetUCCalc(cSubFlowSet);	
	TaskSetPriorityStruct cHPPASchedulable(cFlowSet);
	{
		GET_TASKSET_NUM_PTR(&cSubFlowSet, iTaskNum, pcTaskArray);
		vector<double> vectorJitterLO, vectorJitterHI;
		for (int i = 0; i < iTaskNum; i++)
		{
			vectorJitterLO.push_back(0);
			vectorJitterHI.push_back(0);
		}		
		NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(cSubFlowSet, cSubFlowSet.getDirectJitterTable(), vectorJitterLO, vectorJitterHI, cResultPA);		
		for (int i = 0; i < iTaskNum; i++)
		{
			cout << "Task " << i << " RTLO: " << cRTCalc.RTLO(i);
			if (pcTaskArray[i].getCriticality())
			{
				cout << "/ RTHI: " << cRTCalc.WCRTHI(i);
			}
			cout << "/ Deadline: " << pcTaskArray[i].getDeadline() << endl;						
		}
	}

	{
		cout << "Original Task Set" << endl;
		vector<double> vectorJitterLO, vectorJitterHI;
		for (int i = 0; i < iTaskNum; i++)
		{
			vectorJitterLO.push_back(0);
			vectorJitterHI.push_back(0);
		}		
		NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(cFlowSet, cFlowSet.getDirectJitterTable(), vectorJitterLO, vectorJitterHI, cHPPA);
		for (int i = 0; i < iTaskNum; i++)
		{
			cout << "Task " << i << " RTLO: " << cRTCalc.RTLO(i);
			if (pcTaskArray[i].getCriticality())
			{
				cout << "/ RTHI: " << cRTCalc.WCRTHI(i);
			}
			cout << "/ Deadline: " << pcTaskArray[i].getDeadline() << endl;
		}
	}
	for (int i = 0; i < iTaskNum; i++)
	{
		if (mapIndexMapping.count(i) == 0)	 continue;
		cHPPASchedulable.setPriority(i, cResultPA.getPriorityByTask(mapIndexMapping[i]));
	}
	NoCFlowDLUFocusRefinement cOriginalFlowSetFR(cFlowSet);
	//cOriginalFlowSetFR.getUnschedCoreComputer().getFixedPriorityAssignment().setPriority(iTargetTask, cHPPA.getPriorityByTask(iTargetTask));	
	cHPPASchedulable.unset(iTargetTask);

	//set enforced order
#if 0
	GET_TASKSET_NUM_PTR(&cSubFlowSet, iTaskNum_Sub, pcTaskArray_Sub);
	for (int i = 0; i < iTaskNum_Sub; i++)
	{
		assert(cResultPA.getPriorityByTask(i) != -1);
		for (int j = 0; j < iTaskNum_Sub; j++)
		{
			if (i == j)continue;
			if (cResultPA.getPriorityByTask(i) < cResultPA.getPriorityByTask(j))
			{
				cOriginalFlowSetFR.getUnschedCoreComputer().setEnforcedOrder(mapSub2Full[i], mapSub2Full[j], 0);
			}
			else
			{
				cOriginalFlowSetFR.getUnschedCoreComputer().setEnforcedOrder(mapSub2Full[i], mapSub2Full[j], 1);
			}
		}
	}
#endif

	cOriginalFlowSetFR.getUnschedCoreComputer().getFixedPriorityAssignment() = cHPPASchedulable;
	cOriginalFlowSetFR.setCorePerIter(5);
	cOriginalFlowSetFR.setLogFile("PartialFixedPAFRLog.txt");
	int iStatus = cOriginalFlowSetFR.FocusRefinement(1);
	if (iStatus != 1)	return;

	TaskSetPriorityStruct cOriginalFRResultPA = cOriginalFlowSetFR.getSolPA();	
	cOriginalFRResultPA.PrintByPriority();
	cOriginalFRResultPA.WriteTextFile("HardNoCCaseFeaPA.txt");
	NoCFlowAMCRTCalculator_DerivedJitter cOriginalFRResultPARTCalc(cFlowSet, cOriginalFRResultPA, cFlowSet.getDirectJitterTable());
	cOriginalFRResultPARTCalc.ComputeAllResponseTime();
	cOriginalFRResultPARTCalc.PrintSchedulabilityInfo();
}

void SchedulabilityRunTimeBatchTest()
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	char axBuffer[512] = { 0 };	
	sprintf_s(axBuffer, "NoCFRSchedTest_%dX%d", iXDimension, iYDimension);
	string stringMainFolderName = axBuffer;
	deque<int> dequeNumFlows;
	MakePath(axBuffer);
	for (int i = 5; i < 70; i += 5)
	{
		dequeNumFlows.push_back(i);
	}
	StatisticSet cTotalStatistcSet;	
	char axChar = 'A';
	for (deque<int>::iterator iter = dequeNumFlows.begin(); iter != dequeNumFlows.end(); iter++)
	{	
		sprintf_s(axBuffer, "%s\\N%d", stringMainFolderName.data(), *iter);
		string stringSubFolderName = axBuffer;
		MakePath(axBuffer);
		int iNumFlows = *iter;
		
		for (int i = 0; i < 1000; i++)
		{
			FlowsSet_2DNetwork cFlowSet;			
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;			
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			cout << "\r" << stringFlowSetName.data() << "        ";
			if (IsFileExist((char *)stringThisStatisticName.data()) == true)
			{
				continue;
			}

			//do the test
			cFlowSet.GenRandomFlow_2014Burns(iNumFlows, 1000, 0.5, 0.15, 2, iXDimension, iYDimension);
			cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
			cFlowSet.WriteTextFile(axBuffer);

			//DM
			TaskSetPriorityStruct cDMPA = cFlowSet.GenDMPriroityAssignment();
			NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_DM(cFlowSet, cDMPA, cFlowSet.getDirectJitterTable());
			cDerivedJitterRTCalc_DM.ComputeAllResponseTime();
			bool bDMSchedualbility = cDerivedJitterRTCalc_DM.IsSchedulable();
			cThisStatsitic.setItem("DM Schedulability", bDMSchedualbility);

			//FR
			NoCFlowDLUFocusRefinement cFR(cFlowSet);
			cFR.setCorePerIter(5);			
			sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
			cFR.setLogFile(axBuffer);			
			int iStatus = cFR.FocusRefinement(0, dTimeout);
			sprintf_s(axBuffer, "%s_FRResult", stringFlowSetName.data());
			StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);						
			if (iStatus == 1)
			{
				TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
				assert(cDerivedJitterRTCalc_FR.IsSchedulable());
			}
						
			//Full MILP
			NoCFlowRTMILPModel cFullMILP(cFlowSet);
			int iFullMILPStatus = cFullMILP.Solve(0, dTimeout);
			cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
			cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
			cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
			if (iFullMILPStatus == 1)
			{
				TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
				assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
			}
			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
		}

		//Read back the result
		int iDMAcceptNum = 0;
		int iDMDenyNum = 0;
		int iFRAcceptNum = 0;
		int iFRDenyNum = 0;
		int iMILPAcceptNum = 0;
		int iMILPDenyNum = 0;
		double dFRTime = 0;
		double dMILPTime = 0;
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;			
			cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());

			if (cThisStatsitic.getItem("DM Schedulability").getValue() == 1)
			{
				iDMAcceptNum++;
			}
			else if (cThisStatsitic.getItem("DM Schedulability").getValue() == 0)
			{
				iDMDenyNum++;
			}

			//FR
			StatisticSet cFRResult;
			sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
			cFRResult.ReadStatisticImage(axBuffer);
			if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
			{
				iFRAcceptNum++;
			}
			else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
			{
				iFRDenyNum++;
			}
			dFRTime += cFRResult.getItem("Total Time").getValue();

			//MILP
			if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1)
			{
				iMILPAcceptNum++;
			}
			else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
			{
				iMILPDenyNum++;
			}
			dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();
		}
		dFRTime /= 1000;
		dMILPTime /= 1000;
		sprintf_s(axBuffer, "%c-N%d DM Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iDMAcceptNum);
		sprintf_s(axBuffer, "%c-N%d DM Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iDMDenyNum);
		sprintf_s(axBuffer, "%c-N%d FR Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
		sprintf_s(axBuffer, "%c-N%d FR Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
		sprintf_s(axBuffer, "%c-N%d FR Time", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, dFRTime);
		sprintf_s(axBuffer, "%c-N%d MILP Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
		sprintf_s(axBuffer, "%c-N%d MILP Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
		sprintf_s(axBuffer, "%c-N%d MILP Time", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, dMILPTime);
		axChar++;
		sprintf_s(axBuffer, "%s//%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
		cTotalStatistcSet.WriteStatisticText(axBuffer);
	}
}

void SchedulabilityRunTimeBatchTest_USweep()
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	int iNumFlows = 50;
	char axBuffer[512] = { 0 };
	sprintf_s(axBuffer, "NoCFRSchedTest_%dX%d_N%d_USweep", iXDimension, iYDimension, iNumFlows);
	string stringMainFolderName = axBuffer;
	deque<double> dequeUtils;
	MakePath(axBuffer);
	for (double dUtil = 1.0; dUtil <= 1.5; dUtil+= 0.05)
	{
		dequeUtils.push_back(dUtil);
	}
	StatisticSet cTotalStatistcSet;
	char axChar = 'A';
	for (deque<double>::iterator iter = dequeUtils.begin(); iter != dequeUtils.end(); iter++)
	{
		sprintf_s(axBuffer, "%s\\U%.2f", stringMainFolderName.data(), *iter);
		string stringSubFolderName = axBuffer;
		MakePath(axBuffer);
		double dUtil = *iter;

		for (int i = 0; i < 1000; i++)
		{
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			//cout << "\r" << stringFlowSetName.data() << "        ";
			cout << stringFlowSetName.data() << endl;
			if (IsFileExist((char *)stringThisStatisticName.data()) == true)
			{
				continue;
			}

			//do the test
			cFlowSet.GenRandomTaskSetInteger(iNumFlows, dUtil, 0.5, 2, PERIODOPT_LOG);
			cFlowSet.GenerateRandomFlowRoute(iXDimension, iYDimension);
			cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
			cFlowSet.WriteTextFile(axBuffer);

			//DM
			TaskSetPriorityStruct cDMPA = cFlowSet.GenDMPriroityAssignment();
			NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_DM(cFlowSet, cDMPA, cFlowSet.getDirectJitterTable());
			cDerivedJitterRTCalc_DM.ComputeAllResponseTime();
			bool bDMSchedualbility = cDerivedJitterRTCalc_DM.IsSchedulable();
			cThisStatsitic.setItem("DM Schedulability", bDMSchedualbility);
			cout << "DM Schedulability: " << bDMSchedualbility << endl;

			//FR
			NoCFlowDLUFocusRefinement cFR(cFlowSet);
			cFR.setCorePerIter(5);
			//cFR.setObjType(cFR.MaxMinLaxity);
			sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
			cFR.setLogFile(axBuffer);
			int iStatus = cFR.FocusRefinement(1, dTimeout);
			sprintf_s(axBuffer, "%s_FRResult", stringFlowSetName.data());
			StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);
			if (iStatus == 1)
			{
				TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
				assert(cDerivedJitterRTCalc_FR.IsSchedulable());
				NoCFlowRTMILPModel cVerifyFullMILP(cFlowSet);
				cVerifyFullMILP.setPredefinePA(rcFRResultPA);
				cout << "Verify Using MILP" << endl;
				cout << "Status: " << cVerifyFullMILP.Solve(1) << endl;
			}

			//Full MILP
			cout << "Full MILP" << endl;
			NoCFlowRTMILPModel cFullMILP(cFlowSet);
			//cFullMILP.setObjType(cFullMILP.MaxMinLaxity);
			int iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
			cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
			cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
			cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
			if (iFullMILPStatus == 1)
			{
				TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
				assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
			}
			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
			cout << "----------------------------------------------------------------------" << endl;
		}

		//Read back the result
		int iDMAcceptNum = 0;
		int iDMDenyNum = 0;

		int iFRAcceptNum = 0;
		int iFRDenyNum = 0;
		int iFRTimeout = 0;

		int iMILPAcceptNum = 0;
		int iMILPDenyNum = 0;
		int iMILPTimeout = 0;

		double dFRTime = 0;
		double dMILPTime = 0;
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());

			if (cThisStatsitic.getItem("DM Schedulability").getValue() == 1)
			{
				iDMAcceptNum++;
			}
			else if (cThisStatsitic.getItem("DM Schedulability").getValue() == 0)
			{
				iDMDenyNum++;
			}

			//FR
			StatisticSet cFRResult;
			sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
			cFRResult.ReadStatisticImage(axBuffer);
			if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
			{
				iFRAcceptNum++;
			}
			else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
			{
				iFRDenyNum++;
			}
			else
			{
				iFRTimeout++;
			}
			dFRTime += cFRResult.getItem("Total Time").getValue();

			//MILP
			if ((cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1) ||
				(cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == -3))
			{
				iMILPAcceptNum++;
			}
			else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
			{
				iMILPDenyNum++;
			}
			else
			{
				iMILPTimeout++;
			}
			dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();
		}
		dFRTime /= 1000;
		dMILPTime /= 1000;
		sprintf_s(axBuffer, "%c-U%.2f DM Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iDMAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f DM Deny", axChar, dUtil);

		cTotalStatistcSet.setItem(axBuffer, iDMDenyNum);
		sprintf_s(axBuffer, "%c-U%.2f FR Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f FR Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
		sprintf_s(axBuffer, "%c-U%.2f FR Time", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, dFRTime);
		sprintf_s(axBuffer, "%c-U%.2f FR Timeout", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRTimeout);

		sprintf_s(axBuffer, "%c-U%.2f MILP Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f MILP Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
		sprintf_s(axBuffer, "%c-U%.2f MILP Time", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, dMILPTime);
		sprintf_s(axBuffer, "%c-U%.2f MILP Timeout", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPTimeout);
		axChar++;
		sprintf_s(axBuffer, "%s//%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
		cTotalStatistcSet.WriteStatisticText(axBuffer);
	}
}

void SchedulabilityRunTimeBatchTest_NSweep()
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	char axBuffer[512] = { 0 };
	sprintf_s(axBuffer, "NoCFRSchedTest_%dX%d", iXDimension, iYDimension);
	string stringMainFolderName = axBuffer;
	deque<int> dequeNumFlows;
	MakePath(axBuffer);
	for (int i = 5; i < 70; i += 5)
	{
		dequeNumFlows.push_back(i);
	}
	StatisticSet cTotalStatistcSet;
	char axChar = 'A';
	for (deque<int>::iterator iter = dequeNumFlows.begin(); iter != dequeNumFlows.end(); iter++)
	{
		sprintf_s(axBuffer, "%s\\N%d", stringMainFolderName.data(), *iter);
		string stringSubFolderName = axBuffer;
		MakePath(axBuffer);
		int iNumFlows = *iter;

		for (int i = 0; i < 1000; i++)
		{
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			cout << "\r" << stringFlowSetName.data() << "        ";
			if (IsFileExist((char *)stringThisStatisticName.data()) == true)
			{
				continue;
			}

			//do the test
			double dUtil = 0;
			double dUtilLB = 1.0;
			double dUtilUB = 1.5;
			dUtil = ((double)rand() / (double)RAND_MAX) * (dUtilUB - dUtilLB) + dUtilLB;
			cFlowSet.GenRandomTaskset(iNumFlows, dUtil, 0.5, 2, PERIODOPT_LOG);
			cFlowSet.GenerateRandomFlowRoute(iXDimension, iYDimension);
			cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
			cFlowSet.WriteTextFile(axBuffer);

			//DM
			TaskSetPriorityStruct cDMPA = cFlowSet.GenDMPriroityAssignment();
			NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_DM(cFlowSet, cDMPA, cFlowSet.getDirectJitterTable());
			cDerivedJitterRTCalc_DM.ComputeAllResponseTime();
			bool bDMSchedualbility = cDerivedJitterRTCalc_DM.IsSchedulable();
			cThisStatsitic.setItem("DM Schedulability", bDMSchedualbility);

			//FR
			NoCFlowDLUFocusRefinement cFR(cFlowSet);
			cFR.setCorePerIter(5);
			sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
			cFR.setLogFile(axBuffer);
			int iStatus = cFR.FocusRefinement(0, dTimeout);
			sprintf_s(axBuffer, "%s_FRResult", stringFlowSetName.data());
			StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);
			if (iStatus == 1)
			{
				TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
				assert(cDerivedJitterRTCalc_FR.IsSchedulable());
			}

			//Full MILP
			NoCFlowRTMILPModel cFullMILP(cFlowSet);
			int iFullMILPStatus = cFullMILP.Solve(0, dTimeout);
			cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
			cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
			cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
			if (iFullMILPStatus == 1)
			{
				TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
				assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
			}
			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
		}

		//Read back the result
		int iDMAcceptNum = 0;
		int iDMDenyNum = 0;
		int iFRAcceptNum = 0;
		int iFRDenyNum = 0;
		int iMILPAcceptNum = 0;
		int iMILPDenyNum = 0;
		double dFRTime = 0;
		double dMILPTime = 0;
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());

			if (cThisStatsitic.getItem("DM Schedulability").getValue() == 1)
			{
				iDMAcceptNum++;
			}
			else if (cThisStatsitic.getItem("DM Schedulability").getValue() == 0)
			{
				iDMDenyNum++;
			}

			//FR
			StatisticSet cFRResult;
			sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
			cFRResult.ReadStatisticImage(axBuffer);
			if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
			{
				iFRAcceptNum++;
			}
			else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
			{
				iFRDenyNum++;
			}
			dFRTime += cFRResult.getItem("Total Time").getValue();

			//MILP
			if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1)
			{
				iMILPAcceptNum++;
			}
			else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
			{
				iMILPDenyNum++;
			}
			dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();
		}
		dFRTime /= 1000;
		dMILPTime /= 1000;
		sprintf_s(axBuffer, "%c-N%d DM Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iDMAcceptNum);
		sprintf_s(axBuffer, "%c-N%d DM Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iDMDenyNum);
		sprintf_s(axBuffer, "%c-N%d FR Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
		sprintf_s(axBuffer, "%c-N%d FR Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
		sprintf_s(axBuffer, "%c-N%d FR Time", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, dFRTime);
		sprintf_s(axBuffer, "%c-N%d MILP Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
		sprintf_s(axBuffer, "%c-N%d MILP Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
		sprintf_s(axBuffer, "%c-N%d MILP Time", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, dMILPTime);
		axChar++;
		sprintf_s(axBuffer, "%s//%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
		cTotalStatistcSet.WriteStatisticText(axBuffer);
	}
}

void SchedulabilityRunTimeBatchTes_Stress_USweep()
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	int iNumFlows = 40;
	char axBuffer[512] = { 0 };
	double dHIRatio = 0.50;
	cout << "Dir Change Status: " << _chdir("\\\\RICHIEZHAO-PC\\NoCExp") << endl;
	sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_N%d_HR%.2f_USweep", iXDimension, iYDimension, iNumFlows, dHIRatio);	
	string stringMainFolderName = axBuffer;
	deque<double> dequeUtils;
	MakePath(axBuffer);
	//for (double dUtil = 0.40; dUtil <= 2.0; dUtil += 0.05)
	for (double dUtil = 0.55; dUtil <= 1.50; dUtil += 0.05)
	{
		if (DOUBLE_EQUAL(dUtil, 0.60, 1e-5)) continue;		
		dequeUtils.push_back(dUtil);
	}
	StatisticSet cTotalStatistcSet;
	char axChar = 'A';
	for (deque<double>::iterator iter = dequeUtils.begin(); iter != dequeUtils.end(); iter++)
	{
		sprintf_s(axBuffer, "%s\\U%.2f", stringMainFolderName.data(), *iter);
		string stringSubFolderName = axBuffer;
		MakePath(axBuffer);
		double dUtil = *iter;

		for (int i = 0; i < 1000; i++)
		{
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			
			//cout << "\r" << stringFlowSetName.data() << "        ";
			cout << stringFlowSetName.data() << endl;
			if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
			{
				cout << "Result File Found" << endl;
				cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
				cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
				//continue;
			}
			else
			{
				//do the test
				cout << stringThisStatisticName.data() << endl << stringFlowSetName.data() << endl;				
				if (dUtil >= 0.66)
				{
					cout << "Block" << endl;
					while (1);
				}
				cFlowSet.GenRandomTaskSetInteger(iNumFlows, dUtil, dHIRatio, 2, PERIODOPT_LOG);
				cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
				cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
				sprintf_s(axBuffer, "%s\\FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
				cFlowSet.WriteTextFile(axBuffer);
			}


			//DM
			if (cThisStatsitic.IsItemExist("DM Schedulability") == false)
			{
				TaskSetPriorityStruct cDMPA = cFlowSet.GenDMPriroityAssignment();
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_DM(cFlowSet, cDMPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_DM.ComputeAllResponseTime();
				bool bDMSchedualbility = cDerivedJitterRTCalc_DM.IsSchedulable();
				cThisStatsitic.setItem("DM Schedulability", bDMSchedualbility);
				cout << "DM Schedulability: " << bDMSchedualbility << endl;
			}
			

			//Pesimistic Schedulability
			if (cThisStatsitic.IsItemExist("Pessimistic Test Schedulability") == false)
			{
				bool bPessSchedulability = false;
				TaskSetPriorityStruct cPessPA(cFlowSet);
				NoCFlowDLUUnschedCoreComputer cPessTest(cFlowSet);
				bPessSchedulability = cPessTest.IsSchedulablePessimistic(cPessPA);
				cout << "Pessimistic Test Schedulability: " << bPessSchedulability << endl;
				cThisStatsitic.setItem("Pessimistic Test Schedulability", bPessSchedulability);
			}

			//FR
			sprintf_s(axBuffer, "%s_FRLog.txt.rslt", stringFlowSetName.data());
			string stringFRResultFile(axBuffer);
			sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
			string stringFRResultLegacyName = axBuffer;
			if (IsFileExist((char *)stringFRResultFile.data()) == false && IsFileExist((char *)stringFRResultLegacyName.data()) == false)
			{
				cout << "FR: " << endl;
				NoCFlowDLUFocusRefinement cFR(cFlowSet);
				cFR.setCorePerIter(5);
				//cFR.setObjType(cFR.MaxMinLaxity);
				sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
				cFR.setLogFile(axBuffer);
				int iStatus = cFR.FocusRefinement(1, dTimeout);

				StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);
				if (iStatus == 1)
				{
					TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FR.IsSchedulable());
					NoCFlowRTMILPModel cVerifyFullMILP(cFlowSet);
					cVerifyFullMILP.setPredefinePA(rcFRResultPA);
					cout << "Verify Using MILP" << endl;
					cout << "Status: " << cVerifyFullMILP.Solve(1) << endl;
				}
			}
			

			//Full MILP
			if (cThisStatsitic.IsItemExist("Full-MILP Schedulability") == false)
			{
				cout << "Full MILP" << endl;
				NoCFlowRTMILPModel cFullMILP(cFlowSet);
				//cFullMILP.setObjType(cFullMILP.MaxMinLaxity);
				int iFullMILPStatus = 0;
				if (1)
					iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
				else
					iFullMILPStatus = 0;
				cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
				cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
				cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
				if (iFullMILPStatus == 1)
				{
					TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
				}
			}

			//Heuristic
			if (cThisStatsitic.IsItemExist("Heuristic08Burns") == false)
			{
				cout << "Heuristic 08 Burns" << endl;
				NoCPriorityAssignment_2008Burns cHeuristic(cFlowSet);
				time_t iClocks = clock();
				cHeuristic.setTimeout(600);				
				TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
				int iStatus = cHeuristic.Run(cPriorityAssignment);
				iClocks = clock() - iClocks;
				cThisStatsitic.setItem("Heuristic08Burns", iStatus);			
				cThisStatsitic.setItem("Heuristic08Burns Time", (double)iClocks);
				cout << endl;
			}

			
			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
			cout << "----------------------------------------------------------------------" << endl;
		}

		//Read back the result
		int iDMAcceptNum = 0;
		int iDMDenyNum = 0;

		int iPessAcceptNum = 0;
		int iPessDenyNum = 0;

		int iFRAcceptNum = 0;
		int iFRDenyNum = 0;
		int iFRTimeout = 0;

		int iMILPAcceptNum = 0;
		int iMILPDenyNum = 0;
		int iMILPTimeout = 0;

		int iHeuAcceptNum = 0;
		int iHeuDenyNum = 0;
		int iHeuTimeout = 0;

		double dFRTime = 0;
		double dMILPTime = 0;
		double dHeuTime = 0;
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			cout << "\rReading Case " << i << "    ";
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());

			if (cThisStatsitic.getItem("DM Schedulability").getValue() == 1)
			{
				iDMAcceptNum++;
			}
			else if (cThisStatsitic.getItem("DM Schedulability").getValue() == 0)
			{
				iDMDenyNum++;
			}

			//Pess Test
			if (cThisStatsitic.getItem("Pessimistic Test Schedulability").getValue() == 1)
			{
				iPessAcceptNum++;
			}
			else if (cThisStatsitic.getItem("Pessimistic Test Schedulability").getValue() == 0)
			{
				iPessDenyNum++;
			}

			//FR
			StatisticSet cFRResult;
			sprintf_s(axBuffer, "%s_FRLog.txt.rslt", stringFlowSetName.data());
			if (IsFileExist(axBuffer) == false)
			{
				sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
				cFRResult.ReadStatisticImage(axBuffer);
			}			
			else
			{
				cFRResult.ReadStatisticImage(axBuffer);
			}			
			
			if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
			{
				iFRAcceptNum++;
			}
			else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
			{
				iFRDenyNum++;
			}
			else
			{
				iFRTimeout++;
			}
			dFRTime += cFRResult.getItem("Total Time").getValue();

			//MILP
			if ((cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1) ||
				(cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == -3))
			{
				iMILPAcceptNum++;
			}
			else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
			{
				iMILPDenyNum++;
			}
			else
			{
				iMILPTimeout++;
			}
			dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();

			//Heuristic
			if (cThisStatsitic.IsItemExist("Heuristic08Burns"))
			{
				int iStatus = round(cThisStatsitic.getItem("Heuristic08Burns").getValue());
				if (iStatus == 1)
				{
					iHeuAcceptNum++;
				}
				else if (iStatus == 0)
				{
					iHeuDenyNum++;
				}
				else if (iStatus == -1)
				{
					iHeuTimeout++;
				}
				else
				{
					cout << "Unknown Result from heuristic" << endl;
					while (1);
				}
				dHeuTime += cThisStatsitic.getItem("Heuristic08Burns Time").getValue();
			}
			
		}
		dFRTime /= 1000;
		dMILPTime /= 1000;
		dHeuTime /= 1000;
		sprintf_s(axBuffer, "%c-U%.2f DM Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iDMAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f DM Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iDMDenyNum);

		sprintf_s(axBuffer, "%c-U%.2f Pessimistic Test Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iPessAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f Pessimistic Test Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iPessDenyNum);

		sprintf_s(axBuffer, "%c-U%.2f FR Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f FR Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
		sprintf_s(axBuffer, "%c-U%.2f FR Time", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, dFRTime);
		sprintf_s(axBuffer, "%c-U%.2f FR Timeout", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRTimeout);

		sprintf_s(axBuffer, "%c-U%.2f MILP Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f MILP Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
		sprintf_s(axBuffer, "%c-U%.2f MILP Time", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, dMILPTime);
		sprintf_s(axBuffer, "%c-U%.2f MILP Timeout", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPTimeout);

		sprintf_s(axBuffer, "%c-U%.2f Heu Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iHeuAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f Heu Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iHeuDenyNum);
		sprintf_s(axBuffer, "%c-U%.2f Heu Time", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, dHeuTime);
		sprintf_s(axBuffer, "%c-U%.2f Heu Timeout", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iHeuTimeout);

		axChar++;
		sprintf_s(axBuffer, "%s//%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
		cTotalStatistcSet.WriteStatisticText(axBuffer);
	}
}

void SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest_thread_routine(int iThreadIndex, int * piTaskIndex, double * pdUtilIndex, mutex * pcLock)
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	int iNumFlows = 40;
	char axBuffer[512] = { 0 };
	double dHIRatio = 0.50;
	sprintf_s(axBuffer, "SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest_thread_routine_%d.txt", iThreadIndex);
	ofstream ofstreamOutput(axBuffer);
	cout << "Dir Change Status: " << _chdir("\\\\RICHIEZHAO-PC\\NoCExp") << endl;
	sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_N%d_HR%.2f_USweep", iXDimension, iYDimension, iNumFlows, dHIRatio);
	string stringMainFolderName = axBuffer;			
	while (true)
	{
		//WaitForSingleObject(*pcLock, INFINITE);
		pcLock->lock();
		double dUtil = *pdUtilIndex;
		int i = (*piTaskIndex);
		if (DOUBLE_EQUAL(dUtil, 0.60, 1e-5))
		{
			//ReleaseMutex(*pcLock);
			pcLock->unlock();
			break;
		}
		assert(i < 1000 && i >= 0);		
		(*piTaskIndex)++;
		if ((*piTaskIndex) == 1000)
		{
			*pdUtilIndex += 0.05;
			*piTaskIndex = 0;
		}		
		//ReleaseMutex(*pcLock);
		pcLock->unlock();

		sprintf_s(axBuffer, "%s\\U%.2f", stringMainFolderName.data(), dUtil);
		string stringSubFolderName = axBuffer;
		FlowsSet_2DNetwork cFlowSet;
		sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
		string stringFlowSetName = axBuffer;
		StatisticSet cThisStatsitic;
		sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
		string stringThisStatisticName = axBuffer;

		//cout << "\r" << stringFlowSetName.data() << "        ";
		//cout << stringFlowSetName.data() << endl;
		ofstreamOutput << stringFlowSetName.data() << endl;
		ofstreamOutput.flush();
		StatisticSet cTotalStatistcSet;
		if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
		{			
			cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
			cFlowSet.ReadImageFile((char*)stringFlowSetName.data());			
		}
		else
		{
			cout << "Cannot find target task file or result file" << stringFlowSetName.data() << endl;
			while (1);
		}

		//Heuristic
		if (cThisStatsitic.IsItemExist("Heuristic08Burns") == false)
		{
			cout << "Heuristic 08 Burns" << endl;
			NoCPriorityAssignment_2008Burns cHeuristic(cFlowSet);
			time_t iClocks = clock();
			cHeuristic.setTimeout(600);
			TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
			int iStatus = cHeuristic.Run(cPriorityAssignment);
			iClocks = clock() - iClocks;
			cThisStatsitic.setItem("Heuristic08Burns", iStatus);
			cThisStatsitic.setItem("Heuristic08Burns Time", cHeuristic.getTime());						
			ofstreamOutput << "Finished, time: " << cHeuristic.getTime() << endl;
			ofstreamOutput.flush();						
			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
		}
		else
		{
			ofstreamOutput << "Result already there" << endl;
			ofstreamOutput.flush();
		}
	}
	ofstreamOutput.close();
}

void SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest()
{
	//HANDLE cLock = CreateMutex(FALSE, FALSE, NULL);
	mutex cLock;
	int iThreadNum = 6;
	vector<thread> vectorThreads;
	int iTaskIndex = 0;
	double dUtilIndex = 0.55;
	for (int i = 0; i < iThreadNum; i++)
	{
		vectorThreads.push_back(thread(SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest_thread_routine, i, &iTaskIndex, &dUtilIndex, &cLock));
	}

	for (int i = 0; i < iThreadNum; i++)
	{
		vectorThreads[i].join();
	}
}

void SchedulabilityRunTimeBatchTes_Stress_NSweep()
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	int iNumFlows = 40;
	char axBuffer[512] = { 0 };
	double dUtilLB = 0.8;
	double dUtilUB = 0.95;
	double dHIRatio = 0.5;
	cout << "Dir Change Status: " << _chdir("\\\\RICHIEZHAO-PC\\NoCExp") << endl;
	sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_U%.2f~U%.2f_HR%.2f_NSweep", iXDimension, iYDimension, dUtilLB, dUtilUB, dHIRatio);
	string stringMainFolderName = axBuffer;
	deque<double> dequeUtils;
	deque<int> dequeNs;
	MakePath(axBuffer);
	//for (double dUtil = 0.70; dUtil <= 1.4; dUtil += 0.05)
	for (int iN = 26; iN <= 60; iN+=4)
	//for (int iN = 50; iN >= 50; iN -= 4)
	{
		dequeNs.push_back(iN);
	}
	StatisticSet cTotalStatistcSet;
	char axChar = 'A';
	//for (deque<double>::iterator iter = dequeUtils.begin(); iter != dequeUtils.end(); iter++)
	for (deque<int>::iterator iter = dequeNs.begin(); iter != dequeNs.end(); iter++)
	{
		sprintf_s(axBuffer, "%s\\N%d", stringMainFolderName.data(), *iter);
		string stringSubFolderName = axBuffer;
		MakePath(axBuffer);

		int iNumFlows = *iter;

		for (int i = 0; i < 1000; i++)
		{
			double dUtil = (double)rand() / (double)RAND_MAX;
			dUtil = dUtil * (dUtilUB - dUtilLB) + dUtilLB;			
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;

			//cout << "\r" << stringFlowSetName.data() << "        ";
			cout << stringFlowSetName.data() << endl;
			
			if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
			{
				cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
				cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
				dUtil = cFlowSet.getActualTotalUtil();
				//continue;
			}
			else
			{
				//do the test
				cout << "Cant't find result file" << endl;
				while (1);
				cFlowSet.GenRandomTaskSetInteger(iNumFlows, dUtil, dHIRatio, 2, PERIODOPT_LOG);
				cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
				cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
				sprintf_s(axBuffer, "%s\\FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
				cFlowSet.WriteTextFile(axBuffer);
			}
			cout << "Utilization: " << dUtil << endl;

			//DM
			if (cThisStatsitic.IsItemExist("DM Schedulability") == false)
			{
				TaskSetPriorityStruct cDMPA = cFlowSet.GenDMPriroityAssignment();
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_DM(cFlowSet, cDMPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_DM.ComputeAllResponseTime();
				bool bDMSchedualbility = cDerivedJitterRTCalc_DM.IsSchedulable();
				cThisStatsitic.setItem("DM Schedulability", bDMSchedualbility);
				cout << "DM Schedulability: " << bDMSchedualbility << endl;
			}


			//Pesimistic Schedulability
			if (cThisStatsitic.IsItemExist("Pessimistic Test Schedulability") == false)
			{
				bool bPessSchedulability = false;
				TaskSetPriorityStruct cPessPA(cFlowSet);
				NoCFlowDLUUnschedCoreComputer cPessTest(cFlowSet);
				bPessSchedulability = cPessTest.IsSchedulablePessimistic(cPessPA);
				cout << "Pessimistic Test Schedulability: " << bPessSchedulability << endl;
				cThisStatsitic.setItem("Pessimistic Test Schedulability", bPessSchedulability);
			}

			//FR
			sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
			string stringFRResultFile(axBuffer);
			if (IsFileExist((char *)stringFRResultFile.data()) == false)
			{
				cout << "Can't find FR result" << endl;
				cout << stringFRResultFile.data() << endl;
				while (1);
				NoCFlowDLUFocusRefinement cFR(cFlowSet);
				cFR.setCorePerIter(5);
				//cFR.setObjType(cFR.MaxMinLaxity);
				sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
				cFR.setLogFile(axBuffer);
				int iStatus = cFR.FocusRefinement(1, dTimeout);
				sprintf_s(axBuffer, "%s_FRResult", stringFlowSetName.data());
				StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);
				if (iStatus == 1)
				{
					TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FR.IsSchedulable());
					NoCFlowRTMILPModel cVerifyFullMILP(cFlowSet);
					cVerifyFullMILP.setPredefinePA(rcFRResultPA);
					cout << "Verify Using MILP" << endl;
					cout << "Status: " << cVerifyFullMILP.Solve(1) << endl;
				}
			}
			else
			{
				
			}

			//Full MILP
			if (cThisStatsitic.IsItemExist("Full-MILP Schedulability") == false)
			{
				cout << "Full MILP" << endl;
				NoCFlowRTMILPModel cFullMILP(cFlowSet);
				//cFullMILP.setObjType(cFullMILP.MaxMinLaxity);
				int iFullMILPStatus = 0;
				if (1)
					iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
				else
					iFullMILPStatus = 0;
				cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
				cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
				cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
				if (iFullMILPStatus == 1)
				{
					TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
				}
			}

			//Heuristic
			if (cThisStatsitic.IsItemExist("Heuristic08Burns") == false)
			{
				cout << "Heuristic 08 Burns" << endl;
				NoCPriorityAssignment_2008Burns cHeuristic(cFlowSet);
				time_t iClocks = clock();
				cHeuristic.setTimeout(600);
				TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
				int iStatus = cHeuristic.Run(cPriorityAssignment);
				iClocks = clock() - iClocks;
				cThisStatsitic.setItem("Heuristic08Burns", iStatus);
				cThisStatsitic.setItem("Heuristic08Burns Time", (double)iClocks);
			}


			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
			cout << "----------------------------------------------------------------------" << endl;
		}

		//Read back the result
		int iDMAcceptNum = 0;
		int iDMDenyNum = 0;

		int iPessAcceptNum = 0;
		int iPessDenyNum = 0;

		int iFRAcceptNum = 0;
		int iFRDenyNum = 0;
		int iFRTimeout = 0;

		int iMILPAcceptNum = 0;
		int iMILPDenyNum = 0;
		int iMILPTimeout = 0;

		int iHeuAcceptNum = 0;
		int iHeuDenyNum = 0;
		int iHeuTimeout = 0;

		double dFRTime = 0;
		double dMILPTime = 0;
		double dHeuTime = 0;

		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			cout << "\rReading Case " << i << "    ";
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());

			if (cThisStatsitic.getItem("DM Schedulability").getValue() == 1)
			{
				iDMAcceptNum++;
			}
			else if (cThisStatsitic.getItem("DM Schedulability").getValue() == 0)
			{
				iDMDenyNum++;
			}

			//Pess Test
			if (cThisStatsitic.getItem("Pessimistic Test Schedulability").getValue() == 1)
			{
				iPessAcceptNum++;
			}
			else if (cThisStatsitic.getItem("Pessimistic Test Schedulability").getValue() == 0)
			{
				iPessDenyNum++;
			}

			//FR
			StatisticSet cFRResult;
			sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
			cFRResult.ReadStatisticImage(axBuffer);
			if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
			{
				iFRAcceptNum++;
			}
			else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
			{
				iFRDenyNum++;
			}
			else
			{
				iFRTimeout++;
			}
			dFRTime += cFRResult.getItem("Total Time").getValue();

			//MILP
			if ((cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1) ||
				(cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == -3))
			{
				iMILPAcceptNum++;
			}
			else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
			{
				iMILPDenyNum++;
			}
			else
			{
				iMILPTimeout++;
			}
			dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();

			//Heuristic
			int iStatus = round(cThisStatsitic.getItem("Heuristic08Burns").getValue());
			if (iStatus == 1)
			{
				iHeuAcceptNum++;
			}
			else if (iStatus == 0)
			{
				iHeuDenyNum++;
			}
			else if (iStatus == -1)
			{
				iHeuTimeout++;
			}
			else
			{
				cout << "Unknown Result from heuristic" << endl;
				while (1);
			}
			dHeuTime += cThisStatsitic.getItem("Heuristic08Burns Time").getValue();
		}
		dFRTime /= 1000;
		dMILPTime /= 1000;
		sprintf_s(axBuffer, "%c-%d DM Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iDMAcceptNum);
		sprintf_s(axBuffer, "%c-%d DM Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iDMDenyNum);

		sprintf_s(axBuffer, "%c-%d Pessimistic Test Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iPessAcceptNum);
		sprintf_s(axBuffer, "%c-%d Pessimistic Test Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iPessDenyNum);

		sprintf_s(axBuffer, "%c-%d FR Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
		sprintf_s(axBuffer, "%c-%d FR Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
		sprintf_s(axBuffer, "%c-%d FR Time", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, dFRTime);
		sprintf_s(axBuffer, "%c-%d FR Timeout", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iFRTimeout);

		sprintf_s(axBuffer, "%c-%d MILP Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
		sprintf_s(axBuffer, "%c-%d MILP Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
		sprintf_s(axBuffer, "%c-%d MILP Time", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, dMILPTime);
		sprintf_s(axBuffer, "%c-%d MILP Timeout", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iMILPTimeout);

		sprintf_s(axBuffer, "%c-%d Heu Accept", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iHeuAcceptNum);
		sprintf_s(axBuffer, "%c-%d Heu Deny", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iHeuDenyNum);
		sprintf_s(axBuffer, "%c-%d Heu Time", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, dHeuTime);
		sprintf_s(axBuffer, "%c-%d Heu Timeout", axChar, iNumFlows);
		cTotalStatistcSet.setItem(axBuffer, iHeuTimeout);

		axChar++;
		sprintf_s(axBuffer, "%s//%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
		cTotalStatistcSet.WriteStatisticText(axBuffer);
	}
}

void SchedulabilityRunTimeBatchTes_Stress_NSweep_MultiThreadHeuristicTest_thread_routine(int iThreadIndex, int * piTaskIndex, int * piNIndex, mutex * pcLock)
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	int iNumFlows = 40;
	char axBuffer[512] = { 0 };
	double dUtilLB = 0.8;
	double dUtilUB = 0.95;
	double dHIRatio = 0.50;
	sprintf_s(axBuffer, "SchedulabilityRunTimeBatchTes_Stress_NSweep_MultiThreadHeuristicTest_thread_routine_%d.txt", iThreadIndex);
	ofstream ofstreamOutput(axBuffer);
	cout << "Dir Change Status: " << _chdir("\\\\RICHIEZHAO-PC\\NoCExp") << endl;	
	sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_U%.2f~U%.2f_HR%.2f_NSweep", iXDimension, iYDimension, dUtilLB, dUtilUB, dHIRatio);
	string stringMainFolderName = axBuffer;
	while (true)
	{
		//WaitForSingleObject(*pcLock, INFINITE);
		pcLock->lock();
		int iN = *piNIndex;
		int i = (*piTaskIndex);
		if (iN > 50)
		{
			//ReleaseMutex(*pcLock);
			pcLock->unlock();
			break;
		}
		assert(i < 1000 && i >= 0);
		(*piTaskIndex)++;
		if ((*piTaskIndex) == 1000)
		{
			*piNIndex += 4;
			*piTaskIndex = 0;
		}
		//ReleaseMutex(*pcLock);
		pcLock->unlock();

		sprintf_s(axBuffer, "%s\\N%d", stringMainFolderName.data(), iN);
		string stringSubFolderName = axBuffer;
		FlowsSet_2DNetwork cFlowSet;
		sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
		string stringFlowSetName = axBuffer;
		StatisticSet cThisStatsitic;
		sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
		string stringThisStatisticName = axBuffer;

		//cout << "\r" << stringFlowSetName.data() << "        ";
		//cout << stringFlowSetName.data() << endl;
		ofstreamOutput << stringFlowSetName.data() << endl;
		ofstreamOutput.flush();
		StatisticSet cTotalStatistcSet;
		if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
		{
			cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
			cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
		}
		else
		{
			cout << "Cannot find target task file or result file" << stringFlowSetName.data() << endl;
			while (1);
		}

		//Heuristic
		if (cThisStatsitic.IsItemExist("Heuristic08Burns") == false)
		{
			cout << "Heuristic 08 Burns" << endl;
			NoCPriorityAssignment_2008Burns cHeuristic(cFlowSet);
			time_t iClocks = clock();
			cHeuristic.setTimeout(600);
			TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
			int iStatus = cHeuristic.Run(cPriorityAssignment);
			iClocks = clock() - iClocks;
			cThisStatsitic.setItem("Heuristic08Burns", iStatus);
			cThisStatsitic.setItem("Heuristic08Burns Time", cHeuristic.getTime());
			ofstreamOutput << "Finished, time: " << cHeuristic.getTime() << endl;
			ofstreamOutput.flush();
			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
		}
		else
		{
			ofstreamOutput << "Result already there" << endl;
			ofstreamOutput.flush();
		}
	}
	ofstreamOutput.close();
}

void SchedulabilityRunTimeBatchTes_Stress_NSweep_MultiThreadHeuristicTest()
{
	//HANDLE cLock = CreateMutex(FALSE, FALSE, NULL);
	mutex cLock;
	int iThreadNum = 6;
	vector<thread> vectorThreads;
	int iTaskIndex = 0;
	int iNIndex = 26;
	for (int i = 0; i < iThreadNum; i++)
	{
		vectorThreads.push_back(thread(SchedulabilityRunTimeBatchTes_Stress_NSweep_MultiThreadHeuristicTest_thread_routine, i, &iTaskIndex, &iNIndex, &cLock));
	}

	for (int i = 0; i < iThreadNum; i++)
	{
		vectorThreads[i].join();
	}
}

void SchedulabilityRunTimeBatchTes_Stress_USweep_MaxMinLaxity()
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	int iNumFlows = 40;
	char axBuffer[512] = { 0 };
	double dHIRatio = 0.50;
	//cout << "Dir Change Status: " << _chdir("\\\\RICHIEZHAO-PC\\NoCExp") << endl;
	sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_N%d_HR%.2f_USweep_MaxMinLaxity", iXDimension, iYDimension, iNumFlows, dHIRatio);
	string stringMainFolderName = axBuffer;
	deque<double> dequeUtils;
	MakePath(axBuffer);
	for (double dUtil = 0.70; dUtil <= 1.35; dUtil += 0.05)
	//for (double dUtil = 1.30; dUtil >= 0.65; dUtil -= 0.05)
	{
		dequeUtils.push_back(dUtil);
	}
	StatisticSet cTotalStatistcSet;
	char axChar = 'A';
	for (deque<double>::iterator iter = dequeUtils.begin(); iter != dequeUtils.end(); iter++)
	{
		sprintf_s(axBuffer, "%s\\U%.2f", stringMainFolderName.data(), *iter);
		string stringSubFolderName = axBuffer;
		MakePath(axBuffer);
		double dUtil = *iter;

		for (int i = 0; i < 1000; i++)
		{
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;

			//cout << "\r" << stringFlowSetName.data() << "        ";
			cout << stringFlowSetName.data() << endl;
			bool bRedo = false;
			if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
			{
				cout << "Result File Found" << endl;
				cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
				cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
				//continue;
			}
			else
			{
				//do the test				
				cFlowSet.GenRandomTaskSetInteger(iNumFlows, dUtil, dHIRatio, 2, PERIODOPT_LOG);
				cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
				cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
				sprintf_s(axBuffer, "%s\\FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
				cFlowSet.WriteTextFile(axBuffer);
				bRedo = true;
			}

		


			//FR
			sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
			string stringFRResultFile(axBuffer);
			if (IsFileExist((char *)stringFRResultFile.data()) == false || bRedo)
			{
				cout << "FR: " << endl;
				NoCFlowDLUFocusRefinement cFR(cFlowSet);
				cFR.setCorePerIter(5);
				cFR.setObjType(cFR.MaxMinLaxity);
				sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
				cFR.setLogFile(axBuffer);
				int iStatus = cFR.FocusRefinement(1, dTimeout);
				sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
				StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);
				if (iStatus == 1)
				{
					TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FR.IsSchedulable());
					NoCFlowRTMILPModel cVerifyFullMILP(cFlowSet);
					cVerifyFullMILP.setPredefinePA(rcFRResultPA);
					cout << "Verify Using MILP" << endl;
					cout << "Status: " << cVerifyFullMILP.Solve(1) << endl;
				}
			}


			//Full MILP
			if (cThisStatsitic.IsItemExist("Full-MILP Schedulability") == false || bRedo)
			{
				cout << "Full MILP" << endl;
				NoCFlowRTMILPModel cFullMILP(cFlowSet);
				cFullMILP.setObjType(cFullMILP.MaxMinLaxity);
				int iFullMILPStatus = 0;
				if (1)
					iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
				else
					iFullMILPStatus = 0;
				cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
				cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
				cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
				cThisStatsitic.setItem("Full-MILP Objective", cFullMILP.getObjective());
				if (iFullMILPStatus == 1)
				{
					TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
				}
			}

			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
			cout << "----------------------------------------------------------------------" << endl;
		}

		//Read back the result
		int iDMAcceptNum = 0;
		int iDMDenyNum = 0;

		int iPessAcceptNum = 0;
		int iPessDenyNum = 0;

		int iFRAcceptNum = 0;
		int iFRDenyNum = 0;
		int iFRTimeout = 0;
		double dFRObjective = 0;

		int iMILPAcceptNum = 0;
		int iMILPDenyNum = 0;
		int iMILPTimeout = 0;
		double dMILPObjective = 0;

		int iHeuAcceptNum = 0;
		int iHeuDenyNum = 0;
		int iHeuTimeout = 0;

		double dFRTime = 0;
		double dMILPTime = 0;
		double dHeuTime = 0;
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			cout << "\rReading Case " << i << "    ";
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());

			//Derive an upperbound on the objective
			FlowsSet_2DNetwork cFlowSet;
			cFlowSet.ReadImageFile((char *)stringFlowSetName.data());
			GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
			double dObjUB = pcTaskArray[0].getDeadline() - pcTaskArray[0].getCLO();
			for (int i = 0; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCHI());
				}
				else
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO());
				}
			}


			//FR
			StatisticSet cFRResult;
			sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
			cFRResult.ReadStatisticImage(axBuffer);
			if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
			{
				iFRAcceptNum++;
				dFRObjective += cFRResult.getItem("Objective").getValue() / dObjUB;
			}
			else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
			{
				iFRDenyNum++;				
			}
			else
			{
				iFRTimeout++;
				dFRObjective += 1;
			}
			dFRTime += cFRResult.getItem("Total Time").getValue();
			

			//MILP
			if ((cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1) ||
				(cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == -3))
			{
				iMILPAcceptNum++;
				dMILPObjective += cThisStatsitic.getItem("Full-MILP Objective").getValue() / dObjUB;
			}
			else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
			{
				iMILPDenyNum++;
			}
			else
			{
				iMILPTimeout++;
				dMILPObjective += 1;
			}
			dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();
			
		}
		dFRTime /= 1000;
		dMILPTime /= 1000;

		dFRObjective /= 1000;
		dMILPObjective /= 1000;		

		sprintf_s(axBuffer, "%c-U%.2f FR Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f FR Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
		sprintf_s(axBuffer, "%c-U%.2f FR Time", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, dFRTime);
		sprintf_s(axBuffer, "%c-U%.2f FR Timeout", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iFRTimeout);
		sprintf_s(axBuffer, "%c-U%.2f FR Objective", axChar, dFRObjective);
		cTotalStatistcSet.setItem(axBuffer, iFRTimeout);

		sprintf_s(axBuffer, "%c-U%.2f MILP Accept", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
		sprintf_s(axBuffer, "%c-U%.2f MILP Deny", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
		sprintf_s(axBuffer, "%c-U%.2f MILP Time", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, dMILPTime);
		sprintf_s(axBuffer, "%c-U%.2f MILP Timeout", axChar, dUtil);
		cTotalStatistcSet.setItem(axBuffer, iMILPTimeout);
		sprintf_s(axBuffer, "%c-U%.2f MILP MILP Objective", axChar, dMILPObjective);
		cTotalStatistcSet.setItem(axBuffer, iMILPTimeout);

		axChar++;
		sprintf_s(axBuffer, "%s//%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
		cTotalStatistcSet.WriteStatisticText(axBuffer);
	}
}

void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity()
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;	
	char axBuffer[512] = { 0 };	
	cout << "Dir Change Status: " << _chdir("\\\\RICHIEZHAO-PC\\NoCExp") << endl;
	double dUtilLB = 0.8;
	double dUtilUB = 0.95;
	double dHIRatio = 0.5;
	sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_U%.2f~U%.2f_HR%.2f_NSweep_MaxMinLaxity", iXDimension, iYDimension, dUtilLB, dUtilUB, dHIRatio);
	
	string stringMainFolderName = axBuffer;	
	deque<int> dequeNs;
	MakePath(axBuffer);	
	for (int iN = 26; iN <= 46; iN+=4)
	//for (int iN = 38; iN >= 26; iN -= 4)
	//for (int iN = 34; iN >= 34; iN -= 4)
	{
		dequeNs.push_back(iN);
	}
	StatisticSet cTotalStatistcSet;
	char axChar = 'A';
	for (deque<int>::iterator iter = dequeNs.begin(); iter != dequeNs.end(); iter++)
	{
		sprintf_s(axBuffer, "%s\\N%d", stringMainFolderName.data(), *iter);
		string stringSubFolderName = axBuffer;
		MakePath(axBuffer);
		int iN = *iter;
		int iNumTasks = iN;

		for (int i = 0; i < 1000; i++)
		//for (int i = 1000 - 1; i >= 0; i--)
		{
			double dUtil = (double)rand() / (double)RAND_MAX;
			dUtil = dUtil * (dUtilUB - dUtilLB) + dUtilLB;
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;

			//cout << "\r" << stringFlowSetName.data() << "        ";
			cout << stringFlowSetName.data() << endl;
			bool bRedo = false;
			if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
			{
				cout << "Result File Found" << endl;
				cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
				cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
				//continue;
			}
			else
			{
				//do the test						
				cFlowSet.GenRandomTaskSetInteger(iNumTasks, dUtil, dHIRatio, 2, PERIODOPT_LOG);
				cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
				cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
				sprintf_s(axBuffer, "%s\\FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
				cFlowSet.WriteTextFile(axBuffer);
				bRedo = true;
			}
			cout << "Utilization: " << dUtil << endl;




			//FR
			sprintf_s(axBuffer, "%s_FRResult.rslt.rslt", stringFlowSetName.data());
			string stringFRResultFile(axBuffer);
			if (IsFileExist((char *)stringFRResultFile.data()) == false || bRedo)
			{
				cout << "FR: " << endl;
				NoCFlowDLUFocusRefinement cFR(cFlowSet);
				cFR.setCorePerIter(5);
				cFR.setObjType(cFR.MaxMinLaxity);
				sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
				cFR.setLogFile(axBuffer);
				int iStatus = cFR.FocusRefinement(1, dTimeout);
				sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
				StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);
				if (iStatus == 1)
				{
					TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FR.IsSchedulable());
					NoCFlowRTMILPModel cVerifyFullMILP(cFlowSet);
					cVerifyFullMILP.setPredefinePA(rcFRResultPA);
					cout << "Verify Using MILP" << endl;
					cout << "Status: " << cVerifyFullMILP.Solve(1) << endl;
				}
			}


			//Full MILP
			if (cThisStatsitic.IsItemExist("Full-MILP Schedulability") == false || bRedo)
			{
				cout << "Full MILP" << endl;
				NoCFlowRTMILPModel cFullMILP(cFlowSet);
				cFullMILP.setObjType(cFullMILP.MaxMinLaxity);
				int iFullMILPStatus = 0;
				if (1)
					iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
				else
					iFullMILPStatus = 0;
				cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
				cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
				cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
				cThisStatsitic.setItem("Full-MILP Objective", cFullMILP.getObjective());
				if (iFullMILPStatus == 1)
				{
					TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
				}
			}

			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
			cout << "----------------------------------------------------------------------" << endl;
		}

		//Read back the result
		int iDMAcceptNum = 0;
		int iDMDenyNum = 0;

		int iPessAcceptNum = 0;
		int iPessDenyNum = 0;

		int iFRAcceptNum = 0;
		int iFRDenyNum = 0;
		int iFRTimeout = 0;
		double dFRObjective = 0;

		int iMILPAcceptNum = 0;
		int iMILPDenyNum = 0;
		int iMILPTimeout = 0;
		double dMILPObjective = 0;

		int iHeuAcceptNum = 0;
		int iHeuDenyNum = 0;
		int iHeuTimeout = 0;
		double dHeuObjective = 0;

		double dFRTime = 0;
		double dMILPTime = 0;
		double dHeuTime = 0;
		for (int i = 0; i < 1000; i++)
		{
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			cout << "\rReading Case " << i << "    ";
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;
			cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());

			//Derive an upperbound on the objective
			FlowsSet_2DNetwork cFlowSet;
			cFlowSet.ReadImageFile((char *)stringFlowSetName.data());
			GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
			double dObjUB = pcTaskArray[0].getDeadline() - pcTaskArray[0].getCLO();
			for (int i = 0; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCHI());
				}
				else
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO());
				}
			}


			//FR
			StatisticSet cFRResult;
			sprintf_s(axBuffer, "%s_FRResult.rslt.rslt", stringFlowSetName.data());
			cFRResult.ReadStatisticImage(axBuffer);
			if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
			{
				iFRAcceptNum++;
				dFRObjective += cFRResult.getItem("Objective").getValue() / dObjUB;
			}
			else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
			{
				iFRDenyNum++;
				dFRObjective += -1;
			}
			else
			{
				iFRTimeout++;
				//dFRObjective -= 1;
			}
			dFRTime += cFRResult.getItem("Total Time").getValue();


			//MILP
			if ((cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1) ||
				(cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == -3))
			{
				iMILPAcceptNum++;
				dMILPObjective += cThisStatsitic.getItem("Full-MILP Objective").getValue() / dObjUB;
			}
			else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
			{
				iMILPDenyNum++;
				dMILPObjective += -1;
			}
			else
			{
				iMILPTimeout++;
				//dMILPObjective -= 1;
			}
			dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();
#if 1
			//Heuristic
			int iStatus = round(cThisStatsitic.getItem("Heuristic08Burns").getValue());
			double dObj = cThisStatsitic.getItem("Heuristic08Burns Objective").getValue();
			if (dObj > 0)
			{
				iHeuAcceptNum++;
				dHeuObjective += -dObj / dObjUB;
			}
			else if (iStatus == 0)
			{
				iHeuDenyNum++;
			}			
			else if (dObj < 0)
			{
				if (iStatus != -1)
				{
					cout << "should give credit to Heu" << endl;
					while (1);
				}
				dHeuObjective += -1;
			}

			if (iStatus == -1)
			{
				iHeuTimeout++;						
			}		
			dHeuTime += cThisStatsitic.getItem("Heuristic08Burns Time").getValue();					
#endif

		}
		dFRTime /= 1000;
		dMILPTime /= 1000;
		dHeuTime /= 1000;

		dFRObjective /= 1000;
		dMILPObjective /= 1000;
		dHeuObjective /= 1000;

		sprintf_s(axBuffer, "%c-%d FR Accept", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
		sprintf_s(axBuffer, "%c-%d FR Deny", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
		sprintf_s(axBuffer, "%c-%d FR Time", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, dFRTime);
		sprintf_s(axBuffer, "%c-%d FR Timeout", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iFRTimeout);
		sprintf_s(axBuffer, "%c-%d FR Objective", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, dFRObjective);

		sprintf_s(axBuffer, "%c-%d MILP Accept", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
		sprintf_s(axBuffer, "%c-%d MILP Deny", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
		sprintf_s(axBuffer, "%c-%d MILP Time", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, dMILPTime);
		sprintf_s(axBuffer, "%c-%d MILP Timeout", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iMILPTimeout);
		sprintf_s(axBuffer, "%c-%d MILP MILP Objective", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, dMILPObjective);

#if 1
		sprintf_s(axBuffer, "%c-%d Heu Accept", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iHeuAcceptNum);
		sprintf_s(axBuffer, "%c-%d Heu Deny", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iHeuDenyNum);
		sprintf_s(axBuffer, "%c-%d Heu Time", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, dHeuTime);
		sprintf_s(axBuffer, "%c-%d Heu Timeout", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, iHeuTimeout);
		sprintf_s(axBuffer, "%c-%d Heu Objective", axChar, iN);
		cTotalStatistcSet.setItem(axBuffer, dHeuObjective);
#endif

		axChar++;
		sprintf_s(axBuffer, "%s//%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
		cTotalStatistcSet.WriteStatisticText(axBuffer);
	}
}

void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_HeuristicTest_thread_routine(int iThreadIndex, int * piTaskIndex, int * piNIndex , mutex * pcLock)
{
	int iXDimension = 4;
	int iYDimension = 4;
	double dTimeout = 600;
	int iNumFlows = 40;
	char axBuffer[512] = { 0 };
	double dUtilLB = 0.8;
	double dUtilUB = 0.95;
	double dHIRatio = 0.50;
	sprintf_s(axBuffer, "SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_MultiThreadHeuristicTest_thread_routine_%d.txt", iThreadIndex);
	ofstream ofstreamOutput(axBuffer);
	cout << "Dir Change Status: " << _chdir("\\\\RICHIEZHAO-PC\\NoCExp") << endl;
	sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_U%.2f~U%.2f_HR%.2f_NSweep_MaxMinLaxity", iXDimension, iYDimension, dUtilLB, dUtilUB, dHIRatio);
	string stringMainFolderName = axBuffer;
	while (true)
	{
		//WaitForSingleObject(*pcLock, INFINITE);
		pcLock->lock();
		int iN = *piNIndex;
		int i = (*piTaskIndex);
		if (iN > 46)
		{
			//ReleaseMutex(*pcLock);
			pcLock->unlock();
			break;
		}
		assert(i < 1000 && i >= 0);
		(*piTaskIndex)++;
		if ((*piTaskIndex) == 1000)
		{
			*piNIndex += 4;
			*piTaskIndex = 0;
		}
#if 1
		i = 1000 - i - 1;
#endif
		//ReleaseMutex(*pcLock);
		pcLock->unlock();

		sprintf_s(axBuffer, "%s\\N%d", stringMainFolderName.data(), iN);
		string stringSubFolderName = axBuffer;
		FlowsSet_2DNetwork cFlowSet;
		sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
		string stringFlowSetName = axBuffer;
		StatisticSet cThisStatsitic;
		sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
		string stringThisStatisticName = axBuffer;

		//cout << "\r" << stringFlowSetName.data() << "        ";
		//cout << stringFlowSetName.data() << endl;
		ofstreamOutput << stringFlowSetName.data() << endl;
		ofstreamOutput.flush();
		StatisticSet cTotalStatistcSet;
		if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
		{
			cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
			cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
		}
		else
		{
			cout << "Cannot find target task file or result file" << stringFlowSetName.data() << endl;
			while (1);
		}
		GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
		double dObjUB = pcTaskArray[0].getDeadline() - pcTaskArray[0].getCLO();
		for (int i = 0; i < iTaskNum; i++)
		{
			if (pcTaskArray[i].getCriticality())
			{
				dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCHI());
			}
			else
			{
				dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO());
			}
		}

		//Heuristic
		if (cThisStatsitic.IsItemExist("Heuristic08Burns") == false)
		{
			cout << "Heuristic 08 Burns" << endl;
			NoCPriorityAssignment_2008Burns_MaxMinLaxity cHeuristic(cFlowSet);
			time_t iClocks = clock();
			cHeuristic.setTimeout(600);
			TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
			int iStatus = cHeuristic.Run(cPriorityAssignment);
			iClocks = clock() - iClocks;
			cThisStatsitic.setItem("Heuristic08Burns", iStatus);
			cThisStatsitic.setItem("Heuristic08Burns Time", cHeuristic.getTime());
			cThisStatsitic.setItem("Heuristic08Burns Objective", cHeuristic.getCurrentBest());
			ofstreamOutput << "Finished, time: " << cHeuristic.getTime() << " Objective: " << cHeuristic.getCurrentBest() << " Normalized: " << cHeuristic.getCurrentBest() / dObjUB
				<< endl;;
			ofstreamOutput.flush();
			cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
			sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
			cThisStatsitic.WriteStatisticText(axBuffer);
		}
		else
		{
			ofstreamOutput << "Result already there" << endl;
			ofstreamOutput.flush();
		}
	}
	ofstreamOutput.close();
}

void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_HeuristicTest()
{
	//HANDLE cLock = CreateMutex(FALSE, FALSE, NULL);
	mutex cLock;
	int iThreadNum = 3;
	vector<thread> vectorThreads;
	int iTaskIndex = 0;
	int iNIndex = 46;
	for (int i = 0; i < iThreadNum; i++)
	{
		vectorThreads.push_back(thread(SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_HeuristicTest_thread_routine, i, &iTaskIndex, &iNIndex, &cLock));
	}

	for (int i = 0; i < iThreadNum; i++)
	{
		vectorThreads[i].join();
	}
}

void NoCSchedTest_EmergencyTest()
{
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("NocUSweepTimeoutCases\\FlowSet56.tskst");
	NoCFlowDLUFocusRefinement cFR(cFlowSet);
	cFR.setCorePerIter(5);
	cFR.setLogFile("EmergencyTestLog.txt");
	cFR.getUnschedCoreComputer().InitializeDeadlineLBLB();
	int iStatus = cFR.FocusRefinement(1);
	if (iStatus == 1)
	{
		TaskSetPriorityStruct cPriorityAssignment = cFR.getPriorityAssignmentSol();
		NoCFlowDLUFocusRefinement::SchedConds cSchedCondConfig = cFR.getOptimalSchedCondConfig();
		PrintSetContent(cSchedCondConfig);
		NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cPriorityAssignment, cFlowSet.getDirectJitterTable());
		cRTCalc.ComputeAllResponseTime();
		cRTCalc.PrintResult(cout);
		cRTCalc.PrintSchedulabilityInfo();
	}
}

void NoCSchedTest_FRAndMILP()
{
	FlowsSet_2DNetwork cFlowSet;
	//cFlowSet.ReadImageFile("NocUSweepTimeoutCases\\FlowSet10.tskst");
	//cFlowSet.ReadImageFile("NocHardCases\\FlowSet322.tskst");
	cFlowSet.ReadImageFile("FlowSet426.tskst");
	//cFlowSet.ReadImageFile("FlowSet0.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	typedef NoCFlowDLUFocusRefinement::SchedConds SchedConds;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;

	if (0)
	{
		cout << "MILP MinJitterRTDiff:" << endl;
		NoCFlowRTMILPModel cFullMILP(cFlowSet);
		int iFullMILPStatus = cFullMILP.Solve_MinJitterRTDifference(2);
	}

	if (1)
	{		
		cout << "FR:" << endl;
		NoCFlowDLUFocusRefinement cFR(cFlowSet);
		cFR.setCorePerIter(5);
		//cFR.setCustomParam(string("LUBTolDiff"), 0.1);
		SchedConds cRefSchedConds;
		for (int i = 0; i < iTaskNum; i++)
		{
			if (pcTaskArray[i].getCriticality())
			{
				cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineUBHI)] = pcTaskArray[i].getDeadline();
			}
			
			{
				cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineUBLO)] = pcTaskArray[i].getDeadline();
			}
		}
		cFR.setSubILPDisplay(0);
		cFR.setReferenceSchedConds(cRefSchedConds);		
		cFR.setObjType(cFR.MaxMinLaxity);
		cFR.setLogFile("EmergencyTestLog.txt");
		cFR.getUnschedCoreComputer().InitializeDeadlineLBLB();
		int iStatus = cFR.FocusRefinement(1);
		if (iStatus == 1)
		{
			//Verify using ILP
			NoCFlowRTMILPModel cMILP(cFlowSet);
			TaskSetPriorityStruct cPA = cFR.getSolPA();
			cMILP.setPredefinePA(cPA);
			cMILP.Solve(2);
			
			TaskSetPriorityStruct cPriorityAssignment = cFR.getPriorityAssignmentSol();
			NoCFlowDLUFocusRefinement::SchedConds cSchedConds = cFR.getOptimalSchedCondConfig();
			PrintSetContent(cSchedConds);
			SchedConds cOptSchedConds = cFR.getOptimalSchedCondConfig();
			NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cPriorityAssignment, cFlowSet.getDirectJitterTable());
			cRTCalc.ComputeAllResponseTime();			
			cRTCalc.PrintSchedulabilityInfo();
			for (int i = 0; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
				{
					cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineUBHI)] = cRTCalc.getRTHI(i);
				}
				cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineUBLO)] = cRTCalc.getRTLO(i);
			}
			cRefSchedConds.clear();
			for (int i = 0; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
				{
					cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineUBHI)] =
						round((cOptSchedConds[SchedCondType(i, SchedCondType::DeadlineUBHI)] + cOptSchedConds[SchedCondType(i, SchedCondType::DeadlineLBHI)]) / 2);
				}
				cRefSchedConds[SchedCondType(i, SchedCondType::DeadlineUBLO)] =
					round((cOptSchedConds[SchedCondType(i, SchedCondType::DeadlineUBLO)] + cOptSchedConds[SchedCondType(i, SchedCondType::DeadlineLBLO)]) / 2);
			}

			cout << "Relaxation Guided FR" << endl;
			NoCFlowDLUFocusRefinement cFRGuided(cFlowSet);
			cFRGuided.setReferenceSchedConds(cRefSchedConds);
			cFRGuided.setCorePerIter(5);
			cFRGuided.setObjType(NoCFlowDLUFocusRefinement::ReferenceGuided);
			cFRGuided.FocusRefinement(1);


		}
	}

	if (1)
	{
		cout << "MILP:" << endl;
		NoCFlowRTMILPModel cFullMILP(cFlowSet);		
		int iFullMILPStatus = cFullMILP.Solve(2);
		cout << iFullMILPStatus << endl;
		if (iFullMILPStatus == 1)
		{
			TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
			NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
			cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
			assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
		}
	}
	
	//
}

void NoCTestHeuristic()
{
	FlowsSet_2DNetwork cFlowSet;
#if 1
	cFlowSet.ReadImageFile("NocHardCases\\FlowSet43.tskst");
#else
	cFlowSet.GenRandomTaskSetInteger(40, 1.0, 0.5, 2, PERIODOPT_LOG);
	cFlowSet.GenerateRandomFlowRoute(4, 4);
#endif
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	NoCPriorityAssignment_2008Burns cHeuristic(cFlowSet);
	TaskSetPriorityStruct cPA(cFlowSet);	
	int iStatus = cHeuristic.Run(cPA);
	cout << "Status: " << iStatus << endl;

}

void SmallNoCExample2()
{
	struct ExampleTaskSetConfig
	{
		double dPeriod;
		double dWCET;
		bool bCriticality;
	}atagConfig[] = {
		{ 2000, 0.0, false },
		{ 10, 4, false },
		{ 35, 9, false },
		{ 120, 5, false },
		{ 180, 35, false },
		//{ 200, 17.0 / 200.0, false },
		//{ 400, 32.0 / 400.0, false },
	};
	FlowsSet_2DNetwork cTaskSet;
	int iNTask = sizeof(atagConfig) / sizeof(struct ExampleTaskSetConfig);
	vector<double> vectorDirectJitter, vectorIndirectJitterLO, vectorIndirectJitterHI;
	for (int i = 0; i < iNTask; i++)
	{
		cTaskSet.AddTask(atagConfig[i].dPeriod, atagConfig[i].dPeriod, atagConfig[i].dWCET / atagConfig[i].dPeriod, 2.0, atagConfig[i].bCriticality, i + 1);
		vectorDirectJitter.push_back(0);
		vectorIndirectJitterLO.push_back(0);
		vectorIndirectJitterHI.push_back(0);
	}
	cTaskSet.ConstructTaskArray();
	cTaskSet.GenerateRandomFlowRoute(1, 1);
	cTaskSet.WriteTextFile("SmallNoCExample.txt");
	cTaskSet.WriteImageFile("SmallNoCExample.txt.tskst");
	cTaskSet.DisplayTaskInfo();
	TaskSetPriorityStruct  cPriorityAssignment(cTaskSet);
	cPriorityAssignment = cTaskSet.GetDMPriorityAssignment();
	NoCFlowAMCRTCalculator_DerivedJitter cCalcDJ(cTaskSet, cPriorityAssignment, vectorDirectJitter);
	NoCFlowAMCRTCalculator_SpecifiedJitter cCalcSJ(cTaskSet, vectorDirectJitter, vectorIndirectJitterLO, vectorIndirectJitterHI, cPriorityAssignment);
	typedef NoCFlowDLUUnschedCoreComputer::SchedConds SchedConds;
	typedef NoCFlowDLUUnschedCoreComputer::SchedCondType SchedCondType;
	NoCFlowDLUUnschedCoreComputer cUnschedCoreComputer(cTaskSet);

	{
	//Motivating Example
		SchedConds cSchedCondsME, cDummy;
		TaskSetPriorityStruct cPriorityAssignment(cTaskSet);
		char axBuffer[512] = { 0 };
		double dDeadline_0 = 0;
		double dDeadline_1 = 8;
		double dDeadline_2 = 17;
		double dDeadline_3 = 30;
		double dDeadline_4 = 180;
		sprintf_s(axBuffer, "({0, DUBLO} : %.0f), ({0, DLBLO} : %.0f), ({1, DLBLO} : %.0f), ({1, DUBLO} : %.0f), ({2, DUBLO} : %.0f), ({2, DLBLO} : %.0f), ({3, DUBLO} : %.0f), ({3, DLBLO} : %.0f), ({4, DUBLO} : %.0f), ({4, DLBLO} : %.0f)",
			dDeadline_0, dDeadline_0,
			dDeadline_1, dDeadline_1,
			dDeadline_2, dDeadline_2,
			dDeadline_3, dDeadline_3,
			dDeadline_4, dDeadline_4
			);		
		cUnschedCoreComputer.ReadSchedCondsFromStr(cSchedCondsME, axBuffer);
		cUnschedCoreComputer.PrintSchedConds(cSchedCondsME, cout);
		cout << "Schedulability: " << cUnschedCoreComputer.IsSchedulable(cSchedCondsME, cDummy, cPriorityAssignment) << endl;;
		cPriorityAssignment.PrintByPriority();		
	
	}

	//DLUFR
	NoCFlowDLUFocusRefinement cFR(cTaskSet);
	cFR.setLogFile("SmallNoCExampleFR_Log.txt");
	cFR.setCorePerIter(3);
	int iStatus=  cFR.FocusRefinement(1);
	if (iStatus)
	{
		cFR.getSolPA().PrintByPriority();
		cout << "----------" << endl;
		cFR.getSolPA().Print();
	}
	system("pause");
}

void Construct14MCWormholeCaseStudy(double dTotalUtil, FlowsSet_2DNetwork & rcFlowSet)
{
	double color = 8;
	double resW = 320;
	double resH = 240;
	double flit = 128;
	double highCritFactor = 10;
	vector<double> vectorSizes;
	vector<int> vectorRouteLength;
	vector<int> vectorCriticalityLevel;
	vector<double> vectorPeriod;
	for (int i = 0; i < 39; i++)
	{
		vectorSizes.push_back(0);
		vectorRouteLength.push_back(0);
		vectorCriticalityLevel.push_back(0);
		vectorPeriod.push_back(0);
	}

	vectorCriticalityLevel[24] = 1;
	vectorCriticalityLevel[31] = 1;
	vectorCriticalityLevel[37] = 1;

	for (int i = 1; i <= 4; i++) vectorPeriod[i] = (0.5);
	for (int i = 5; i <= 7; i++) vectorPeriod[i] = (0.1);
	for (int i = 8; i <= 30; i++) vectorPeriod[i] = (0.04);
	vectorPeriod[31] = (0.5);
	vectorPeriod[32] = (0.1);
	vectorPeriod[33] = 1;
	vectorPeriod[34] = 0.5;
	vectorPeriod[35] = 0.1;
	vectorPeriod[36] = 1;
	vectorPeriod[37] = 0.1;
	vectorPeriod[38] = 0.1;

	for (int i = 1; i <= 38; i++)
	{
		vectorPeriod[i] *= 1e6;
	}
	vectorSizes[1] = (resW / 2) * (resH / 2) * color / flit * 20;
	vectorSizes[2] = ((resW / 2) * (resH / 2) * color) / flit;
	vectorSizes[3] = 16384 / flit;
	vectorSizes[4] = 16384 / flit * 10;
	vectorSizes[5] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[6] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[7] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[8] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[9] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[10] = (resW / 2) * (resH / 2) * color / flit * 10;
	vectorSizes[11] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[12] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[13] = (((resW / 2) * (resH / 2) * color) / 20) / flit * 100;
	vectorSizes[14] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[15] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[16] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[17] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[18] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[19] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[20] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[21] = (((resW / 2) * (resH / 2) * color) / 5) / flit * 100;
	vectorSizes[22] = (((resW / 2) * (resH / 2) * color) / 5) / flit;
	vectorSizes[23] = 131072 / flit;
	vectorSizes[24] = highCritFactor * 16384 / flit;
	vectorSizes[25] = 16384 / flit;
	vectorSizes[26] = 32768 / flit;
	vectorSizes[27] = 32768 / flit * 100;
	vectorSizes[28] = 16384 / flit;
	vectorSizes[29] = 32768 / flit;
	vectorSizes[30] = 16384 / flit;
	vectorSizes[31] = highCritFactor * 16384 / flit;
	vectorSizes[32] = 16384 / flit;
	vectorSizes[33] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[34] = 131072 / flit;
	vectorSizes[35] = 32768 / flit;
	vectorSizes[36] = 524288 / flit;
	vectorSizes[37] = highCritFactor * 32768 / flit;
	vectorSizes[38] = 524288 / flit;

	FlowsSet_2DNetwork cFlowSet;
	vector<FlowsSet_2DNetwork::Route> vectorRoutes;
	for (int i = 0; i < 39; i++)	vectorRoutes.push_back(FlowsSet_2DNetwork::Route());
	map<string, FlowsSet_2DNetwork::Coordinate, StringCompObj> mapName2Router;
	mapName2Router["FBU4"] = { 0, 0 };
	mapName2Router["BFE4"] = { 1, 0 };
	mapName2Router["BFE8"] = { 2, 0 };
	mapName2Router["FBU8"] = { 3, 0 };
	mapName2Router["FBU3"] = { 1, 1 };
	mapName2Router["BFE3"] = { 0, 1 };
	mapName2Router["BFE7"] = { 2, 1 };
	mapName2Router["FBU7"] = { 3, 1 };
	mapName2Router["FBU2"] = { 0, 2 };
	mapName2Router["BFE2"] = { 1, 2 };
	mapName2Router["BFE6"] = { 3, 2 };
	mapName2Router["FBU6"] = { 2, 2 };
	mapName2Router["FBU1"] = { 0, 3 };
	mapName2Router["BFE1"] = { 1, 3 };
	mapName2Router["BFE5"] = { 2, 3 };
	mapName2Router["FBU5"] = { 3, 3 };
	mapName2Router["TPMS"] = { 0, 0 };
	mapName2Router["THRC"] = { 1, 0 };
	mapName2Router["VOD2"] = { 2, 0 };
	mapName2Router["DIRC"] = { 3, 0 };
	mapName2Router["OBDB"] = { 3, 0 };
	mapName2Router["FDF2"] = { 1, 1 };
	mapName2Router["STPH"] = { 2, 1 };
	mapName2Router["SPES"] = { 3, 1 };
	mapName2Router["OBMG"] = { 0, 2 };
	mapName2Router["NAVC"] = { 1, 2 };
	mapName2Router["FDF1"] = { 2, 2 };
	mapName2Router["STAC"] = { 3, 2 };
	mapName2Router["POSI"] = { 0, 3 };
	mapName2Router["VOD1"] = { 1, 3 };
	mapName2Router["VIBS"] = { 2, 3 };
	mapName2Router["TPRC"] = { 3, 3 };
	mapName2Router["USOS"] = { 0, 0 };
	cFlowSet.XYRouting(mapName2Router["POSI"], mapName2Router["NAVC"], vectorRoutes[1]);
	cFlowSet.XYRouting(mapName2Router["NAVC"], mapName2Router["OBDB"], vectorRoutes[2]);
	cFlowSet.XYRouting(mapName2Router["OBDB"], mapName2Router["NAVC"], vectorRoutes[3]);
	cFlowSet.XYRouting(mapName2Router["OBDB"], mapName2Router["OBMG"], vectorRoutes[4]);
	cFlowSet.XYRouting(mapName2Router["NAVC"], mapName2Router["DIRC"], vectorRoutes[5]);
	cFlowSet.XYRouting(mapName2Router["SPES"], mapName2Router["NAVC"], vectorRoutes[6]);
	cFlowSet.XYRouting(mapName2Router["NAVC"], mapName2Router["THRC"], vectorRoutes[7]);
	cFlowSet.XYRouting(mapName2Router["FBU3"], mapName2Router["VOD1"], vectorRoutes[8]);
	cFlowSet.XYRouting(mapName2Router["FBU8"], mapName2Router["VOD2"], vectorRoutes[9]);
	cFlowSet.XYRouting(mapName2Router["VOD1"], mapName2Router["NAVC"], vectorRoutes[10]);
	cFlowSet.XYRouting(mapName2Router["VOD2"], mapName2Router["NAVC"], vectorRoutes[11]);
	cFlowSet.XYRouting(mapName2Router["FBU1"], mapName2Router["BFE1"], vectorRoutes[12]);
	cFlowSet.XYRouting(mapName2Router["FBU2"], mapName2Router["BFE2"], vectorRoutes[13]);
	cFlowSet.XYRouting(mapName2Router["FBU3"], mapName2Router["BFE3"], vectorRoutes[14]);
	cFlowSet.XYRouting(mapName2Router["FBU4"], mapName2Router["BFE4"], vectorRoutes[15]);
	cFlowSet.XYRouting(mapName2Router["FBU5"], mapName2Router["BFE5"], vectorRoutes[16]);
	cFlowSet.XYRouting(mapName2Router["FBU6"], mapName2Router["BFE6"], vectorRoutes[17]);
	cFlowSet.XYRouting(mapName2Router["FBU7"], mapName2Router["BFE7"], vectorRoutes[18]);
	cFlowSet.XYRouting(mapName2Router["FBU8"], mapName2Router["BFE8"], vectorRoutes[19]);
	cFlowSet.XYRouting(mapName2Router["BFE1"], mapName2Router["FDF1"], vectorRoutes[20]);
	cFlowSet.XYRouting(mapName2Router["BFE2"], mapName2Router["FDF1"], vectorRoutes[21]);
	cFlowSet.XYRouting(mapName2Router["BFE3"], mapName2Router["FDF1"], vectorRoutes[22]);
	cFlowSet.XYRouting(mapName2Router["BFE4"], mapName2Router["FDF1"], vectorRoutes[23]);
	cFlowSet.XYRouting(mapName2Router["BFE5"], mapName2Router["FDF2"], vectorRoutes[24]);
	cFlowSet.XYRouting(mapName2Router["BFE6"], mapName2Router["FDF2"], vectorRoutes[25]);
	cFlowSet.XYRouting(mapName2Router["BFE7"], mapName2Router["FDF2"], vectorRoutes[26]);
	cFlowSet.XYRouting(mapName2Router["BFE8"], mapName2Router["FDF2"], vectorRoutes[27]);
	cFlowSet.XYRouting(mapName2Router["FDF1"], mapName2Router["STPH"], vectorRoutes[28]);
	cFlowSet.XYRouting(mapName2Router["FDF2"], mapName2Router["STPH"], vectorRoutes[29]);
	cFlowSet.XYRouting(mapName2Router["STPH"], mapName2Router["OBMG"], vectorRoutes[30]);
	cFlowSet.XYRouting(mapName2Router["POSI"], mapName2Router["OBMG"], vectorRoutes[31]);
	cFlowSet.XYRouting(mapName2Router["USOS"], mapName2Router["OBMG"], vectorRoutes[32]);
	cFlowSet.XYRouting(mapName2Router["OBMG"], mapName2Router["OBDB"], vectorRoutes[33]);
	cFlowSet.XYRouting(mapName2Router["TPMS"], mapName2Router["STAC"], vectorRoutes[34]);
	cFlowSet.XYRouting(mapName2Router["VIBS"], mapName2Router["STAC"], vectorRoutes[35]);
	cFlowSet.XYRouting(mapName2Router["STAC"], mapName2Router["TPRC"], vectorRoutes[36]);
	cFlowSet.XYRouting(mapName2Router["SPES"], mapName2Router["STAC"], vectorRoutes[37]);
	cFlowSet.XYRouting(mapName2Router["STAC"], mapName2Router["THRC"], vectorRoutes[38]);

	vector<double> vectorWCETScore;
	vectorWCETScore.push_back(0);
	double dSumScore = 0;
	for (int i = 1; i <= 38; i++)
	{
		vectorWCETScore.push_back(vectorRoutes[i].size() * vectorSizes[i]);
		dSumScore += vectorWCETScore[i];
	}

	vector<double> vectorWCET;
	vectorWCET.push_back(0);
	for (int i = 1; i <= 38; i++)
	{
		double dThisUtil = dTotalUtil * (vectorWCETScore[i] / dSumScore);
		double dWCET = vectorPeriod[i] * dThisUtil;
		if (vectorCriticalityLevel[i])
		{
			dWCET /= highCritFactor;
		}
		dWCET = round(dWCET);
		vectorWCET.push_back(dWCET);

	}
	rcFlowSet = FlowsSet_2DNetwork();
	for (int i = 1; i <= 38; i++)
	{
		rcFlowSet.AddTask(vectorPeriod[i], vectorPeriod[i], vectorWCET[i] / vectorPeriod[i], vectorCriticalityLevel[i] ? highCritFactor : 1.0, vectorCriticalityLevel[i]);
	}
	rcFlowSet.ConstructTaskArray();
	rcFlowSet.GeneratePlaceHolder(4, 4);
	for (int i = 1; i <= 38; i++)
	{
		rcFlowSet.setDirectJitter(i - 1, 0);
		rcFlowSet.setHIPeriod(i - 1, vectorPeriod[i]);
		rcFlowSet.setSource(i - 1, vectorRoutes[i].front());
		rcFlowSet.setDestination(i - 1, vectorRoutes[i].back());
	}
	rcFlowSet.DeriveFlowRouteInfo();
	rcFlowSet.WriteImageFile("MCWormole2014CaseStudy.tskst");
	
}

void Construct14MCWormholeCaseStudy_v2(double dTotalUtil, FlowsSet_2DNetwork & rcFlowSet)
{
	double color = 8;
	double resW = 320;
	double resH = 240;
	double flit = 128;
	double highCritFactor = 10;
	vector<double> vectorSizes;
	vector<int> vectorRouteLength;
	vector<int> vectorCriticalityLevel;
	vector<double> vectorPeriod;
	vector<double> vectorCritifactor;
	for (int i = 0; i < 39; i++)
	{
		vectorSizes.push_back(0);
		vectorRouteLength.push_back(0);
		vectorCriticalityLevel.push_back(0);
		vectorPeriod.push_back(0);
		vectorCritifactor.push_back(1.0);
	}

	vectorCriticalityLevel[23] = 1;
	vectorCriticalityLevel[24] = 1;
	vectorCriticalityLevel[31] = 1;
	vectorCriticalityLevel[34] = 1;
	vectorCriticalityLevel[36] = 1;
	vectorCriticalityLevel[37] = 1;

	vectorCritifactor[23] = 1;
	vectorCritifactor[24] = 10;
	vectorCritifactor[31] = 10;
	vectorCritifactor[34] = 1;
	vectorCritifactor[36] = 1;
	vectorCritifactor[37] = 10;

	for (int i = 1; i <= 23; i++) vectorPeriod[i] = 0.08;
	for (int i = 24; i <= 30; i++)	vectorPeriod[i] = 0.2;
	for (int i = 31; i <= 33; i++)	vectorPeriod[i] = 1.0;
	for (int i = 34; i <= 36; i++)	vectorPeriod[i] = 2.0;
	for (int i = 37; i <= 38; i++)	vectorPeriod[i] = 4.0;

	for (int i = 1; i <= 38; i++)
	{
		vectorPeriod[i] *= 1e6;
	}
	vectorSizes[1] = (resW / 2) * (resH / 2) * color / flit * 20;
	vectorSizes[2] = ((resW / 2) * (resH / 2) * color) / flit;
	vectorSizes[3] = 16384 / flit;
	vectorSizes[4] = 16384 / flit * 10;
	vectorSizes[5] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[6] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[7] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[8] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[9] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[10] = (resW / 2) * (resH / 2) * color / flit * 10;
	vectorSizes[11] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[12] = (resW / 2) * (resH / 2) * color / flit;
	vectorSizes[13] = (((resW / 2) * (resH / 2) * color) / 20) / flit * 100;
	vectorSizes[14] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[15] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[16] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[17] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[18] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[19] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[20] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[21] = (((resW / 2) * (resH / 2) * color) / 5) / flit * 100;
	vectorSizes[22] = (((resW / 2) * (resH / 2) * color) / 5) / flit;
	vectorSizes[23] = 131072 / flit;
	vectorSizes[24] = highCritFactor * 16384 / flit;
	vectorSizes[25] = 16384 / flit;
	vectorSizes[26] = 32768 / flit;
	vectorSizes[27] = 32768 / flit * 100;
	vectorSizes[28] = 16384 / flit;
	vectorSizes[29] = 32768 / flit;
	vectorSizes[30] = 16384 / flit;
	vectorSizes[31] = highCritFactor * 16384 / flit;
	vectorSizes[32] = 16384 / flit;
	vectorSizes[33] = (((resW / 2) * (resH / 2) * color) / 20) / flit;
	vectorSizes[34] = 131072 / flit;
	vectorSizes[35] = 32768 / flit;
	vectorSizes[36] = 524288 / flit;
	vectorSizes[37] = highCritFactor * 32768 / flit;
	vectorSizes[38] = 524288 / flit;

	FlowsSet_2DNetwork cFlowSet;
	vector<FlowsSet_2DNetwork::Route> vectorRoutes;
	for (int i = 0; i < 39; i++)	vectorRoutes.push_back(FlowsSet_2DNetwork::Route());
	map<string, FlowsSet_2DNetwork::Coordinate, StringCompObj> mapName2Router;
	mapName2Router["FBU4"] = { 0, 0 };
	mapName2Router["BFE4"] = { 1, 0 };
	mapName2Router["BFE8"] = { 2, 0 };
	mapName2Router["FBU8"] = { 3, 0 };
	mapName2Router["FBU3"] = { 1, 1 };
	mapName2Router["BFE3"] = { 0, 1 };
	mapName2Router["BFE7"] = { 2, 1 };
	mapName2Router["FBU7"] = { 3, 1 };
	mapName2Router["FBU2"] = { 0, 2 };
	mapName2Router["BFE2"] = { 1, 2 };
	mapName2Router["BFE6"] = { 3, 2 };
	mapName2Router["FBU6"] = { 2, 2 };
	mapName2Router["FBU1"] = { 0, 3 };
	mapName2Router["BFE1"] = { 1, 3 };
	mapName2Router["BFE5"] = { 2, 3 };
	mapName2Router["FBU5"] = { 3, 3 };
	mapName2Router["TPMS"] = { 0, 0 };
	mapName2Router["THRC"] = { 1, 0 };
	mapName2Router["VOD2"] = { 2, 0 };
	mapName2Router["DIRC"] = { 3, 0 };
	mapName2Router["OBDB"] = { 3, 0 };
	mapName2Router["FDF2"] = { 1, 1 };
	mapName2Router["STPH"] = { 2, 1 };
	mapName2Router["SPES"] = { 3, 1 };
	mapName2Router["OBMG"] = { 0, 2 };
	mapName2Router["NAVC"] = { 1, 2 };
	mapName2Router["FDF1"] = { 2, 2 };
	mapName2Router["STAC"] = { 3, 2 };
	mapName2Router["POSI"] = { 0, 3 };
	mapName2Router["VOD1"] = { 1, 3 };
	mapName2Router["VIBS"] = { 2, 3 };
	mapName2Router["TPRC"] = { 3, 3 };
	mapName2Router["USOS"] = { 0, 0 };	

	vector<std::pair<string, string> > vecSrcDst = {
		{ "", "" },
		{ "FBU3", "BFE3" },
		{ "FBU8", "BFE8" },
		{ "VOD1", "NAVC" },
		{ "VOD2", "NAVC" },
		{ "FBU1", "BFE1" },
		{ "FBU2", "BFE2" },
		{ "FBU3", "VOD1" },
		{ "FBU4", "BFE4" },
		{ "FBU5", "BFE5" },
		{ "FBU6", "BFE6" },
		{ "FBU7", "BFE7" },
		{ "FBU8", "VOD2" },
		{ "BFE1", "FDF2" },
		{ "BFE2", "FDF1" },
		{ "BFE3", "FDF1" },
		{ "BFE4", "FDF1" },
		{ "BFE5", "FDF2" },
		{ "BFE6", "FDF2" },
		{ "BFE7", "FDF2" },
		{ "BFE8", "FDF2" },
		{ "FDF1", "STPH" },
		{ "FDF2", "STPH" },
		{ "STPH", "OBMG" },
		{ "NAVC", "DIRC" },
		{ "SPES", "NAVC" },
		{ "NAVC", "THRC" },
		{ "USOS", "OBMG" },
		{ "VIBS", "STAC" },
		{ "SPES", "STAC" },
		{ "STAC", "THRC" },
		{ "POSI", "NAVC" },
		{ "POSI", "OBMG" },
		{ "TPMS", "STAC" },
		{ "OBMG", "OBDB" },
		{ "STAC", "TPRC" },
		{ "OBDB", "OBMG" },
		{ "NAVC", "OBDB" },
		{ "OBDB", "NAVC" },
	};

	for (int i = 1; i <= 38; i++)
	{
		int iValue = 0;			
		assert(mapName2Router.count(vecSrcDst[i].first));
		assert(mapName2Router.count(vecSrcDst[i].second));
		cFlowSet.XYRouting(mapName2Router[vecSrcDst[i].first], mapName2Router[vecSrcDst[i].second], vectorRoutes[i]);
	}

	vector<double> vectorWCETScore;
	vectorWCETScore.push_back(0);
	double dSumScore = 0;
	for (int i = 1; i <= 38; i++)
	{
		vectorWCETScore.push_back(vectorRoutes[i].size() * vectorSizes[i]);
		dSumScore += vectorWCETScore[i];
	}

	vector<double> vectorWCET;
	vectorWCET.push_back(0);
	for (int i = 1; i <= 38; i++)
	{
		double dThisUtil = dTotalUtil * (vectorWCETScore[i] / dSumScore);
		double dWCET = vectorPeriod[i] * dThisUtil;
		if (vectorCriticalityLevel[i])
		{
			dWCET /= vectorCritifactor[i];
		}
		dWCET = round(dWCET);
		vectorWCET.push_back(dWCET);

	}
	rcFlowSet = FlowsSet_2DNetwork();
	for (int i = 1; i <= 38; i++)
	{
		rcFlowSet.AddTask(vectorPeriod[i], vectorPeriod[i], vectorWCET[i] / vectorPeriod[i], vectorCritifactor[i], vectorCriticalityLevel[i]);
	}
	rcFlowSet.ConstructTaskArray();
	rcFlowSet.GeneratePlaceHolder(4, 4);
	for (int i = 1; i <= 38; i++)
	{
		rcFlowSet.setDirectJitter(i - 1, 0);
		rcFlowSet.setHIPeriod(i - 1, vectorPeriod[i]);
		rcFlowSet.setSource(i - 1, vectorRoutes[i].front());
		rcFlowSet.setDestination(i - 1, vectorRoutes[i].back());
	}
	rcFlowSet.DeriveFlowRouteInfo();	
	//rcFlowSet.WriteImageFile("MCWormole2014CaseStudy.tskst");

}

void MCWormholeCaseStudy2014()
{
	FlowsSet_2DNetwork cFlowSet;
	double dUtil = 1.65;
	Construct14MCWormholeCaseStudy_v2(dUtil, cFlowSet);
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	typedef NoCFlowDLUFocusRefinement::SchedConds SchedConds;
	typedef NoCFlowDLUFocusRefinement::SchedCondType SchedCondType;
	cFlowSet.WriteImageFile("MCWormole2014CaseStudy.tskst");
	cFlowSet.WriteTextFile("MCWormole2014CaseStudy.tskst.txt");
	cout << "Util: " << dUtil << endl;
	if (0)
	{
		cout << "FR:" << endl;
		NoCFlowDLUFocusRefinement cFR(cFlowSet);
		
		cFR.setCorePerIter(5);				
		cFR.setSubILPDisplay(0);		
		cFR.setObjType(cFR.MaxMinLaxity);
		//cFR.setObjType(cFR.MinMaxLaxity);
		cFR.setLogFile("MCWormholeCaseStudy2014.txt");		
		int iStatus = cFR.FocusRefinement(1);
		if (iStatus == 1)
		{
			//Verify using ILP
			NoCFlowRTMILPModel cMILP(cFlowSet);
			TaskSetPriorityStruct cPA = cFR.getSolPA();
			cMILP.setPredefinePA(cPA);
			cMILP.Solve(2);	

			NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cPA, cFlowSet.getDirectJitterTable());
			cRTCalc.ComputeAllResponseTime();
			cRTCalc.PrintSchedulabilityInfo();
			double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cRTCalc.getRTHI(0) : cRTCalc.getRTLO(0));
			for (int i = 0; i < iTaskNum; i++)
			{
				dMinLaxity = min(dMinLaxity,
					pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)));
			}
			cout << "Min Laxity: " << dMinLaxity << endl;
		}
	}

	if (1)
	{
		cout << "MILP:" << endl;		
		NoCFlowRTMILPModel cFullMILP(cFlowSet);
		cFullMILP.setObjType(NoCFlowRTMILPModel::MaxMinLaxity);
		int iFullMILPStatus = cFullMILP.Solve(2, 24 * 3600);
		cout << iFullMILPStatus << endl;
		if (iFullMILPStatus == 1)
		{
			TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
			rcMILPResultPA.WriteTextFile("MCNoCMILPPA.txt");
			NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
			cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
			assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());			
			double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cDerivedJitterRTCalc_FullMILP.getRTHI(0) : cDerivedJitterRTCalc_FullMILP.getRTLO(0));
			for (int i = 0; i < iTaskNum; i++)
			{
				dMinLaxity = min(dMinLaxity,
					pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cDerivedJitterRTCalc_FullMILP.getRTHI(i) : cDerivedJitterRTCalc_FullMILP.getRTLO(i)));
			}
			cout << "Min Laxity: " << dMinLaxity << endl;
		}
	}

	if (1)
	{
		cout << "Heuristic: " << endl;
		NoCPriorityAssignment_2008Burns_MaxMinLaxity cHeu(cFlowSet);
		TaskSetPriorityStruct cPA(cFlowSet);
		cHeu.setTimeout(24 * 3600);
		int iStatus = cHeu.Run(cPA);
		cout << iStatus << endl;
		if (iStatus)
		{
			NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cPA, cFlowSet.getDirectJitterTable());
			cRTCalc.ComputeAllResponseTime();
			cRTCalc.PrintSchedulabilityInfo();
			double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cRTCalc.getRTHI(0) : cRTCalc.getRTLO(0));
			for (int i = 0; i < iTaskNum; i++)
			{
				cout << (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)) << " " << pcTaskArray[i].getDeadline() << " "
					<< pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)) << endl;;
				dMinLaxity = min(dMinLaxity,
					pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)));
			}
			cout << "Min Laxity: " << dMinLaxity << endl;
		}
	}
}

void MCWormholeCaseStudy2014_HeuBnB_MultiThread_Routine(int iThreadIndex, double dUtil)
{
	FlowsSet_2DNetwork cFlowSet;
	//cout << "Utilization: " << dUtil << endl;
	char axBuffer[512] = { 0 };
	sprintf_s(axBuffer, "MCWormholeCaseStudy2014_HeuBnB_MultiThread_Routine_%.2f_Output.txt", dUtil);
	string stringLogFileName(axBuffer);
	Construct14MCWormholeCaseStudy_v2(dUtil, cFlowSet);
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);	
	
	if (1)
	{
		//cout << "Heuristic: " << endl;
		NoCPriorityAssignment_2008Burns_MaxMinLaxity cHeu(cFlowSet);
		cHeu.setLogFile(stringLogFileName);
		TaskSetPriorityStruct cPA(cFlowSet);
		cHeu.setTimeout(24 * 3600);
		int iStatus = cHeu.Run(cPA);		
		if (iStatus)
		{
			NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cPA, cFlowSet.getDirectJitterTable());
			cRTCalc.ComputeAllResponseTime();
			cRTCalc.PrintSchedulabilityInfo();
			double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cRTCalc.getRTHI(0) : cRTCalc.getRTLO(0));
			for (int i = 0; i < iTaskNum; i++)
			{
				cout << (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)) << " " << pcTaskArray[i].getDeadline() << " "
					<< pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)) << endl;;
				dMinLaxity = min(dMinLaxity,
					pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)));
			}
			{
				ofstream ofstreamLogFile(stringLogFileName.data(), ios::app);
				ofstreamLogFile << "Min Laxity: " << dMinLaxity << endl;
				ofstreamLogFile.flush();
				ofstreamLogFile.close();
			}			
		}
	}
}

void MCWormholeCaseStudy2014_HeuBnB_MultiThread()
{
	//double adUtil[] = { 1.5, 1.55, 1.6, 1.65 };
	double adUtil[] = { 1.45};
	const int iNumUtil = sizeof(adUtil) / sizeof(double);
	vector<thread> vectorThreads;
	for (int i = 0; i < iNumUtil; i++)
	{
		vectorThreads.push_back(thread(MCWormholeCaseStudy2014_HeuBnB_MultiThread_Routine, i, adUtil[i]));
	}

	for (int i = 0; i < iNumUtil; i++)
	{
		vectorThreads[i].join();
	}
}

void TestMCNoCMILPResult()
{
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\MCWormole2014CaseStudy.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	TaskSetPriorityStruct cPA(cFlowSet);
	cPA.ReadTextFile("F:\\C++Project\\MCMILPPact\\MCMILPPact\\MCMILPPact\\ExpSpace\\MCNoCMILPPA.txt");
	NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cPA, cFlowSet.getDirectJitterTable());
	cRTCalc.ComputeAllResponseTime();
	cRTCalc.PrintSchedulabilityInfo();
	double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cRTCalc.getRTHI(0) : cRTCalc.getRTLO(0));
	for (int i = 0; i < iTaskNum; i++)
	{
		dMinLaxity = min(dMinLaxity,
			pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)));
	}
	cout << "Min Laxity: " << dMinLaxity << endl;

	NoCFlowRTMILPModel cFullMILPModel(cFlowSet);
	cFullMILPModel.setPredefinePA(cPA);
	cFullMILPModel.setObjType(NoCFlowRTMILPModel::MaxMinLaxity);
	cFullMILPModel.Solve(2);
}
//#include <Windows.h>
namespace NoCFlowCaseStudy_ArtifactEvaluationWrapUp
{
	void NoCRealCaseStudy_EnhancedBnB_threadsroutine(NoCPriorityAssignment_2008Burns_MaxMinLaxity * pcSearcher)
	{
		pcSearcher->setTimeout(24 * 3600);
		TaskSetPriorityStruct cPASol(*pcSearcher->getFlowSetPtr());
		int iStatus = pcSearcher->Run(cPASol);		
	}

	void NoCAutonomousVehicleCaseStudy_EnhancedBnB(int argc, char * argv[])
	{		
		char axBuffer[512] = { 0 };
		double adUtilLHHISum[] = { 1.45, 1.50, 1.55, 1.60, 1.65 };
		const int iUtilNum = sizeof(adUtilLHHISum) / sizeof(double);
		double adCurrentSolution[iUtilNum];
		std::thread acTestThreads[iUtilNum];
		FlowsSet_2DNetwork acFlowSets[iUtilNum];
		NoCPriorityAssignment_2008Burns_MaxMinLaxity  * cSearcher[iUtilNum];
		TaskSetPriorityStruct cSolPA[iUtilNum];

		for (int i = 0; i < iUtilNum; i++)
		{
			double dThisUtil = adUtilLHHISum[i];
			adCurrentSolution[i] = -1;						
			Construct14MCWormholeCaseStudy_v2(dThisUtil, acFlowSets[i]);
			cSearcher[i] = new NoCPriorityAssignment_2008Burns_MaxMinLaxity(acFlowSets[i]);
			acTestThreads[i] = thread(NoCRealCaseStudy_EnhancedBnB_threadsroutine, cSearcher[i]);
		}

		bool bFlag = true;	
		time_t iStart = time(NULL);		
		while (bFlag)
		{
			bFlag = false;
			ofstream ofstreamOutputFile("MCNoCAutonomousCaseStudy_EnhancedBnB_Result.txt");
			for (int i = 0; i < iUtilNum; i++)
			{
				if (acTestThreads[i].joinable())
				{
					bFlag = true;
				}
				volatile double dSol = 0;
				TaskSetPriorityStruct cPASol(acFlowSets[i]);
				cSearcher[i]->QueryCurrentBestSolution(dSol, cPASol);
				if (dSol > 0)
					sprintf_s(axBuffer, "Util %.4f : %lf\n", acFlowSets[i].getActualTotalUtil(), dSol);	
				else
					sprintf_s(axBuffer, "Util %.4f : N/A\n", acFlowSets[i].getActualTotalUtil());
				ofstreamOutputFile << axBuffer << endl;
			}
			sprintf_s(axBuffer, "Time Elapsed : %ds\n", (time(NULL) - iStart) );	
			ofstreamOutputFile << axBuffer << endl;
			ofstreamOutputFile.close();
			Sleep(300);
		}
	}

	void NoCAutonomousVehicleCaseStudy_MUTERGuided(int argc, char * argv[])
	{
		double adUtilLHHISum[] = { 1.45, 1.50, 1.55, 1.60, 1.65 };
		const int iUtilNum = sizeof(adUtilLHHISum) / sizeof(double);
		for (int i = 0; i < iUtilNum; i++)
		{
			FlowsSet_2DNetwork cFlowSet;
			double dUtil = adUtilLHHISum[i];
			Construct14MCWormholeCaseStudy_v2(dUtil, cFlowSet);
			GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
			if (1)
			{
				cout << "---------------------------------------------------" << endl;
				cout << "U(LO): " << cFlowSet.getActualTotalUtil() << endl;
				NoCFlowDLUFocusRefinement cFR(cFlowSet);

				cFR.setCorePerIter(5);
				cFR.setSubILPDisplay(0);
				cFR.setObjType(cFR.MaxMinLaxity);											
				int iStatus = cFR.FocusRefinement(1, 24*3600);
				if (iStatus == 1)
				{
					//Verify using ILP
					TaskSetPriorityStruct cPA = cFR.getSolPA();
					if (0)
					{
						NoCFlowRTMILPModel cMILP(cFlowSet);
						
						cMILP.setPredefinePA(cPA);
						cMILP.Solve(2);
					}


					NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cPA, cFlowSet.getDirectJitterTable());
					cRTCalc.ComputeAllResponseTime();					
					double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cRTCalc.getRTHI(0) : cRTCalc.getRTLO(0));
					for (int i = 0; i < iTaskNum; i++)
					{
						dMinLaxity = min(dMinLaxity,
							pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)));
					}
					cout << "Objective: " << dMinLaxity << endl;
				}
				else
				{
					cout << "Infeasible" << endl;
				}
			}
		}
		
	}

	void NoCAutonomousVehicleCaseStudy_MUTERGuided_PerUtilization(int argc, char * argv[])
	{
		//Usage: MUTER.exe NoCAutonomousVehicleCaseStudy_MUTERGuided [1-5]
		double adUtilLHHISum[] = { 1.45, 1.50, 1.55, 1.60, 1.65 };
		const int iUtilNum = sizeof(adUtilLHHISum) / sizeof(double);
		if (argc != 3)
		{
			cout << "Illegal Number of Argumnents" << endl;
			return;
		}

		int iOpt = atoi(argv[2]);
		if (iOpt < 1 || iOpt > 5)
		{
			cout << "please choose between 1 to 5" << endl;
			return;
		}

		double dUtil = adUtilLHHISum[iOpt - 1];
		FlowsSet_2DNetwork cFlowSet;
		Construct14MCWormholeCaseStudy_v2(dUtil, cFlowSet);
		GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
		if (1)
		{
			cout << "---------------------------------------------------" << endl;
			cout << "U(LO): " << cFlowSet.getActualTotalUtil() << endl;
			NoCFlowDLUFocusRefinement cFR(cFlowSet);

			cFR.setCorePerIter(5);
			cFR.setSubILPDisplay(0);
			cFR.setObjType(cFR.MaxMinLaxity);			
			int iStatus = cFR.FocusRefinement(1, 24 * 3600);
			if (iStatus == 1)
			{				
				TaskSetPriorityStruct cPA = cFR.getSolPA();
				NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(cFlowSet, cPA, cFlowSet.getDirectJitterTable());
				cRTCalc.ComputeAllResponseTime();
				double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cRTCalc.getRTHI(0) : cRTCalc.getRTLO(0));
				for (int i = 0; i < iTaskNum; i++)
				{
					dMinLaxity = min(dMinLaxity,
						pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)));
				}
				cout << "Objective: " << dMinLaxity << endl;
			}
			else if (iStatus == 0)
			{
				cout << "Status: Infeasible" << endl;
			}
			else if (iStatus == -1)
			{
				cout << "Status: Timeout" << endl;
			}
			else
			{
				cout << "Status: Timeout" << endl;
			}
		}

	}

	void NoCAutonomousVehicleCaseStudy_MILP_PerUtilization(int argc, char * argv[])
	{
		//Usage: MUTER.exe NoCAutonomousVehicleCaseStudy_MILPGuided [1-5]
		double adUtilLHHISum[] = { 1.45, 1.50, 1.55, 1.60, 1.65 };
		const int iUtilNum = sizeof(adUtilLHHISum) / sizeof(double);
		if (argc != 3)
		{
			cout << "Illegal Number of Argumnents" << endl;
			return;
		}

		int iOpt = atoi(argv[2]);
		if (iOpt < 1 || iOpt > 5)
		{
			cout << "please choose between 1 to 5" << endl;
			return;
		}

		double dUtil = adUtilLHHISum[iOpt - 1];
		FlowsSet_2DNetwork cFlowSet;		
		Construct14MCWormholeCaseStudy_v2(dUtil, cFlowSet);
		GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
		if (1)
		{
			cout << "---------------------------------------------------" << endl;
			cout << "U(LO): " << cFlowSet.getActualTotalUtil() << endl;
			NoCFlowRTMILPModel cFullMILP(cFlowSet);
			cFullMILP.setObjType(NoCFlowRTMILPModel::MaxMinLaxity);
			cout << "******************CPLEX Solver Output******************" << endl;
			int iFullMILPStatus = cFullMILP.Solve(3, 24*3600);
			cout << "******************CPLEX Solver Complete******************" << endl;
			if (iFullMILPStatus == 1 || iFullMILPStatus == -3)
			{
				TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
				//rcMILPResultPA.WriteTextFile("MCNoCMILPPA.txt");
				NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
				cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
				assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
				double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cDerivedJitterRTCalc_FullMILP.getRTHI(0) : cDerivedJitterRTCalc_FullMILP.getRTLO(0));
				for (int i = 0; i < iTaskNum; i++)
				{
					dMinLaxity = min(dMinLaxity,
						pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cDerivedJitterRTCalc_FullMILP.getRTHI(i) : cDerivedJitterRTCalc_FullMILP.getRTLO(i)));
				}
				cout << "Result: " << endl;
				cout << "Objective: " << dMinLaxity << endl;
				if (iFullMILPStatus == 1)
				{
					cout << "Status: Terminate" << endl;
				}
				else
				{
					cout << "Status: Timeout" << endl;
				}

			}
			else if (iFullMILPStatus == -1)
			{
				cout << "Not able to find any solution" << endl;
				cout << "Status: Timeout" << endl;
			}
			else if (iFullMILPStatus == 0)
			{
				cout << "Result: " << endl;
				cout << "Infeasible" << endl;
				cout << "Status: Terminate" << endl;
			}
		}

	}

	void NoCAutonomousVehicleCaseStudy_MILP(int argc, char * argv[])
	{
		double adUtilLHHISum[] = { 1.45, 1.50, 1.55, 1.60, 1.65 };
		const int iUtilNum = sizeof(adUtilLHHISum) / sizeof(double);
		for (int i = 0; i < iUtilNum; i++)
		{
			FlowsSet_2DNetwork cFlowSet;
			double dUtil = adUtilLHHISum[i];
			Construct14MCWormholeCaseStudy_v2(dUtil, cFlowSet);
			GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
			if (1)
			{
				cout << "---------------------------------------------------" << endl;
				cout << "U(LO): " << cFlowSet.getActualTotalUtil() << endl;
				NoCFlowRTMILPModel cFullMILP(cFlowSet);
				cFullMILP.setObjType(NoCFlowRTMILPModel::MaxMinLaxity);
				cout << "******************CPLEX Solver Output******************" << endl;
				int iFullMILPStatus = cFullMILP.Solve(3, 24 * 3600);	
				cout << "******************CPLEX Solver Complete******************" << endl;
				if (iFullMILPStatus == 1 || iFullMILPStatus == -3)
				{
					TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
					//rcMILPResultPA.WriteTextFile("MCNoCMILPPA.txt");
					NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
					cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
					assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
					double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cDerivedJitterRTCalc_FullMILP.getRTHI(0) : cDerivedJitterRTCalc_FullMILP.getRTLO(0));
					for (int i = 0; i < iTaskNum; i++)
					{
						dMinLaxity = min(dMinLaxity,
							pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cDerivedJitterRTCalc_FullMILP.getRTHI(i) : cDerivedJitterRTCalc_FullMILP.getRTLO(i)));
					}
					if (iFullMILPStatus == 1)
					{
						cout << "Status: Terminate" << endl;
					}
					else
					{
						cout << "Status: Timeout" << endl;
					}
					cout << "Min Laxity: " << dMinLaxity << endl;
				}				
				else if (iFullMILPStatus == -1)
				{
					cout << "Not able to find any solution" << endl;
					cout << "Status: Timeout" << endl;					
				}
				else if (iFullMILPStatus == 0)
				{					
					cout << "Infeasible" << endl;
					cout << "Status: Terminate" << endl;
				}
			}
		}
	}

	double dUtilStart = 0.70;
	double dUtilEnd = 1.50;
	
	void NoCSyntheticUSweep_MUTERGuided_MILP()
	{
		int iXDimension = 4;
		int iYDimension = 4;
		double dTimeout = 600;
		int iNumFlows = 40;
		char axBuffer[512] = { 0 };
		double dHIRatio = 0.50;		
		sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_N%d_HR%.2f_USweep", iXDimension, iYDimension, iNumFlows, dHIRatio);
		string stringMainFolderName = axBuffer;
		sprintf_s(axBuffer, "mkdir -p %s", stringMainFolderName.data());				
		deque<double> dequeUtils;				
		ofstream ofstreamTimeoutRatioLogFile("TimeoutRatio.txt");
		ofstreamTimeoutRatioLogFile << "Util\t" << "MUTER-guided" << "\t" << "MILP" << endl;
		for (double dUtil = dUtilStart; dUtil <= dUtilEnd; dUtil += 0.05)		
		{
			dequeUtils.push_back(dUtil);
		}
		StatisticSet cTotalStatistcSet;
		char axChar = 'A';
		for (deque<double>::iterator iter = dequeUtils.begin(); iter != dequeUtils.end(); iter++)
		{
			sprintf_s(axBuffer, "%s/U%.2f", stringMainFolderName.data(), *iter);
			string stringSubFolderName = axBuffer;
			MakePath(axBuffer);
			double dUtil = *iter;

			for (int i = 0; i < 1000; i++)
			{
				FlowsSet_2DNetwork cFlowSet;
				sprintf_s(axBuffer, "%s/FlowSet%d.tskst", stringSubFolderName.data(), i);
				string stringFlowSetName = axBuffer;
				StatisticSet cThisStatsitic;
				sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
				string stringThisStatisticName = axBuffer;
				
				cout << stringFlowSetName.data() << endl;
				if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
				{
					cout << "Result File Found" << endl;
					cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
					cFlowSet.ReadImageFile((char*)stringFlowSetName.data());					
				}
				else
				{
					//do the test
					cout << stringThisStatisticName.data() << endl << stringFlowSetName.data() << endl;					
					cFlowSet.GenRandomTaskSetInteger(iNumFlows, dUtil, dHIRatio, 2, PERIODOPT_LOG);
					cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
					cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
					sprintf_s(axBuffer, "%s/FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
					cFlowSet.WriteTextFile(axBuffer);
				}			

				//FR
				sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
				string stringFRResultFile(axBuffer);
				if (IsFileExist((char *)stringFRResultFile.data()) == false)
				{
					cout << "FR: " << endl;
					NoCFlowDLUFocusRefinement cFR(cFlowSet);
					cFR.setCorePerIter(5);
					//cFR.setObjType(cFR.MaxMinLaxity);
					sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
					cFR.setLogFile(axBuffer);
					int iStatus = cFR.FocusRefinement(1, dTimeout);
					sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
					StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);
					if (iStatus == 1)
					{
						TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
						NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
						cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
						assert(cDerivedJitterRTCalc_FR.IsSchedulable());
						NoCFlowRTMILPModel cVerifyFullMILP(cFlowSet);
						cVerifyFullMILP.setPredefinePA(rcFRResultPA);
						cout << "Verify Using MILP" << endl;
						cout << "Status: " << cVerifyFullMILP.Solve(1) << endl;
					}
				}


				//Full MILP
				if (cThisStatsitic.IsItemExist("Full-MILP Schedulability") == false)
				{
					cout << "Full MILP" << endl;
					NoCFlowRTMILPModel cFullMILP(cFlowSet);
					//cFullMILP.setObjType(cFullMILP.MaxMinLaxity);
					int iFullMILPStatus = 0;
					if (1)
						iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
					else
						iFullMILPStatus = 0;
					cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
					cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
					cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
					if (iFullMILPStatus == 1)
					{
						TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
						NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
						cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
						assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
					}
				}
			
				cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
				sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
				cThisStatsitic.WriteStatisticText(axBuffer);
				cout << "----------------------------------------------------------------------" << endl;
			}

			//Read back the result
			int iFRAcceptNum = 0;
			int iFRDenyNum = 0;
			int iFRTimeout = 0;

			int iMILPAcceptNum = 0;
			int iMILPDenyNum = 0;
			int iMILPTimeout = 0;

			double dFRTime = 0;
			double dMILPTime = 0;
			for (int i = 0; i < 1000; i++)
			{
				sprintf_s(axBuffer, "%s/FlowSet%d.tskst", stringSubFolderName.data(), i);
				cout << "\rReading Case " << i << "    ";
				string stringFlowSetName = axBuffer;
				StatisticSet cThisStatsitic;
				sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
				string stringThisStatisticName = axBuffer;
				cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());
			
				//FR
				StatisticSet cFRResult;
				sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
				cFRResult.ReadStatisticImage(axBuffer);
				if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
				{
					iFRAcceptNum++;
				}
				else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
				{
					iFRDenyNum++;
				}
				else
				{
					iFRTimeout++;
				}
				dFRTime += cFRResult.getItem("Total Time").getValue();

				//MILP
				if ((cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1) ||
					(cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == -3))
				{
					iMILPAcceptNum++;
				}
				else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
				{
					iMILPDenyNum++;
				}
				else
				{
					iMILPTimeout++;
				}
				dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();
				
			}
			dFRTime /= 1000;
			dMILPTime /= 1000;			

			sprintf_s(axBuffer, "%c-U%.2f FR Accept", axChar, dUtil);
			cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
			sprintf_s(axBuffer, "%c-U%.2f FR Deny", axChar, dUtil);
			cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
			sprintf_s(axBuffer, "%c-U%.2f FR Time", axChar, dUtil);
			cTotalStatistcSet.setItem(axBuffer, dFRTime);
			sprintf_s(axBuffer, "%c-U%.2f FR Timeout", axChar, dUtil);
			cTotalStatistcSet.setItem(axBuffer, iFRTimeout);

			sprintf_s(axBuffer, "%c-U%.2f MILP Accept", axChar, dUtil);
			cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
			sprintf_s(axBuffer, "%c-U%.2f MILP Deny", axChar, dUtil);
			cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
			sprintf_s(axBuffer, "%c-U%.2f MILP Time", axChar, dUtil);
			cTotalStatistcSet.setItem(axBuffer, dMILPTime);
			sprintf_s(axBuffer, "%c-U%.2f MILP Timeout", axChar, dUtil);
			cTotalStatistcSet.setItem(axBuffer, iMILPTimeout);

			ofstreamTimeoutRatioLogFile << dUtil << "\t" << (double)iFRTimeout / 1000.0 << "\t" << (double)iMILPTimeout / 1000.0 << endl;

			axChar++;
			sprintf_s(axBuffer, "%s/%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
			cTotalStatistcSet.WriteStatisticText(axBuffer);
		}
		ofstreamTimeoutRatioLogFile.close();
	}

	void SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest_thread_routine_artifacteval
		(int iThreadIndex, int * piTaskIndex, double * pdUtilIndex, mutex * pcLock, map<double, int> * pmapTimeoutCount)
	{
		int iXDimension = 4;
		int iYDimension = 4;
		double dTimeout = 600;
		int iNumFlows = 40;
		char axBuffer[512] = { 0 };
		double dHIRatio = 0.50;
		sprintf_s(axBuffer, "SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest_thread_routine_%d.txt", iThreadIndex);
		ofstream ofstreamOutput(axBuffer);		
		sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_N%d_HR%.2f_USweep", iXDimension, iYDimension, iNumFlows, dHIRatio);
		string stringMainFolderName = axBuffer;
		while (true)
		{
			//WaitForSingleObject(*pcLock, INFINITE);
			pcLock->lock();
			double dUtil = *pdUtilIndex;
			int i = (*piTaskIndex);
			if (DOUBLE_EQUAL(dUtil, dUtilEnd, 1e-5))
			{
				//ReleaseMutex(*pcLock);
				pcLock->unlock();
				break;
			}
			assert(i < 1000 && i >= 0);
			(*piTaskIndex)++;
			if ((*piTaskIndex) == 1000)
			{
				*pdUtilIndex += 0.05;
				*piTaskIndex = 0;
			}
			//ReleaseMutex(*pcLock);
			pcLock->unlock();

			sprintf_s(axBuffer, "%s/U%.2f", stringMainFolderName.data(), dUtil);
			string stringSubFolderName = axBuffer;
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "%s/FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;

			//cout << "\r" << stringFlowSetName.data() << "        ";
			//cout << stringFlowSetName.data() << endl;
			ofstreamOutput << stringFlowSetName.data() << endl;
			ofstreamOutput.flush();
			StatisticSet cTotalStatistcSet;
			if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
			{
				cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
				cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
			}
			else
			{
				cout << "Cannot find target task file or result file" << stringFlowSetName.data() << endl;
				while (1);
			}

			//Heuristic
			if (cThisStatsitic.IsItemExist("Heuristic08Burns") == false)
			{
				cout << "Heuristic 08 Burns" << endl;
				NoCPriorityAssignment_2008Burns cHeuristic(cFlowSet);
				time_t iClocks = clock();
				cHeuristic.setTimeout(600);
				TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
				int iStatus = cHeuristic.Run(cPriorityAssignment);
				iClocks = clock() - iClocks;
				if (pmapTimeoutCount->count(dUtil) == 0)
					(*pmapTimeoutCount)[dUtil] = 1;
				else
					(*pmapTimeoutCount)[dUtil]++;
				cThisStatsitic.setItem("Heuristic08Burns", iStatus);
				cThisStatsitic.setItem("Heuristic08Burns Time", cHeuristic.getTime());
				ofstreamOutput << "Finished, time: " << cHeuristic.getTime() << endl;
				ofstreamOutput.flush();
				cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
				sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
				cThisStatsitic.WriteStatisticText(axBuffer);
			}
			else
			{
				ofstreamOutput << "Result already there" << endl;
				ofstreamOutput.flush();
			}
		}
		ofstreamOutput.close();
	}

	void SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest()
	{
		//HANDLE cLock = CreateMutex(FALSE, FALSE, NULL);
		mutex cLock;
		const int iThreadNum = 6;
		vector<thread> vectorThreads;
		int iTaskIndex = 0;
		double dUtilIndex = dUtilStart;
		map<double, int> amapTimeoutCount[iThreadNum];
		for (int i = 0; i < iThreadNum; i++)
		{
			vectorThreads.push_back(
				thread(SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest_thread_routine_artifacteval
				, i, &iTaskIndex, &dUtilIndex, &cLock, &amapTimeoutCount[i]));
		}

		for (int i = 0; i < iThreadNum; i++)
		{
			vectorThreads[i].join();
		}
		ofstream ofstreamOutputTimeoutRatioFile("EnhancedBnB_TimeoutRatio_vs_Util.txt");
		ofstreamOutputTimeoutRatioFile << "Util\t" << "Timeout_Ratio" << endl;
		for (double dUtil = dUtilStart; dUtil <= dUtilEnd; dUtil += 0.05)
		{			
			int iCount = 0;
			ofstreamOutputTimeoutRatioFile << dUtil << "\t";
			for (int i = 0; i < iThreadNum; i++)
			{
				iCount += amapTimeoutCount[i][dUtil];
			}
			ofstreamOutputTimeoutRatioFile << iCount / 1000.0 << endl;
		}
		ofstreamOutputTimeoutRatioFile.close();
	}

	int iNStart = 26;
	int iNEnd = 46;

	void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity()
	{
		int iXDimension = 4;
		int iYDimension = 4;
		double dTimeout = 600;
		char axBuffer[512] = { 0 };		
		double dUtilLB = 0.8;
		double dUtilUB = 0.95;
		double dHIRatio = 0.5;
		sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_U%.2f~U%.2f_HR%.2f_NSweep_MaxMinLaxity", iXDimension, iYDimension, dUtilLB, dUtilUB, dHIRatio);
		ofstream ofstreamOutputResultFile("ObjRunTime_vs_N_MILPMUTER.txt");
		ofstreamOutputResultFile << "N \t MILP_obj \t MILP_runtime \t MUTER_obj \t MUTER_runtime" << endl;
		string stringMainFolderName = axBuffer;
		deque<int> dequeNs;
		//MakePath(axBuffer);
		sprintf_s(axBuffer, "mkdir -p %s", stringMainFolderName.data());
		system(axBuffer);
		for (int iN = iNStart; iN <= iNEnd; iN += 4)
			//for (int iN = 38; iN >= 26; iN -= 4)
			//for (int iN = 34; iN >= 34; iN -= 4)
		{
			dequeNs.push_back(iN);
		}
		StatisticSet cTotalStatistcSet;
		char axChar = 'A';
		for (deque<int>::iterator iter = dequeNs.begin(); iter != dequeNs.end(); iter++)
		{
			sprintf_s(axBuffer, "%s\\N%d", stringMainFolderName.data(), *iter);
			string stringSubFolderName = axBuffer;
			MakePath(axBuffer);
			int iN = *iter;
			int iNumTasks = iN;

			for (int i = 0; i < 1000; i++)
				//for (int i = 1000 - 1; i >= 0; i--)
			{
				double dUtil = (double)rand() / (double)RAND_MAX;
				dUtil = dUtil * (dUtilUB - dUtilLB) + dUtilLB;
				FlowsSet_2DNetwork cFlowSet;
				sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
				string stringFlowSetName = axBuffer;
				StatisticSet cThisStatsitic;
				sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
				string stringThisStatisticName = axBuffer;

				//cout << "\r" << stringFlowSetName.data() << "        ";
				cout << stringFlowSetName.data() << endl;
				bool bRedo = false;
				if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
				{
					cout << "Result File Found" << endl;
					cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
					cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
					//continue;
				}
				else
				{
					//do the test						
					cFlowSet.GenRandomTaskSetInteger(iNumTasks, dUtil, dHIRatio, 2, PERIODOPT_LOG);
					cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
					cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
					sprintf_s(axBuffer, "%s\\FlowSet%d.tskst.txt", stringSubFolderName.data(), i);
					cFlowSet.WriteTextFile(axBuffer);
					bRedo = true;
				}
				cout << "Utilization: " << dUtil << endl;




				//FR
				sprintf_s(axBuffer, "%s_FRResult.rslt.rslt", stringFlowSetName.data());
				string stringFRResultFile(axBuffer);
				if (IsFileExist((char *)stringFRResultFile.data()) == false || bRedo)
				{
					cout << "FR: " << endl;
					NoCFlowDLUFocusRefinement cFR(cFlowSet);
					cFR.setCorePerIter(5);
					cFR.setObjType(cFR.MaxMinLaxity);
					sprintf_s(axBuffer, "%s_FRLog.txt", stringFlowSetName.data());
					cFR.setLogFile(axBuffer);
					int iStatus = cFR.FocusRefinement(1, dTimeout);
					sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
					StatisticSet cFRResult = cFR.GenerateStatisticFile(axBuffer);
					if (iStatus == 1)
					{
						TaskSetPriorityStruct & rcFRResultPA = cFR.getPriorityAssignmentSol();
						NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FR(cFlowSet, rcFRResultPA, cFlowSet.getDirectJitterTable());
						cDerivedJitterRTCalc_FR.ComputeAllResponseTime();
						assert(cDerivedJitterRTCalc_FR.IsSchedulable());
						NoCFlowRTMILPModel cVerifyFullMILP(cFlowSet);
						cVerifyFullMILP.setPredefinePA(rcFRResultPA);
						cout << "Verify Using MILP" << endl;
						cout << "Status: " << cVerifyFullMILP.Solve(1) << endl;
					}
				}


				//Full MILP
				if (cThisStatsitic.IsItemExist("Full-MILP Schedulability") == false || bRedo)
				{
					cout << "Full MILP" << endl;
					NoCFlowRTMILPModel cFullMILP(cFlowSet);
					cFullMILP.setObjType(cFullMILP.MaxMinLaxity);
					int iFullMILPStatus = 0;
					if (1)
						iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
					else
						iFullMILPStatus = 0;
					cThisStatsitic.setItem("Full-MILP Schedulability", iFullMILPStatus);
					cThisStatsitic.setItem("Full-MILP Wall-Time", cFullMILP.getWallTime());
					cThisStatsitic.setItem("Full-MILP CPU-Time", cFullMILP.getCPUTime());
					cThisStatsitic.setItem("Full-MILP Objective", cFullMILP.getObjective());
					if (iFullMILPStatus == 1)
					{
						TaskSetPriorityStruct rcMILPResultPA = cFullMILP.getPrioritySol();
						NoCFlowAMCRTCalculator_DerivedJitter cDerivedJitterRTCalc_FullMILP(cFlowSet, rcMILPResultPA, cFlowSet.getDirectJitterTable());
						cDerivedJitterRTCalc_FullMILP.ComputeAllResponseTime();
						assert(cDerivedJitterRTCalc_FullMILP.IsSchedulable());
					}
				}

				cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
				sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
				cThisStatsitic.WriteStatisticText(axBuffer);
				cout << "----------------------------------------------------------------------" << endl;
			}

			//Read back the result
			int iDMAcceptNum = 0;
			int iDMDenyNum = 0;

			int iPessAcceptNum = 0;
			int iPessDenyNum = 0;

			int iFRAcceptNum = 0;
			int iFRDenyNum = 0;
			int iFRTimeout = 0;
			double dFRObjective = 0;

			int iMILPAcceptNum = 0;
			int iMILPDenyNum = 0;
			int iMILPTimeout = 0;
			double dMILPObjective = 0;

			int iHeuAcceptNum = 0;
			int iHeuDenyNum = 0;
			int iHeuTimeout = 0;
			double dHeuObjective = 0;

			double dFRTime = 0;
			double dMILPTime = 0;
			double dHeuTime = 0;
			for (int i = 0; i < 1000; i++)
			{
				sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
				cout << "\rReading Case " << i << "    ";
				string stringFlowSetName = axBuffer;
				StatisticSet cThisStatsitic;
				sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
				string stringThisStatisticName = axBuffer;
				cThisStatsitic.ReadStatisticImage((char*)stringThisStatisticName.data());

				//Derive an upperbound on the objective
				FlowsSet_2DNetwork cFlowSet;
				cFlowSet.ReadImageFile((char *)stringFlowSetName.data());
				GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
				double dObjUB = pcTaskArray[0].getDeadline() - pcTaskArray[0].getCLO();
				for (int i = 0; i < iTaskNum; i++)
				{
					if (pcTaskArray[i].getCriticality())
					{
						dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCHI());
					}
					else
					{
						dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO());
					}
				}


				//FR
				StatisticSet cFRResult;
				sprintf_s(axBuffer, "%s_FRResult.rslt.rslt", stringFlowSetName.data());
				cFRResult.ReadStatisticImage(axBuffer);
				if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
				{
					iFRAcceptNum++;
					dFRObjective += cFRResult.getItem("Objective").getValue() / dObjUB;
				}
				else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
				{
					iFRDenyNum++;
					dFRObjective += -1;
				}
				else
				{
					iFRTimeout++;
					//dFRObjective -= 1;
				}
				dFRTime += cFRResult.getItem("Total Time").getValue();


				//MILP
				if ((cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 1) ||
					(cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == -3))
				{
					iMILPAcceptNum++;
					dMILPObjective += cThisStatsitic.getItem("Full-MILP Objective").getValue() / dObjUB;
				}
				else if (cThisStatsitic.getItem("Full-MILP Schedulability").getValue() == 0)
				{
					iMILPDenyNum++;
					dMILPObjective += -1;
				}
				else
				{
					iMILPTimeout++;
					//dMILPObjective -= 1;
				}
				dMILPTime += cThisStatsitic.getItem("Full-MILP Wall-Time").getValue();
#if 0
				//Heuristic
				int iStatus = round(cThisStatsitic.getItem("Heuristic08Burns").getValue());
				double dObj = cThisStatsitic.getItem("Heuristic08Burns Objective").getValue();
				if (dObj > 0)
				{
					iHeuAcceptNum++;
					dHeuObjective += -dObj / dObjUB;
				}
				else if (iStatus == 0)
				{
					iHeuDenyNum++;
				}
				else if (dObj < 0)
				{
					if (iStatus != -1)
					{
						cout << "should give credit to Heu" << endl;
						while (1);
					}
					dHeuObjective += -1;
				}

				if (iStatus == -1)
				{
					iHeuTimeout++;
				}
				dHeuTime += cThisStatsitic.getItem("Heuristic08Burns Time").getValue();
#endif

			}
			dFRTime /= 1000;
			dMILPTime /= 1000;
			dHeuTime /= 1000;

			dFRObjective /= 1000;
			dMILPObjective /= 1000;
			dHeuObjective /= 1000;

			sprintf_s(axBuffer, "%c-%d FR Accept", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iFRAcceptNum);
			sprintf_s(axBuffer, "%c-%d FR Deny", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iFRDenyNum);
			sprintf_s(axBuffer, "%c-%d FR Time", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, dFRTime);
			sprintf_s(axBuffer, "%c-%d FR Timeout", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iFRTimeout);
			sprintf_s(axBuffer, "%c-%d FR Objective", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, dFRObjective);

			sprintf_s(axBuffer, "%c-%d MILP Accept", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iMILPAcceptNum);
			sprintf_s(axBuffer, "%c-%d MILP Deny", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iMILPDenyNum);
			sprintf_s(axBuffer, "%c-%d MILP Time", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, dMILPTime);
			sprintf_s(axBuffer, "%c-%d MILP Timeout", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iMILPTimeout);
			sprintf_s(axBuffer, "%c-%d MILP MILP Objective", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, dMILPObjective);

#if 0
			sprintf_s(axBuffer, "%c-%d Heu Accept", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iHeuAcceptNum);
			sprintf_s(axBuffer, "%c-%d Heu Deny", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iHeuDenyNum);
			sprintf_s(axBuffer, "%c-%d Heu Time", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, dHeuTime);
			sprintf_s(axBuffer, "%c-%d Heu Timeout", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, iHeuTimeout);
			sprintf_s(axBuffer, "%c-%d Heu Objective", axChar, iN);
			cTotalStatistcSet.setItem(axBuffer, dHeuObjective);
#endif

			axChar++;
			sprintf_s(axBuffer, "%s//%s_Result.txt", stringMainFolderName.data(), stringMainFolderName.data());
			cTotalStatistcSet.WriteStatisticText(axBuffer);
			
			ofstreamOutputResultFile << iN << "\t" << dMILPObjective << "\t" << dMILPTime << "\t " << dFRObjective << "\t" << dFRTime << endl;
		}
		ofstreamOutputResultFile.close();
	}

	typedef map<int, std::pair<double, double> > N2Result;
	void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_HeuristicTest_thread_routine_artifacteval
		(int iThreadIndex, int * piTaskIndex, int * piNIndex, mutex * pcLock, N2Result * pmapN2Result)
	{
		int iXDimension = 4;
		int iYDimension = 4;
		double dTimeout = 600;
		int iNumFlows = 40;
		char axBuffer[512] = { 0 };
		double dUtilLB = 0.8;
		double dUtilUB = 0.95;
		double dHIRatio = 0.50;
		sprintf_s(axBuffer, "SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_MultiThreadHeuristicTest_thread_routine_%d.txt", iThreadIndex);
		ofstream ofstreamOutput(axBuffer);		
		sprintf_s(axBuffer, "NoCFRSchedTest_Stress_%dX%d_U%.2f~U%.2f_HR%.2f_NSweep_MaxMinLaxity", iXDimension, iYDimension, dUtilLB, dUtilUB, dHIRatio);
		string stringMainFolderName = axBuffer;
		while (true)
		{
			//WaitForSingleObject(*pcLock, INFINITE);
			pcLock->lock();
			int iN = *piNIndex;
			int i = (*piTaskIndex);
			if (iN > iNEnd)
			{
				//ReleaseMutex(*pcLock);
				pcLock->unlock();
				break;
			}
			assert(i < 1000 && i >= 0);
			(*piTaskIndex)++;
			if ((*piTaskIndex) == 1000)
			{
				*piNIndex += 4;
				*piTaskIndex = 0;
			}
#if 1
			i = 1000 - i - 1;
#endif
			//ReleaseMutex(*pcLock);
			pcLock->unlock();

			sprintf_s(axBuffer, "%s\\N%d", stringMainFolderName.data(), iN);
			string stringSubFolderName = axBuffer;
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "%s\\FlowSet%d.tskst", stringSubFolderName.data(), i);
			string stringFlowSetName = axBuffer;
			StatisticSet cThisStatsitic;
			sprintf_s(axBuffer, "%s.rslt", stringFlowSetName.data());
			string stringThisStatisticName = axBuffer;

			//cout << "\r" << stringFlowSetName.data() << "        ";
			//cout << stringFlowSetName.data() << endl;
			ofstreamOutput << stringFlowSetName.data() << endl;
			ofstreamOutput.flush();
			StatisticSet cTotalStatistcSet;
			if ((IsFileExist((char *)stringThisStatisticName.data()) == true) && (IsFileExist((char *)stringFlowSetName.data()) == true))
			{
				cThisStatsitic.ReadStatisticImage((char *)stringThisStatisticName.data());
				cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
			}
			else
			{
				cout << "Cannot find target task file or result file" << stringFlowSetName.data() << endl;
				while (1);
			}
			GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
			double dObjUB = pcTaskArray[0].getDeadline() - pcTaskArray[0].getCLO();
			for (int i = 0; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCHI());
				}
				else
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO());
				}
			}

			//Heuristic
			if (cThisStatsitic.IsItemExist("Heuristic08Burns") == false)
			{
				cout << "Heuristic 08 Burns" << endl;
				NoCPriorityAssignment_2008Burns_MaxMinLaxity cHeuristic(cFlowSet);
				time_t iClocks = clock();
				cHeuristic.setTimeout(600);
				TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
				int iStatus = cHeuristic.Run(cPriorityAssignment);
				iClocks = clock() - iClocks;
				cThisStatsitic.setItem("Heuristic08Burns", iStatus);
				cThisStatsitic.setItem("Heuristic08Burns Time", cHeuristic.getTime());
				double dNormalizedObj = cHeuristic.getCurrentBest() / dObjUB;
				if (iStatus == 0)
					dNormalizedObj = -1;
				else if (iStatus == 1)
					dNormalizedObj = cHeuristic.getCurrentBest() / dObjUB;
				else if (iStatus == -1)
					dNormalizedObj = 1;
				else
				{
					assert(0);
				}
				(*pmapN2Result)[iN].first = dNormalizedObj;
				(*pmapN2Result)[iN].second = cHeuristic.getTime();
				cThisStatsitic.setItem("Heuristic08Burns Objective", cHeuristic.getCurrentBest());
				ofstreamOutput << "Finished, time: " << cHeuristic.getTime() << " Objective: " << cHeuristic.getCurrentBest() << " Normalized: " << cHeuristic.getCurrentBest() / dObjUB
					<< endl;;
				ofstreamOutput.flush();
				cThisStatsitic.WriteStatisticImage((char *)stringThisStatisticName.data());
				sprintf_s(axBuffer, "%s.txt", stringThisStatisticName.data());
				cThisStatsitic.WriteStatisticText(axBuffer);
			}
			else
			{
				ofstreamOutput << "Result already there" << endl;
				ofstreamOutput.flush();
			}
		}
		ofstreamOutput.close();
	}

	void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_HeuristicTest()
	{
		//HANDLE cLock = CreateMutex(FALSE, FALSE, NULL);
		mutex cLock;
		const int iThreadNum = 3;
		vector<thread> vectorThreads;
		int iTaskIndex = 0;
		int iNIndex = 46;
		typedef map<int, std::pair<double, double> > N2Result;
		N2Result amapResult[iThreadNum];
		for (int i = 0; i < iThreadNum; i++)
		{
			vectorThreads.push_back(
				thread(
				SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_HeuristicTest_thread_routine_artifacteval,
				i, &iTaskIndex, &iNIndex, &cLock, &amapResult[i]));
		}

		for (int i = 0; i < iThreadNum; i++)
		{
			vectorThreads[i].join();
		}

		ofstream ofstreamOutputFile("ObjRunTime_vs_NumberofTask_EnhancedBnB.txt");
		ofstreamOutputFile << "N \t Obj \t RunTime" << endl;
		for (int i = iNStart; i <= iNEnd; i += 4)
		{
			double dSumObjective = 0;
			double dRunTime = 0;
			for (int j = 0; j < iThreadNum; j++)
			{
				dSumObjective += amapResult[j][i].first;
				dRunTime += amapResult[j][i].second;
			}
			dSumObjective /= 1000.0;
			dRunTime /= 1000.0;
			ofstreamOutputFile << i << "\t" << dSumObjective << "\t" << dRunTime << endl;
		}
		ofstreamOutputFile.close();
	}

	void NoCSyntheticUSweep_PerUtilization_MethodOption_MultiThreadBnB_threadroutine(double dUtil, int *piIndex,
		int iThreadIndex, int * piTimeoutCases, int * piTotalCases, string stringResultFileName, std::mutex * pcMutex)
	{
		char axBuffer[512] = { 0 };
		string stringFolderName;
		sprintf_s(axBuffer, "USweep/U%.2f", dUtil);
		stringFolderName = axBuffer;
		while (true)
		{
			int iThisTaskIndex = 0;
			pcMutex->lock();
			iThisTaskIndex = *piIndex;
			(*piIndex)++;
			pcMutex->unlock();
			if (iThisTaskIndex >= 1000)
			{
				break;
			}

			sprintf_s(axBuffer, "%s/FlowSet%d.tskst", stringFolderName.data(), iThisTaskIndex);
			string stringFlowSetName = axBuffer;	
			FlowsSet_2DNetwork cFlowSet;
			cFlowSet.ReadImageFile((char *)stringFlowSetName.data());
			NoCPriorityAssignment_2008Burns cHeuristic(cFlowSet);
			cHeuristic.setTimeout(600);
			TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
			int iStatus = cHeuristic.Run(cPriorityAssignment);	

			pcMutex->lock();			
			cout << "Case #" << iThisTaskIndex << endl;
			cout << "Thread ID: " << iThreadIndex << endl;
			
			if (iStatus == -1)
			{
				(*piTimeoutCases)++;
				cout << "Status: Timeout" << endl;
			}
			else
			{				
				cout << "Status: Terminate" << endl;
			}
			(*piTotalCases)++;
			ofstream ofstreamOutputFile(stringResultFileName.data());
			ofstreamOutputFile << "Total Timeout Cases: " << (*piTimeoutCases) << endl;
			ofstreamOutputFile << "Total Cases: " << (*piTotalCases) << endl;
			ofstreamOutputFile << "Timeout Ratio: " << (double)(*piTimeoutCases) / (double)(*piTotalCases) << endl;
			ofstreamOutputFile.close();
			cout << "Timeout Cases: " << (*piTimeoutCases) << endl;
			cout << "Total Cases: " << (*piTotalCases) << endl;
			cout << "Timeout Ratio: " << (double)(*piTimeoutCases) / (double)(*piTotalCases) << endl;
			cout << "---------------------------------------------------" << endl;
			pcMutex->unlock();
		}
	}

	void NoCSyntheticUSweep_PerUtilization_MethodOption_MultiThreadBnB(double dUtil, int iThreadNum, string stringResultFileName)
	{
		list<std::thread> listThreads;
		std::mutex cLock;
		int iIndex = 0;
		int iTimeoutCases = 0;
		int iTotalCases = 0;
		for (int i = 0; i < iThreadNum; i++)
		{
			listThreads.push_back(thread(NoCSyntheticUSweep_PerUtilization_MethodOption_MultiThreadBnB_threadroutine, 
				dUtil, &iIndex, i, &iTimeoutCases, &iTotalCases, stringResultFileName, &cLock));
		}

		for (list<thread>::iterator iter = listThreads.begin(); iter != listThreads.end(); iter++)
		{
			iter->join();
		}
	}

	void NoCSyntheticUSweep_PerUtilization_MethodOption(int argc, char * argv[])		
	{
		//usage: ./MUTER.exe NoCSyntheticUSweep [Enhanced-BnB, MUTER-guided, MILP] [Util]
		if (argc < 4)
		{
			cout << "illegal number of argument" << endl;
			cout << "./MUTER.exe NoCSyntheticUSweep [Enhanced-BnB, MUTER-guided, MILP] [Util]" << endl;
			return;
		}

		if (strcmp(argv[2], "Enhanced-BnB") != 0 &&
			strcmp(argv[2], "MUTER-guided") != 0 &&
			strcmp(argv[2], "MILP") != 0
			)
		{
			cout << "Method must be one of [Enhanced-BnB, MUTER-guided, MILP] " << endl;
			return;
		}	
		double dTimeout = 600;		
		char axBuffer[512] = { 0 };
		double dHIRatio = 0.50;		
		double dUtil = atof(argv[3]);	
		cout << "Utilization: " << dUtil << endl;		
		sprintf_s(axBuffer, "USweep/NoCSyntheticUSweep_%.2f_%s.txt", dUtil, argv[2]);	
		string stringResultFileName(axBuffer);
		int iTimeoutCases = 0;
		int iTotalCases = 0;

		//make sure all cases are there
		cout << "Verifying/Generating synthetic systems....";		
		NoCSyntheticUSweep_PerUtilization_GenerateTestCases(dUtil);
		cout << "Complete" << endl;
		cout << "--------------------------------" << endl;
		if (strcmp("Enhanced-BnB", argv[2]) == 0)
		{
			int iThreadNum = 8;
			unsigned int uiMaxThreads = std::thread::hardware_concurrency();
			iThreadNum = uiMaxThreads;
			cout << "Using maximum number of concurrent threads: " << iThreadNum << endl;
			if (argc == 5)
			{
				iThreadNum = atoi(argv[4]);
			}
			NoCSyntheticUSweep_PerUtilization_MethodOption_MultiThreadBnB(dUtil, iThreadNum, stringResultFileName);
			return;
		}
		
		
		for (int i = 0; i < 1000; i++)
		{						
			cout << "Case #" << i << endl;						
			FlowsSet_2DNetwork cFlowSet;									
			sprintf_s(axBuffer, "USweep/U%.2f/FlowSet%d.tskst", dUtil, i);
			string stringFlowSetName = axBuffer;
			cFlowSet.ReadImageFile((char*)stringFlowSetName.data());			
			bool bIsTimeout = false;
			if (strcmp(argv[2], "MUTER-guided") == 0)
			{ 
				cout << "Method: MUTER-guided" << endl;
				sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
				string stringFRResultFile(axBuffer);
				NoCFlowDLUFocusRefinement cFR(cFlowSet);
				cFR.setCorePerIter(5);												
				int iStatus = cFR.FocusRefinement(1, dTimeout);
				StatisticSet cFRResult = cFR.GenerateStatisticFile("");
				if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
				{
					
				}
				else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
				{
					
				}
				else
				{					
					bIsTimeout = true;
				}				
			}
			else if (strcmp(argv[2], "MILP") == 0)
			{
				cout << "Method: MILP-guided" << endl;
				NoCFlowRTMILPModel cFullMILP(cFlowSet);				
				int iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
				if (iFullMILPStatus != 1 && iFullMILPStatus != -3 && iFullMILPStatus != 0)
				{
					bIsTimeout = true;
				}
			}
			else if (strcmp(argv[2], "Enhanced-BnB"))
			{
				cout << "Method: Enhanced-BnB" << endl;
				cout << "Running..." << endl;
				NoCPriorityAssignment_2008Burns cHeuristic(cFlowSet);				
				cHeuristic.setTimeout(600);
				TaskSetPriorityStruct cPriorityAssignment(cFlowSet);

				int iStatus = cHeuristic.Run(cPriorityAssignment);
				cout << endl;
				if (iStatus == -1)
				{
					bIsTimeout = true;
				}
			}
			else
			{
				assert(0);
			}
			cout << "Method Complete" << endl;
			iTotalCases++;
			if (bIsTimeout)
			{
				iTimeoutCases++;
				cout << "Status: Timeout" << endl;
			}
			else
			{
				
				cout << "Status: Terminated" << endl;
			}
			ofstream ofstreamOutputFile(stringResultFileName.data());
			ofstreamOutputFile << "Total Timeout Cases: " << iTimeoutCases << endl;
			ofstreamOutputFile << "Total Cases: " << iTotalCases << endl;
			ofstreamOutputFile << "Timeout Ratio: " <<  (double)iTimeoutCases / (double)iTotalCases << endl;

			cout << "Timeout Cases: " << iTimeoutCases << endl;
			cout << "Total Cases: " << iTotalCases << endl;
			cout << "Timeout Ratio: " << (double)iTimeoutCases / (double)iTotalCases << endl;
			cout << "--------------------------------" << endl;
			ofstreamOutputFile.close();
		}
	}

	void NoCSyntheticNSweep_PerN_MethodOption_MultiThreadBnB_threadroutine(int iN, int *piIndex,
		int iThreadIndex, double * pdTotalNormalizedObjective, double * pdTotalTime, 
		int * piTotalCases, string stringResultFileName, std::mutex * pcMutex)
	{
		char axBuffer[512] = { 0 };
		string stringFolderName;
		sprintf_s(axBuffer, "NSweep/N%d", iN);
		stringFolderName = axBuffer;
		while (true)
		{
			int iThisTaskIndex = 0;
			pcMutex->lock();
			iThisTaskIndex = *piIndex;
			(*piIndex)++;
			pcMutex->unlock();
			if (iThisTaskIndex >= 1000)
			{
				break;
			}

			sprintf_s(axBuffer, "%s/FlowSet%d.tskst", stringFolderName.data(), iThisTaskIndex);
			string stringFlowSetName = axBuffer;
			FlowsSet_2DNetwork cFlowSet;
			cFlowSet.ReadImageFile((char *)stringFlowSetName.data());
			GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
			double dObjUB = pcTaskArray[0].getDeadline() - pcTaskArray[0].getCLO();
			for (int i = 0; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCHI());
				}
				else
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO());
				}
			}
			NoCPriorityAssignment_2008Burns_MaxMinLaxity cHeuristic(cFlowSet);
			cHeuristic.setTimeout(600);
			TaskSetPriorityStruct cPriorityAssignment(cFlowSet);
			int iStatus = cHeuristic.Run(cPriorityAssignment);
			pcMutex->lock();	
			if (cHeuristic.getCurrentBest() > 0)
			{
				(*pdTotalNormalizedObjective) += cHeuristic.getCurrentBest() / dObjUB;
			}
			else if (iStatus == 0)
			{
				(*pdTotalNormalizedObjective) += 1;
			}
			else
			{
				(*pdTotalNormalizedObjective) += 0;
			}
			(*pdTotalTime) += cHeuristic.getTime() / 1000.0;

			cout << "Case #" << iThisTaskIndex << endl;
			cout << "Thread ID: " << iThreadIndex << endl;			
			(*piTotalCases)++;
			ofstream ofstreamOutputFile(stringResultFileName.data());
			ofstreamOutputFile << "Average Normalized Objective: " << (*pdTotalNormalizedObjective) / (*piTotalCases) << endl;
			ofstreamOutputFile << "Average Time: " << (*pdTotalTime) / (*piTotalCases) << " seconds" << endl;
			ofstreamOutputFile << "Total Cases: " << (*piTotalCases) << endl;
			ofstreamOutputFile.close();
			cout << "Average Normalized Objective: " << (*pdTotalNormalizedObjective) / (*piTotalCases) << endl;
			cout << "Average Time: " << (*pdTotalTime) / (*piTotalCases) << " seconds" << endl;
			cout << "Total Cases: " << (*piTotalCases) << endl;
			cout << "---------------------------------------------------" << endl;
			pcMutex->unlock();
		}
	}

	void NoCSyntheticNSweep_PerN_MethodOption_MultiThreadBnB(int iN, int iThreadNum, string stringResultFileName)
	{
		list<std::thread> listThreads;
		std::mutex cLock;
		int iIndex = 0;
		double dTotalNormalizedObjective = 0;
		double dTotalTime = 0;
		int iTotalCases = 0;
		for (int i = 0; i < iThreadNum; i++)
		{
			listThreads.push_back(thread(NoCSyntheticNSweep_PerN_MethodOption_MultiThreadBnB_threadroutine,
				iN, &iIndex, i, &dTotalNormalizedObjective, &dTotalTime, &iTotalCases, stringResultFileName, &cLock));
		}

		for (list<thread>::iterator iter = listThreads.begin(); iter != listThreads.end(); iter++)
		{
			iter->join();
		}
	}

	void NoCOptimizationNSweep_PerN_MethodOption(int argc, char * argv[])
	{
		//usage: ./MUTER.exe NoCSyntheticUSweep [Enhanced-BnB, MUTER-guided, MILP] [Util] [Number of Synthetic Cases]
		if (argc < 4)
		{
			cout << "illegal number of argument" << endl;
			cout << "./MUTER.exe NoCSyntheticUSweep [Enhanced-BnB, MUTER-guided, MILP] [N] " << endl;
			return;
		}

		if (strcmp(argv[2], "Enhanced-BnB") != 0 &&
			strcmp(argv[2], "MUTER-guided") != 0 &&
			strcmp(argv[2], "MILP") != 0
			)
		{
			cout << "Method must be one of [Enhanced-BnB, MUTER-guided, MILP] " << endl;
			return;
		}
		double dUtilLB = 0.8;
		double dUtilUB = 0.95;
		int iXDimension = 4;
		int iYDimension = 4;
		double dTimeout = 600;
		int iNumFlows = 40;
		char axBuffer[512] = { 0 };
		double dHIRatio = 0.50;
		int iN = atoi(argv[3]);		
		cout << "Number of Flows: " << iN << endl;
		string stringResultFileName;
		sprintf_s(axBuffer, "NSweep/NoCSyntheticNSweep_N%d_%s", iN, argv[2]);
		stringResultFileName = axBuffer;
		stringResultFileName += ".txt";
		double dTotalNormalizedObjective = 0;
		double dTotalTime = 0;
		int iTotalCases = 0;
		iNumFlows = iN;

		//make sure all cases are there
		cout << "Verifying/Generating synthetic systems....";
		NoCOptimizationNSweep_PerN_GenerateTestCases(iN);
		cout << "Complete" << endl;
		cout << "--------------------------------" << endl;
		if (strcmp("Enhanced-BnB", argv[2]) == 0)
		{
			int iThreadNum = 8;
			unsigned int uiMaxThreads = std::thread::hardware_concurrency();
			iThreadNum = uiMaxThreads;
			cout << "Using maximum number of concurrent threads: " << iThreadNum << endl;
			if (argc == 5)
			{
				iThreadNum = atoi(argv[4]);
			}
			NoCSyntheticNSweep_PerN_MethodOption_MultiThreadBnB(iN, iThreadNum, stringResultFileName);
			return;
		}

		//generate test cases first
		for (int i = 0; i < 1000; i++)
		{
			//double dUtil = (double)rand() / (double)RAND_MAX;
			//dUtil = dUtil * (dUtilUB - dUtilLB) + dUtilLB;
			
			cout << "Case #" << i << endl;			
			FlowsSet_2DNetwork cFlowSet;
			sprintf_s(axBuffer, "NSweep/N%d/FlowSet%d.tskst", iN, i);
			string stringFlowSetName = axBuffer;
			//cFlowSet.GenRandomTaskSetInteger(iNumFlows, dUtil, dHIRatio, 2, PERIODOPT_LOG);
			//cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
			cFlowSet.ReadImageFile((char*)stringFlowSetName.data());
			GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
			double dObjUB = pcTaskArray[0].getDeadline() - pcTaskArray[0].getCLO();
			for (int i = 0; i < iTaskNum; i++)
			{
				if (pcTaskArray[i].getCriticality())
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCHI());
				}
				else
				{
					dObjUB = min(dObjUB, pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO());
				}
			}

			if (strcmp(argv[2], "MUTER-guided") == 0)
			{
				cout << "Method: MUTER-guided" << endl;
				sprintf_s(axBuffer, "%s_FRResult.rslt", stringFlowSetName.data());
				string stringFRResultFile(axBuffer);
				NoCFlowDLUFocusRefinement cFR(cFlowSet);
				cFR.setObjType(NoCFlowDLUFocusRefinement::MaxMinLaxity);
				cFR.setCorePerIter(5);
				int iStatus = cFR.FocusRefinement(1, dTimeout);
				StatisticSet cFRResult = cFR.GenerateStatisticFile("");
				if (strcmp(cFRResult.getItem("Status").getString(), "Optimal") == 0)
				{
					dTotalNormalizedObjective += -cFR.getObj() / dObjUB;
				}
				else if (strcmp(cFRResult.getItem("Status").getString(), "Infeasible") == 0)
				{
					dTotalNormalizedObjective += 1;
				}
				else
				{
					dTotalNormalizedObjective += 0;
				}
				dTotalTime += cFRResult.getItem("Total Time").getValue() / 1000.0;
			}
			else if (strcmp(argv[2], "MILP") == 0)
			{
				cout << "Method: MILP-guided" << endl;
				NoCFlowRTMILPModel cFullMILP(cFlowSet);
				cFullMILP.setObjType(NoCFlowRTMILPModel::MaxMinLaxity);
				int iFullMILPStatus = cFullMILP.Solve(2, dTimeout);
				double dThisObjective = 0;
				if (iFullMILPStatus == 1 || iFullMILPStatus == -3 )
				{
					dTotalNormalizedObjective += -cFullMILP.getObjective() / dObjUB;
					dThisObjective = -cFullMILP.getObjective() / dObjUB;
				}
				else if (iFullMILPStatus == 0)
				{
					dTotalNormalizedObjective += 1;
					dThisObjective = 1;
				}
				else if (iFullMILPStatus == -1)
				{
					dTotalNormalizedObjective += 0;
					dThisObjective = 0;
				}
				dTotalTime += cFullMILP.getWallTime();
				//cout << "Obj UB" << dObjUB << endl;
				//cout << "Objective on this case: " << dThisObjective << ", " << iFullMILPStatus <<  endl;
			}
			else if (strcmp(argv[2], "BnB"))
			{
				cout << "Method: Enhanced-BnB" << endl;
				cout << "Running..." << endl;
				NoCPriorityAssignment_2008Burns_MaxMinLaxity cHeuristic(cFlowSet);				
				cHeuristic.setTimeout(600);
				TaskSetPriorityStruct cPriorityAssignment(cFlowSet);

				int iStatus = cHeuristic.Run(cPriorityAssignment);
				cout << endl;
				if (cHeuristic.getCurrentBest() > 0)
				{
					dTotalNormalizedObjective += cHeuristic.getCurrentBest() / dObjUB;
				}
				if (iStatus == 0)
				{
					dTotalNormalizedObjective += 0;
				}
				else
				{
					dTotalNormalizedObjective += 1;
				}
				dTotalTime += cHeuristic.getTime();
			}
			else
			{
				assert(0);
			}
			iTotalCases++;
			cout << "Method Complete" << endl;
			cout << "Average Normalized Objective: " << dTotalNormalizedObjective / iTotalCases << endl;
			cout << "Average Time: " << dTotalTime / iTotalCases << endl;
			cout << "Total Cases: " << iTotalCases << endl;
		
			ofstream ofstreamOutputFile(stringResultFileName.data());
			ofstreamOutputFile << "Average Normalized Objective: " << dTotalNormalizedObjective / iTotalCases << endl;
			ofstreamOutputFile << "Average Time: " << dTotalTime / iTotalCases << endl;
			ofstreamOutputFile << "Total Cases: " << iTotalCases << endl;
			ofstreamOutputFile.close();
			cout << "--------------------------------" << endl;
		}
	}

	void NoCSyntheticUSweep_PerUtilization_GenerateTestCases(double dUtil)
	{
		char axBuffer[512] = { 0 };
		int iNCases = 1000;
		int iXDimension = 4;
		int iYDimension = 4;
		int iNumFlows = 40;		
		double dHIRatio = 0.50;
		sprintf_s(axBuffer, "USweep/U%.2f", dUtil);
		string stringFolderName(axBuffer);
		sprintf_s(axBuffer, "mkdir -p %s", stringFolderName.data());
		system(axBuffer);
		for (int i = 0; i < 1000; i++)
		{
			string stringFlowSetName;
			sprintf_s(axBuffer, "%s/FlowSet%d.tskst", stringFolderName.data(), i);
			stringFlowSetName = axBuffer;
			if (IsFileExist(axBuffer))	continue;
			FlowsSet_2DNetwork cFlowSet;			
			cFlowSet.GenRandomTaskSetInteger(iNumFlows, dUtil, dHIRatio, 2, PERIODOPT_LOG);
			cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
			cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
			sprintf_s(axBuffer, "%s.txt", stringFlowSetName.data(), i);
			cFlowSet.WriteTextFile(axBuffer);
		}
	}

	void NoCOptimizationNSweep_PerN_GenerateTestCases(int iN)
	{
		char axBuffer[512] = { 0 };
		int iNCases = 1000;
		int iXDimension = 4;
		int iYDimension = 4;	
		double dUtilLB = 0.8;
		double dUtilUB = 0.95;
		int iNumFlows = 40;		
		double dHIRatio = 0.50;
		sprintf_s(axBuffer, "NSweep/N%d", iN);
		string stringFolderName = axBuffer;
		sprintf_s(axBuffer, "mkdir -p %s", stringFolderName.data());
		system(axBuffer);
		for (int i = 0; i < 1000; i++)
		{
			string stringFlowSetName;
			sprintf_s(axBuffer, "%s/FlowSet%d.tskst", stringFolderName.data(), i);
			stringFlowSetName = axBuffer;
			if (IsFileExist(axBuffer))	continue;
			FlowsSet_2DNetwork cFlowSet;	
			double dUtil = (double)rand() / (double)RAND_MAX;
			dUtil = dUtil * (dUtilUB - dUtilLB) + dUtilLB;
			cFlowSet.GenRandomTaskSetInteger(iN, dUtil, dHIRatio, 2, PERIODOPT_LOG);
			cFlowSet.GenRandomFlowRouteStress_2015Burns_ECRTS(iXDimension, iYDimension);
			cFlowSet.WriteImageFile((char *)stringFlowSetName.data());
			sprintf_s(axBuffer, "%s.txt", stringFlowSetName.data(), i);
			cFlowSet.WriteTextFile(axBuffer);
		}
	}
}

