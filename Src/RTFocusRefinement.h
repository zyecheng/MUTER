#pragma once
#include "GeneralUnschedCoreComputer.hpp"

class WeightedTaskSet :public TaskSet
{
public:

protected:
	vector<double> m_vectorWeightTable;
public:
	WeightedTaskSet();
	~WeightedTaskSet();
	void setWeightTable(vector<double> & rvectorWeightTable);
	double getWeight(int iTaskIndex);
	const vector<double> & getWeightTable()	{ return m_vectorWeightTable; }
	void GenRandomWeight(int iRange = 10000);
protected:	
	void WriteExtendedParameterImage(ofstream & OutputFile);
	void ReadExtendedParameterImage(ifstream & InputFile);
	void WriteExtendedParameterText(ofstream & OuputFile);
};


class RTRange
{
public:
	int m_iTaskIndex;
	double m_dUpperBound;	
public:
	RTRange();
	RTRange(int iTaskIndex, double dUpperBound);
	~RTRange()	{}
	friend bool operator < (const RTRange & rcLHS, const RTRange & rcRHS);	
};

class RTRangeUniqueComparator
{
public:
	bool operator () (const RTRange & rcLHS, const RTRange & rcRHS);
};

class RTUnschedCoreComputer : public GeneralUnschedCoreComputer < RTRange, RTRangeUniqueComparator >
{
public:
	typedef map< int, MySet<double> > TasksRTSamples;	
protected:	
	vector<double> m_vectorRTLimits;	
	TasksRTSamples m_cTasksRTSamples;	
	double m_dLastRT;
public:
	RTUnschedCoreComputer();
	RTUnschedCoreComputer(System & rcSystem);
	~RTUnschedCoreComputer()	{}	
	void GenerateRTSamplesByResolution(double dResolution);
	void GenerateRTSamplesByResolution(int iTaskIndex, double dLB, double dUB, double dResolution);
	void AddRTSamples(int iTaskIndex, double dSample);	
	double getRTSampleLB(int iTaskIndex, double dQuery);
	double getRTSampleUB(int iTaskIndex, double dQuery);	
	void CombineSchedConds(SchedConds & rcShedCondsA, SchedConds & rcSchedCondsB, SchedConds & rcSchedCondsCombined);
	void CombineSchedConds(SchedConds & rcSchedCondsCombined, SchedConds & rcSchedCondsB);
protected:	
	virtual void IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual void ConvertToMUS_Schedulable(RTRange & rcErased, SchedConds & rcSchedCondsFlex, SchedConds & rcShcedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	virtual void NextCore_PerturbSchedCondEle(SchedCondElement rcFlexEle, SchedCondElement rcRefCoreEle, SchedConds & rcSchedConds);
	virtual void NextCore_RestoreSchedCondEle(SchedCondElement rcFlexEle, SchedCondElement rcRefCoreELe, SchedConds & rcSchedConds);
	virtual bool IsCoreContained(SchedConds & rcSchedConds, UnschedCore & rcCore);
public:

protected:
};


class MinRTSum : public RTUnschedCoreComputer
{
public:
	int m_iDisplay;
protected:	
	SchedConds m_cResultDConfigs;
	TaskSetPriorityStruct m_cOptPASol;
	double m_dMinSum;	
	MySet<int> m_setConcernTask;
public:
	MinRTSum();
	MinRTSum(System & rcTaskSet);
	~MinRTSum()	{}			
	double ComputeMinRTSum(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex,
		SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
protected:
};

//Exp Stuff: Summation Core
class RTSumRange_Type
{
public:
	MySet<int> m_setTasks;
	RTSumRange_Type();
	RTSumRange_Type(MySet<int> & rsetTasks);
	RTSumRange_Type(int iTaskIndex);
	~RTSumRange_Type()	{}
	friend bool operator < (const RTSumRange_Type & rcLHS, const RTSumRange_Type & rcRHS);
	friend ostream & operator << (ostream & os, const RTSumRange_Type & rcRHS);
};



class RTSumRangeUnschedCoreComputer : public GeneralUnschedCoreComputer_MK < RTSumRange_Type, int, TaskSet>
{
public:	
protected:
	vector<int> m_vectorRTLimits;	
	MySet<int> m_setConcernedTasks;
	double m_dSummationBound;	
	vector<double> m_vectorVirtualDeadline;
	vector<int> m_vectorWCETSort;
public:
	RTSumRangeUnschedCoreComputer();
	RTSumRangeUnschedCoreComputer(SystemType & rcSystem);
	~RTSumRangeUnschedCoreComputer()	{}
	virtual bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual int ComputeAllCores(UnschedCores & rcUnschedCores, int iLimit, double dTimeout = 1e74);
	double ComputeMinRTSum(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	double ComputeMinRTSum_v2(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
protected:
	virtual double ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);	
	virtual void ConvertToMUS_Schedulable(SchedCond & rcErased, SchedConds & rcSchedCondsFlex, SchedConds & rcShcedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);	
	void SortTaskByWCET(vector<int> & rvectorSort);
	virtual void IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);		
	virtual void Recur_MinimalRelaxation(SchedConds & rcSchedCondsFlex, SchedCond & rcSchedCondInFlex, SchedCond & rcSchedCondRef);
public:

protected:

};

class NonpreemptiveMinAvgRT_Exp : public RTSumRangeUnschedCoreComputer
{
public:
	NonpreemptiveMinAvgRT_Exp();
	NonpreemptiveMinAvgRT_Exp(SystemType & rcSystem);
	~NonpreemptiveMinAvgRT_Exp()	{};
	virtual double ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
protected:	
	double ComputeBlocking(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
	double NonpreemptiveRTRHS(double dTime, int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
};

class MinWeightedRTSumHeuristic : public GeneralUnschedCoreComputer_MK < RTSumRange_Type, int64_t, WeightedTaskSet>
{
protected:
	vector<double> m_vectorWeight;	
	vector<double> m_vectorVirtualDeadline;
	vector<int> m_vectorNominalWCETSort;
	vector<int> m_vectorMasterOrder;
	//Temporary Data
	vector<double> m_vectorMinWeightRTSumRTTable;

public:
	MinWeightedRTSumHeuristic();
	~MinWeightedRTSumHeuristic()	{}
	MinWeightedRTSumHeuristic(SystemType & rcSystem);
	double ComputeMinWeightedRTSum(vector<int> & rvectorExamOrder, MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorRTTable);
	double ComputeMinWeightedRTSum(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorRTTable);
	double ComputeMinWeightedRTSum(MySet<int> & rsetConcernedTasks, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);		
	void setWeight(vector<double> & rvectorWeight);
	virtual bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	void setMasterOrder(vector<int> & rvectorMasterOrder)	{ m_vectorMasterOrder = rvectorMasterOrder; }
protected:
	virtual void IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	void SortTaskByNonminalWCET(vector<int> & rvectorSort);	
public:
	void GenConcernedTaskFlag(MySet<int> & rsetConcernedTasks, vector<bool> & rvectorConcernedTaskFlag);
	//Some heuristic adjustment.	
	int RollingDownAdjustment_v2(int iTaskIndex, vector<bool> & rvectorConcernedTaskFlag, TaskSetPriorityStruct & rcPriorityAssignment,
		double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable);
	int RollingDownAdjustment(int iTaskIndex, vector<bool> & rvectorConcernedTaskFlag, TaskSetPriorityStruct & rcPriorityAssignment,
		double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable);
	double RollingDownToFixedPointAdjustment(MySet<int> & rsetConcernedTask, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeightTable);
	double RollingDownToFixedPointAdjustment(vector<bool> & rvectorConcernedTaskFlag,
		TaskSetPriorityStruct & rcPriorityAssignment, double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable);

	int RollingUpAdjustment(int iTaskIndex, vector<bool> & rvectorConcernedTaskFlag, TaskSetPriorityStruct & rcPriorityAssignment,
		double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable);
	double RollingUpToFixedPointAdjustment(MySet<int> & rsetConcernedTask, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeightTable);
	double RollingUpToFixedPointAdjustment(vector<bool> & rvectorConcernedTaskFlag,
		TaskSetPriorityStruct & rcPriorityAssignment, double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable);

	double RollingToFixedPointAdjustment(MySet<int> & rsetConcernedTask, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeightTable);
	double RollingToFixedPointAdjustment(vector<bool> & rvectorConcernedTaskFlag,
		TaskSetPriorityStruct & rcPriorityAssignment, double & rdCurrentCost, vector<double> & rvectorRTTable, vector<double> & rvectorWeightTable);

	virtual double ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);

	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual void ConvertToMUS_Schedulable(SchedCond & rcErased, SchedConds & rcSchedCondsFlex, SchedConds & rcShcedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	virtual void Recur_MinimalRelaxation(SchedConds & rcSchedCondsFlex, SchedCond & rcSchedCondInFlex, SchedCond & rcSchedCondRef);
};

class MinWeightedRTSumHeuristic_NonPreemptive : public MinWeightedRTSumHeuristic
{
public:

protected:

public:
	MinWeightedRTSumHeuristic_NonPreemptive();
	MinWeightedRTSumHeuristic_NonPreemptive(SystemType & rcSystem);
protected:
	virtual double ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);

};

void TestNPMinAvgRTAlg();
void TestNonpreemptveRTSum();
void RTSumUnschedCoreOptExample();
void Paper_SmallExampleDemo();

class WeightedRTSumBnB
{
public:

public:
	TaskSet * m_pcTaskSet;
	vector<double> m_vectorWeightTable;
	vector<double> m_vectorRTTable;	
	vector<double> m_vectorBestRTTable;
	double m_dBestCost;
	TaskSetPriorityStruct m_cOptimalPriorityAssignment;
public:
	WeightedRTSumBnB();
	WeightedRTSumBnB(TaskSet & rcTaskSet);
	~WeightedRTSumBnB();
	void setWeightTable(vector<double> & rvectorWeightTable);
	int Run();
protected:
	int Recur(int iLevel, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorRTTable, double dCurrentCost, double & rdBestCost);
	virtual double ComputeRT(int iTaskIndex, TaskSetPriorityStruct & rcPA);
};

void AnalyzeSuboptimalityFromSmallExample_PrintInfo(TaskSet & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & rvectorWeight);
#ifdef MCMILPPACT
#include <ilcplex/ilocplex.h>
#include "GeneralFocusRefinement.h"
#include "MCRTMILPModel.h"
class RTFocusRefinement : public GeneralFocusRefinement < RTUnschedCoreComputer >
{
public:
	typedef map<int, std::pair<double, double> > SpeciryRTRange;
		
protected:
	double m_dResolution;
	SpeciryRTRange m_mapSpecifiedRTRange;
public:
	RTFocusRefinement();
	RTFocusRefinement(SystemType &rcSystem);
	~RTFocusRefinement()	{}
	void SpecifyRTLB(int iTaskIndex, double dLB, double dUB);
	void PrintResult(ostream & os);
	void ClearFR();
protected:	
	int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout = 1e74);
	IloExpr ObjExpr(IloEnv & rcEnv, IloNumVarArray & rcRTVars);
	void CreateRTModelVar(const IloEnv & rcEnv, IloNumVarArray & rcRTVars);
	void GenSpecifiedRTRangeConst(const IloEnv & rcEnv, IloNumVarArray & rcRTVars, IloRangeArray & rcConst);
	void GenRTModelConst(const IloEnv & rcEnv, SchedCondVars & rcSchedCondVars, IloNumVarArray & rcRTVars, IloRangeArray & rcConst);
	void GenObjLBConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst);
	void GenSchedCondConsistencyConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst);
	void ExtractSolution(IloCplex & rcSolver, int iSolIndex, IloNumVarArray & cRTVars, SchedConds & rcSchedCondsConfig);
	void ExtractAllSolution(IloCplex & rcSolver, IloNumVarArray & rcRTVars);

};

