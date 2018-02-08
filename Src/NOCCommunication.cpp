#include "stdafx.h"
#include "NoCCommunication.h"
#include "Timer.h"
#define ceil ceil_tol
bool operator < (const FlowsSet_2DNetwork::Coordinate & rcLHS, const FlowsSet_2DNetwork::Coordinate & rcRHS)
	{
	if (rcLHS.iXPos == rcRHS.iXPos)
	{
		return rcLHS.iYPos < rcRHS.iYPos;
	}
	else
	{
		return rcLHS.iXPos < rcRHS.iXPos;
	}
}

FlowsSet_2DNetwork::Coordinate::Coordinate()
{
	iXPos = -1;
	iYPos = -1;
}

FlowsSet_2DNetwork::Coordinate::Coordinate(int iXPos_in, int iYPos_in)
{
	iXPos = iXPos_in;
	iYPos = iYPos_in;
}

bool FlowsSet_2DNetwork::Coordinate::IsValid()
{
	return (iXPos != -1) && (iYPos != -1);
}

bool FlowsSet_2DNetwork::Coordinate::Compare(Coordinate & rcOther)
{
	return (rcOther.iXPos == iXPos && rcOther.iYPos == iYPos);
}

FlowsSet_2DNetwork::FlowsSet_2DNetwork()
{

}

FlowsSet_2DNetwork::~FlowsSet_2DNetwork()
{

}

FlowsSet_2DNetwork::Route FlowsSet_2DNetwork::getRoute(int iFlowIndex)
{
	assert(iFlowIndex < m_iTaskNum);
	return m_dequeFlowPaths[iFlowIndex];
}

FlowsSet_2DNetwork::Coordinate FlowsSet_2DNetwork::getSource(int iFlowIndex)
{
	assert(iFlowIndex < m_iTaskNum);
	return m_dequeSourcesTable[iFlowIndex];
}

FlowsSet_2DNetwork::Coordinate FlowsSet_2DNetwork::getDestination(int iFlowIndex)
{
	assert(iFlowIndex < m_iTaskNum);
	return m_dequeDestinationsTable[iFlowIndex];
}

void FlowsSet_2DNetwork::ClearFlowData()
{
	m_dequeDestinationsTable.clear();
	m_dequeSourcesTable.clear();
	m_dequeFlowPaths.clear();
	m_dequeFlowsOnEachRouter.clear();
	m_cShareRouterTable.clear();
	m_cUpDownStreamTable.clear();
	m_dequeHIPeriod.clear();
	m_vectorDirectJitter.clear();
	m_iNetworkXDimension = 0;
	m_iNetworkYDimension = 0;
}

MySet<int> & FlowsSet_2DNetwork::getFlowsOnRouter(int iXPos, int iYPos)
{
	return m_dequeFlowsOnEachRouter[getLinearRouterIndex(iXPos, iYPos)];
}

void FlowsSet_2DNetwork::GeneratePlaceHolder(int iXDimension, int iYDimension)
{
	ClearFlowData();
	m_iNetworkXDimension = iXDimension;
	m_iNetworkYDimension = iYDimension;
	assert(m_pcTaskArray);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_dequeSourcesTable.push_back(Coordinate());
		m_dequeDestinationsTable.push_back(Coordinate());
		m_dequeFlowPaths.push_back(Route());	
		m_dequeHIPeriod.push_back(0);
		m_vectorDirectJitter.push_back(0);
	}
}

int FlowsSet_2DNetwork::XYRouting(Coordinate & rcSource, Coordinate & rcDestination, Route & rcPath)
{
	int iXDirection = (rcSource.iXPos < rcDestination.iXPos) ? 1 : -1;
	int iYDirection = (rcSource.iYPos < rcDestination.iYPos) ? 1 : -1;
	rcPath.clear();	
	int iXPos = rcSource.iXPos;
	int iYPos = rcSource.iYPos;	
	for (; iXPos != rcDestination.iXPos; iXPos += iXDirection)
	{
		rcPath.push_back(Coordinate(iXPos, iYPos));		
	}

	for (; iYPos != rcDestination.iYPos; iYPos += iYDirection)
	{
		rcPath.push_back(Coordinate(iXPos, iYPos));
	}
	
	rcPath.push_back(Coordinate(iXPos, iYPos));

	return rcPath.size();
}

void FlowsSet_2DNetwork::GenerateRandomHIPeriod()
{
	m_dequeHIPeriod.clear();
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_dequeHIPeriod.push_back(m_pcTaskArray[i].getPeriod());
	}
}

void FlowsSet_2DNetwork::GenerateRandomDirectJitter()
{
	m_vectorDirectJitter.clear();
	m_vectorDirectJitter.reserve(m_iTaskNum);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_vectorDirectJitter.push_back(0);
	}
}

double FlowsSet_2DNetwork::getHIPeriod(int iTaskIndex)
{
	assert(m_dequeHIPeriod.size() > iTaskIndex);
	return m_dequeHIPeriod[iTaskIndex];
}

double FlowsSet_2DNetwork::getLOPeriod(int iTaskIndex)
{
	return m_pcTaskArray[iTaskIndex].getPeriod();
}

void FlowsSet_2DNetwork::GenerateRandomFlowRoute(int iXDimension, int iYDimension)
{
	ClearFlowData();
	assert(m_pcTaskArray);	
	GeneratePlaceHolder(iXDimension, iYDimension);

	for (int i = 0; i < m_iTaskNum; i++)
	{
		Coordinate cSource, cDestination;
		cSource.iXPos = rand() % iXDimension;
		cSource.iYPos = rand() % iYDimension;
		cDestination.iXPos = rand() % iXDimension;
		cDestination.iYPos = rand() % iYDimension;
		m_dequeSourcesTable[i] = (cSource);
		m_dequeDestinationsTable[i] = (cDestination);		
		XYRouting(cSource, cDestination, m_dequeFlowPaths[i]);		
	}
	GenerateRandomDirectJitter();
	GenerateRandomHIPeriod();
	ResolveFlowsOnEachRouter();
	DeriveShareRouterTable();
	DeriveUpDownStreamLOTable();
}

void FlowsSet_2DNetwork::ResolveFlowsOnEachRouter()
{
	assert(m_dequeFlowPaths.size() == m_iTaskNum);
	m_dequeFlowsOnEachRouter.clear();	
	int iRoutersNum = m_iNetworkYDimension * m_iNetworkXDimension;
	for (int i = 0; i < iRoutersNum; i++)
	{
		m_dequeFlowsOnEachRouter.push_back(MySet<int>());
	}

	for (int i = 0; i < m_iTaskNum; i++)
	{		
		Route & rcThisRoute = m_dequeFlowPaths[i];
		for (Route::iterator iter = rcThisRoute.begin(); iter != rcThisRoute.end(); iter++)
		{
			assert(iter->IsValid());
			m_dequeFlowsOnEachRouter[getLinearRouterIndex(iter->iXPos, iter->iYPos)].insert(i);
		}

	}
}

void FlowsSet_2DNetwork::DeriveShareRouterTable()
{
	m_cShareRouterTable.clear();
	assert(m_dequeFlowsOnEachRouter.size() == m_iNetworkXDimension * m_iNetworkYDimension);
	m_cShareRouterTable.reserve(m_iTaskNum);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_cShareRouterTable.push_back(vector<int>());
		m_cShareRouterTable[i].reserve(m_iTaskNum);
		for (int j = 0; j < m_iTaskNum; j++)
		{
			m_cShareRouterTable[i].push_back(0);
		}
	}

	for (int iXPos = 0; iXPos < m_iNetworkXDimension; iXPos++)
	{
		for (int iYPos = 0; iYPos < m_iNetworkYDimension; iYPos++)
		{
			MySet<int> & rsetFlows = getFlowsOnRouter(iXPos, iYPos);
			for (MySet<int>::iterator iterA = rsetFlows.begin(); iterA != rsetFlows.end(); iterA++)
			{
				for (MySet<int>::iterator iterB = rsetFlows.begin(); iterB != rsetFlows.end(); iterB++)
				{
					m_cShareRouterTable[*iterA][*iterB] = 1;
					m_cShareRouterTable[*iterB][*iterA] = 1;
				}
			}
		}
	}
	return;
}

void FlowsSet_2DNetwork::DeriveUpDownStreamLOTable()
{
	m_cUpDownStreamTable.clear();
	assert(m_dequeFlowsOnEachRouter.size() == m_iNetworkXDimension * m_iNetworkYDimension);	
	m_cUpDownStreamTable.reserve(m_iTaskNum);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		m_cUpDownStreamTable.push_back(vector<int>());
		m_cUpDownStreamTable[i].reserve(m_iTaskNum);
		for (int j = 0; j < m_iTaskNum; j++)
		{
			m_cUpDownStreamTable[i].push_back(-1);
		}
	}

	for (int i = 0; i < m_iTaskNum; i++)
	{
		Route & rcRoute = m_dequeFlowPaths[i];
		bool bHIMet = false;
		for (Route::reverse_iterator riter = rcRoute.rbegin(); riter != rcRoute.rend(); riter++)
		{
			MySet<int> & rsetFlowsOnRouter = m_dequeFlowsOnEachRouter[getLinearRouterIndex(riter->iXPos, riter->iYPos)];
			if (!bHIMet)
			{
				for (MySet<int>::iterator iterFlow = rsetFlowsOnRouter.begin(); iterFlow != rsetFlowsOnRouter.end(); iterFlow++)
				{
					if (m_pcTaskArray[*iterFlow].getCriticality())
					{
						bHIMet = true;
						break;
					}
				}
			}

			if (bHIMet)
			{
				for (MySet<int>::iterator iterFlow = rsetFlowsOnRouter.begin(); iterFlow != rsetFlowsOnRouter.end(); iterFlow++)
				{
					m_cUpDownStreamTable[i][*iterFlow] = 1;//up stream
				}
			}
			else
			{
				for (MySet<int>::iterator iterFlow = rsetFlowsOnRouter.begin(); iterFlow != rsetFlowsOnRouter.end(); iterFlow++)
				{
					m_cUpDownStreamTable[i][*iterFlow] = 0;//down stream
				}
			}												
		}
	}



}

int FlowsSet_2DNetwork::getUpDownStreamLOType(int iTaskIndex, int iOtherTaskIndex)
{
	return m_cUpDownStreamTable[iTaskIndex][iOtherTaskIndex];
}

int FlowsSet_2DNetwork::IsSharingRouter(int iTaskIndex, int iOtherTaskIndex)
{
	return m_cShareRouterTable[iTaskIndex][iOtherTaskIndex];
}

int FlowsSet_2DNetwork::getLinearRouterIndex(int iXPos, int iYPos)
{
	assert(iXPos < m_iNetworkXDimension);
	assert(iYPos < m_iNetworkYDimension);
	return iXPos * m_iNetworkXDimension + iYPos;
}

void FlowsSet_2DNetwork::WriteExtendedParameterImage(ofstream & OutputFile)
{
	OutputFile.write((const char *)&m_iNetworkXDimension, sizeof(int));
	OutputFile.write((const char *)&m_iNetworkYDimension, sizeof(int));
	assert(m_iTaskNum > 0);
	for (int i = 0; i < m_iTaskNum; i++)
	{
		OutputFile.write((const char *)&m_dequeSourcesTable[i], sizeof(Coordinate));
		OutputFile.write((const char *)&m_dequeDestinationsTable[i], sizeof(Coordinate));		
		Route & rcThisRoute = m_dequeFlowPaths[i];
		int iRouteLength = rcThisRoute.size();
		OutputFile.write((const char *)&iRouteLength, sizeof(int));
		for (Route::iterator iterRoute = rcThisRoute.begin(); iterRoute != rcThisRoute.end(); iterRoute++)
		{
			OutputFile.write((const char *)&(*iterRoute), sizeof(Coordinate));
		}
		OutputFile.write((const char *)&m_dequeHIPeriod[i], sizeof(double));
		OutputFile.write((const char *)&m_vectorDirectJitter[i], sizeof(double));
		
	}
	
}

