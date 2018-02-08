#pragma once
#include <ilcplex/ilocplex.h>
#include "TaskSet.h"
class MCRTMILPModelTaskModel
{
public:
	IloNumVarArray m_cPriorityVarArray;//binary variable
	IloNumVar m_cLORTVar;
	IloNumVar m_cMixRTVar;
	IloExpr m_cLORTExpr;//continuous varialbe
	IloExpr m_cMixRTExpr;
	IloNumVarArray m_cLOIINVarArray;// IIN short for LO Criticality Interference Instantces Number variable; Integer type
	IloNumVarArray m_cMixIINVarArray;// Similar as above but for Mix Criticality; Integer Type
	IloNumVarArray m_cLOPIINVarArray;//to model multiplication of a p * I (binary multiply integer), another set of integer variable need to be included; Integer Type	
	IloNumVarArray m_cMixPIINVarArray;//Similar as above but for Mix Criticality; Integer Type
	IloNumArray m_cLOIINResultArray;
	IloNumArray m_cLOPIINResultArray;
	IloNumArray m_cMixIINResultArray;
	IloNumArray m_cMixPIINResultArray;
	IloRangeArray m_cLORTConstraint;//Constraint for Modeling LO RT
	IloRangeArray m_cMixRTConstraint;//Constraint for Modeling HI RT
	IloRangeArray m_cSchedConstraint;//Constraint for schedulability
	int m_iPriority;
	double m_dRTLO;
	double m_dRTMix;
	IloNum m_cLORTResult;
	IloNum m_cMixRTResult;
public:
	MCRTMILPModelTaskModel();
	~MCRTMILPModelTaskModel();
	void DisplayConstraint();
};

class MCRTMILPModel
{
protected:
	TaskSet * m_pcTargetTaskSet;
	MCRTMILPModelTaskModel * m_pcTaskModelObj;
	int m_iTaskNum;
	IloRangeArray m_cPriorityConsistencyConstraint;
	IloNumVarArray m_cRTDetVariables;
	IloRangeArray  m_cRTDetVarConstraint;
	 double m_dObjectiveValue;
	int m_iSolveTime;
public:
	MCRTMILPModel();
	~MCRTMILPModel();
	void Initialize(TaskSet & rcTaskSet);
	void DisplayConstraints();
	void DisplayResult();
	bool SolveModel(double dTimeout = 1e74, int iShowWhileDisplay = 0);
	double getObjectiveValue()	{ return m_dObjectiveValue; }
	int getSolveTime()	{ return m_iSolveTime; }
	double ComputeMemoryCost();
protected:
	void IniPriorityVariable();
	void IniTaskModelObj();
	void BuildTaskLORTConstraint();
	void BuildTaskLORTConstraintTask(int iTaskIndex);
	void BuildTaskMixRTConstraint();
	void BuildTaskMixRTConstraintTask(int iTaskIndex);
	IloNumVar & getPriorityVariable(int i, int j);
	void ExtractResultTask(IloCplex & rcSolver,int iTaskIndex);
	void ExtractResult(IloCplex & rcSolver);
	bool VerifyInterResultConsistency(IloCplex & rcSolver);
	bool VerifyPriorityConsistency(IloCplex & rcSolver);
	bool VerifySchedulability(IloCplex & rcSolver);
	void CalculateResponseTime();
	void PriorityEnforcement();
	IloRange MemoryConstraint();
	virtual IloObjective Objective();	
	double DetermineBigMLO(int iTaskIndex);
	double DetermineBigMHI(int iTaskIndex);	
	double getPriorityResult(int iTaskA, int iTaskB);
	void GenUnschedCoreConst();
};


class MinimizeLORTModel : public MCRTMILPModel
{
public:
	MinimizeLORTModel();
	MinimizeLORTModel(TaskSet & rcTaskSet);
	~MinimizeLORTModel()	{};
	bool SolveModel(double dTimeout = 1e74, int iShowWhileDisplay = 0);	
protected:
	IloObjective Objective();	
	bool VerifyObjective();
};


