#pragma once
#include "TaskSet.h"
#include "Util.h"
#include "ResponseTimeCalculator.h"
#include "GeneralUnschedCoreComputer.hpp"
#include "Timer.h"

class FlowsSet_2DNetwork : public TaskSet
{
public:
	struct Coordinate
	{
		int iXPos;
		int iYPos;

		Coordinate();
		Coordinate(int iXPos_in, int iYPos_in);

		bool IsValid();
		bool Compare(Coordinate & rcOther);
	};
	typedef deque<Coordinate> Route;
protected:
	int m_iNetworkXDimension;
	int m_iNetworkYDimension;
	
	deque<Coordinate> m_dequeSourcesTable;
	deque<Coordinate> m_dequeDestinationsTable;
	deque<double> m_dequeHIPeriod;
	vector<double> m_vectorDirectJitter;

	deque<Route> m_dequeFlowPaths;
	deque< MySet<int> > m_dequeFlowsOnEachRouter;	
	typedef vector< vector<int> > IntVector2D;
	IntVector2D m_cShareRouterTable;
	IntVector2D m_cUpDownStreamTable;

public:
	FlowsSet_2DNetwork();	
	~FlowsSet_2DNetwork();
public:
	void WriteExtendedParameterImage(ofstream & OutputFile);	
	void WriteExtendedParameterText(ofstream & OuputFile);
	void ReadExtendedParameterImage(ifstream & InputFile);
	Route getRoute(int iFlowIndex);
	Coordinate getSource(int iFlowIndex);
	Coordinate getDestination(int iFlowIndex);
	void setSource(int iFlowIndex, Coordinate & rcSource)	{ m_dequeSourcesTable[iFlowIndex] = rcSource; }
	void setDestination(int iFlowindex, Coordinate & rcDestination)	{ m_dequeDestinationsTable[iFlowindex] = rcDestination; }
	int getXDimension()	{ return m_iNetworkXDimension; }
	int getYDimension()	{ return m_iNetworkYDimension; }
	void setXDimension(int iXDimension)	{ m_iNetworkXDimension = iXDimension; }
	void setYDimension(int iYDimension)	{ m_iNetworkYDimension = iYDimension; }
	void setHIPeriod(int iTaskIndex, double dHIPeriod)	{ m_dequeHIPeriod[iTaskIndex] = dHIPeriod; }
	void setDirectJitter(int iTaskIndex, double dJitter)	{ m_vectorDirectJitter[iTaskIndex] = dJitter; }
	void GeneratePlaceHolder(int iXDimension, int iYDimension);
	int XYRouting(Coordinate & rcSource, Coordinate & rcDestination, Route & rcPath);
	void GenerateRandomFlowRoute(int iXDimension, int iYDimension);
	int getUpDownStreamLOType(int iTaskIndex, int iOtherTaskIndex);
	int IsSharingRouter(int iTaskIndex, int iOtherTaskIndex);
	MySet<int> & getFlowsOnRouter(int iXPos, int iYPos);
	double getLOPeriod(int iTaskIndex);
	double getHIPeriod(int iTaskIndex);
	vector<double> & getDirectJitterTable()	{ return m_vectorDirectJitter; }
	void GenRandomFlow_2014Burns(int iTaskNum, double dPeriodRange, double dHIPercent, double cTCRatio, double dCriticalityFactor, int iXDimension, int iYDimension);	
	void GenRandomFlowRouteStress_2015Burns_ECRTS(int iXDimension, int iYDimension);
	TaskSetPriorityStruct GetDMPriorityAssignment();
	void GenSubFlowSet(MySet<int> & rsetSubsets, FlowsSet_2DNetwork & rcFlowSet, map<int, int> & rmapFull2Sub, map<int, int> & rmapSub2Full);
	void DeriveFlowRouteInfo();
protected:
	void GenerateRandomHIPeriod();
	void GenerateRandomDirectJitter();
	void ResolveFlowsOnEachRouter();
	void DeriveShareRouterTable();
	void DeriveUpDownStreamLOTable();		
	void ClearFlowData();	
	int getLinearRouterIndex(int iXPos, int iYPos);
	void FindXYRouteIntersection(Route & rcRouteA, Route & rcRouteB, Route & rcIntersection);
	Coordinate FindEarliestIntersection(Route & rcRouteA, Route & rcRouteB);
};