void TestRTFocusRefinement();



//Experimental Stuff

//Temporary for experimenting idea
class RTFocusRefinement_TempExp : public RTFocusRefinement
{
public:
	typedef std::pair<SchedConds, UnschedCores> SchedCondsImplication;
	typedef deque<SchedCondsImplication> SchedCondsImplications;
protected:
	SchedCondsImplication m_dequeCoreImplication;
public:
	RTFocusRefinement_TempExp();
	~RTFocusRefinement_TempExp()	{}
	RTFocusRefinement_TempExp(SystemType & rcSystem);
protected:
	void ComputeSchedCondImplicationFromCore(UnschedCore & rcCore, SchedConds & rcPostCond, UnschedCores & rcPostCondCores);
	void ComputeSchedConsImplication(SchedConds & rcPreCond, SchedConds & rcPostCond, UnschedCores & rcPostCondCores);
	void ShuffleAndComputeMore(UnschedCores & rcCores);
	void GenCoreImplicationConst(SchedCondsImplications & rcCoreImplications, SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst);
	virtual void PostComputeUnschedCore(UnschedCores & rcUnschedCores);	
	void GenConjunctUnschedCoreCost(UnschedCores & rcCores, IloNumVarArray & rcRTVars, IloRangeArray & rcConst);
	virtual int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout /* = 1e74 */);
};