void FlowsSet_2DNetwork::WriteExtendedParameterText(ofstream & OutputFile)
{
	OutputFile << endl;
	OutputFile << "Network X Dimension: " << m_iNetworkXDimension << endl;
	OutputFile << "Network Y Dimension: " << m_iNetworkYDimension << endl;
	char axBuffer[512] = { 0 };
	for (int i = 0; i < m_iTaskNum; i++)
	{
		OutputFile << "**********************************" << endl;
		OutputFile << "Flow " << i << endl;
		OutputFile << "Path: ";
		Route & rcRoute = m_dequeFlowPaths[i];
		for (Route::iterator iter = rcRoute.begin(); iter != rcRoute.end(); iter++)
		{
			sprintf_s(axBuffer, "(%d, %d), ", iter->iXPos, iter->iYPos);
			OutputFile << axBuffer;
		}
		OutputFile << endl;
		OutputFile << "HI Period: " << m_dequeHIPeriod[i] << endl;
		OutputFile << "Direct Jitter: " << m_vectorDirectJitter[i] << endl;
	}

}

void FlowsSet_2DNetwork::ReadExtendedParameterImage(ifstream & InputFile)
{
	InputFile.read((char *)&m_iNetworkXDimension, sizeof(int));
	InputFile.read((char *)&m_iNetworkYDimension, sizeof(int));
	GeneratePlaceHolder(m_iNetworkXDimension, m_iNetworkYDimension);
	for (int i = 0; i < m_iTaskNum; i++)
	{		
		InputFile.read((char *)&m_dequeSourcesTable[i], sizeof(Coordinate));
		InputFile.read((char *)&m_dequeDestinationsTable[i], sizeof(Coordinate));
		int iRouteLength = 0;
		InputFile.read((char *)&iRouteLength, sizeof(int));
		for (int iRoute = 0; iRoute < iRouteLength; iRoute++)
		{
			Coordinate cPos;
			InputFile.read((char *)&cPos, sizeof(Coordinate));
			m_dequeFlowPaths[i].push_back(cPos);
		}
		InputFile.read((char *)&m_dequeHIPeriod[i], sizeof(double));
		InputFile.read((char *)&m_vectorDirectJitter[i], sizeof(double));
	}

	ResolveFlowsOnEachRouter();
	DeriveShareRouterTable();
	DeriveUpDownStreamLOTable();
}

void FlowsSet_2DNetwork::FindXYRouteIntersection(Route & rcRouteA, Route & rcRouteB, Route & rcIntersection)
{
	Route cIntersection;
	for (Route::iterator iter = rcRouteA.begin(); iter != rcRouteA.end(); iter++)
	{
		for (Route::iterator iterB = rcRouteB.begin(); iterB != rcRouteB.end(); iterB++)
		{
			if (iterB->Compare(*iter))
			{
				cIntersection.push_back(*iter);
			}
		}
	}
	//A Horizontal Intersection	
}


FlowsSet_2DNetwork::Coordinate FlowsSet_2DNetwork::FindEarliestIntersection(Route & rcRouteA, Route & rcRouteB)
{
	Coordinate & rcASource = rcRouteA.front();
	Coordinate & rcADestination = rcRouteA.back();
	Coordinate & rcBSource = rcRouteA.front();
	Coordinate & rcBDestination = rcRouteA.back();
	int iAXStart = min(rcASource.iXPos, rcADestination.iXPos);
	int iAXEnd = max(rcASource.iXPos, rcADestination.iXPos);
	int iBXStart = min(rcBSource.iXPos, rcBDestination.iXPos);
	int iBXEnd = max(rcBSource.iXPos, rcBDestination.iXPos);
	int iAYStart = min(rcASource.iYPos, rcBDestination.iYPos);
	int iAYEnd = max(rcASource.iYPos, rcBDestination.iYPos);
	int iBYStart = min(rcBSource.iYPos, rcBDestination.iYPos);
	int iBYEnd = max(rcBSource.iYPos, rcBDestination.iYPos);

	//Case 1: Horizontal Overlapped
	int iYPos = rcASource.iYPos;
	if (rcASource.iYPos == rcBSource.iYPos)
	{
		int iXPos = -1;		
		if (rcASource.iXPos >= iBXStart && rcASource.iXPos <= iBXEnd)
		{
			iXPos = rcASource.iXPos;
			return Coordinate(iXPos, iYPos);
		}
		else if (rcADestination.iXPos >= rcASource.iXPos)
		{
			if (iBXStart >= rcASource.iXPos && iBXStart <= rcADestination.iXPos)
			{
				iXPos = iBXStart;
				return Coordinate(iXPos, iYPos);
			}
		}
		else if (rcADestination.iXPos <= rcASource.iXPos)
		{
			if (iBXEnd <= rcASource.iXPos && iBXEnd >= rcADestination.iXPos)
			{
				iXPos = iBXEnd;
				return Coordinate(iXPos, iYPos);
			}
		}		
	}
	else if (rcASource.iYPos <= iBYEnd && rcASource.iYPos >= iBYStart)
	{
		if (rcBDestination.iXPos >= iAXStart && rcBDestination.iXPos <= iAXEnd)
		{
			return Coordinate(rcBDestination.iXPos, iYPos);
		}
	}

	//Vertical Path Intersection
	int iXPos = rcADestination.iXPos;
	if (rcADestination.iXPos == rcBDestination.iXPos)
	{
		if (rcASource.iYPos >= iBYStart && rcASource.iYPos <= iBYEnd)
		{
			return Coordinate(iXPos, rcASource.iYPos);
		}
		else if (rcADestination.iYPos >= rcASource.iYPos)
		{
			if (iBYStart <= rcADestination.iYPos && iBYStart >= rcASource.iYPos)
			{
				return Coordinate(iXPos, iBYStart);
			}
		}
		else if (rcADestination.iYPos <= rcASource.iYPos)
		{
			if (iBYEnd >= rcADestination.iYPos && iBYEnd <= rcASource.iYPos)
			{
				return Coordinate(iXPos, iBYEnd);
			}
		}
	}
	else if (rcADestination.iXPos >= iBXStart && rcADestination.iXPos <= iBXEnd)
	{
		if (rcBSource.iYPos >= iAYStart && rcBSource.iYPos <= iAYEnd)
		{
			return Coordinate(iXPos, rcBSource.iYPos);
		}		
	}


	return Coordinate();
}

void FlowsSet_2DNetwork::GenRandomFlow_2014Burns(int iTaskNum, double dPeriodRange, double dHIPercent, double cTCRatio, double dCriticalityFactor, int iXDimension, int iYDimension)
{
	if (dHIPercent > 1)
	{
		cout << "HI Task Percentage Can Not Exceed 1 !!!" << endl;
		while (1);
	}
	int iHITaskNum = iTaskNum * dHIPercent;
	/////////////////////
	double * pfUtilTable = new double[iTaskNum];
	double * pfPeriodTable = new double[iTaskNum];
	bool * pbCritiTable = new bool[iTaskNum];
	
	GeneratePeriodTable(pfPeriodTable, iTaskNum, PERIODOPT_LOG);
	GenerateCritiTable(pbCritiTable, iHITaskNum, iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		pfPeriodTable[i] = round(pfPeriodTable[i]);
		pfPeriodTable[i] *= 1000;
		//double dWCET = floor((pbCritiTable ? fCriticalityFactor : 1) * pfUtilTable[i] * pfPeriodTable[i]);
		int iWCET = ((double)rand() / (double)RAND_MAX * cTCRatio * pfPeriodTable[i]);
		pfUtilTable[i] = (double)iWCET / pfPeriodTable[i];
		m_setTasks.insert(Task(pfPeriodTable[i], pfPeriodTable[i], pfUtilTable[i], dCriticalityFactor, pbCritiTable[i], i));


	}
	ConstructTaskArray();
	delete pfUtilTable;
	delete pfPeriodTable;
	delete pbCritiTable;
	GenerateRandomFlowRoute(iXDimension, iYDimension);
}

void FlowsSet_2DNetwork::GenRandomFlowRouteStress_2015Burns_ECRTS(int iXDimension, int iYDimension)
{
	ClearFlowData();
	assert(m_pcTaskArray);
	GeneratePlaceHolder(iXDimension, iYDimension);

	//find a HI flow to be the long crossing flow
	int iCrossHIFlow = -1;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (m_pcTaskArray[i].getCriticality())
		{
			iCrossHIFlow = i;
			break;
		}
	}
	if (iCrossHIFlow == -1)
	{
		return GenerateRandomFlowRoute(iXDimension, iYDimension);
	}
	m_dequeSourcesTable[iCrossHIFlow] = Coordinate(0, 0);
	m_dequeDestinationsTable[iCrossHIFlow] = Coordinate(iXDimension - 1, iYDimension - 1);
	XYRouting(m_dequeSourcesTable[iCrossHIFlow], m_dequeDestinationsTable[iCrossHIFlow], m_dequeFlowPaths[iCrossHIFlow]);

	for (int i = 0; i < m_iTaskNum; i++)
	{
		if (i == iCrossHIFlow) continue;
		Coordinate cSource, cDestination;
		if (m_pcTaskArray[i].getCriticality())
		{
			cDestination = Coordinate(iXDimension - 1, iYDimension - 1);
			cSource = Coordinate(iXDimension - 2, iYDimension - 2);
			int iOpt = rand() % 3;
			switch (iOpt)
			{
			case 0:
				cSource.iXPos += 1;
				break;
			case 1:
				cSource.iYPos += 1;
				break;
			default:
				break;
			}
		}
		else
		{
			cSource = Coordinate(0, 0);
			cDestination = Coordinate(1, 1);			
			int iOpt = rand() % 3;
			switch (iOpt)
			{
			case 0:
				cDestination.iXPos -= 1;
				break;
			case 1:
				cDestination.iYPos -= 1;
				break;
			default:
				break;
			}
		}			
		m_dequeSourcesTable[i] = (cSource);
		m_dequeDestinationsTable[i] = (cDestination);
		XYRouting(cSource, cDestination, m_dequeFlowPaths[i]);
	}
	GenerateRandomDirectJitter();
	GenerateRandomHIPeriod();
	ResolveFlowsOnEachRouter();
	DeriveShareRouterTable();
	DeriveUpDownStreamLOTable();
}

TaskSetPriorityStruct FlowsSet_2DNetwork::GetDMPriorityAssignment()
{
	TaskSetPriorityStruct cPriorityAssignment(*this);
	map< double, set<int> > mapSort;
	for (int i = 0; i < m_iTaskNum; i++)
	{
		mapSort[m_pcTaskArray[i].getDeadline()].insert(i);
	}

	int iPriority = 0;
	for (map<double, set<int> > ::iterator iter = mapSort.begin(); iter != mapSort.end(); iter++)
	{
		for (set<int>::iterator iterTask = iter->second.begin(); iterTask != iter->second.end(); iterTask++)
		{
			cPriorityAssignment.setPriority(*iterTask, iPriority);
			iPriority++;
		}
	}
	return cPriorityAssignment;
}

void FlowsSet_2DNetwork::GenSubFlowSet(MySet<int> & rsetSubsets, FlowsSet_2DNetwork & rcFlowSet, map<int, int> & rmapFull2Sub, map<int, int> & rmapSub2Full)
{
	rcFlowSet.ClearAll();
	int iNewIndex = 0;
	for (MySet<int>::iterator iter = rsetSubsets.begin(); iter != rsetSubsets.end(); iter++)
	{
		rmapFull2Sub[*iter] = iNewIndex;
		rmapSub2Full[iNewIndex] = *iter;
		iNewIndex++;
		rcFlowSet.AddTask(m_pcTaskArray[*iter]);
	}
	rcFlowSet.ConstructTaskArray();
	rcFlowSet.setXDimension(m_iNetworkXDimension);
	rcFlowSet.setYDimension(m_iNetworkYDimension);
	rcFlowSet.GeneratePlaceHolder(m_iNetworkXDimension, m_iNetworkYDimension);

	for (MySet<int>::iterator iter = rsetSubsets.begin(); iter != rsetSubsets.end(); iter++)
	{
		int iMappedIndex = rmapFull2Sub[*iter];
		rcFlowSet.setDirectJitter(iMappedIndex, m_vectorDirectJitter[*iter]);
		rcFlowSet.setHIPeriod(iMappedIndex, m_dequeHIPeriod[*iter]);
		rcFlowSet.setSource(iMappedIndex, m_dequeSourcesTable[*iter]);
		rcFlowSet.setDestination(iMappedIndex, m_dequeDestinationsTable[*iter]);
	}
	rcFlowSet.DeriveFlowRouteInfo();
}

void FlowsSet_2DNetwork::DeriveFlowRouteInfo()
{
	for (int i = 0; i < m_iTaskNum; i++)
	{
		XYRouting(m_dequeSourcesTable[i], m_dequeDestinationsTable[i], m_dequeFlowPaths[i]);
	}
	ResolveFlowsOnEachRouter();
	DeriveShareRouterTable();
	DeriveUpDownStreamLOTable();
}

