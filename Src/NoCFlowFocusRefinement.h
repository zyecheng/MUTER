#pragma once
#include "NoCCommunication.h"
#include "GeneralFocusRefinement.h"
class NoCFlowFocusRefinement:public GeneralFocusRefinement<NoCFlowRTJUnschedCoreComputer>
{
public:


protected:
	class SchedCondComp
	{
	public:
		bool operator() (const SchedCondElement & rcLHS, const SchedCondElement & rcRHS);
	};

	typedef map<SchedCondElement, IloNumVar, SchedCondComp> SchedCondVars;

public:
	NoCFlowFocusRefinement();
	NoCFlowFocusRefinement(FlowsSet_2DNetwork & rcFlowsSet);
	~NoCFlowFocusRefinement();
	
protected:
	void CreateJitterRTVariable(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars, IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars);	
	void CreateSchedCondVars(const IloEnv & rcEnv, SchedCondVars & rcSchedCondVars, UnschedCores & rcUnschedCores);
	IloNumVar & getJitterLO(int iTaskIndex, IloNumVarArray & rcJitterLOVars);
	IloNumVar & getJitterHI(int iTaskIndex, IloNumVarArray & rcJitterHIVars);
	IloNumVar & getRTLO(int iTaskIndex, IloNumVarArray & rcRTLOVars);
	IloNumVar & getRTHI(int iTaskIndex, IloNumVarArray & rcRTHIVars);
	IloNumVar & getSchedCondVars(const SchedCondElement & rcElement, SchedCondVars & rcSchedCondVars);

	void GenJitterRTConst(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars, IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloRangeArray & rcConst);
	void GenJitterRTSchedCondVarConst(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
		IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, SchedCondVars & rcSchedCondVars, UnschedCores & rcUnschedCores, IloRangeArray & rcConst);	
	void GenSchedCondVarDominanceConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst);
	virtual int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */);
	void ExtractSolution(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars, IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloCplex & rcSolver, int iSolIndex, SolutionSchedCondsDeque & rdequeSolutionSchedConds);
	void ExtractSolutionAll(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars, IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloCplex & rcSolver, SolutionSchedCondsDeque & rdequeSolutionSchedConds);

	void OptimalHook();
	void PrintExtraInfoToLog(ofstream & ofstreamLogFile);
};


//A reference Full response time based ILP model

class NoCFlowRTMILPModel
{
public:
	enum ObjType
	{
		None,
		MaxMinLaxity
	};
protected:
	FlowsSet_2DNetwork * m_pcFlowSet;
	TaskSetPriorityStruct  m_cPrioritySolution;
	double m_dObjective;
	double m_dSolveCPUTime;
	double m_dSolveWallTime;
	TaskSetPriorityStruct * m_pcPredefinePriorityAssignment;
	ObjType m_enumObjType;

	map<int, IloExpr> m_mapTempExpr;
public:
	NoCFlowRTMILPModel();
	NoCFlowRTMILPModel(FlowsSet_2DNetwork & rcFlowSet);
	~NoCFlowRTMILPModel();
	int Solve(int iDisplay = 0, double dTimeout = 1e74);
	int Solve_MinJitterRTDifference(int iDisplay, double dTimeout = 1e74);
	void setPredefinePA(TaskSetPriorityStruct & rcPriorityAssignment)	{ m_pcPredefinePriorityAssignment = &rcPriorityAssignment; }
	double getWallTime()	{ return m_dSolveWallTime; }
	double getCPUTime()	{ return m_dSolveCPUTime; }
	TaskSetPriorityStruct getPrioritySol()	{ return m_cPrioritySolution; }
	void setObjType(ObjType enumObjType)	{ m_enumObjType = enumObjType; }
	double getObjective()	{ return m_dObjective; }
protected:
	void CreateJitterRTVariable(IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars, IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars);
	void CreatePriorityVariable(IloNumVarArray & rcPVars);
	void CreateRTHIVariable(IloNumVarArray & rcRTHI_aVars, IloNumVarArray & rcRTHI_bVars, IloNumVarArray & rcRTHI_cVars);	
	void CreateInterfereVariable(IloNumVarArray & rcRTLOInterVars, IloNumVarArray & rcRT_aInterVars, IloNumVarArray & rcRT_bInterVars, IloNumVarArray & rcRT_cInterVars);	
	void CreatePInterfereVariable(IloNumVarArray & rcRTLOPInterVars, IloNumVarArray & rcRT_aPInterVars, IloNumVarArray & rcRT_bPInterVars, IloNumVarArray & rcRT_cPInterVars);