class RTSumFocusRefinement : public GeneralFocusRefinement_MK < RTSumRangeUnschedCoreComputer >
{
public:
	typedef map<int, std::pair<double, double> > SpeciryRTRange;	
	typedef map<int, double> LinearRTConst;
	typedef map<int, map<int, IloNumVar> > Variable2D;
protected:
	double m_dResolution;
	SpeciryRTRange m_mapSpecifiedRTRange;
	deque<LinearRTConst> m_dequeLinearRTConst;
public:
	RTSumFocusRefinement();
	RTSumFocusRefinement(SystemType &rcSystem);
	~RTSumFocusRefinement()	{}
	void SpecifyRTLB(int iTaskIndex, double dLB, double dUB);
	void PrintResult(ostream & os);
	void ClearFR();
	void AddLinearRTConst(LinearRTConst & rcLinearConst);
protected:
	void CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars);
	int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout = 1e74);
	IloExpr ObjExpr(IloEnv & rcEnv, SchedCondTypeVars & rcRTVars);	
	void CreateRTModelVar(const IloEnv & rcEnv, IloNumVarArray & rcRTVars);
	void GenSpecifiedRTRangeConst(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);	
	void GenRTSumModelConst(const IloEnv & rcEnv, SchedCondVars & rcSchedCondVars, SchedCondTypeVars & rcRTSumVars, IloRangeArray & rcConst);	
	void GenObjLBConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst);
	void GenSchedCondConsistencyConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst);
	void ExtractSolution(IloCplex & rcSolver, int iSolIndex, SchedCondTypeVars & rcSchedCondTypeVars, SchedConds & rcSchedCondsConfig);
	void ExtractAllSolution(IloCplex & rcSolver, SchedCondTypeVars & rcSchedCondTypeVars);
	void GenLinearRTConst(deque<LinearRTConst> & rdequeLinearRTConst, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
	void GenE2ELatencyConst(IloEnv & rcEnv, SchedCondTypeVars & rcRTVars, IloRangeArray & rcConst);
	IloNumVar CreateSchedCondTypeVar(IloEnv & rcEnv, MySet<int> & rsetTasks);

	//Experimental Strengthening
	void CreateInterfereVariable(const IloEnv & rcEnv, Variable2D & rcInterVars);
	void GenInterfereConst(const IloEnv & rcEnv, Variable2D & rcInterVars, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
};

class WRTSDFocusRefinement : public GeneralFocusRefinement_MK < MinWeightedRTSumHeuristic >
{
public:
	typedef map<int, std::pair<double, double> > SpeciryRTRange;
	typedef map<int, double> LinearRTConst;
	typedef map<int, map<int, IloNumVar> > Variable2D;
protected:
	double m_dResolution;
	SpeciryRTRange m_mapSpecifiedRTRange;
	deque<LinearRTConst> m_dequeLinearRTConst;
public:
	WRTSDFocusRefinement();
	WRTSDFocusRefinement(SystemType &rcSystem);
	~WRTSDFocusRefinement()	{}	
	void PrintResult(ostream & os);
	void ClearFR();
	void AddLinearRTConst(LinearRTConst & rcLinearConst);
protected:
	void CreateSchedCondTypeVars(const IloEnv & rcEnv, SchedCondTypeVars & rcSchedCondTypeVars);
	int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout = 1e74);
	IloExpr ObjExpr(IloEnv & rcEnv, SchedCondTypeVars & rcRTVars);
	void CreateRTModelVar(const IloEnv & rcEnv, IloNumVarArray & rcRTVars);	
	void GenRTSumModelConst(const IloEnv & rcEnv, SchedCondVars & rcSchedCondVars, SchedCondTypeVars & rcRTSumVars, IloRangeArray & rcConst);
	void GenObjLBConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst);
	void GenSchedCondConsistencyConst(SchedCondVars & rcSchedCondVars, IloRangeArray & rcConst);
	void ExtractSolution(IloCplex & rcSolver, int iSolIndex, SchedCondTypeVars & rcSchedCondTypeVars, SchedConds & rcSchedCondsConfig);
	void ExtractAllSolution(IloCplex & rcSolver, SchedCondTypeVars & rcSchedCondTypeVars);
	void GenLinearRTConst(deque<LinearRTConst> & rdequeLinearRTConst, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
	void GenE2ELatencyConst(IloEnv & rcEnv, SchedCondTypeVars & rcRTVars, IloRangeArray & rcConst);
	IloNumVar CreateSchedCondTypeVar(IloEnv & rcEnv, MySet<int> & rsetTasks);
};