void TestSubFlowSetGen()
{
	FlowsSet_2DNetwork cFlowSet;
	cFlowSet.ReadImageFile("NoCFlowSetForFR.tskst");
	GET_TASKSET_NUM_PTR(&cFlowSet, iTaskNum, pcTaskArray);
	FlowsSet_2DNetwork cSubFlowSet;
	NoCFlowDLUUnschedCoreComputer cUCCalc(cFlowSet);
	TaskSetPriorityStruct cPA(cFlowSet);
	typedef NoCFlowDLUUnschedCoreComputer::SchedConds SchedConds;
	SchedConds cDummy;
	cUCCalc.HighestPriorityAssignment(28, cDummy, cDummy, cPA);
	MySet<int> setSub;
	map<int, int> mapIndexMapping;
	map<int, int> mapSub2Full;
	int iHPPPriority = cPA.getPriorityByTask(28);
	for (int i = iHPPPriority; i >= 0; i--)
	{
		setSub.insert(cPA.getTaskByPriority(i));
	}
	cFlowSet.GenSubFlowSet(setSub, cSubFlowSet, mapIndexMapping, mapSub2Full);
	cSubFlowSet.WriteTextFile("SubFlowSet.txt");
	for (map<int, int>::iterator iter = mapIndexMapping.begin(); iter != mapIndexMapping.end(); iter++)
	{
		cout << iter->first << " " << iter->second << endl;
	}
	
}

//Response Time Calculator
NoCFlowAMCRTCalculator_SpecifiedJitter::NoCFlowAMCRTCalculator_SpecifiedJitter(FlowsSet_2DNetwork & rcTaskSet,
	vector<double> & rvectorDJitter, vector<double> & rvectorIJitterLO, vector<double> & rvectorIJitterHI, TaskSetPriorityStruct & rcPriorityAssignment)
{
	m_pvectorDJitter = &rvectorDJitter;
	m_pvectorIJitterLO = &rvectorIJitterLO;
	m_pvectorIJitterHI = &rvectorIJitterHI;
	m_pcTaskSet = &rcTaskSet;
	m_pcPriorityAssignment = &rcPriorityAssignment;
}

NoCFlowAMCRTCalculator_SpecifiedJitter::NoCFlowAMCRTCalculator_SpecifiedJitter()
{
	m_pcTaskSet = NULL;
	m_pvectorIJitterHI = NULL;
	m_pvectorIJitterLO = NULL;
	m_pvectorDJitter = NULL;
	m_pcPriorityAssignment = NULL;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::RHS_LO(int iTaskIndex, double dRT)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dValue = pcTaskArray[iTaskIndex].getCLO();
	int iTaskPriority = m_pcPriorityAssignment->getPriorityByTask(iTaskIndex);
	vector<double> & rvectorDJitter = *m_pvectorDJitter;
	vector<double> & rvectorIJitterLO = *m_pvectorIJitterLO;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		int iThisPriority = m_pcPriorityAssignment->getPriorityByTask(i);
		if ((iThisPriority < iTaskPriority) && (m_pcTaskSet->IsSharingRouter(iTaskIndex, i)))
		{
			dValue += ceil((dRT + rvectorDJitter[i] + rvectorIJitterLO[i]) / m_pcTaskSet->getLOPeriod(i)) * pcTaskArray[i].getCLO();
		}
	}
	return dValue;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::RHS_a(int iTaskIndex, double dRT)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dValue = pcTaskArray[iTaskIndex].getCHI();
	int iTaskPriority = m_pcPriorityAssignment->getPriorityByTask(iTaskIndex);
	vector<double> & rvectorDJitter = *m_pvectorDJitter;
	vector<double> & rvectorIJitterLO = *m_pvectorIJitterLO;
	vector<double> & rvectorIJitterHI = *m_pvectorIJitterHI;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		int iThisPriority = m_pcPriorityAssignment->getPriorityByTask(i);
		if ((iThisPriority < iTaskPriority) && (m_pcTaskSet->IsSharingRouter(iTaskIndex, i)))
		{
			if (pcTaskArray[i].getCriticality())
			{
				dValue += ceil((dRT + rvectorDJitter[i] + rvectorIJitterHI[i]) / m_pcTaskSet->getHIPeriod(i)) * pcTaskArray[i].getCHI();				
			}
		}
	}
	return dValue;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::RHS_b(int iTaskIndex, double dRT)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dValue = pcTaskArray[iTaskIndex].getCLO();
	int iTaskPriority = m_pcPriorityAssignment->getPriorityByTask(iTaskIndex);
	vector<double> & rvectorDJitter = *m_pvectorDJitter;
	vector<double> & rvectorIJitterLO = *m_pvectorIJitterLO;
	vector<double> & rvectorIJitterHI = *m_pvectorIJitterHI;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		int iThisPriority = m_pcPriorityAssignment->getPriorityByTask(i);
		if ((iThisPriority < iTaskPriority) && (m_pcTaskSet->IsSharingRouter(iTaskIndex, i)))
		{
			if (pcTaskArray[i].getCriticality())
			{
				dValue += ceil((dRT + rvectorDJitter[i] + rvectorIJitterHI[i]) / m_pcTaskSet->getLOPeriod(i)) * pcTaskArray[i].getCLO();				
			}
			else
			{
				dValue += ceil((dRT + rvectorDJitter[i] + rvectorIJitterLO[i]) / m_pcTaskSet->getLOPeriod(i)) * pcTaskArray[i].getCLO();				
			}
		}
	}
	return dValue;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::RHS_c(int iTaskIndex, double dRT, double dRT_b)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dValue = pcTaskArray[iTaskIndex].getCLO();
	int iTaskPriority = m_pcPriorityAssignment->getPriorityByTask(iTaskIndex);
	vector<double> & rvectorDJitter = *m_pvectorDJitter;
	vector<double> & rvectorIJitterLO = *m_pvectorIJitterLO;
	vector<double> & rvectorIJitterHI = *m_pvectorIJitterHI;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex)	continue;
		int iThisPriority = m_pcPriorityAssignment->getPriorityByTask(i);
		if ((iThisPriority < iTaskPriority) && (m_pcTaskSet->IsSharingRouter(iTaskIndex, i)))
		{
			if (pcTaskArray[i].getCriticality())
			{
				dValue += ceil((dRT + rvectorDJitter[i] + rvectorIJitterHI[i]) / m_pcTaskSet->getHIPeriod(i) ) * pcTaskArray[i].getCHI();
			}
			else
			{
				if (IsUpStreamLO(iTaskIndex, i))
				{
					dValue += ceil((dRT + rvectorDJitter[i] + rvectorIJitterLO[i]) / m_pcTaskSet->getLOPeriod(i)) * pcTaskArray[i].getCLO();
				}
				else
				{
					dValue += ceil((dRT_b + rvectorDJitter[i] + rvectorIJitterLO[i]) / m_pcTaskSet->getLOPeriod(i)) * pcTaskArray[i].getCLO();
				}
			}
		}
	}
	return dValue;
}

bool NoCFlowAMCRTCalculator_SpecifiedJitter::IsUpStreamLO(int iTaskIndex, int iOtherTaskIndex)
{
#if 1
	int iStatus = m_pcTaskSet->getUpDownStreamLOType(iTaskIndex, iOtherTaskIndex);
	if (iStatus == 1)
		return true;
	else if (iStatus == 0)
		return false;
	else
	{
		assert(0);
		return false;
	}

#endif
#if 0
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	FlowsSet_2DNetwork::Route & rcRoute = m_pcTaskSet->getRoute(iTaskIndex);
	FlowsSet_2DNetwork::Route & rcOtherRoute = m_pcTaskSet->getRoute(iOtherTaskIndex);
	FlowsSet_2DNetwork::Coordinate cEarliestIntersection = m_pcTaskSet->FindEarliestIntersection(rcRoute, rcOtherRoute);
	bool bMet = false;
	for (FlowsSet_2DNetwork::Route::iterator iter = rcRoute.begin(); iter != rcRoute.end(); iter++)
	{
		if ((!bMet) && (iter->Compare(cEarliestIntersection)))
		{
			bMet = true;
		}
		if (!bMet)	continue;

		FlowsSet_2DNetwork::Coordinate & rcRouter = *iter;
		MySet<int> & rsetPassingFlow = m_pcTaskSet->getFlowsOnRouter(rcRouter.iXPos, rcRouter.iYPos);
		for (MySet<int>::iterator iterPassingFlows = rsetPassingFlow.begin(); iterPassingFlows != rsetPassingFlow.end(); iterPassingFlows++)
		{
			if (pcTaskArray[*iterPassingFlows].getCriticality())
			{
				return true;
			}
		}
	}
	return false;
#endif
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::RTLO(int iTaskIndex)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dRT = pcTaskArray[iTaskIndex].getCLO();
	double dRHS = RHS_LO(iTaskIndex, dRT);
	while (!DOUBLE_EQUAL(dRT, dRHS, 1e-7))
	{
		dRT = dRHS;		
		if (dRHS > pcTaskArray[iTaskIndex].getDeadline())
		{
			break;
		}
		dRHS = RHS_LO(iTaskIndex, dRT);
	}
	return dRT;;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::RT_a(int iTaskIndex)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dRT = pcTaskArray[iTaskIndex].getCLO();
	double dRHS = RHS_a(iTaskIndex, dRT);
	while (!DOUBLE_EQUAL(dRT, dRHS, 1e-7))
	{
		dRT = dRHS;
		if (dRHS > pcTaskArray[iTaskIndex].getDeadline())
		{
			break;
		}
		dRHS = RHS_a(iTaskIndex, dRT);	
	}

	return dRT;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::RT_b(int iTaskIndex)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dRT = pcTaskArray[iTaskIndex].getCLO();
	double dRHS = RHS_b(iTaskIndex, dRT);
	while (!DOUBLE_EQUAL(dRT, dRHS, 1e-7))
	{
		dRT = dRHS;		
		if (dRHS > pcTaskArray[iTaskIndex].getDeadline())
		{
			break;
		}

		dRHS = RHS_b(iTaskIndex, dRT);
	}
	return dRT;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::RT_c(int iTaskIndex, double dRT_b)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	double dRT = pcTaskArray[iTaskIndex].getCLO();
	double dRHS = RHS_c(iTaskIndex, dRT, dRT_b);
	while (!DOUBLE_EQUAL(dRT, dRHS, 1e-7))
	{
		dRT = dRHS;		
		if (dRHS > pcTaskArray[iTaskIndex].getDeadline())
		{
			break;
		}

		dRHS = RHS_c(iTaskIndex, dRT, dRT_b);
	}
	return dRT;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::WCRTHI(int iTaskIndex)
{
	double dRTLO = RTLO(iTaskIndex);
	double dRTHI_a = RT_a(iTaskIndex);
	double dRTHI_b = RT_b(iTaskIndex);
	double dRTHI_c = RT_c(iTaskIndex, dRTHI_b);
	double dWCRTHI = max(dRTHI_a, dRTHI_c);
	return dWCRTHI;
}

double NoCFlowAMCRTCalculator_SpecifiedJitter::WCRT(int iTaskIndex)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRTHI = WCRTHI(iTaskIndex);
		return dRTHI;
	}
	else
	{
		double dRTLO = RTLO(iTaskIndex);
		return dRTLO;
	}
}

//NoCFlowAMCRTCalculator_SpecifiedJitter

NoCFlowAMCRTCalculator_DerivedJitter::NoCFlowAMCRTCalculator_DerivedJitter()
{
	m_pcTaskSet = NULL;
	m_pvectorIJitterHI = NULL;
	m_pvectorIJitterLO = NULL;
	m_pvectorDJitter = NULL;
	m_pcPriorityAssignment = NULL;
}

NoCFlowAMCRTCalculator_DerivedJitter::NoCFlowAMCRTCalculator_DerivedJitter(FlowsSet_2DNetwork & rcTaskSet, TaskSetPriorityStruct & rcPriorityAssignment, vector<double> & vectorDJitter)
{
	m_pcTaskSet = &rcTaskSet;	
	m_pcPriorityAssignment = &rcPriorityAssignment;
	m_pvectorDJitter = &vectorDJitter;
	Initialize();
}