class NoCFlowAMCRTCalculator_SpecifiedJitter
{
public:
	FlowsSet_2DNetwork * m_pcTaskSet;
	vector<double> * m_pvectorDJitter;
	vector<double> * m_pvectorIJitterLO;
	vector<double> * m_pvectorIJitterHI;
	TaskSetPriorityStruct * m_pcPriorityAssignment;
public:
	NoCFlowAMCRTCalculator_SpecifiedJitter();
	NoCFlowAMCRTCalculator_SpecifiedJitter(FlowsSet_2DNetwork & rcTaskSet, vector<double> & rvectorDJitter,
		vector<double> & rvectorIJitterLO, vector<double> & rvectorIJitterHI, TaskSetPriorityStruct & rcPriorityAssignment);
	double RTLO(int iTaskIndex);
	double RT_a(int iTaskIndex);
	double RT_b(int iTaskIndex);
	double RT_c(int iTaskIndex, double dRT_b);
	double WCRTHI(int iTaskIndex);
	double WCRT(int iTaskIndex);
protected:
	double RHS_LO(int iTaskIndex, double dRT);
	double RHS_a(int iTaskIndex, double dRT);
	double RHS_b(int iTaskIndex, double dRT);
	double RHS_c(int iTaskIndex, double dRT, double dRT_b);


		
	bool IsUpStreamLO(int iTaskIndex, int iOtherTaskIndex);

};

class NoCFlowAMCRTCalculator_DerivedJitter : public NoCFlowAMCRTCalculator_SpecifiedJitter
{
protected:	
	vector<double> m_vectorIJitterLO;
	vector<double> m_vectorIJitterHI;

	vector<double> m_vectorRTLO;
	vector<double> m_vectorRT_a;
	vector<double> m_vectorRT_b;
	vector<double> m_vectorRT_c;
public:
	NoCFlowAMCRTCalculator_DerivedJitter();
	NoCFlowAMCRTCalculator_DerivedJitter(FlowsSet_2DNetwork & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & vectorDJitter);
	
	void Initialize();
	void Clear();

	void ComputeAllResponseTime();
	double getJitterLO(int iTaskIndex);
	double getJitterHI(int iTaskIndex);
	double getRTLO(int iTaskIndex);
	double getRTHI(int iTaskIndex);
	double getRTHI_a(int iTaskIndex);
	double getRTHI_b(int iTaskIndex);
	double getRTHI_c(int iTaskIndex);
	bool IsSchedulable();
	void PrintSchedulabilityInfo();
	void PrintResult(std::ostream & rostreamOutput);
};

class NoCFlowRTJSchedCond
{
public:
	enum CondType
	{
		JitterLO = 0,
		JitterHI,
		ResponseTimeLO,
		ResponseTimeHI
	};
	CondType iCondType;//0: Jitter; 2: Response Time
	int iTaskIndex;
	int iValue;
	friend bool operator < (const NoCFlowRTJSchedCond & rcLHS, const NoCFlowRTJSchedCond & rcRHS);
	friend ostream & operator << (ostream & rcOS, const NoCFlowRTJSchedCond & rcSchedCond);	
};

class NoCFlowRTJUnschedCoreComputer : public GeneralUnschedCoreComputer < NoCFlowRTJSchedCond, less<NoCFlowRTJSchedCond>, FlowsSet_2DNetwork >
{
protected:
	vector<double> m_vectorIJitterLO;
	vector<double> m_vectorIJitterHI;
	vector<double> m_vectorRTLOBound;
	vector<double> m_vectorRTHIBound;
public:
	NoCFlowRTJUnschedCoreComputer();
	NoCFlowRTJUnschedCoreComputer(FlowsSet_2DNetwork & rcFlowSet);

	bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	void ConvertToMUS_Schedulable(SchedCondElement & rcErased, SchedConds & rcSchedCondsFlex, SchedConds & rcShcedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	void IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
};

class NoCFlowDLUSchedCond
{
public:
	int iTaskIndex;
	enum CondType
	{
		DeadlineLBLO,
		DeadlineUBLO,
		DeadlineLBHI,
		DeadlineUBHI,
	} enumCondType;
	NoCFlowDLUSchedCond();
	NoCFlowDLUSchedCond(const char axStr[]);
	NoCFlowDLUSchedCond(int iTaskIndex_in, CondType CondType_in);
};

bool operator < (const NoCFlowDLUSchedCond & rcLHS, const NoCFlowDLUSchedCond & rcRHS);
ostream & operator << (ostream & rcOS, const NoCFlowDLUSchedCond & rcLHS);
istream & operator >> (istream & rcIS, NoCFlowDLUSchedCond & rcRHS);

class NoCFlowDLUUnschedCoreComputer : public GeneralUnschedCoreComputer_MonoInt < FlowsSet_2DNetwork, NoCFlowDLUSchedCond >
{
public:

protected:
	vector<double> m_vectorIJitterLO;
	vector<double> m_vectorIJitterHI;
	vector<double> m_vectorDeadlineLO;
	vector<double> m_vectorDeadlineHI;

	vector<double> m_vectorDeadlineLBLOLB;
	vector<double> m_vectorDeadlineLBHILB;