//Experimental Idea: weighted RT sum FR
class WeightedRTSumFocusRefinement : public RTSumFocusRefinement
{
public:

protected:
	vector<double> m_vectorWeightTable;
	deque<double> m_dequeUnschedCoreSummationBound;
	MySet<int> m_setConcernedTasks;
public:
	WeightedRTSumFocusRefinement();
	WeightedRTSumFocusRefinement(SystemType & rcSystem);
	~WeightedRTSumFocusRefinement()	{}
	int SolveFR(int iDisplay, SchedConds & rcSchedCondFixed, UnschedCores & rcGenUC, double dObjLB, double dObjUB, double dTimeout = 1e74);
	void setWeight(vector<double> & rvectorWeight);
	void setConcernedTasks(MySet<int> & rsetConcernedTasks)	{ m_setConcernedTasks = rsetConcernedTasks; }
protected:
	void GenerateUnschedCoreStrengtheningConst(const IloEnv & rcEnv, UnschedCores & rcGenUC, SchedCondTypeVars & rcSchedCondTypeVars, IloRangeArray & rcConst);
	virtual void PostComputeUnschedCore(UnschedCores & rcUnschedCores);
};

void TestRTSumFocusRefinement();
class RTSRTPMILPModel
{
public:
	typedef map<int, map<int, IloNumVar> > Variable2D;
	typedef IloNumVarArray Variable1D;
protected:
	TaskSet * m_pcTaskSet;
	double m_dSolveCPUTime;
	double m_dSolveWallTime;
	double m_dObjective;
	TaskSetPriorityStruct m_cPrioritySol;
public:
	RTSRTPMILPModel();
	~RTSRTPMILPModel()	{}
	RTSRTPMILPModel(TaskSet & rcTaskSet);
	int Solve(int iDisplayLevel, double dTimeout = 1e74);
	int Solve(MySet<int> & rsetConcernedTasks, int iDisplayLevel, double dTimeout = 1e74);
	void PrintResult(ostream & os);
	void PrintResult(MySet<int> & rsetConcernedTasks, ostream & os);
	double getObj()	{ return m_dObjective; }
	TaskSetPriorityStruct getPrioritySol()	{ return m_cPrioritySol; }
	void setPredefinedPriority(TaskSetPriorityStruct & rcPredefinedPriority);
	double getWallTime()	{ return m_dSolveWallTime; }
	double getCPUTime()	{ return m_dSolveCPUTime; }
protected:
	void CreateRTVariable(const IloEnv & rcEnv, Variable1D & rcVarArray);
	void CreateInterInstVariable(const IloEnv & rcEnv, Variable2D & rcInterInstVars);
	void CreatePriorityVariable(const IloEnv & rcEnv, Variable2D & rcPriorityVariable);	
	void CreatePInterProdVaraible(const IloEnv & rcEnv, Variable2D & rcPIPVariable);
	void GenInterInstConst(const IloEnv & rcEnv, Variable2D & rcInterInstVars, Variable1D & rcRTVars, IloRangeArray & rcConst);
	void GenRTVarConst(const IloEnv & rcEnv, Variable1D & rcRTVars, Variable2D & rcPIPVars, IloRangeArray & rcConst);
	void GenPIPModelConst(const IloEnv & rcEnv, Variable2D & rcPIPVars, Variable2D & rcInterInstVars, Variable2D & rcPVars, IloRangeArray & rcConst);
	void GenPriorityModelConst(const IloEnv & rcEnv, Variable2D & rcPVars, IloRangeArray & rcConst);
	double DetermineBigMForPIP(int i, int j);
	void ExtractPrioritySol(IloCplex & rcSolver, Variable2D & rcPVars, TaskSetPriorityStruct & rcPriorityAssignment);
	void ExtractRTSol(IloCplex & rcSOlver, Variable1D & rcRTVars, map<int, double> & rmapTasksRT);
	void EnableSolverDisp(int iLevel, IloCplex & rcSolver);	
	void setSolverParam(IloCplex & rcSolver);
	void GenE2ELatencyConst(IloEnv & rcEnv, IloNumVarArray & rcRTVars, IloRangeArray & rcConst);
};