void NoCFlowAMCRTCalculator_DerivedJitter::Clear()
{
	m_vectorIJitterHI.clear();
	m_vectorIJitterLO.clear();
	m_vectorRTLO.clear();
	m_vectorRT_a.clear();
	m_vectorRT_b.clear();
	m_vectorRT_c.clear();
}

void NoCFlowAMCRTCalculator_DerivedJitter::Initialize()
{
	assert(m_pcTaskSet);
	Clear();
	m_pvectorIJitterHI = &m_vectorIJitterHI;
	m_pvectorIJitterLO = &m_vectorIJitterLO;	

	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	m_vectorIJitterHI.reserve(iTaskNum);
	m_vectorIJitterLO.reserve(iTaskNum);
	m_vectorRTLO.reserve(iTaskNum);
	m_vectorRT_a.reserve(iTaskNum);
	m_vectorRT_b.reserve(iTaskNum);
	m_vectorRT_c.reserve(iTaskNum);
		
	for (int i = 0; i < iTaskNum; i++)
	{		
		m_vectorIJitterHI.push_back(-1);
		m_vectorIJitterLO.push_back(-1);
		m_vectorRTLO.push_back(-1);
		m_vectorRT_a.push_back(-1);
		m_vectorRT_b.push_back(-1);
		m_vectorRT_c.push_back(-1);
	}
}

void NoCFlowAMCRTCalculator_DerivedJitter::ComputeAllResponseTime()
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		int iTaskIndex = m_pcPriorityAssignment->getTaskByPriority(i);
		m_vectorRTLO[iTaskIndex] = RTLO(iTaskIndex);
		m_vectorIJitterLO[iTaskIndex] = m_vectorRTLO[iTaskIndex] - pcTaskArray[iTaskIndex].getCLO();

		if (pcTaskArray[iTaskIndex].getCriticality())
		{
			m_vectorRT_a[iTaskIndex] = RT_a(iTaskIndex);
			m_vectorRT_b[iTaskIndex] = RT_b(iTaskIndex);
			m_vectorRT_c[iTaskIndex] = RT_c(iTaskIndex, m_vectorRT_b[iTaskIndex]);
			m_vectorIJitterHI[iTaskIndex] = max(m_vectorRT_a[iTaskIndex], m_vectorRT_c[iTaskIndex]) - pcTaskArray[iTaskIndex].getCLO();
		}
	}
}

double NoCFlowAMCRTCalculator_DerivedJitter::getJitterHI(int iTaskIndex)
{
	assert(iTaskIndex < m_pcTaskSet->getTaskNum());
	return m_vectorIJitterHI[iTaskIndex];
}

double NoCFlowAMCRTCalculator_DerivedJitter::getJitterLO(int iTaskIndex)
{
	assert(iTaskIndex < m_pcTaskSet->getTaskNum());
	return m_vectorIJitterLO[iTaskIndex];
}

double NoCFlowAMCRTCalculator_DerivedJitter::getRTHI(int iTaskIndex)
{
	assert(iTaskIndex < m_pcTaskSet->getTaskNum());
	return max(m_vectorRT_a[iTaskIndex], m_vectorRT_c[iTaskIndex]);
}

double NoCFlowAMCRTCalculator_DerivedJitter::getRTHI_a(int iTaskIndex)
{
	assert(iTaskIndex < m_pcTaskSet->getTaskNum());
	return m_vectorRT_a[iTaskIndex];
}

double NoCFlowAMCRTCalculator_DerivedJitter::getRTHI_b(int iTaskIndex)
{
	assert(iTaskIndex < m_pcTaskSet->getTaskNum());
	return m_vectorRT_b[iTaskIndex];
}

double NoCFlowAMCRTCalculator_DerivedJitter::getRTHI_c(int iTaskIndex)
{
	assert(iTaskIndex < m_pcTaskSet->getTaskNum());
	return m_vectorRT_c[iTaskIndex];
}

double NoCFlowAMCRTCalculator_DerivedJitter::getRTLO(int iTaskIndex)
{
	assert(iTaskIndex < m_pcTaskSet->getTaskNum());
	return m_vectorRTLO[iTaskIndex];
}

bool NoCFlowAMCRTCalculator_DerivedJitter::IsSchedulable()
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		if (getRTLO(i) > pcTaskArray[i].getDeadline())
		{
			//cout << "Task " << i << endl;
			//cout << getRTLO(i) << " " << pcTaskArray[i].getDeadline() << endl;
			return false;
		}
		if (pcTaskArray[i].getCriticality())
		{
			if (getRTHI(i) > pcTaskArray[i].getDeadline())
			{
				//cout << "Task " << i << endl;
				//cout << getRTHI(i) << " " << pcTaskArray[i].getDeadline() << endl;
				return false;
			}
		}
	}
	return true;
}

void NoCFlowAMCRTCalculator_DerivedJitter::PrintSchedulabilityInfo()
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	cout << "Unschedulable Tasks: " << endl;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (getRTLO(i) > pcTaskArray[i].getDeadline())
		{
			cout << "Task " << i << " | " << getRTLO(i) << " " << pcTaskArray[i].getDeadline() << endl;			
		}
		if (pcTaskArray[i].getCriticality())
		{
			if (getRTHI(i) > pcTaskArray[i].getDeadline())
			{
				cout << "Task " << i  << " | " << getRTHI(i) << " " << pcTaskArray[i].getDeadline() << endl;				
			}
		}
	}	
}

void NoCFlowAMCRTCalculator_DerivedJitter::PrintResult(std::ostream & rostreamOutput)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		rostreamOutput << "Task " << i << " RTLO: " << m_vectorRTLO[i] << ", IJitterLO: " << m_vectorIJitterLO[i] << endl;
		if (pcTaskArray[i].getCriticality())
		{
			rostreamOutput << "Task " << i << " RTHI: " << max(m_vectorRT_a[i], m_vectorRT_c[i]) << ", IJitterLO: " << m_vectorIJitterHI[i] << endl;
		}
	}
}

//NoCFlowRTJSchedCond

bool operator < (const NoCFlowRTJSchedCond & rcLHS, const NoCFlowRTJSchedCond & rcRHS)
{
#if 0
	if (rcLHS.iTaskIndex == rcRHS.iTaskIndex)
	{
		return (rcLHS.iCondType < rcRHS.iCondType);
	}

	return rcLHS.iTaskIndex < rcRHS.iTaskIndex;
#else
	if (rcLHS.iCondType == rcRHS.iCondType)
	{
		return rcLHS.iTaskIndex < rcRHS.iTaskIndex;
	}
	
	return rcLHS.iCondType < rcRHS.iCondType;
#endif
}

ostream & operator << (ostream & rcOS, const NoCFlowRTJSchedCond & rcSchedCond)
{
	switch (rcSchedCond.iCondType)
	{
	case NoCFlowRTJSchedCond::JitterHI:
		rcOS << "(JHI, " << rcSchedCond.iTaskIndex << ", " << rcSchedCond.iValue << ")";
		break;
	case NoCFlowRTJSchedCond::JitterLO:
		rcOS << "(JLO, " << rcSchedCond.iTaskIndex << ", " << rcSchedCond.iValue << ")";
		break;
	case NoCFlowRTJSchedCond::ResponseTimeLO:
		rcOS << "(RTLO, " << rcSchedCond.iTaskIndex << ", " << rcSchedCond.iValue << ")";
		break;
	case NoCFlowRTJSchedCond::ResponseTimeHI:
		rcOS << "(RTHI, " << rcSchedCond.iTaskIndex << ", " << rcSchedCond.iValue << ")";
		break;
	default:
		assert(0);
		break;
	}
	return rcOS;
}

NoCFlowRTJUnschedCoreComputer::NoCFlowRTJUnschedCoreComputer()
{

}

NoCFlowRTJUnschedCoreComputer::NoCFlowRTJUnschedCoreComputer(FlowsSet_2DNetwork & rcFlowSet)
	:GeneralUnschedCoreComputer(rcFlowSet)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	m_vectorIJitterHI.reserve(iTaskNum);
	m_vectorIJitterLO.reserve(iTaskNum);
	m_vectorRTHIBound.reserve(iTaskNum);
	m_vectorRTLOBound.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorIJitterLO.push_back(0);
		m_vectorIJitterHI.push_back(0);
		m_vectorRTLOBound.push_back(0);
		m_vectorRTHIBound.push_back(0);
	}
}

void NoCFlowRTJUnschedCoreComputer::ConvertToMUS_Schedulable(SchedCondElement & rcErased, SchedConds & rcSchedCondsFlex, SchedConds & rcShcedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR((FlowsSet_2DNetwork *)m_pcTaskSet, iTaskNum, pcTaskArray);	
	int iTaskIndex = rcErased.iTaskIndex;
	SchedCondElement::CondType  enumCondType = rcErased.iCondType;
	if ((rcErased.iCondType == SchedCondElement::JitterLO) || (rcErased.iCondType == SchedCondElement::JitterHI))
	{		
		
		int iLB = 0;	
		int iUB = rcErased.iValue;						

		TaskSetPriorityStruct cPriorityAssignmentThis(*m_pcTaskSet);
		TaskSetPriorityStruct & cPriorityAssignmentLastUnsched = rcPriorityAssignment;
		cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
		while (iUB - iLB > 1)
		{
			int iCurrent = (iUB + iLB) / 2;
			rcSchedCondsFlex.insert({ enumCondType, iTaskIndex, iCurrent });			
			bool bStatus = IsSchedulable(rcSchedCondsFlex, rcShcedCondsFixed, cPriorityAssignmentThis);
			rcSchedCondsFlex.erase({ enumCondType, iTaskIndex, iCurrent });
			if (bStatus)
			{				
				iLB = iCurrent;				
				cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
			}
			else
			{				
				iUB = iCurrent;
				cPriorityAssignmentLastUnsched.CopyFrom_Strict(cPriorityAssignmentThis);
			}			
		}
		rcSchedCondsFlex.insert({ enumCondType, iTaskIndex, iUB });
	}
	else if ((enumCondType == SchedCondElement::ResponseTimeLO) || (enumCondType == SchedCondElement::ResponseTimeHI))
	{
		int iLB = rcErased.iValue;
		int iUB = pcTaskArray[iTaskIndex].getDeadline();
		TaskSetPriorityStruct cPriorityAssignmentThis(*m_pcTaskSet);
		TaskSetPriorityStruct & cPriorityAssignmentLastUnsched = rcPriorityAssignment;
		cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
		while (iUB - iLB > 1)
		{
			int iCurrent = (iUB + iLB) / 2;
			rcSchedCondsFlex.insert({ enumCondType, iTaskIndex, iCurrent });
			bool bStatus = IsSchedulable(rcSchedCondsFlex, rcShcedCondsFixed, cPriorityAssignmentThis);
			rcSchedCondsFlex.erase({ enumCondType, iTaskIndex, iCurrent });
			if (bStatus)
			{
				iUB = iCurrent;
				cPriorityAssignmentThis.CopyFrom_Strict(cPriorityAssignmentLastUnsched);
			}
			else
			{
				iLB = iCurrent;
				cPriorityAssignmentLastUnsched.CopyFrom_Strict(cPriorityAssignmentThis);
			}
			
		}
		rcSchedCondsFlex.insert({ enumCondType, iTaskIndex, iLB });
	}
	else
	{
		assert(0);
	}
}

bool NoCFlowRTJUnschedCoreComputer::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	FlowsSet_2DNetwork & rcFlowsSet = *(FlowsSet_2DNetwork*)m_pcTaskSet;


	NoCFlowAMCRTCalculator_SpecifiedJitter cCalculator(rcFlowsSet, 
		rcFlowsSet.getDirectJitterTable(), m_vectorIJitterLO, m_vectorIJitterHI, rcPriorityAssignment);
	double dResponseTimeLOBound = m_vectorRTLOBound[iTaskIndex];
	double dResponseTimeHIBound = m_vectorRTHIBound[iTaskIndex];
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRTLO = cCalculator.RTLO(iTaskIndex);
		double dRTHI_a = cCalculator.RT_a(iTaskIndex);
		double dRTHI_b = cCalculator.RT_b(iTaskIndex);
		double dRTHI_c = cCalculator.RT_c(iTaskIndex, dRTHI_b);
		double dWCRTHI = max(dRTHI_a, dRTHI_c);
		if ((dRTLO <= dResponseTimeLOBound) && (dWCRTHI <= dResponseTimeHIBound))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		double dRTLO = cCalculator.RTLO(iTaskIndex);
		if (dRTLO <= dResponseTimeLOBound)
		{
			return true;
		}
		else
		{
			return false;
		}
	}	
}

