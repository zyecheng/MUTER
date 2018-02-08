#pragma once
#include "TaskSet.h"
#include <vector>
class ResponseTimeCalculator
{
#define RTCALC_LO	0
#define RTCALC_HI	1
#define RTCALC_MIX	2
#define RTCALC_AMCMAX	2
#define RTCALC_AMCRTB	3
#define RTCALC_VESTAL	4
protected:
	TaskSet * m_pcTargetTaskSet;

	//following is the data structure that store the results
	int * m_piPriorityTable;
	double * m_pdResponseTime;
	double * m_pdWorstCaseCritiChange;
	bool m_bIsMissingDeadline;	
public:
	ResponseTimeCalculator();
	~ResponseTimeCalculator();
	void Initialize(TaskSet & rcTaskSet);
	void setPrioirty(int iTaskIndex, int iPriority);
	int getPriority(int iTaskIndex);
	void Calculate(int iRTType);
	double getResponseTime(int iTaskIndex);
	double CalcResponseTimeTask(int iTaskIndex, int iRTType);
	double getWCCCT(int iTaskIndex);
	bool getDeadlineMiss()	{ return m_bIsMissingDeadline; }
	double LOResponseTime(int iTaskIndex);
	double HIResponseTime(int iTaskIndex);
	double WorstCasedMixResponseTime(int iTaskIndex, double * pfWCCCT = NULL);//or amc-max		
	double WorstCasedMixResponseTimeHuang(int iTaskIndex, double * pfWCCCT = NULL);//or amc-max	
	double ResponseTimeAMCRTB(int iTaskIndex);
	double ResponseTimeVestal(int iTaskIndex);
protected:
	void CreateTable();
	void ClearTable();
	//response time calculation
	void CalcResponseTime(int iRTType);
	double MixResponseTime(int iTaskIndex, double fCritiChange, double dLORT);
	double MixResponseTimeHuang(int iTaskIndex, double fCritiChange);
	double LOResponseTime_RHS(int iTaskIndex, double fGuess);
	double HIResponseTime_RHS(int iTaskIndex, double fGuess);
	double MixResponseTime_RHS(int iTaskIndex, double fGuess, double fCritiChange);
	double MixResponseTimeHuang_RHS(int iTaskIndex, double fGuess, double fCritiChange);
	double ResponseTimeAMCRTB_RHS(int iTaskIndex, double fGuess, double fLOResponseTime = -1);
	double ResponseTimeVestal_RHS(int iTaskIndex, double fGuess);
	
};

class _LORTCalculator
{
protected:
	TaskSet * m_pcTaskSet;
	vector<double> m_vectorResponseTime;
	bool m_bDeadlineMiss;
public:
	_LORTCalculator();
	_LORTCalculator(TaskSet & rcTaskSet);
	~_LORTCalculator();
	double CalculateRTTask(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, double dLB = 0);
	double CalculateRTTaskWithBlocking(int iTaskIndex, double dBlocking, TaskSetPriorityStruct & rcPriorityAssignment);
	double CalculateRTTaskGCD(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);	
protected:
	virtual double RHS(int iTaskIndex, double dRT, double dBlocking, TaskSetPriorityStruct & rcPriorityAssignment, void * pvExtraData);
	double CalculateTaskFP(int iTaskIndex, double dBlocking, double dIniRT, double dLimit, TaskSetPriorityStruct & rcPriorityAssignment, void * pvExtraData);
};

class _NonPreemptiveLORTCalculator
{
public:

protected:
	TaskSet * m_pcTaskSet;
public:
	_NonPreemptiveLORTCalculator();
	_NonPreemptiveLORTCalculator(TaskSet & rcTaskSet);
	~_NonPreemptiveLORTCalculator()	{}
	double CalculateRT(int iTaskIndex, TaskSetPriorityStruct & rcPA);
	double ComputeBlocking(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
protected:
	
	double NonpreemptiveRTRHS(double dTime, int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
};

class _AMCMaxRTCalculator: public _LORTCalculator
{
protected:
	virtual double RHS(int iTaskIndex, double dRT, double dBlocking, TaskSetPriorityStruct & rcPriorityAssignment, void * pvExtraData);
	double ResponseTime(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, double dCritiChange, double dLORT);
	double WCRT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
	double WCRT_MT(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, int iThreadNum);
public:
	_AMCMaxRTCalculator(TaskSet & rcTaskSet);
	double CalculateRTTask(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);	
};

class _AMCRtbRTCalculator : public _LORTCalculator
{
protected:
	virtual double RHS(int iTaskIndex, double dRT, double dBlocking, TaskSetPriorityStruct & rcPriorityAssignment, void * pvExtraData);
public:
	_AMCRtbRTCalculator(TaskSet & rcTaskSet);
	double CalculateRTTask(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment);
};