class NonpreemptiveRTMILP : public RTSRTPMILPModel
{
public:
	NonpreemptiveRTMILP();
	NonpreemptiveRTMILP(TaskSet & rcTaskSet);
	~NonpreemptiveRTMILP();
	int Solve(int iDisplayLevel, double dTimeout = 1e74);
	void PrintResult(ostream & os);
protected:
	void CreateBlockingVars(const IloEnv & rcEnv, Variable1D & rcBlockingVars);
	void GenBlockingVarConst(const IloEnv & rcEnv, Variable1D & rcBlockingVars, Variable2D & rcPVars, IloRangeArray & rcConst);
	void GenInterInstConst(const IloEnv & rcEnv, Variable2D & rcInterInstVars, Variable1D & rcRTVars, IloRangeArray & rcConst);
	void GenRTVarConst(const IloEnv & rcEnv, Variable1D & rcRTVars, Variable1D & rcBlockingVars, Variable2D & rcPIPVars, IloRangeArray & rcConst);
	void GenPIPModelConst(const IloEnv & rcEnv, Variable2D & rcPIPVars, Variable2D & rcInterInstVars, Variable2D & rcPVars, IloRangeArray & rcConst);
};

class MinWeightedRTSumMILP : public RTSRTPMILPModel
{
protected:
	vector<double> m_vectorWeight;
	MySet<int> m_setConcernedTasks;
public:
	MinWeightedRTSumMILP();
	MinWeightedRTSumMILP(TaskSet & rcTaskSet);
	~MinWeightedRTSumMILP()	{}	
	void PrintResult(ostream & os);
	void setConcernedTasks(MySet<int> & rsetConcernedTasks)	{ m_setConcernedTasks = rsetConcernedTasks; }
	int Solve(int iDisplayLevel, double dTimeout = 1e74);
	void setWeight(vector<double> & rvectorWeight);	
protected:
};