void NoCFlowRTJUnschedCoreComputer::IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorIJitterLO[i] = 0;
		m_vectorRTLOBound[i] = pcTaskArray[i].getDeadline();
		if (pcTaskArray[i].getCriticality())
		{
			m_vectorIJitterHI[i] = 0;
			m_vectorRTHIBound[i] = pcTaskArray[i].getDeadline();
		}
	}

	for (SchedConds::iterator iter = rcSchedCondsFlex.begin(); iter != rcSchedCondsFlex.end(); iter++)
	{
		int iThisTaskIndex = iter->iTaskIndex;
		switch (iter->iCondType)
		{
		case SchedCondElement::JitterHI:
			m_vectorIJitterHI[iThisTaskIndex] = max(m_vectorIJitterHI[iThisTaskIndex], (double)iter->iValue);
			break;
		case SchedCondElement::JitterLO:
			m_vectorIJitterLO[iThisTaskIndex] = max(m_vectorIJitterLO[iThisTaskIndex], (double)iter->iValue);
			break;
		case SchedCondElement::ResponseTimeLO:
			m_vectorRTLOBound[iThisTaskIndex] = min(m_vectorRTLOBound[iThisTaskIndex], (double)iter->iValue);
			break;
		case SchedCondElement::ResponseTimeHI:
			m_vectorRTHIBound[iThisTaskIndex] = min(m_vectorRTHIBound[iThisTaskIndex], (double)iter->iValue);
			break;
		default:
			assert(0);
		}
	}

	for (SchedConds::iterator iter = rcSchedCondsFixed.begin(); iter != rcSchedCondsFixed.end(); iter++)
	{
		int iThisTaskIndex = iter->iTaskIndex;
		switch (iter->iCondType)
		{
		case SchedCondElement::JitterHI:
			m_vectorIJitterHI[iThisTaskIndex] = max(m_vectorIJitterHI[iThisTaskIndex], (double)iter->iValue);
			break;
		case SchedCondElement::JitterLO:
			m_vectorIJitterLO[iThisTaskIndex] = max(m_vectorIJitterLO[iThisTaskIndex], (double)iter->iValue);
			break;
		case SchedCondElement::ResponseTimeLO:
			m_vectorRTLOBound[iThisTaskIndex] = min(m_vectorRTLOBound[iThisTaskIndex], (double)iter->iValue);
			break;
		case SchedCondElement::ResponseTimeHI:
			m_vectorRTHIBound[iThisTaskIndex] = min(m_vectorRTHIBound[iThisTaskIndex], (double)iter->iValue);
			break;
		default:
			assert(0);
		}
	}
}

//Mono int version of NoC unsched core
NoCFlowDLUSchedCond::NoCFlowDLUSchedCond()
{
	iTaskIndex = -1;
	enumCondType = DeadlineUBLO;
}

NoCFlowDLUSchedCond::NoCFlowDLUSchedCond(const char axStr[])
{
	stringstream ss(axStr);
	ss >> *this;
}

NoCFlowDLUSchedCond::NoCFlowDLUSchedCond(int iTaskIndex_in, CondType iType_in)
{
	iTaskIndex = iTaskIndex_in;
	enumCondType = iType_in;
}

bool operator < (const NoCFlowDLUSchedCond & rcLHS, const NoCFlowDLUSchedCond & rcRHS)
{
	if (rcLHS.iTaskIndex == rcRHS.iTaskIndex)
	{
		return rcLHS.enumCondType < rcRHS.enumCondType;
	}
	return rcLHS.iTaskIndex < rcRHS.iTaskIndex;
}

ostream & operator << (ostream & rcOS, const NoCFlowDLUSchedCond & rcLHS)
{
	rcOS << "{";
	rcOS << rcLHS.iTaskIndex << ", ";
	switch (rcLHS.enumCondType)
	{
	case NoCFlowDLUSchedCond::DeadlineLBLO:
		rcOS << "DLBLO";
		break;
	case NoCFlowDLUSchedCond::DeadlineUBLO:
		rcOS << "DUBLO";
		break;
	case NoCFlowDLUSchedCond::DeadlineLBHI:
		rcOS << "DLBHI";
		break;
	case NoCFlowDLUSchedCond::DeadlineUBHI:
		rcOS << "DUBHI";
		break;
	default:
		break;
	}
	rcOS << "}";
	return rcOS;
}

istream & operator >> (istream & rcIS, NoCFlowDLUSchedCond & rcRHS)
{
	char xChar;
	char axBuffer[32] = { 0 };
	rcRHS.iTaskIndex = -1;
	rcRHS.enumCondType = NoCFlowDLUSchedCond::DeadlineUBLO;
	NoCFlowDLUSchedCond cTemp;
	rcIS >> xChar;
	if (xChar != '{')	return rcIS;
	rcIS >> cTemp.iTaskIndex;
	rcIS >> xChar;
	if (xChar != ',')	return rcIS;
	rcIS >> axBuffer[0] >> axBuffer[1] >> axBuffer[2] >> axBuffer[3] >> axBuffer[4];
	if (axBuffer[0] != 'D')	return rcIS;
	if (axBuffer[2] != 'B')	return rcIS;
	if ((axBuffer[1] == 'U'))
	{
		if ((axBuffer[3] == 'H') && (axBuffer[4] == 'I'))
		{
			cTemp.enumCondType = NoCFlowDLUSchedCond::DeadlineUBHI;
		}
		else if ((axBuffer[3] == 'L') && (axBuffer[4] == 'O'))
		{
			cTemp.enumCondType = NoCFlowDLUSchedCond::DeadlineUBLO;
		}
		else
		{
			//
			cout << "Instream NoCFlowDLUSchedCond error. Do not proceed." << endl;
			while (1);
			return rcIS;
		}

	}
	else if ((axBuffer[1] == 'L'))
	{
		if ((axBuffer[3] == 'H') && (axBuffer[4] == 'I'))
		{
			cTemp.enumCondType = NoCFlowDLUSchedCond::DeadlineLBHI;
		}
		else if ((axBuffer[3] == 'L') && (axBuffer[4] == 'O'))
		{
			cTemp.enumCondType = NoCFlowDLUSchedCond::DeadlineLBLO;
		}
		else
		{
			cout << "Instream NoCFlowDLUSchedCond error. Do not proceed." << endl;
			while (1);
			return rcIS;
		}
	}
	else
	{
		cout << "Instream NoCFlowDLUSchedCond error. Do not proceed." << endl;
		while (1);
		return rcIS;
	}
	rcIS >> xChar;
	if (xChar != '}')
	{
		cout << "Instream NoCFlowDLUSchedCond error. Do not proceed." << endl;
		while (1);
		return rcIS;
	}
	rcRHS = cTemp;
	return rcIS;
}

NoCFlowDLUUnschedCoreComputer::NoCFlowDLUUnschedCoreComputer()
{

}

NoCFlowDLUUnschedCoreComputer::NoCFlowDLUUnschedCoreComputer(SystemType & rcSystem)
	:GeneralUnschedCoreComputer_MonoInt(rcSystem)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	m_vectorIJitterHI.reserve(iTaskNum);
	m_vectorIJitterLO.reserve(iTaskNum);
	m_vectorDeadlineLO.reserve(iTaskNum);
	m_vectorDeadlineHI.reserve(iTaskNum);
	m_vectorDeadlineLBHILB.reserve(iTaskNum);
	m_vectorDeadlineLBLOLB.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorIJitterLO.push_back(0);
		m_vectorIJitterHI.push_back(0);
		m_vectorDeadlineLO.push_back(0);
		m_vectorDeadlineHI.push_back(0);
		m_vectorDeadlineLBHILB.push_back(pcTaskArray[i].getCHI());
		m_vectorDeadlineLBLOLB.push_back(pcTaskArray[i].getCLO());
	}
	//InitializeDeadlineLBLB();
	m_cFixedPriorityAssignment = TaskSetPriorityStruct(rcSystem);
}

NoCFlowDLUUnschedCoreComputer::SchedCondContent NoCFlowDLUUnschedCoreComputer::SchedCondUpperBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	return pcTaskArray[rcSchedCondType.iTaskIndex].getDeadline();	
}

NoCFlowDLUUnschedCoreComputer::SchedCondContent NoCFlowDLUUnschedCoreComputer::SchedCondLowerBound(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	switch (rcSchedCondType.enumCondType)
	{
	case SchedCondType::DeadlineUBHI:
	case SchedCondType::DeadlineLBHI:
		//return pcTaskArray[rcSchedCondType.iTaskIndex].getCHI();
		return m_vectorDeadlineLBHILB[rcSchedCondType.iTaskIndex];
		break;					
	case SchedCondType::DeadlineUBLO:
	case SchedCondType::DeadlineLBLO:
		return m_vectorDeadlineLBLOLB[rcSchedCondType.iTaskIndex];		
		break;
	default:
		cout << "Unknown SchedCondType" << endl;
		break;
	}
	return -1;
}

NoCFlowDLUUnschedCoreComputer::Monotonicity NoCFlowDLUUnschedCoreComputer::SchedCondMonotonicity(const SchedCondType & rcSchedCondType)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	switch (rcSchedCondType.enumCondType)
	{
	case SchedCondType::DeadlineLBHI:
	case SchedCondType::DeadlineLBLO:	
		return Monotonicity::Down;
		break;
	case SchedCondType::DeadlineUBLO:
	case SchedCondType::DeadlineUBHI:
		return Monotonicity::Up;
		break;
	default:
		cout << "Unknown SchedCondType" << endl;
		break;
	}
	return Monotonicity::Up;
}

void NoCFlowDLUUnschedCoreComputer::IsSchedulablePreWork(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorIJitterLO[i] = SchedCondLowerBound(SchedCondType(i, SchedCondType::DeadlineLBLO)) - pcTaskArray[i].getCLO();
		m_vectorIJitterHI[i] = SchedCondLowerBound(SchedCondType(i, SchedCondType::DeadlineLBHI)) - pcTaskArray[i].getCLO();;
		m_vectorDeadlineLO[i] = pcTaskArray[i].getDeadline();
		m_vectorDeadlineHI[i] = pcTaskArray[i].getDeadline();
	}


	for (SchedConds::iterator iter = rcSchedCondsFlex.begin(); iter != rcSchedCondsFlex.end(); iter++)
	{
		switch (iter->first.enumCondType)
		{
		case SchedCondType::DeadlineLBLO:
			m_vectorIJitterLO[iter->first.iTaskIndex] = max(m_vectorIJitterLO[iter->first.iTaskIndex], iter->second - pcTaskArray[iter->first.iTaskIndex].getCLO());
			break;
		case SchedCondType::DeadlineUBLO:	
			m_vectorDeadlineLO[iter->first.iTaskIndex] = min(m_vectorDeadlineLO[iter->first.iTaskIndex], iter->second + 0.0);
			break;
		case SchedCondType::DeadlineLBHI:
			m_vectorIJitterHI[iter->first.iTaskIndex] = max(m_vectorIJitterHI[iter->first.iTaskIndex], iter->second - pcTaskArray[iter->first.iTaskIndex].getCLO());
			break;
		case SchedCondType::DeadlineUBHI:
			m_vectorDeadlineHI[iter->first.iTaskIndex] = min(m_vectorDeadlineHI[iter->first.iTaskIndex], iter->second + 0.0);
			break;
		default:
			cout << "Unknown sched cond type" << endl;
			while (1);
			break;
		}
	}

	for (SchedConds::iterator iter = rcSchedCondsFixed.begin(); iter != rcSchedCondsFixed.end(); iter++)
	{
		switch (iter->first.enumCondType)
		{
		case SchedCondType::DeadlineLBLO:
			m_vectorIJitterLO[iter->first.iTaskIndex] = max(m_vectorIJitterLO[iter->first.iTaskIndex], iter->second - pcTaskArray[iter->first.iTaskIndex].getCLO());
			break;
		case SchedCondType::DeadlineUBLO:
			m_vectorDeadlineLO[iter->first.iTaskIndex] = min(m_vectorDeadlineLO[iter->first.iTaskIndex], iter->second + 0.0);
			break;
		case SchedCondType::DeadlineLBHI:
			m_vectorIJitterHI[iter->first.iTaskIndex] = max(m_vectorIJitterHI[iter->first.iTaskIndex], iter->second - pcTaskArray[iter->first.iTaskIndex].getCLO());
			break;
		case SchedCondType::DeadlineUBHI:
			m_vectorDeadlineHI[iter->first.iTaskIndex] = min(m_vectorDeadlineHI[iter->first.iTaskIndex], iter->second + 0.0);
			break;
		default:
			cout << "Unknown sched cond type" << endl;
			while (1);
			break;
		}
	}

}

