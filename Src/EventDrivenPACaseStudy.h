#pragma once
#include "GeneralUnschedCoreComputer.hpp"
#include <unordered_map>
#ifdef MCMILPPACT
#include "GeneralFocusRefinement.h"
#endif

class DistributedEventDrivenTaskSet :public TaskSet
{
public:
	class End2EndPath
	{
	public:

	protected:
		deque<int> m_dequeNodes;
		double m_dDeadline;
	public:
		End2EndPath();
		void AddNode(int iObjIndex);
		void setDeadline(double dDeadline)	{ m_dDeadline = dDeadline; }
		double getDeadline() const	 { return m_dDeadline; }
		const deque<int> & getNodes() const;
		void WriteImage(ofstream & rcOutputFile);
		void ReadImage(ifstream & rcIfnputFile);
	protected:
	};
protected:
	//primary data
	vector<int> m_vectorJitterInheritanceSource;
	vector<int> m_vectorAllocation;
	vector<int> m_vectorObjectType; //0:Task; 1:Message
	deque<End2EndPath> m_dequeEnd2EndPaths;

	//derived data
	unordered_map<int, set<int> > m_umapPartition;
	unordered_map<int, double> m_umapHyperperiod;
	set<int> m_setAllocationIndices;
	set<int> m_setObjectsOnPaths;
	set<int> m_setJitterPropagator;
	map<int, double> m_mapUtilization;
	//For schedulability uses
	//A possibly tighter bound on response time
	vector<double> m_vectorDeadlineBound;	
	vector<double> m_vectorWaitingTimeBound;
public:
	DistributedEventDrivenTaskSet()	{}
	void InitializeDistributedSystemData();
	void setAllocation(int iTaskIndex, int iAllocation);
	void setJiterInheritanceSource(int iTaskIndex, int iSource);
	void setObjectType(int iTaskIndex, int iType);
	void AddEnd2EndPaths(End2EndPath & rcEnd2EndPath);
	void InitializeDerivedData();	
	int getAllocation(int iTaskIndex)	{ assert(iTaskIndex < m_iTaskNum);  return m_vectorAllocation[iTaskIndex]; }
	int getJitterSource(int iTaskIndex)	{ assert(iTaskIndex < m_iTaskNum);  return m_vectorJitterInheritanceSource[iTaskIndex]; }
	int getPartitionObjectType(int iPartitionIndex);
	const set<int> & getPartition(int iTaskIndex);
	const set<int> & getPartition_ByAllocation(int iAllocationIndex);
	int getObjectType(int iTaskIndex)	{ assert(iTaskIndex < m_iTaskNum);  return m_vectorObjectType[iTaskIndex]; }
	double getPartitionHyperperiod_ByTask(int iTaskIndex);
	double getPartitionHyperperiod_ByAllocation(int iTaskIndex);
	const set<int> & getAllocationIndices()	{ return m_setAllocationIndices; }
	const deque<End2EndPath> getEnd2EndPaths();
	const set<int> & getJitterPropogator()	{ return m_setJitterPropagator; }
	const set<int> & getObjectsOnPaths()	{ return m_setObjectsOnPaths; }
	const vector<double> & getTighterDeadlineBound()	{ return m_vectorDeadlineBound; }	
	double getDeadlineBound(int iTaskIndex);
	double getWaitingTimeBound(int iTaskIndex);
	double getPartitionUtilization_ByObjIndex(int iObjIndex);
	double getPartitionUtilization_ByPartition(int iPartitionIndex);
	double ScalePartitionUtil(int iPartitioIndex, double dUtil);
	void WriteExtendedParameterImage(ofstream & OutputFile);
	void ReadExtendedParameterImage(ifstream & InputFile);	
protected:
	void DeriveHyperperiod();
	void ResolveObjectsOnPaths();
	void ResolveJitterPropagator();
	void DerivePartition();
	void DeriveTighterDeadlineBound();
	void DeriveWaitingTimeBound();
	void DeriveSchedTestInstancesBound();
	void DerivePartitionUtilization();
};

class DistributedEventDrivenResponseTimeCalculator
{
public:

protected:
	DistributedEventDrivenTaskSet * m_pcSystem;
	TaskSetPriorityStruct * m_pcPriorityAssignment;