class MinWeightedRTSumMILP_NonPreemptive : public NonpreemptiveRTMILP
{
protected:
	vector<double> m_vectorWeight;
	MySet<int> m_setConcernedTasks;
	TaskSetPriorityStruct m_cStartingSolution;
public:
	MinWeightedRTSumMILP_NonPreemptive();
	MinWeightedRTSumMILP_NonPreemptive(TaskSet & rcTaskSet);
	~MinWeightedRTSumMILP_NonPreemptive()	{}
	void PrintResult(ostream & os);
	void setConcernedTasks(MySet<int> & rsetConcernedTasks)	{ m_setConcernedTasks = rsetConcernedTasks; }
	int Solve(int iDisplayLevel, double dTimeout = 1e74);
	void setWeight(vector<double> & rvectorWeight);

	void setStartingSolution(TaskSetPriorityStruct & rcStartingPA)	{ m_cStartingSolution = rcStartingPA; }
	int TestPriorityAssignment(TaskSetPriorityStruct & rcPA, int iDisplayLevel, double dTimeout = 1e74);
protected:
	void AddMILPStart(IloCplex & rcSolver, TaskSetPriorityStruct & rcPriorityAssignment,
		Variable1D & rcRTVars,
		Variable2D & rcInterInstVars,
		Variable2D & rcPIPVars,
		Variable2D & rcPVars,
		Variable1D & rcBlockingVars
		);
};

void TestRTSumAlgorithm();
void TestMinUnweightAlg();
void TestMinUnweightHeuristicSuboptimality();
void AnalyzeSuboptimalityFromSmallExample();
double GreedyLowestPriorityMinWeightedRTSum(TaskSet & rcTaskSet, vector<double> & rvectorWeightTable);


int MinWeightedRTSumTest_Preemptive_USweep(int argc, char * argv[]);
int MinWeightedRTSumTest_Preemptive_NSweep(int argc, char * argv[]);
int MinWeightedRTSumTest_NonPreemptive_USweep(int argc, char * argv[]);
int MinWeightedRTSumTest_NonPreemptive_NSweep(int argc, char * argv[]);

void testMinWeightedAvgRTMILP();
#endif