bool NoCFlowDLUUnschedCoreComputer::SchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	FlowsSet_2DNetwork & rcFlowsSet = *(FlowsSet_2DNetwork*)m_pcTaskSet;


	NoCFlowAMCRTCalculator_SpecifiedJitter cCalculator(rcFlowsSet,
		rcFlowsSet.getDirectJitterTable(), m_vectorIJitterLO, m_vectorIJitterHI, rcPriorityAssignment);
	double dResponseTimeLOBound = m_vectorDeadlineLO[iTaskIndex];
	double dResponseTimeHIBound = m_vectorDeadlineHI[iTaskIndex];
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRTLO = cCalculator.RTLO(iTaskIndex);
		double dRTHI_a = cCalculator.RT_a(iTaskIndex);
		double dRTHI_b = cCalculator.RT_b(iTaskIndex);
		double dRTHI_c = cCalculator.RT_c(iTaskIndex, dRTHI_b);
		double dWCRTHI = max(dRTHI_a, dRTHI_c);
		if ((dRTLO <= dResponseTimeLOBound) && (dWCRTHI <= dResponseTimeHIBound))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		double dRTLO = cCalculator.RTLO(iTaskIndex);
		if (dRTLO <= dResponseTimeLOBound)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

int NoCFlowDLUUnschedCoreComputer::DeriveDeadlineLBLB(SchedConds & rcDeadlineLBLB)
{
	vector<double> vectorDeadlineLBHI;
	vector<double> vectorDeadlineLBLO;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	vectorDeadlineLBHI.reserve(iTaskNum);
	vectorDeadlineLBLO.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorDeadlineLBLO.push_back(pcTaskArray[i].getCLO());
		rcDeadlineLBLB[SchedCondType(i, SchedCondType::DeadlineLBLO)] = pcTaskArray[i].getCLO();
		if (pcTaskArray[i].getCriticality())
		{
			rcDeadlineLBLB[SchedCondType(i, SchedCondType::DeadlineLBHI)] = pcTaskArray[i].getCHI();
			vectorDeadlineLBHI.push_back(pcTaskArray[i].getCHI());
		}
	}

	bool bChange = true;
	while (bChange)
	{		
		bChange = false;
		for (int i = 0; i < iTaskNum; i++)
		{
			SchedConds rcFlex;
			SchedCondType cUBKey = SchedCondType(i, SchedCondType::DeadlineUBLO);
			SchedCondType cLBKey = SchedCondType(i, SchedCondType::DeadlineLBLO);
			rcFlex[cUBKey] = pcTaskArray[i].getCLO() - 1;
			SchedCondContent & rcThisDeadlineLBLBValue = rcDeadlineLBLB[cLBKey];
			UnschedCores cUnschedCores;
			ComputeUnschedCores(rcFlex, rcDeadlineLBLB, cUnschedCores, -1);
			assert(cUnschedCores.size() <= 1);// With only one element in flex sched cond, there can be at most 1 unsched core.
			if ((cUnschedCores.size() < 1) || cUnschedCores.front().empty()) 
			{
				//This should indicate unschedulability.
				return -1;
			}
			else
			{
				SchedCondContent iNewValue = cUnschedCores.front().begin()->second + 1;
				if (iNewValue > rcThisDeadlineLBLBValue)
				{
					bChange = true;
					rcThisDeadlineLBLBValue = iNewValue;
				}
			}
		}

		//For HI Criticality Ones
		for (int i = 0; i < iTaskNum; i++)
		{
			if (!pcTaskArray[i].getCriticality())	continue;
			SchedConds rcFlex;
			SchedCondType cUBKey = SchedCondType(i, SchedCondType::DeadlineUBHI);
			SchedCondType cLBKey = SchedCondType(i, SchedCondType::DeadlineLBHI);
			rcFlex[cUBKey] = pcTaskArray[i].getCHI() - 1;
			SchedCondContent & rcThisDeadlineLBLBValue = rcDeadlineLBLB[cLBKey];
			UnschedCores cUnschedCores;
			ComputeUnschedCores(rcFlex, rcDeadlineLBLB, cUnschedCores, -1);
			assert(cUnschedCores.size() <= 1);// With only one element in flex sched cond, there can be at most 1 unsched core.
			if ((cUnschedCores.size() < 1) || cUnschedCores.front().empty())
			{
				//This should indicate unschedulability.
				return -1;
			}
			else
			{
				SchedCondContent iNewValue = cUnschedCores.front().begin()->second + 1;
				if (iNewValue > rcThisDeadlineLBLBValue)
				{
					bChange = true;
					rcThisDeadlineLBLBValue = iNewValue;
				}
			}
		}
	}
	return 0;
}

int NoCFlowDLUUnschedCoreComputer::DeriveDeadlineLBLB_v2(SchedConds & rcDeadlineLBLB)
{
	vector<double> vectorDeadlineLBHI;
	vector<double> vectorDeadlineLBLO;
	SchedConds cDummy;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	vectorDeadlineLBHI.reserve(iTaskNum);
	vectorDeadlineLBLO.reserve(iTaskNum);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorDeadlineLBLO.push_back(pcTaskArray[i].getCLO());
		rcDeadlineLBLB[SchedCondType(i, SchedCondType::DeadlineLBLO)] = pcTaskArray[i].getCLO();
		if (pcTaskArray[i].getCriticality())
		{
			rcDeadlineLBLB[SchedCondType(i, SchedCondType::DeadlineLBHI)] = pcTaskArray[i].getCHI();
			vectorDeadlineLBHI.push_back(pcTaskArray[i].getCHI());
		}
	}

	bool bChange = true;
	while (bChange)
	{
		bChange = false;
		for (int i = 0; i < iTaskNum; i++)
		{			
			SchedCondType cLBKey = SchedCondType(i, SchedCondType::DeadlineLBLO);
			SchedCondContent & rcThisDeadlineLBLBValue = rcDeadlineLBLB[cLBKey];
			TaskSetPriorityStruct cPA(*m_pcTaskSet);
			bool bStatus = HighestPriorityAssignment(i, rcDeadlineLBLB, cDummy, cPA);
			if (!bStatus)	return -1;
			NoCFlowAMCRTCalculator_SpecifiedJitter cCalculator(*m_pcTaskSet,
				m_pcTaskSet->getDirectJitterTable(), m_vectorIJitterLO, m_vectorIJitterHI, cPA);
			SchedCondContent iRT = cCalculator.RTLO(i);
			if (iRT > rcThisDeadlineLBLBValue)
			{
				rcThisDeadlineLBLBValue = iRT;
				bChange = true;
			}

			if (pcTaskArray[i].getCriticality())
			{
				SchedCondType cHILBKey = SchedCondType(i, SchedCondType::DeadlineLBHI);
				SchedCondContent & rcThisHIDeadlineLBLBValue = rcDeadlineLBLB[cHILBKey];
				SchedCondContent iRTHI = cCalculator.WCRTHI(i);
				if (iRTHI > rcThisHIDeadlineLBLBValue)
				{
					rcThisHIDeadlineLBLBValue = iRTHI;
					bChange = true;
				}
			}
		}
		
	}
	return 0;
}

void NoCFlowDLUUnschedCoreComputer::InitializeDeadlineLBLB()
{
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorDeadlineLBHILB[i] = pcTaskArray[i].getCHI();
		m_vectorDeadlineLBLOLB[i] = pcTaskArray[i].getCLO();
	}
#if 0
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "Task " << i << " Deadline LB: " << m_vectorDeadlineLBLOLB[i];
		if (pcTaskArray[i].getCriticality())	cout << " " << m_vectorDeadlineLBHILB[i];
		cout << endl;
	}
#endif

	SchedConds cSchedConds;
	int iStatus = DeriveDeadlineLBLB_v2(cSchedConds);
	for (SchedConds::iterator iter = cSchedConds.begin(); iter != cSchedConds.end(); iter++)
	{
		switch (iter->first.enumCondType)
		{
		case SchedCondType::DeadlineLBLO:
			m_vectorDeadlineLBLOLB[iter->first.iTaskIndex] = max(m_vectorDeadlineLBLOLB[iter->first.iTaskIndex], iter->second + 0.0);
			break;
		case SchedCondType::DeadlineLBHI:
			m_vectorDeadlineLBHILB[iter->first.iTaskIndex] = max(m_vectorDeadlineLBHILB[iter->first.iTaskIndex], iter->second + 0.0);
			break;
		default:
			cout << "Shouldn't have other type here" << endl;
			break;
		}
	}
#if 0
	cout << "--------------------------------------" << endl;
	cout << "Status: " << iStatus << endl;
	for (int i = 0; i < iTaskNum; i++)
	{
		cout << "Task " << i << " Deadline LB: " << m_vectorDeadlineLBLOLB[i];
		if (pcTaskArray[i].getCriticality())	cout << " " << m_vectorDeadlineLBHILB[i];
		cout << endl;
	}
#endif
}

void NoCFlowDLUUnschedCoreComputer::HighestPriorityAssignment_Prework(int iTaskIndex, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
}

bool NoCFlowDLUUnschedCoreComputer::IsSchedulable_AdaptiveLB(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	SchedConds cFlexCopy = rcSchedCondsFlex;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	bool bChange = true;	
	for (int i = 0; i < iTaskNum; i++)
	{
		SchedCondType cLBKey = SchedCondType(i, SchedCondType::DeadlineLBLO);
		if (rcSchedCondsFlex.count(cLBKey) == 0)
		{
			cFlexCopy[cLBKey] = SchedCondLowerBound(cLBKey);
		}

		if (pcTaskArray[i].getCriticality())
		{
			SchedCondType cHILBKey = SchedCondType(i, SchedCondType::DeadlineLBHI);
			if (rcSchedCondsFlex.count(cHILBKey) == 0)
			{
				cFlexCopy[cHILBKey] = SchedCondLowerBound(cHILBKey);
			}
		}
	}

	while (bChange)
	{
		bChange = false;
		for (int i = 0; i < iTaskNum; i++)
		{
			SchedCondType cLBKey = SchedCondType(i, SchedCondType::DeadlineLBLO);
			SchedCondContent & rcThisDeadlineLBLBValue = cFlexCopy[cLBKey];
			TaskSetPriorityStruct cPA(*m_pcTaskSet);
			bool bStatus = HighestPriorityAssignment(i, cFlexCopy, rcSchedCondsFixed, cPA);
			if (!bStatus)	return false;
			NoCFlowAMCRTCalculator_SpecifiedJitter cCalculator(*m_pcTaskSet,
				m_pcTaskSet->getDirectJitterTable(), m_vectorIJitterLO, m_vectorIJitterHI, cPA);
			SchedCondContent iRT = cCalculator.RTLO(i);
			if (iRT > rcThisDeadlineLBLBValue)
			{
				rcThisDeadlineLBLBValue = iRT;
				bChange = true;
			}

			if (pcTaskArray[i].getCriticality())
			{
				SchedCondType cHILBKey = SchedCondType(i, SchedCondType::DeadlineLBHI);
				SchedCondContent & rcThisHIDeadlineLBLBValue = cFlexCopy[cHILBKey];
				SchedCondContent iRTHI = cCalculator.WCRTHI(i);
				if (iRTHI > rcThisHIDeadlineLBLBValue)
				{
					rcThisHIDeadlineLBLBValue = iRTHI;
					bChange = true;
				}
			}
		}

	}
	return true;
}

double NoCFlowDLUUnschedCoreComputer::ComputePessimisticRT(int iTaskIndex, SchedConds & rcSchedCondsConfig, TaskSetPriorityStruct & rcPA)
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
			vectorJitterLO[iTaskIndex] = min(vectorJitterLO[iTaskIndex], iter->second + 0.0);
		}
		else if (iter->first.enumCondType == SchedCondType::DeadlineUBHI)
		{
			vectorJitterHI[iTaskIndex] = min(vectorJitterHI[iTaskIndex], iter->second + 0.0);
		}
		else
		{
			
		}
	}

	for (int i = 0; i < iTaskNum; i++)
	{
		//cout << "Task " << i << ": " << vectorJitterLO[i] << " " << vectorJitterHI[i] << endl;
		vectorJitterHI[i] = vectorJitterHI[i] - pcTaskArray[i].getCHI();
		vectorJitterLO[i] = vectorJitterLO[i] - pcTaskArray[i].getCLO();
	}

	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(*m_pcTaskSet, m_pcTaskSet->getDirectJitterTable(), vectorJitterLO, vectorJitterHI, rcPA);
		
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRTHI = cRTCalc.WCRTHI(iTaskIndex);
		return dRTHI;
	}
	else
	{
		double dRTLO = cRTCalc.RTLO(iTaskIndex);
		return dRTLO;
	}
	return -1;
}