	TaskSetPriorityStruct m_cFixedPriorityAssignment;
public:
	NoCFlowDLUUnschedCoreComputer();
	NoCFlowDLUUnschedCoreComputer(SystemType & rcSystem);
	virtual SchedCondContent SchedCondUpperBound(const SchedCondType & rcSchedCondType);
	virtual SchedCondContent SchedCondLowerBound(const SchedCondType & rcSchedCondType);
	virtual Monotonicity SchedCondMonotonicity(const SchedCondType & rcSchedCondType);
	int DeriveDeadlineLBLB(SchedConds & rcDeadlineLBLB);
	int DeriveDeadlineLBLB_v2(SchedConds & rcDeadlineLBLB);
	void InitializeDeadlineLBLB();//
	bool IsSchedulable_AdaptiveLB(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	double ComputePessimisticRT(int iTaskIndex, SchedConds & rcSchedConds, TaskSetPriorityStruct & rcPA);
	double ComputeOptimisticRT(int iTaskIndex, SchedConds & rcSchedConds, TaskSetPriorityStruct & rcPA);
	bool SchedTestPessimistic(int iTaskIndex, SchedConds & rcSchedConds, TaskSetPriorityStruct & rcPA);
	bool SchedTestOptimistic(int iTaskIndex, SchedConds & rcSchedConds, TaskSetPriorityStruct & rcPA);
	virtual bool SchedTest_Public(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	TaskSetPriorityStruct & getFixedPriorityAssignment()	{ return m_cFixedPriorityAssignment; }
	bool IsSchedulablePessimistic(TaskSetPriorityStruct & rcPriorityAssignment);
protected:
	virtual bool SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
	virtual void IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	virtual void HighestPriorityAssignment_Prework(int iTaskIndex, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
	virtual int PreSchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, bool & rbSchedStatus, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed);
#if 0
	virtual void Recur_MinimalRelaxation(SchedConds & rcSchedCondsFlex, SchedCond & rcSchedCondInFlex, SchedCond & rcSchedCondRef)
	{
		rcSchedCondsFlex.erase(rcSchedCondInFlex.first);
	}
#endif
	
};

class NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA : public NoCFlowDLUUnschedCoreComputer
{
public:

protected:
	TaskSetPriorityStruct m_cCurrentAudsleyPA;
	int m_iCurrentAudsleyCandidate;
public:
	NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA();
	NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA(SystemType & rcSystem);
protected:
	virtual bool IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment);
};

class NoCPriorityAssignment_2008Burns
{
public:
protected:
	FlowsSet_2DNetwork * m_pcFlowSet;
	vector<double> m_vectorDirectJitter;
	vector<double> m_vectorIJitterLO_UpperBound;
	vector<double> m_vectorIJitterHI_UpperBound;
	vector<double> m_vectorIJitterLO_LowerBound;
	vector<double> m_vectorIJitterHI_LowerBound;

	time_t m_dTimeout;
	time_t m_dStartTime;
	int m_iExaminedPA;
	int m_iNodesVisited;
	double m_dTime;	
	Timer m_cTimer;
	int m_iDisplay;
public:
	NoCPriorityAssignment_2008Burns();
	NoCPriorityAssignment_2008Burns(FlowsSet_2DNetwork & rcFlowSet);
	int Run(TaskSetPriorityStruct & rcPriorityAssignment);
	void setTimeout(time_t dTimeout)	{ m_dTimeout = dTimeout; }
	int getExaminePA()	{ return m_iExaminedPA; }
	int getNodesVisited()	{ return m_iNodesVisited; }
	double getTime()	{ return m_dTime; }
	void setDisplay(int iDisplay)	{ m_iDisplay = iDisplay; }
protected:
	bool UpperBoundAnalysis(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
	bool LowerBoundAnalysis(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
	double Fitnesss(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);

	virtual int Recur(int iPriorityLevel, TaskSetPriorityStruct & rcPriorityAssignment);
	void setWCET(int iTaskIndex, bool bCriticality, double dWCET);
};
#include <mutex>

class NoCPriorityAssignment_2008Burns_MaxMinLaxity : public NoCPriorityAssignment_2008Burns
{
public:
	
protected:
	double m_dCurrentBest;
	TaskSetPriorityStruct m_cBestAssignment;
	string m_stringLogFile;
	std::mutex m_cQueryLock;	
public:
	NoCPriorityAssignment_2008Burns_MaxMinLaxity();
	NoCPriorityAssignment_2008Burns_MaxMinLaxity(FlowsSet_2DNetwork & rcFlowSet);	
	FlowsSet_2DNetwork * getFlowSetPtr()	{ return m_pcFlowSet; }
	double getCurrentBest()	{ return m_dCurrentBest; }
	int Run(TaskSetPriorityStruct & rcPriorityAssignment);
	void setLogFile(string stringName)	{ m_stringLogFile = stringName; }
	void QueryCurrentBestSolution(volatile double & rdSol, TaskSetPriorityStruct & rcPASol);
protected:
	virtual int Recur(int iPriorityLevel, TaskSetPriorityStruct & rcPriorityAssignment);
};

void TestSubFlowSetGen();
