#include "stdafx.h"
#include <deque>
#include <set>
#include <fstream>
class Task
{
public:	
	class CommunicationLink
	{
	public:
		int m_iTargetTask;
		double m_dMemoryCost;
		double m_dDelayCost;
		CommunicationLink(int iTargetTask, double fMemoryCost,double fDelayCost)
		{
			m_iTargetTask = iTargetTask;
			m_dMemoryCost = fMemoryCost;
			m_dDelayCost = fDelayCost;
		}
		friend bool operator < (const CommunicationLink & rcRhs, const CommunicationLink & rcLhs)
		{
			return rcRhs.m_iTargetTask < rcLhs.m_iTargetTask;
		}
	};
	typedef set<CommunicationLink>::iterator CommIter;
private:
	//primary parameter
	int m_iIndex;
	double m_dPeriod;
	double m_dDeadline;
	double m_dUtilization;
	double m_dCriticalityFactor;
	bool m_bCriticalityLevel;
	//derivative parameter
	double m_dCHI;	//WCET in high criticality mode
	double m_dCLO;	//WCET in low criticality mode
	//predecessor and sucessor
	set<CommunicationLink> m_setPredecessor;
	set<CommunicationLink> m_setSuccessor;
	string m_stringName;
	
public:
	Task();	
	Task(double dPeriod, double dDeadline, double dUtil, double dCritiFactor, bool bCriticality, int iTaskIndex);
	
	~Task();
	//interface function
	inline double getCHI()		{ return m_dCHI; }
	inline double getCLO()		{ return m_dCLO; }
	inline double getPeriod() const	{ return m_dPeriod; }
	inline double getDeadline()	{ return m_dDeadline; }
	double getUtilization() const	{ return m_dUtilization; }
	bool getCriticality() const	{ return m_bCriticalityLevel; }
	double getCriticalityFactor()	{ return m_dCriticalityFactor; }
	int getTaskIndex() const	{ return m_iIndex; }
	void setPeriod(double fPeriod)	{ m_dPeriod = fPeriod; }
	void setDeadline(double dDeadline) { m_dDeadline = dDeadline; }
	void setUtilization(double fUtil)	{ m_dUtilization = fUtil; }
	void setCritiFactor(double fCritiFac)	{ m_dCriticalityFactor = fCritiFac; }
	void setCriticality(bool bCriti)	{ m_bCriticalityLevel = bCriti; }
	void setIndex(int iIndex)	{ m_iIndex = iIndex; }
	void setName(char axName[])	{ m_stringName = string(axName); };
	void setName(const char axName[])	{ m_stringName = string(axName); };
	string getName() const	{ return m_stringName; }
	void Construct();
	void AddPredecessor(int iTargetTaskIndex, double fMemoryCost, double fDelayCost);
	void AddSuccessor(int iTargetTaskIndex, double fMemoryCost, double fDelayCost);	
	bool IsRootTask()	{ return m_setPredecessor.size() == 0; }
	bool isOutputTask()	{ return m_setSuccessor.size() == 0; }
	set<CommunicationLink> & getSuccessorSet()	{ return m_setSuccessor; }
	set<CommunicationLink> & getPredecessorSet()	{ return m_setPredecessor; }
	int getOutputLink(int iTaskIndex, CommunicationLink & rcLink);
	int getInputLink(int iTaskIndex, CommunicationLink & rcLink);
	int isSuccessor(int iTaskIndex)	{ return m_setSuccessor.count({ iTaskIndex, 0, 0 }); };
	int isPredecessor(int iTaskIndex)	{ return m_setPredecessor.count({ iTaskIndex, 0, 0 }); };
	void WriteImage(ofstream & OutputFile);
	void ReadImage(ifstream & InputFile);	

	CommIter SuccessorBegin()	{ return m_setSuccessor.begin(); }
	CommIter SuccessorEnd()	{ return m_setSuccessor.end(); }
	CommIter PredecessorBegin()	{ return m_setPredecessor.begin(); }
	CommIter PredecessorEnd()	{ return m_setPredecessor.end(); }

	friend bool operator < (const Task & rcLhs, const Task & rcRhs);
	friend std::ostream & operator << (std::ostream & ostreamOutput, const Task & rcTask);

};