double NoCFlowDLUUnschedCoreComputer::ComputeOptimisticRT(int iTaskIndex, SchedConds & rcSchedCondsConfig, TaskSetPriorityStruct & rcPA)
{
	vector<double> vectorJitterLO, vectorJitterHI;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorJitterLO.push_back(pcTaskArray[i].getCLO());
		vectorJitterHI.push_back(pcTaskArray[i].getCHI());
	}

	for (SchedConds::iterator iter = rcSchedCondsConfig.begin(); iter != rcSchedCondsConfig.end(); iter++)
	{
		int iTaskIndex = iter->first.iTaskIndex;
		if (iter->first.enumCondType == SchedCondType::DeadlineLBLO)
		{
			vectorJitterLO[iTaskIndex] = max(vectorJitterLO[iTaskIndex], iter->second + 0.0);
		}
		else if (iter->first.enumCondType == SchedCondType::DeadlineLBHI)
		{
			vectorJitterHI[iTaskIndex] = max(vectorJitterHI[iTaskIndex], iter->second + 0.0);
		}
		else
		{			
		}
	}

	for (int i = 0; i < iTaskNum; i++)
	{
		//cout << "Task " << i << ": " << vectorJitterLO[i] << " " << vectorJitterHI[i] << endl;
		vectorJitterHI[i] = vectorJitterHI[i] - pcTaskArray[i].getCHI();
		vectorJitterLO[i] = vectorJitterLO[i] - pcTaskArray[i].getCLO();
	}

	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(*m_pcTaskSet, m_pcTaskSet->getDirectJitterTable(), vectorJitterLO, vectorJitterHI, rcPA);
	double dRT = cRTCalc.WCRT(iTaskIndex);
	return dRT;
}

bool NoCFlowDLUUnschedCoreComputer::SchedTestPessimistic(int iTaskIndex, SchedConds & rcSchedCondsConfig, TaskSetPriorityStruct & rcPA)
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
			vectorJitterLO[iTaskIndex] = min(vectorJitterLO[iTaskIndex], iter->second + 0.0);
		}
		else if (iter->first.enumCondType == SchedCondType::DeadlineUBHI)
		{
			vectorJitterHI[iTaskIndex] = min(vectorJitterHI[iTaskIndex], iter->second + 0.0);
		}
		else
		{
		}
	}
	double dDeadlineLO = vectorJitterLO[iTaskIndex];
	double dDeadlineHI = vectorJitterHI[iTaskIndex];
	for (int i = 0; i < iTaskNum; i++)
	{
		//cout << "Task " << i << ": " << vectorJitterLO[i] << " " << vectorJitterHI[i] << endl;
		vectorJitterHI[i] = vectorJitterHI[i] - pcTaskArray[i].getCHI();
		vectorJitterLO[i] = vectorJitterLO[i] - pcTaskArray[i].getCLO();
	}



	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(*m_pcTaskSet, m_pcTaskSet->getDirectJitterTable(), vectorJitterLO, vectorJitterHI, rcPA);

	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRTHI = cRTCalc.WCRTHI(iTaskIndex);
		if (dRTHI > dDeadlineHI)
			return false;
	}

	{
		double dRTLO = cRTCalc.RTLO(iTaskIndex);
		if (dRTLO > dDeadlineLO)
			return false;
	}
	return true;
}

bool NoCFlowDLUUnschedCoreComputer::SchedTestOptimistic(int iTaskIndex, SchedConds & rcSchedCondsConfig, TaskSetPriorityStruct & rcPA)
{
	vector<double> vectorJitterLO, vectorJitterHI;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		vectorJitterLO.push_back(pcTaskArray[i].getCLO());
		vectorJitterHI.push_back(pcTaskArray[i].getCHI());
	}

	for (SchedConds::iterator iter = rcSchedCondsConfig.begin(); iter != rcSchedCondsConfig.end(); iter++)
	{
		int iTaskIndex = iter->first.iTaskIndex;
		if (iter->first.enumCondType == SchedCondType::DeadlineLBLO)
		{
			vectorJitterLO[iTaskIndex] = max(vectorJitterLO[iTaskIndex], iter->second + 0.0);
		}
		else if (iter->first.enumCondType == SchedCondType::DeadlineLBHI)
		{
			vectorJitterHI[iTaskIndex] = max(vectorJitterHI[iTaskIndex], iter->second + 0.0);
		}
		else
		{
		}
	}
	double dDeadlineLO = pcTaskArray[iTaskIndex].getDeadline();
	double dDeadlineHI = pcTaskArray[iTaskIndex].getDeadline();
	if (rcSchedCondsConfig.count(SchedCondType(iTaskIndex, SchedCondType::DeadlineUBLO)))
		dDeadlineLO = min(dDeadlineLO, rcSchedCondsConfig[SchedCondType(iTaskIndex, SchedCondType::DeadlineUBLO)] + 0.0 );
	if (rcSchedCondsConfig.count(SchedCondType(iTaskIndex, SchedCondType::DeadlineUBHI)))
		dDeadlineHI = min(dDeadlineHI, rcSchedCondsConfig[SchedCondType(iTaskIndex, SchedCondType::DeadlineUBHI)] + 0.0);
	for (int i = 0; i < iTaskNum; i++)
	{
		//cout << "Task " << i << ": " << vectorJitterLO[i] << " " << vectorJitterHI[i] << endl;
		vectorJitterHI[i] = vectorJitterHI[i] - pcTaskArray[i].getCHI();
		vectorJitterLO[i] = vectorJitterLO[i] - pcTaskArray[i].getCLO();
	}

	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(*m_pcTaskSet, m_pcTaskSet->getDirectJitterTable(), vectorJitterLO, vectorJitterHI, rcPA);
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRTHI = cRTCalc.WCRTHI(iTaskIndex);
		if (dRTHI > dDeadlineHI)
			return false;
	}

	{
		double dRTLO = cRTCalc.RTLO(iTaskIndex);
		if (dRTLO > dDeadlineLO)
			return false;
	}
	return true;
}

bool NoCFlowDLUUnschedCoreComputer::SchedTest_Public(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{
	IsSchedulablePreWork(rcSchedCondsFlex, rcSchedCondsFixed, rcPriorityAssignment);
	return SchedTest(iTaskIndex, rcPriorityAssignment, rcSchedCondsFlex, rcSchedCondsFixed);
}

int NoCFlowDLUUnschedCoreComputer::PreSchedTest(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment, bool & rbSchedStatus, SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed)
{	
#if 1
	if (!AudsleyPartialOrderBurden(iTaskIndex, rcPriorityAssignment, m_cEnforcedPartialOrder)) return 0;

	int iThisPriority = rcPriorityAssignment.getPriorityByTask(iTaskIndex);
	int iFixedTask = m_cFixedPriorityAssignment.getTaskByPriority(iThisPriority);
	
	//if the target task has a pre-assigned priority level
	int iFixedPriority = m_cFixedPriorityAssignment.getPriorityByTask(iTaskIndex);
	if (iFixedPriority != iThisPriority && iFixedPriority != -1)	return 0;

	//if current priority level is reserved for other task
	if (iTaskIndex != iFixedTask && iFixedTask != -1)	return 0;
	return 1;
#else
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	int iFixedPriority
#endif
}

bool NoCFlowDLUUnschedCoreComputer::IsSchedulablePessimistic(TaskSetPriorityStruct & rcPriorityAssignment)
{
	SchedConds cSchedConds, cDummy;
	GET_TASKSET_NUM_PTR(m_pcTaskSet, iTaskNum, pcTaskArray);
	for (int i = 0; i < iTaskNum; i++)
	{
		if (pcTaskArray[i].getCriticality())
		{
			cSchedConds[SchedCondType(i, SchedCondType::DeadlineUBHI)] = pcTaskArray[i].getDeadline();
			cSchedConds[SchedCondType(i, SchedCondType::DeadlineLBHI)] = pcTaskArray[i].getDeadline();
		}

		cSchedConds[SchedCondType(i, SchedCondType::DeadlineUBLO)] = pcTaskArray[i].getDeadline();
		cSchedConds[SchedCondType(i, SchedCondType::DeadlineLBLO)] = pcTaskArray[i].getDeadline();
	}
	return IsSchedulable(cSchedConds, cDummy, rcPriorityAssignment);
}

//NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA
NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA::NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA()
{

}

NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA::NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA(SystemType & rcSystem)
	:NoCFlowDLUUnschedCoreComputer(rcSystem)
{

}

bool NoCFlowDLUUnschedCoreComputer_AudsleyPartialPA::IsSchedulable(SchedConds & rcSchedCondsFlex, SchedConds & rcSchedCondsFixed, TaskSetPriorityStruct & rcPriorityAssignment)
{
	//Existing Assigned Task Must be schedulable
	return false;
}


//NoCPriorityAssignment_2008Burns
NoCPriorityAssignment_2008Burns::NoCPriorityAssignment_2008Burns()
{

}

NoCPriorityAssignment_2008Burns::NoCPriorityAssignment_2008Burns(FlowsSet_2DNetwork & rcFlowSet)
{
	m_pcFlowSet = &rcFlowSet;
	m_dStartTime = 0;
	m_dTimeout = -1;
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	m_vectorDirectJitter = vector<double>(iTaskNum, 0);
	m_vectorIJitterLO_LowerBound = vector<double>(iTaskNum, 0);
	m_vectorIJitterLO_UpperBound = vector<double>(iTaskNum, 0);
	m_vectorIJitterHI_LowerBound = vector<double>(iTaskNum, 0);
	m_vectorIJitterHI_UpperBound = vector<double>(iTaskNum, 0);
	for (int i = 0; i < iTaskNum; i++)
	{
		m_vectorDirectJitter[i] = 0;
		m_vectorIJitterLO_LowerBound[i] = 0;
		m_vectorIJitterLO_UpperBound[i] = pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO();
		m_vectorIJitterHI_LowerBound[i] = 0;
		m_vectorIJitterHI_UpperBound[i] = pcTaskArray[i].getDeadline() - pcTaskArray[i].getCLO();
	}

}

bool NoCPriorityAssignment_2008Burns::LowerBoundAnalysis(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);	
	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(*m_pcFlowSet, m_vectorDirectJitter, m_vectorIJitterLO_LowerBound, m_vectorIJitterHI_LowerBound, rcPriorityAssignment);
	assert(rcPriorityAssignment.getPriorityByTask(iTaskIndex) != -1);
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRT = cRTCalc.WCRTHI(iTaskIndex);
		bool bStatus = dRT <= pcTaskArray[iTaskIndex].getDeadline();
		return bStatus;
	}
	else
	{
		double dRT = cRTCalc.RTLO(iTaskIndex);
		bool bStatus = dRT <= pcTaskArray[iTaskIndex].getDeadline();
		return bStatus;
	}
}

bool NoCPriorityAssignment_2008Burns::UpperBoundAnalysis(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	NoCFlowAMCRTCalculator_SpecifiedJitter cRTCalc(*m_pcFlowSet, m_vectorDirectJitter, m_vectorIJitterLO_UpperBound, m_vectorIJitterHI_UpperBound, rcPriorityAssignment);
	assert(rcPriorityAssignment.getPriorityByTask(iTaskIndex) != -1);
	if (pcTaskArray[iTaskIndex].getCriticality())
	{
		double dRT = cRTCalc.WCRTHI(iTaskIndex);
		bool bStatus = dRT <= pcTaskArray[iTaskIndex].getDeadline();
		return bStatus;
	}
	else
	{
		double dRT = cRTCalc.RTLO(iTaskIndex);
		bool bStatus = dRT <= pcTaskArray[iTaskIndex].getDeadline();
		return bStatus;
	}
}