	IloNumVar & GetPriorityVariable(int iTaskA, int iTaskB, IloNumVarArray & rcPVars);	
	IloNumVar & GetJitterVariable(int iTaskIndex, IloNumVarArray & rcJitterVars);	
	IloNumVar & GetRTVariable(int iTaskIndex, IloNumVarArray & rcRTLVars);	
	IloNumVar & GetRTInterfereVariable(int iTaskA, int iTaskB, IloNumVarArray & rcRTLOInterVars);	
	IloNumVar & GetRTPInterfereVariable(int iTaskA, int iTaskB, IloNumVarArray & rcRTLOPPInterVars);	
	

	void GenRTLOInterVarConst(
		IloNumVarArray & rcPVars, 
		IloNumVarArray & rcJitterLOVars, 
		IloNumVarArray & rcRTLOVars, 
		IloNumVarArray & rcRTLOInterVars, IloNumVarArray & rcRTLOPInterVars,
		IloRangeArray & rcConst
		);

	void GenRTHI_aInterVarConst(
		IloNumVarArray & rcPVars,
		IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
		IloNumVarArray & rcRTHI_aVar,
		IloNumVarArray & rcRTHI_aInterVars, IloNumVarArray & rcRTHI_aPInterVars,
		IloRangeArray & rcConst
		);

	void GenRTHI_bInterVarConst(
		IloNumVarArray & rcPVars,
		IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
		IloNumVarArray & rcRTHI_bVars,
		IloNumVarArray & rcRTHI_bInterVars, IloNumVarArray & rcRTHI_bPInterVars,
		IloRangeArray & rcConst
		);

	void GenRTHI_cInterVarConst(
		IloNumVarArray & rcPVars,
		IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
		IloNumVarArray & rcRTHI_cVars, IloNumVarArray & rcRTHI_bVars,
		IloNumVarArray & rcRTHI_cInterVars, IloNumVarArray & rcRTHI_cPInterVars,
		IloRangeArray & rcConst
		);

	void GenRTJitterConst(
		IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars,
		IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
		IloRangeArray & rcConst
		);
	

	void GenSchedConst(
		IloNumVarArray & rcPVars,		
		IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloNumVarArray & rcRTHI_aVars, IloNumVarArray & rcRTHI_bVars, IloNumVarArray & rcRTHI_cVars,
		IloNumVarArray & rcRTLOPInterVars, IloNumVarArray & rcRT_aPInterVars, IloNumVarArray & rcRT_bPInterVars, IloNumVarArray & rcRT_cPInterVars, 
		IloRangeArray & rcConst
		);

	void GenPriorityConst(IloNumVarArray & rcPVars, IloRangeArray & rcRangeArray);
	void GenPriorityConst(int i, int j, IloNumVarArray & rcPVars, IloRangeArray & rcRangeArray);
	void GenPriorityConst(int i, int j, int k, IloNumVarArray & rcPVars, IloRangeArray & rcRangeArray);


	void VerifyResult(
		IloCplex & rcSolver, 
		IloNumVarArray & rcPVars,
		IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
		IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloNumVarArray & rcRTHI_aVars, IloNumVarArray & rcRTHI_bVars, IloNumVarArray & rcRTHI_cVars,
		IloNumVarArray & rcRTLOInterVars, IloNumVarArray & rcRT_aInterVars, IloNumVarArray & rcRT_bInterVars, IloNumVarArray & rcRT_cInterVars,
		IloNumVarArray & rcRTLOPInterVars, IloNumVarArray & rcRT_aPInterVars, IloNumVarArray & rcRT_bPInterVars, IloNumVarArray & rcRT_cPInterVars		
		);