	//Derived Data
	vector<double> m_vectorExactResponseTime;
public:
	DistributedEventDrivenResponseTimeCalculator();
	DistributedEventDrivenResponseTimeCalculator(DistributedEventDrivenTaskSet & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment);
	double ComputeResponseTime_SpecifiedResponseTimes(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes);
	int ComputeAllResponseTimeExact(vector<double> & rvectorResponseTimes);
	int ComputeAllResponseTime_Approx(vector<double> & rvectorResponseTimes);
	double ComputeResponseTime_SpecifiedResponseTimes_Approx(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes);
	double getWaitingTime(int iTaskIndex, const vector<double> & rvectorResponseTimes);
	double getJitter(int iTaskIndex, const vector<double> & rvectorResponseTimes);
	double ComputeWaitingTime_SpecifiedResponseTimes(int iTaskIndex, double dWTLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWaitingTime_SpecifiedResponseTimes_Approx(int iTaskIndex, double dWTLimit, const vector<double> & rvectorResponseTimes);
//protected:
public:
	double RHS_Task(int iInstance, int iTaskIndex, double d_w, const vector<double> & rvectorResponseTimes);
	double ComputeWaitingTimeQ_Task(int iInstance, int iTaskIndex, double dWaitingTimeQLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWaitingTime_Task(int iInstance, int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWorstCaseWaitingTime_Task(int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes);
	double ComputeResponseTime_Task(int iInstance, int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWorstCaseResponseTime_Task(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWorstCaseResponseTime_Task_Approx(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWorstCaseWaitingTime_Task_Approx(int iTaskIndex, double dWTLimit, const vector<double> & rvectorResponseTimes);

	double RHS_Message(int iInstance, int iTaskIndex, double d_w, const vector<double> & rvectorResponseTimes);
	double ComputeWaitingTimeQ_Message(int iInstance, int iTaskIndex, double dWaitingTimeQLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWaitingTime_Message(int iInstance, int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes);
	double ComputeResponseTime_Message(int iInstance, int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWorstCaseResponseTime_Message(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWorstCaseWaitingTime_Message(int iTaskIndex, double dWaitingTimeLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWorstCaseResponseTime_Message_Approx(int iTaskIndex, double dRTLimit, const vector<double> & rvectorResponseTimes);
	double ComputeWorstCaseWaitingTime_Message_Approx(int iTaskIndex, double dWTLimit, const vector<double> & rvectorResponseTimes);

	double DerivedBlocking(int iTaskIndex);	

};

class DistributedEventDrivenSchedCondType
{
public:
	enum CondType
	{
		UB,
		LB
	}m_enumCondType;
	int m_iTaskIndex;
	DistributedEventDrivenSchedCondType()
	{
		m_enumCondType = UB;
		m_iTaskIndex = -1;
	}

	DistributedEventDrivenSchedCondType(int iTaskIndex, CondType enumCondType)
	{
		m_iTaskIndex = iTaskIndex;
		m_enumCondType = enumCondType;
	}

	DistributedEventDrivenSchedCondType(const char axStr[]);
};

bool operator < (const DistributedEventDrivenSchedCondType & rcLHS, const DistributedEventDrivenSchedCondType & rcRHS);
ostream & operator << (ostream & rcOS, const DistributedEventDrivenSchedCondType & rcRHS);
istream & operator >> (istream & rcIS, DistributedEventDrivenSchedCondType & rcLHS);

class DistributedEventDrivenMUDARComputer : public GeneralUnschedCoreComputer_MonoInt < DistributedEventDrivenTaskSet, DistributedEventDrivenSchedCondType >
{
public:

protected:	
	//Run time data
	vector<double> m_vectorDeadlineLB;
	vector<double> m_vectorDeadlineUB;
	
	//pre priority assignment
	TaskSetPriorityStruct m_cPreAssignment;
public:
	DistributedEventDrivenMUDARComputer();
	DistributedEventDrivenMUDARComputer(SystemType & rcSystem);
	virtual Monotonicity SchedCondMonotonicity(const SchedCondType & rcSchedCondType);
	virtual SchedCondContent SchedCondLowerBound(const SchedCondType & rcSchedCondType);
	virtual SchedCondContent SchedCondUpperBound(const SchedCondType & rcSchedCondType);
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual void IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	virtual bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	void DeriveSafePriorityAssignment(TaskSetPriorityStruct & rcPriorityAssignment);
	void DeriveSafePriorityAssignment_Approx(TaskSetPriorityStruct & rcPriorityAssignment);
	void setPreAssignment(TaskSetPriorityStruct & rcPriorityAssignment)	{ m_cPreAssignment = rcPriorityAssignment; }
protected:	
	void ExtractDeadlineUBLB(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, vector<double> & rvectorDeadlineUB, vector<double> & rvectorDeadlineLB);		
	//virtual int ConvertToMUS(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, SchedConds & rcCore);
};

class DistributedEventDrivenMUDARComputer_Approx : public DistributedEventDrivenMUDARComputer
{
public:

protected:
	
public:
	DistributedEventDrivenMUDARComputer_Approx();
	DistributedEventDrivenMUDARComputer_Approx(SystemType & rcSystem);
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual SchedCondContent SchedCondUpperBound(const SchedCondType & rcSchedCondType);

protected:			
};

class DistributedEventDrivenMUDARComputer_WaitingTimeBased : public DistributedEventDrivenMUDARComputer
{
public:

protected:
	vector<double> m_vectorDerivedResponseTime;
public:
	DistributedEventDrivenMUDARComputer_WaitingTimeBased();
	DistributedEventDrivenMUDARComputer_WaitingTimeBased(SystemType & rcSystem);
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual SchedCondContent SchedCondUpperBound(const SchedCondType & rcSchedCondType);
	virtual SchedCondContent SchedCondLowerBound(const SchedCondType & rcSchedCondType);
	void ConstructResponseTimeFromWaitingTime(const vector<double> & rvectorWaitingTimes, vector<double> & rvectorResponseTimes);
	virtual void IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
protected:	
	double ResolveResponseTime(int iTaskIndex, const vector<double> & rvectorWaitingTimes, vector<double> & rvectorResponseTimes);
};

class DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx : public DistributedEventDrivenMUDARComputer_WaitingTimeBased
{
public:

protected:	
public:
	DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx();
	DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx(SystemType & rcSystem);
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual SchedCondContent SchedCondUpperBound(const SchedCondType & rcSchedCondType);
protected:
};

class DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime : public DistributedEventDrivenMUDARComputer_WaitingTimeBased
{
public:

protected:

public:
	DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime();
	DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime(SystemType & rcSystem);
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual SchedCondContent SchedCondUpperBound(const SchedCondType & rcSchedCondType);
	virtual SchedCondContent SchedCondLowerBound(const SchedCondType & rcSchedCondType);
protected:

};

class DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx : public DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime
{
public:

protected:

public:
	DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx();
	DistributedEventDrivenMUDARComputer_MixedWaitingResponseTime_Approx(SystemType & rcSystem);
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual SchedCondContent SchedCondUpperBound(const SchedCondType & rcSchedCondType);
protected:

};

#ifdef MCMILPPACT
class DistributedEventDrivenMUDARGuidedOpt : public GeneralFocusRefinement_MonoInt < DistributedEventDrivenMUDARComputer >
{
public:

protected:

public:
	DistributedEventDrivenMUDARGuidedOpt();
	DistributedEventDrivenMUDARGuidedOpt(SystemType & rcSystem);
protected:
	int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */);
	void CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars);
	void GenEnd2EndConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
	void GenUBLBEqualConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
	void GenPositiveWatingTimeConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
	IloExpr WaitingTimeExpr(IloEnv & rcEnv, int iTaskIndex, SchedCondTypeVars & rcShcedCondTypeVars);
};

class DistributedEventDrivenMUDARGuidedOpt_Approx : public  GeneralFocusRefinement_MonoInt < DistributedEventDrivenMUDARComputer_Approx >
{
public:
	
protected:
		
public:
	DistributedEventDrivenMUDARGuidedOpt_Approx();
	DistributedEventDrivenMUDARGuidedOpt_Approx(SystemType & rcSystem);
protected:
	int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */);
	void CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars);
	void GenEnd2EndConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
	void GenUBLBEqualConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);	
	void GenMaxMinLaxityModel(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel);	
	void GenPositiveWatingTimeConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
};

template<class UnschedCoreComputerT = DistributedEventDrivenMUDARComputer_WaitingTimeBased>
class DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased : public GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>
{
public:
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::UnschedCoreComputer_MonoInt UnschedCoreComputer_MonoInt;
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::UnschedCore UnschedCore;
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::UnschedCores UnschedCores;
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::SchedConds SchedConds;
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::SchedCond SchedCond;
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::SchedCondType SchedCondType;
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::SchedCondContent SchedCondContent;
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::SystemType SystemType;

	enum OptimizationObjective
	{
		None,
		MaxMinEnd2EndLaxity,
		MinMaxNormalizedLaxity,
		ReferenceGuided,
	};
protected:
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::SchedCondTypeVars SchedCondTypeVars;
	typedef typename GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::SchedCondVars SchedCondVars;
	OptimizationObjective m_enumObjective;
public:
	DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased()
	{

	}

	DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased(SystemType & rcSystem)
		:GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::GeneralFocusRefinement_MonoInt(rcSystem)
	{

	}

	void setObjective(OptimizationObjective enumObjective)	{ m_enumObjective = enumObjective; }

	void CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars)
	{
		//Got to figure out what are the necessary task to export sched conds.
		GET_TASKSET_NUM_PTR(GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet, iTaskNum, pcTaskArray);
		//All tasks that propagate jitter should have a lower and upper bound
		const set<int> & rsetJitterPropagator = GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet->getJitterPropogator();
		for (set<int>::const_iterator iter = rsetJitterPropagator.begin(); iter != rsetJitterPropagator.end(); iter++)
		{
			int iTaskIndex = *iter;
			SchedCondType cLBType(iTaskIndex, SchedCondType::LB);
			SchedCondType cUBType(iTaskIndex, SchedCondType::UB);
			rcSchedCondTypeVars[cLBType] = IloNumVar(rcEnv, GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cUnschedCoreComputer.SchedCondLowerBound(cLBType), GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cUnschedCoreComputer.SchedCondUpperBound(cLBType), IloNumVar::Int);
			rcSchedCondTypeVars[cUBType] = IloNumVar(rcEnv, GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cUnschedCoreComputer.SchedCondLowerBound(cUBType), GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cUnschedCoreComputer.SchedCondUpperBound(cUBType), IloNumVar::Int);

			//cout << GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cUnschedCoreComputer.SchedCondLowerBound(cLBType) << " " << GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cUnschedCoreComputer.SchedCondUpperBound(cLBType) << endl;
		}

		//All tasks involved on a path should have a upper bound
		const set<int> & rsetObjectsOnPaths = GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet->getObjectsOnPaths();
		for (set<int>::const_iterator iter = rsetObjectsOnPaths.begin(); iter != rsetObjectsOnPaths.end(); iter++)
		{
			int iTaskIndex = *iter;
			SchedCondType cUBType(iTaskIndex, SchedCondType::UB);
			if (rcSchedCondTypeVars.count(cUBType)) continue;
			rcSchedCondTypeVars[cUBType] = IloNumVar(rcEnv, GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cUnschedCoreComputer.SchedCondLowerBound(cUBType), GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cUnschedCoreComputer.SchedCondUpperBound(cUBType), IloNumVar::Int);
		}

	}

	int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout)
	{
		try
		{
			IloEnv cSolveEnv;
			IloRangeArray cConst(cSolveEnv);
			SchedCondTypeVars cSchedCondTypeVars;
			SchedCondVars cSchedCondVars;
			CreateSchedCondTypeVars(cSolveEnv, cSchedCondTypeVars);
			this->CreateSchedCondVars(cSolveEnv, rcGenUC, cSchedCondVars);

			GenUBLBEqualConst(cSolveEnv, cSchedCondTypeVars, cConst);
			GenEnd2EndConst(cSolveEnv, cSchedCondTypeVars, cConst);
			this->GenUnschedCoreConst(cSolveEnv, rcGenUC, cSchedCondVars, cConst);
			this->GenSchedCondTypeConst(cSolveEnv, cSchedCondTypeVars, cSchedCondVars, cConst);

			IloModel cModel(cSolveEnv);
			cModel.add(cConst);
			switch (m_enumObjective)
			{
			case None:
				break;
			case MaxMinEnd2EndLaxity:				
				GenMaxMinEnd2EndLaxityModel(cSolveEnv, cSchedCondTypeVars, dObjLB, cModel);
				break;
			case MinMaxNormalizedLaxity:
				GenMinMaxNormalizedLatencyModel(cSolveEnv, cSchedCondTypeVars, cModel);
				break;
			case ReferenceGuided:								
				this->GenRefGuidedSearchModel(cSolveEnv, GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cReferenceSchedConds, cSchedCondTypeVars, cModel);
				break;
			default:
				break;
			}

			IloCplex cSolver(cModel);
			this->EnableSolverDisp(iDisplay, cSolver);
			this->setSolverParam(cSolver);

			cSolver.setParam(IloCplex::Param::TimeLimit, dTimeout);
			int iRetStatus = 0;

			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cSchedCondsConfig.clear();
			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dequeSolutionSchedConds.clear();
			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_iBestSolFeasible = 0;
			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dSolveCPUTime = getCPUTimeStamp();
			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dSolveWallTime = cSolver.getCplexTime();
			bool bStatus = cSolver.solve();
			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dSolveCPUTime = getCPUTimeStamp() - GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dSolveCPUTime;
			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dSolveWallTime = cSolver.getCplexTime() - GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dSolveWallTime;
			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cCplexStatus = cSolver.getCplexStatus();

			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dequeSolutionSchedConds.clear();
			GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_cSchedCondsConfig.clear();
			if (bStatus)
			{
				this->ExtractAllSolution(cSolver, cSchedCondTypeVars);
				this->MarshalAllSolution(GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_dequeSolutionSchedConds);				
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

	void GenEnd2EndConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
	{
		/*
		\sum w_i <= L
		w_i = R_i - J_i
		*/
		GET_TASKSET_NUM_PTR(GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet, iTaskNum, pcTaskArray);
		typedef DistributedEventDrivenTaskSet::End2EndPath End2EndPath;
		const deque<End2EndPath> & rdequeEnd2EndPaths = GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet->getEnd2EndPaths();
		for (deque<End2EndPath>::const_iterator iter = rdequeEnd2EndPaths.begin(); iter != rdequeEnd2EndPaths.end(); iter++)
		{
			const deque<int> & rdequeNodes = iter->getNodes();
			IloExpr cEnd2EndLatency(rcEnv);
			for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
			{
				int iNodeIndex = *iterNode;
				int iJitterSource = GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet->getJitterSource(iNodeIndex);
				assert(rcSchedCondTypeVars.count(SchedCondType(iNodeIndex, SchedCondType::UB)) == 1);
				cEnd2EndLatency += rcSchedCondTypeVars[SchedCondType(iNodeIndex, SchedCondType::UB)];
			}
			rcConst.add(cEnd2EndLatency <= iter->getDeadline());
		}
	}

	void GenUBLBEqualConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst)
	{
		GET_TASKSET_NUM_PTR(GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet, iTaskNum, pcTaskArray);
		for (typename SchedCondTypeVars::iterator iter = rcSchedCondTypeVars.begin(); iter != rcSchedCondTypeVars.end(); iter++)
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

	void GenMaxMinEnd2EndLaxityModel(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, double dLB, IloModel & rcModel)
	{
		GET_TASKSET_NUM_PTR(GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet, iTaskNum, pcTaskArray);
		typedef DistributedEventDrivenTaskSet::End2EndPath End2EndPath;
		IloNumVar cMinEnd2EndLaxity(rcEnv, 0.0, IloInfinity, IloNumVar::Float);
		IloRangeArray cConst(rcEnv);
		const deque<End2EndPath> & rdequeEnd2EndPaths = GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet->getEnd2EndPaths();
		for (deque<End2EndPath>::const_iterator iter = rdequeEnd2EndPaths.begin(); iter != rdequeEnd2EndPaths.end(); iter++)
		{
			const deque<int> & rdequeNodes = iter->getNodes();
			IloExpr cEnd2EndLatency(rcEnv);
			for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
			{
				int iNodeIndex = *iterNode;
				int iJitterSource = GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet->getJitterSource(iNodeIndex);
				assert(rcSchedCondTypeVars.count(SchedCondType(iNodeIndex, SchedCondType::UB)) == 1);
				cEnd2EndLatency += rcSchedCondTypeVars[SchedCondType(iNodeIndex, SchedCondType::UB)];
			}
			cConst.add(IloRange(rcEnv,
				-IloInfinity,
				cMinEnd2EndLaxity - (iter->getDeadline() - cEnd2EndLatency),
				0.0
				));
		}
		rcModel.add(cConst);
		rcModel.add(IloMinimize(rcEnv, -cMinEnd2EndLaxity));
		rcModel.add((-cMinEnd2EndLaxity) >= dLB);
	}

	void GenMinMaxNormalizedLatencyModel(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloModel & rcModel)
	{
		GET_TASKSET_NUM_PTR(GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet, iTaskNum, pcTaskArray);
		typedef DistributedEventDrivenTaskSet::End2EndPath End2EndPath;
		IloNumVar cMaxNormalizedEnd2EndLatency(rcEnv, 0.0, IloInfinity, IloNumVar::Float);
		IloRangeArray cConst(rcEnv);
		const deque<End2EndPath> & rdequeEnd2EndPaths = GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet->getEnd2EndPaths();
		for (deque<End2EndPath>::const_iterator iter = rdequeEnd2EndPaths.begin(); iter != rdequeEnd2EndPaths.end(); iter++)
		{
			const deque<int> & rdequeNodes = iter->getNodes();
			IloExpr cEnd2EndLatency(rcEnv);
			for (deque<int>::const_iterator iterNode = rdequeNodes.begin(); iterNode != rdequeNodes.end(); iterNode++)
			{
				int iNodeIndex = *iterNode;
				int iJitterSource = GeneralFocusRefinement_MonoInt <UnschedCoreComputerT>::m_pcTaskSet->getJitterSource(iNodeIndex);
				assert(rcSchedCondTypeVars.count(SchedCondType(iNodeIndex, SchedCondType::UB)) == 1);
				cEnd2EndLatency += rcSchedCondTypeVars[SchedCondType(iNodeIndex, SchedCondType::UB)];
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
};


typedef DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased<DistributedEventDrivenMUDARComputer_WaitingTimeBased_Approx> DistributedEventDrivenMUDARGuidedOpt_WaitingTimeBased_Approx;

class BinaryArrayInteger
{
public:
	IloNumVarArray m_cBinVars;
	IloNum m_dLB;
	IloNum m_dUB;
public:
	BinaryArrayInteger()	{ m_dLB = 0; m_dUB = 0; }
	BinaryArrayInteger(const IloEnv & rcEnv, IloNum dLB, IloNum dUB);
	~BinaryArrayInteger()	{}
	IloExpr getExpr();
	IloExpr getLinearizeProduct(const IloEnv & rcEnv, IloNumVar & rcFactor, IloNum dFactorUB, IloRangeArray & rcConst);
	void CreateBinVars(const IloEnv & rcEnv, IloNum dUB);
	void GenBoundConst(IloRangeArray & rcConst);
	double getValue(IloCplex & rcSolver);
};

class DistributedEventDrivenFullMILP
{
public:
	typedef map<int, IloNumVar> Int2Var;
	typedef map<int, Int2Var>  Variable2D;
	typedef IloNumVarArray Variable1D;
	enum OptimizationObjective
	{
		None,
		MaxMinEnd2EndLaxity,
		MinMaxNormalizedLaxity
	};

protected:
	DistributedEventDrivenTaskSet * m_pcTaskSet;
	double m_dObjective;
	double m_dSolveWallTime;
	double m_dSolveCPUTime;	
	TaskSetPriorityStruct m_cPreAssignment;
	OptimizationObjective m_enumObjective;
	//Result data
	TaskSetPriorityStruct m_cSolPA;
	vector<double> m_vectorSolResponseTimes;
	vector<double> m_vectorSolWatingTimes;	

	//Temporay data
	map<int, map<int, IloExpr> > m_mapInterVarLBExpr;

public:
	DistributedEventDrivenFullMILP();
	DistributedEventDrivenFullMILP(DistributedEventDrivenTaskSet & rcTaskSet);
	int Solve(int iDisplayLevel, double dTimeout = 1e74);
	int SolveBreakDownUtilization(int iECUIndex, int iDisplayLevel, double dTimeout = 1e74);
	void setPreAssignment(TaskSetPriorityStruct & rcPreAssignment)	{ m_cPreAssignment = rcPreAssignment; }
	TaskSetPriorityStruct getSolPA()	{ return m_cSolPA; }
	vector<double> getSolRT()	{ return m_vectorSolResponseTimes; }
	int VerifyPA(TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorResponseTimes, int iDisplayLevel, double dTimeout = 1e74);
	void setObjective(OptimizationObjective enumObjective)	{ m_enumObjective = enumObjective; }
	double getObjective()	{ return m_dObjective; }	
	double getWallTime()	{ return m_dSolveWallTime; }
protected:
	void CreatePVars(const IloEnv & rcEnv, Variable2D & rcPVars);
	void CreateInterVars(const IloEnv & rcEnv, Variable2D & rcInterVars);
	void CreatePIPVars(const IloEnv & rcEnv, Variable2D & rcPIPVars);
	void CreateRTVars(const IloEnv & rcEnv, Variable1D & rcRTVars);

	IloExpr WaitingTimeExpr(int iTaskIndex, Variable1D & rcRTVars);	
	void GenPVarConst(const IloEnv & rcEnv, int iAllocation, Variable2D & rcPVars, IloRangeArray & rcConst);
	void GenInterVarConst(const IloEnv & rcEnv, int iAllocation, Variable2D & rcInterVars, Variable1D & rcRTVars, IloRangeArray & rcConst);
	void GenPIPVarConst(const IloEnv & rcEnv, int iAllocation, Variable2D & rcPVars, Variable2D & rcInterVars, Variable2D & rcPIPVars, IloRangeArray & rcConst);
	void GenRTVarConst(const IloEnv & rcEnv, int iAllocation, Variable2D & rcPVars, Variable2D & rcPIPIVars, Variable1D & rcRTVars, IloRangeArray & rcConst);
	void GenEnd2EndConst(const IloEnv & rcEnv, Variable1D & rcRTVars, IloRangeArray & rcConst);
	void GenMaxMinEnd2EndLaxityModel(const IloEnv & rcEnv, Variable1D rcRTVars, IloModel & rcModel);
	void GenMinMaxNormalizedEnd2EndLaxityModel(const IloEnv & rcEnv, Variable1D rcRTVars, IloModel & rcModel);
	double DeterminedPIPBigM(int iTask_i, int iTask_j);	
	void EnableSolverDisp(int iLevel, IloCplex & rcSolver);
	void setSolverParam(IloCplex & rcSolver);
	void GenPreAssignmentConst(Variable2D & rcPVars, IloRangeArray & rcConst);
	void ExtractSolutionPA(Variable2D & rcPVars, IloCplex & cSolver, TaskSetPriorityStruct & rcPriorityAssignment);
	void ExtractSolutionRT(Variable1D & rcRTVars, IloCplex & rcSolver, vector<double> & rvectorResponsetimes);
	void ExtractSolutionWaitingTimes(Variable1D & rcRTVars, IloCplex & rcSolver, vector<double> & rvectorWaitingTimes);	

	void VerifyResult(Variable1D & rcRTVars, Variable2D & rcPVars, Variable2D & rcInterVars, Variable2D & rcPIPVars, IloCplex & rcSolver);

	void GenBreakDownUtilization_Model(const IloEnv & rcEnv, int iAllocation, Variable2D & rcPVars, Variable2D & rcPIPVars, Variable2D & rcInterVars, Variable1D & rcRTVars, IloModel & rcModel);
};
void TestMILPBreakDownUtilization();
//

#endif
void ReadDAC07CaseStudy(const char axFileName[], DistributedEventDrivenTaskSet & rcTaskSet, double dWCETScale = 1.0);
void DAC07CaseStudy();
void DAC07CaseStudy_WaitingTimeBasedFR();
void DAC07CaseStudy_MixedWTRT();
void DAC07CaseStudy_MaxSchedulableUtil(int argc, char * argv[]);
void DAC07CaseStudy_MaxSchedulableUtil_Accurate(int argc, char * argv[]);
void DAC07CaseStudy_MaxSchedulableUtil_v2();
void DisplayMaxSchedUtilResult();
void TestBinaryInteger();
void DAC07CaseStudy_BreakDownUtilization_Approx(int argc, char * argv[]);
void DAC07CaseStudy_BreakDownUtilization_Accurate(int argc, char * argv[]);
void DAC07CaseStudy_BreakDownUtilization_MILP(int argc, char * argv[]);
void MarshallBreakDownUtilizationResult();

namespace EventDrivenPACaseStudy_ArtifactEvaluation
{			
	void VehicleSystem_All(int argc, char * argv[]);

	void VechicleSystem_BreakDownUtil_MILPApprox(int argc, char * argv[]);
	void VechicleSystem_BreakDownUtil_MUTERApprox(int argc, char * argv[]);
	void VechicleSystem_BreakDownUtil_MUTERAccurate(int argc, char * argv[]);
	void VechicleSystem_BreakDownUtil_PerAllocation(int argc, char * argv[]);
}