void NoCPriorityAssignment_2008Burns::setWCET(int iTaskIndex, bool bCriticality, double dWCET)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	double dOriginalCLO = pcTaskArray[iTaskIndex].getCLO();
	double dOriginalCHI = pcTaskArray[iTaskIndex].getCHI();
	if (bCriticality)
	{
		assert(pcTaskArray[iTaskIndex].getCriticality());
		double dNewCriticalityFactor = dWCET / dOriginalCLO;
		pcTaskArray[iTaskIndex].setCritiFactor(dNewCriticalityFactor);
		pcTaskArray[iTaskIndex].Construct();
	}
	else
	{
		double dNewCriticalityFactor = dOriginalCHI / dWCET;
		double dNewUtil = dWCET / pcTaskArray[iTaskIndex].getPeriod();
		pcTaskArray[iTaskIndex].setCritiFactor(dNewCriticalityFactor);
		pcTaskArray[iTaskIndex].setUtilization(dNewUtil);
		pcTaskArray[iTaskIndex].Construct();
	}
}

double NoCPriorityAssignment_2008Burns::Fitnesss(int iTaskIndex, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);	
	double dOriginalWCET = pcTaskArray[iTaskIndex].getCriticality() ? pcTaskArray[iTaskIndex].getCHI() : pcTaskArray[iTaskIndex].getCLO();
	//binary search?
	double dLB = dOriginalWCET;
	double dUB = pcTaskArray[iTaskIndex].getDeadline();
	while (dUB - dLB > 1)
	{
		double dCurrent = floor((dUB + dLB) / 2);
		setWCET(iTaskIndex, pcTaskArray[iTaskIndex].getCriticality(), dCurrent);
		bool bSchedTest = LowerBoundAnalysis(iTaskIndex, rcPriorityAssignment);
		if (bSchedTest)
		{
			dLB = dCurrent;
		}
		else
		{
			dUB = dCurrent;
		}
	}
	double dHigherPriorityUtil = 0;
	for (int i = 0; i < iTaskNum; i++)
	{
		if (i == iTaskIndex) continue;
		if (m_pcFlowSet->IsSharingRouter(i, iTaskIndex) && (rcPriorityAssignment.getPriorityByTask(i) < rcPriorityAssignment.getPriorityByTask(iTaskIndex)))
		{
			dHigherPriorityUtil += pcTaskArray[i].getUtilization();
		}
	}

	double dFitness = (dLB - dOriginalWCET) / dHigherPriorityUtil;
	setWCET(iTaskIndex, pcTaskArray[iTaskIndex].getCriticality(), dOriginalWCET);
	return dFitness;
}

int NoCPriorityAssignment_2008Burns::Recur(int iPriorityLevel, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	m_iNodesVisited++;
	//cout << "\r# Nodes Visited: " << m_iNodesVisited << "     ";	
	m_cTimer.StopThread();
	double dCurrentTime_ms = m_cTimer.getElapsedThreadTime_ms();
	if ((m_dTimeout > 0) && (dCurrentTime_ms >= m_dTimeout * 1000))
	{
		return -1;
	}
	deque<pair<int, double> > dequeLBCandidates;
	class Comp
	{
	public:
		bool operator () (const pair<int, double> & rcLHS, const pair<int, double> & rcRHS)
		{
			return rcLHS.second > rcRHS.second;
		}
	};
	if (iPriorityLevel < 0)
	{
		m_iExaminedPA++;
		if (m_iDisplay)
			cout << "\rExamined Priority Assignments: " << m_iExaminedPA << "          " ;
		NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(*m_pcFlowSet, rcPriorityAssignment, m_vectorDirectJitter);
		cRTCalc.ComputeAllResponseTime();
		bool bStatus = cRTCalc.IsSchedulable();
		if (bStatus) return 1;
		else return -2;		
	}
	for (int i = 0; i < iTaskNum; i++)
	{
		if (rcPriorityAssignment.getPriorityByTask(i) != -1) continue;
		rcPriorityAssignment.setPriority(i, iPriorityLevel);
		bool bUBSchedTest = UpperBoundAnalysis(i, rcPriorityAssignment);
		if (bUBSchedTest)
		{
			//cout << "UB passed" << endl;
			int iStatus = Recur(iPriorityLevel - 1, rcPriorityAssignment);
			if (iStatus == 1)	return 1;
			if (iStatus == -1)	return -1;
			if (iStatus == 0)	return 0;
		}		
		bool bLBSchedTest = LowerBoundAnalysis(i, rcPriorityAssignment);
		if (bLBSchedTest)
		{
			double dFitness = Fitnesss(i, rcPriorityAssignment);
			//double dFitness = 0;
			dequeLBCandidates.push_back({ i, dFitness });
		}
		rcPriorityAssignment.unset(i);
	}
	Comp cmp;
	if (dequeLBCandidates.empty())
	{
		//must be infeasible	
		return 0;
	}
	sort(dequeLBCandidates.begin(), dequeLBCandidates.end(), cmp);
	
	for (deque<pair<int, double> >::iterator iter = dequeLBCandidates.begin(); iter != dequeLBCandidates.end(); iter++)
	{
		rcPriorityAssignment.setPriority(iter->first, iPriorityLevel);
		//cout << "LB cand" << endl;
		int iStatus = Recur(iPriorityLevel - 1, rcPriorityAssignment);
		if (iStatus == 1)
		{
			return 1;
		}
		if (iStatus == -1)	return -1;	
		if (iStatus == 0)	return 0;
		rcPriorityAssignment.unset(iter->first);
	}
	return -2;
}

int NoCPriorityAssignment_2008Burns::Run(TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	m_dStartTime = time(NULL);
	m_dTime = 0;	
	m_cTimer.StartThread();	
	m_iNodesVisited = 0;
	m_iExaminedPA = 0;
	int iStatus = Recur(iTaskNum - 1, rcPriorityAssignment);
	m_cTimer.StopThread();
	m_dTime = m_cTimer.getElapsedThreadTime_ms();
	return iStatus;
}

NoCPriorityAssignment_2008Burns_MaxMinLaxity::NoCPriorityAssignment_2008Burns_MaxMinLaxity()
{

}

NoCPriorityAssignment_2008Burns_MaxMinLaxity::NoCPriorityAssignment_2008Burns_MaxMinLaxity(FlowsSet_2DNetwork & rcFlowSet)
	:NoCPriorityAssignment_2008Burns(rcFlowSet)
{
	m_dCurrentBest = -1;
	m_cBestAssignment = TaskSetPriorityStruct(rcFlowSet);
}

int NoCPriorityAssignment_2008Burns_MaxMinLaxity::Recur(int iPriorityLevel, TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	m_iNodesVisited++;
	//cout << "\r# Nodes Visited: " << m_iNodesVisited << "     ";	
	m_cTimer.StopThread();
	double dCurrentTime_ms = m_cTimer.getElapsedThreadTime_ms();
	if ((m_dTimeout > 0) && (dCurrentTime_ms >= m_dTimeout * 1000))
	{
		return -1;
	}
	deque<pair<int, double> > dequeLBCandidates;
	class Comp
	{
	public:
		bool operator () (const pair<int, double> & rcLHS, const pair<int, double> & rcRHS)
		{
			return rcLHS.second > rcRHS.second;
		}
	};
	if (iPriorityLevel < 0)
	{
		m_iExaminedPA++;
		if (m_iExaminedPA % 100000 == 0)
		{
			//cout << "\rExamined PA: " << m_iExaminedPA << "          ";			
		}
		m_cQueryLock.lock();
		NoCFlowAMCRTCalculator_DerivedJitter cRTCalc(*m_pcFlowSet, rcPriorityAssignment, m_vectorDirectJitter);
		cRTCalc.ComputeAllResponseTime();
		double dMinLaxity = pcTaskArray[0].getDeadline() - (pcTaskArray[0].getCriticality() ? cRTCalc.getRTHI(0) : cRTCalc.getRTLO(0));
		bool bStatus = cRTCalc.IsSchedulable();
		if (bStatus)
		{
			for (int i = 0; i < iTaskNum; i++)
			{
				dMinLaxity = min(dMinLaxity,
					pcTaskArray[i].getDeadline() - (pcTaskArray[i].getCriticality() ? cRTCalc.getRTHI(i) : cRTCalc.getRTLO(i)));
			}
			if (m_dCurrentBest == -1 || m_dCurrentBest < dMinLaxity)
			{
				m_dCurrentBest = dMinLaxity;
				m_cBestAssignment = rcPriorityAssignment;
				if (m_iDisplay)
					cout << "\r Best Result: " << dMinLaxity << "                          " << endl;				
				if (m_stringLogFile.empty() == false)
				{
					ofstream ofstreamOutputFile(m_stringLogFile.data(), ios::app);
					ofstreamOutputFile << currentDateTime().data() << endl;
					ofstreamOutputFile << "Best Result: " << dMinLaxity << endl;
					ofstreamOutputFile.flush();
					ofstreamOutputFile.close();
				}
			}
		}		
		m_cQueryLock.unlock();
		return -2;
	}
	for (int i = 0; i < iTaskNum; i++)
	{
		if (rcPriorityAssignment.getPriorityByTask(i) != -1) continue;
		rcPriorityAssignment.setPriority(i, iPriorityLevel);
		bool bUBSchedTest = UpperBoundAnalysis(i, rcPriorityAssignment);
		if (bUBSchedTest)
		{
			//cout << "UB passed" << endl;
			int iStatus = Recur(iPriorityLevel - 1, rcPriorityAssignment);			
			if (iStatus == -1)	return -1;
			if (iStatus == 0)	return 0;
		}
		bool bLBSchedTest = LowerBoundAnalysis(i, rcPriorityAssignment);
		if (bLBSchedTest)
		{
			double dFitness = Fitnesss(i, rcPriorityAssignment);
			//double dFitness = 0;
			dequeLBCandidates.push_back({ i, dFitness });
		}
		rcPriorityAssignment.unset(i);
	}
	Comp cmp;
	if (dequeLBCandidates.empty())
	{
		//must be infeasible	
		return 0;
	}
	sort(dequeLBCandidates.begin(), dequeLBCandidates.end(), cmp);

	for (deque<pair<int, double> >::iterator iter = dequeLBCandidates.begin(); iter != dequeLBCandidates.end(); iter++)
	{
		rcPriorityAssignment.setPriority(iter->first, iPriorityLevel);
		//cout << "LB cand" << endl;
		int iStatus = Recur(iPriorityLevel - 1, rcPriorityAssignment);		
		if (iStatus == -1)	return -1;
		if (iStatus == 0)	return 0;
		rcPriorityAssignment.unset(iter->first);
	}
	return 1;
}

int NoCPriorityAssignment_2008Burns_MaxMinLaxity::Run(TaskSetPriorityStruct & rcPriorityAssignment)
{
	GET_TASKSET_NUM_PTR(m_pcFlowSet, iTaskNum, pcTaskArray);
	if (m_stringLogFile.empty() == false)
	{
		ofstream ofstreamOutputFile(m_stringLogFile.data(), ios::app);
		ofstreamOutputFile << currentDateTime().data() << endl;
		ofstreamOutputFile << "Start" << endl;
		ofstreamOutputFile.flush();
		ofstreamOutputFile.close();
	}
	m_dCurrentBest = -1;
	m_dStartTime = time(NULL);
	m_dTime = 0;
	m_cTimer.StartThread();
	m_iNodesVisited = 0;
	m_iExaminedPA = 0;
	int iStatus = Recur(iTaskNum - 1, rcPriorityAssignment);
	m_cTimer.StopThread();
	m_dTime = m_cTimer.getElapsedThreadTime_ms();
	if (m_stringLogFile.empty() == false)
	{
		ofstream ofstreamOutputFile(m_stringLogFile.data(), ios::app);
		ofstreamOutputFile << currentDateTime().data() << endl;
		ofstreamOutputFile << "Best Result: " << m_dCurrentBest << endl;
		ofstreamOutputFile << "Total Time: " << m_dTime << endl;
		ofstreamOutputFile.flush();
		ofstreamOutputFile.close();
	}
	return iStatus;
}

void NoCPriorityAssignment_2008Burns_MaxMinLaxity::QueryCurrentBestSolution(volatile double & rdSol, TaskSetPriorityStruct & rcPASol)
{
	volatile double  dCopyt  = -1;
	m_cQueryLock.lock();
	dCopyt = m_dCurrentBest;
	rcPASol.CopyFrom_Strict(m_cBestAssignment);
	m_cQueryLock.unlock();
	rdSol = dCopyt;	
}