	void VerifyResult2(
		IloCplex & rcSolver,
		IloNumVarArray & rcPVars,
		IloNumVarArray & rcJitterLOVars, IloNumVarArray & rcJitterHIVars,
		IloNumVarArray & rcRTLOVars, IloNumVarArray & rcRTHIVars, IloNumVarArray & rcRTHI_aVars, IloNumVarArray & rcRTHI_bVars, IloNumVarArray & rcRTHI_cVars,
		IloNumVarArray & rcRTLOInterVars, IloNumVarArray & rcRT_aInterVars, IloNumVarArray & rcRT_bInterVars, IloNumVarArray & rcRT_cInterVars,
		IloNumVarArray & rcRTLOPInterVars, IloNumVarArray & rcRT_aPInterVars, IloNumVarArray & rcRT_bPInterVars, IloNumVarArray & rcRT_cPInterVars
		);

	void ExtractPriorityResult(IloNumVarArray & rcPVars, IloCplex & rcSolver);	

	void EnableSolverDisp(int iLevel, IloCplex & rcSolver);

	void setSolverParam(IloCplex & rcSolver);

	void DisplayExactResult();

	void GenPredefinePriorityConst(TaskSetPriorityStruct & rcPriorityAssignment, IloNumVarArray & rcPVars, IloRangeArray & rcConst);
};

class NoCFlowDLUFocusRefinement :public GeneralFocusRefinement_MonoInt < NoCFlowDLUUnschedCoreComputer >
{
public:
	enum ObjType
	{
		None,
		MaxMinLaxity,
		MinMaxLaxity,
		ReferenceGuided,
		SuccessiveReferenceGuided,
		IntervalGuided,
	};
protected:
	map<int, TaskSetPriorityStruct> m_mapMustHPSets;
	ObjType m_enumObjType;
	Dictionary<double> m_cCustomParam;
public:
	NoCFlowDLUFocusRefinement();
	NoCFlowDLUFocusRefinement(SystemType & rcSystem);
	double ComputeLORTByMILP(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, UnschedCores & rcGenUC);
	double ComputeHIRTByMILP(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, UnschedCores & rcGenUC);
	void setMustHPSets(map<int, TaskSetPriorityStruct> & rmapMustHPSets)	{ m_mapMustHPSets = rmapMustHPSets; }
	void setObjType(ObjType enumObjType)	{ m_enumObjType = enumObjType; }
	void setCustomParam(string & rcKey, double dData)	{ m_cCustomParam[rcKey] = dData; }
protected:
	void GenMaxMinLaxityObjectiveOpt(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypes, IloModel & rcModel, double dLB = -1e74);
	void GenMinMaxLaxityObjectiveOpt(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypes, IloModel & rcModel, double dLB = -1e74);
	void GenLUBEqualConst(SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
	virtual int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout = 1e74);
	virtual void CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars);
	void GenRTConst(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel);
	void GenRTLOModel(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel);
	void GenRTHIModel(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel);
	void GenMustHPSetsConst(map<int, TaskSetPriorityStruct> & rmapMustHPSets, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel);
	void GenFixedHighestPAConst(TaskSetPriorityStruct & rcFixedHighestPA, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel);
};



void TestNoCFR_MonoInt();
void TestNoCMILPRT();
void TestNoCFlowDLU_SingleTaskSched_FocusRefinement();
void IC3StyleNoCAudsley();
void TestNoCFRBasedSchedTest();
void NoCAudsleyBnBFR();
int NoCAudsleyBnBFR_Recur(int iCurrentPriority, FlowsSet_2DNetwork & rcFlowSet, TaskSetPriorityStruct & rcPriorityAssignment,
	NoCFlowDLUUnschedCoreComputer & rcUCCalc);
void NoCAudsleyFR();

class NoCFlowDLU_SingleTaskSched_FocusRefinement : public NoCFlowDLUFocusRefinement
{
public:
	typedef map<int, map<int, IloNumVar> > Variable2D;
protected:
	int m_iTargetTask;
	TaskSetPriorityStruct m_cTargetPriorityAssignment;
	SchedConds m_cConstSchedConds;
	UnschedCores m_cLoadedUnschedCores;	
public:
	NoCFlowDLU_SingleTaskSched_FocusRefinement();
	NoCFlowDLU_SingleTaskSched_FocusRefinement(SystemType & rcSystem);
	void setTarget(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcConstSchedConds);	
protected:	
	void CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars);
	virtual int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout = 1e74);		
	bool VerifyResult(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsConfig);	
	int RelaxSchedConds(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsConfig);
	
};

//SingleSchedFR based UnschedCore Computer

class NoCDLUSingleSchedFRUnschedCoreComputer : public NoCFlowDLUUnschedCoreComputer
{
public:

protected:
	NoCFlowDLU_SingleTaskSched_FocusRefinement m_cFRTester;
	map<int, UnschedCores> m_mapTask2Cores;
public:
	NoCDLUSingleSchedFRUnschedCoreComputer();
	NoCDLUSingleSchedFRUnschedCoreComputer(SystemType & rcSystem);
	virtual bool HighestPriorityAssignment(int iTaskIndex, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	NoCFlowDLU_SingleTaskSched_FocusRefinement & getFRTestObj()	{ return m_cFRTester; }
protected:
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual void IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	virtual bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);	
	virtual bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
	{
		TaskSetPriorityStruct cDummyPA(*m_pcTaskSet);
		return IsSchedulable(rcSchedCondsFlex, rcSchedCondsFixed, cDummyPA);
	}
};
void TestSingleSchedFRAnalysis();
void TestHPPBasedAnalysis();
void SchedulabilityRunTimeBatchTest();
void NoCSchedTest_EmergencyTest();
void SchedulabilityRunTimeBatchTest_USweep();
void SmallNoCExample2();
void NoCSchedTest_FRAndMILP();

void SchedulabilityRunTimeBatchTes_Stress_USweep();
void SchedulabilityRunTimeBatchTes_Stress_NSweep();
void NoCTestHeuristic();
void SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest();
void SchedulabilityRunTimeBatchTes_Stress_NSweep_MultiThreadHeuristicTest();
void SchedulabilityRunTimeBatchTes_Stress_USweep_MaxMinLaxity();
void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity();
void MCWormholeCaseStudy2014();
void Construct14MCWormholeCaseStudy_v2(double dTotalUtil, FlowsSet_2DNetwork & rcFlowSet);
void TestMCNoCMILPResult();
void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity_HeuristicTest();
void MCWormholeCaseStudy2014_HeuBnB_MultiThread();

namespace NoCFlowCaseStudy_ArtifactEvaluationWrapUp
{
	
	void NoCAutonomousVehicleCaseStudy_EnhancedBnB(int argc, char * argv[]);
	void NoCAutonomousVehicleCaseStudy_MUTERGuided(int argc, char * argv[]);
	void NoCAutonomousVehicleCaseStudy_MILP(int argc, char * argv[]);

	void NoCSyntheticNSweep_EnhancedBnB();
	void NoCSyntheticNSweep_MILP();
	void NoCSyntheticNSweep_MUTERGuided();
	
	void NoCSyntheticUSweep_MUTERGuided_MILP();

	void SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest_thread_routine(int iThreadIndex, int * piTaskIndex, double * pdUtilIndex, mutex * pcLock);

	void SchedulabilityRunTimeBatchTes_Stress_USweep_MultiThreadHeuristicTest();

	void SchedulabilityRunTimeBatchTes_Stress_NSweep_MaxMinLaxity();

	void NoCSyntheticUSweep_PerUtilization_MethodOption(int argc, char * argv[]);

	void NoCOptimizationNSweep_PerN_MethodOption(int argc, char * argv[]);

	void NoCAutonomousVehicleCaseStudy_MILP_PerUtilization(int argc, char * argv[]);

	void NoCSyntheticUSweep_PerUtilization_GenerateTestCases(double dUtil);

	void NoCOptimizationNSweep_PerN_GenerateTestCases(int iN);

	void NoCAutonomousVehicleCaseStudy_MUTERGuided_PerUtilization(int argc, char * argv[]);
}