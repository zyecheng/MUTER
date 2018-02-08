#include "stdafx.h"
#include "BitMaskStorage_N_LowestSet.h"
#include <iomanip>
#include <assert.h>


BitMaskStorage_N_LowestSet::BitMaskStorage_N_LowestSet()
{
	m_cStorage = NULL;
	m_cCommonSetBit = NULL;
	m_cTotalSetBit = NULL;
}

void BitMaskStorage_N_LowestSet::Initialize(unsigned int uiWidth)
{
	m_uiWidth = uiWidth;
	m_uiSize = 0;
	m_vectorLowestSetLB.reserve(m_uiWidth + 1);
	m_vectorLowestSetUB.reserve(m_uiWidth + 1);
	for (unsigned int i = 0; i < m_uiWidth + 1; i++)
	{
		m_vectorLowestSetLB.push_back(m_uiWidth + 1);
		m_vectorLowestSetUB.push_back(m_uiWidth + 1);
		m_vectorCommonSetBit.push_back(BitMask());
		m_vectorTotalSetBit.push_back(BitMask());
	}

	m_cStorage = new BitMaskSetPtr1D[m_uiWidth + 1];
	for (int i = 0; i < m_uiWidth + 1; i++)
	{
		m_cStorage[i] = new BitMaskSetPtr[m_uiWidth + 1];
		for (int j = 0; j < m_uiWidth + 1; j++)
		{
			m_cStorage[i][j] = NULL;
		}
	}

	m_cCommonSetBit = new BitMask1D[m_uiWidth + 1];
	m_cTotalSetBit = new BitMask1D[m_uiWidth + 1];
	for (int i = 0; i < m_uiWidth + 1; i++)
	{
		m_cCommonSetBit[i] = new BitMask[m_uiWidth];
		m_cTotalSetBit[i] = new BitMask[m_uiWidth];
	}

	m_uiNSetLB = m_uiWidth + 1;
	m_uiNSetUB = m_uiWidth + 1;

	//InitializeMTMutexHandle();	
}

BitMaskStorage_N_LowestSet::~BitMaskStorage_N_LowestSet()
{
	if (m_cStorage)
	{
		for (unsigned int i = 0; i < m_uiWidth + 1; i++)
		{
			for (unsigned int j = 0; j < m_uiWidth; j++)
			{
				if (m_cStorage[i][j])
				{
					delete m_cStorage[i][j];
				}
			}
			delete m_cStorage[i];
		}
		delete m_cStorage;
		m_cStorage = NULL;
	}

	if (m_cCommonSetBit)
	{
		for (unsigned int i = 0; i < m_uiWidth + 1; i++)
		{
			delete[] m_cCommonSetBit[i];
		}
		delete m_cCommonSetBit;
		m_cCommonSetBit = NULL;
	}

	if (m_cTotalSetBit)
	{
		for (unsigned int i = 0; i < m_uiWidth + 1; i++)
		{
			delete[] m_cTotalSetBit[i];
		}
		delete m_cTotalSetBit;
		m_cTotalSetBit = NULL;
	}

	//DestroyMTMutexHandle();	
}

void BitMaskStorage_N_LowestSet::Insert(BitMask & rcBitMask)
{
	rcBitMask.ConstructBitMaskData();
	assert(rcBitMask.m_iBitChange == 0);
	unsigned int iFirstIndex = rcBitMask.m_iNBitSnapshot;
	unsigned int iSecondIndex = rcBitMask.m_uiLowestSetBit;

	if (iSecondIndex == m_uiWidth + 1)
	{
		m_iZeroBitMask = 1;
		return;
	}

	if (!m_cStorage[iFirstIndex][iSecondIndex])
	{
		CreateBitMaskSet(iFirstIndex, iSecondIndex);
	}

	std::pair<BitMaskSet::iterator, bool> cStatus = m_cStorage[iFirstIndex][iSecondIndex]->insert(rcBitMask);
	if (!cStatus.second)	return;

	UpdateTotalCommonSetBit(rcBitMask);
	UpdateBoundData(rcBitMask);
}

void BitMaskStorage_N_LowestSet::CreateBitMaskSet(unsigned int iNBit, unsigned int iLowestSetBit)
{
	m_cStorage[iNBit][iLowestSetBit] = new BitMaskSet;	
}

void BitMaskStorage_N_LowestSet::UpdateBoundData(BitMask & rcBitMask)
{
	unsigned int iFirstIndex = rcBitMask.m_iNBitSnapshot;
	unsigned int iSecondIndex = rcBitMask.m_uiLowestSetBit;

	m_vectorLowestSetLB[iFirstIndex] = m_vectorLowestSetLB[iFirstIndex] == (m_uiWidth + 1) ?
	iSecondIndex : min(m_vectorLowestSetLB[iFirstIndex], iSecondIndex);
	m_vectorLowestSetUB[iFirstIndex] = m_vectorLowestSetUB[iFirstIndex] == (m_uiWidth + 1) ?
	iSecondIndex : max(m_vectorLowestSetUB[iFirstIndex], iSecondIndex);
	m_uiNSetLB = (m_uiNSetLB == m_uiWidth + 1) ? iFirstIndex : min(m_uiNSetLB, iFirstIndex);
	m_uiNSetUB = (m_uiNSetUB == m_uiWidth + 1) ? iFirstIndex : max(m_uiNSetUB, iFirstIndex);
	m_uiSize++;
}

void BitMaskStorage_N_LowestSet::UpdateTotalCommonSetBit(BitMask & rcBitMask)
{
	unsigned int iFirstIndex = rcBitMask.m_iNBitSnapshot;
	unsigned int iSecondIndex = rcBitMask.m_uiLowestSetBit;

	if (m_vectorCommonSetBit[iFirstIndex].getWidth())
	{
		m_vectorCommonSetBit[iFirstIndex] = m_vectorCommonSetBit[iFirstIndex] & rcBitMask;
		m_vectorCommonSetBit[iFirstIndex].ConstructBitMaskData();
	}
	else
	{
		m_vectorCommonSetBit[iFirstIndex] = rcBitMask;
	}

	if (m_vectorTotalSetBit[iFirstIndex].getWidth())
	{
		m_vectorTotalSetBit[iFirstIndex] = m_vectorTotalSetBit[iFirstIndex] | rcBitMask;
		m_vectorTotalSetBit[iFirstIndex].ConstructBitMaskData();
	}
	else
	{
		m_vectorTotalSetBit[iFirstIndex] = rcBitMask;
	}

	if (m_cCommonSetBit[iFirstIndex][iSecondIndex].getWidth())
	{
		m_cCommonSetBit[iFirstIndex][iSecondIndex] = m_cCommonSetBit[iFirstIndex][iSecondIndex] & rcBitMask;
		m_cCommonSetBit[iFirstIndex][iSecondIndex].ConstructBitMaskData();
	}
	else
	{
		m_cCommonSetBit[iFirstIndex][iSecondIndex] = rcBitMask;
	}

	if (m_cTotalSetBit[iFirstIndex][iSecondIndex].getWidth())
	{
		m_cTotalSetBit[iFirstIndex][iSecondIndex] = m_cTotalSetBit[iFirstIndex][iSecondIndex] | rcBitMask;
		m_cTotalSetBit[iFirstIndex][iSecondIndex].ConstructBitMaskData();
	}
	else
	{
		m_cTotalSetBit[iFirstIndex][iSecondIndex] = rcBitMask;
	}
}

bool BitMaskStorage_N_LowestSet::Find(BitMask & rcBitMask)
{
	assert(rcBitMask.m_iBitChange == 0);
	unsigned int iFirstIndex = rcBitMask.m_iNBitSnapshot;
	unsigned int iSecondIndex = rcBitMask.m_uiLowestSetBit;
	if (iSecondIndex == m_uiWidth + 1)
	{
		return m_iZeroBitMask == 1;
	}

	if (!m_cStorage[iFirstIndex][iSecondIndex])
		return false;
	return m_cStorage[iFirstIndex][iSecondIndex]->count(rcBitMask) == 1;
}

bool BitMaskStorage_N_LowestSet::ExistSubSet(BitMask & rcBitMask, int * piNAttempts, int iLimit)
{
	if (m_uiSize == 0)
		return false;
	assert(rcBitMask.m_iBitChange == 0);
	//cout << rcBitMask << endl;
	if (m_iZeroBitMask == 1)
	{
		return true;
	}
	else if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
	{
		return false;
	}
	int iNAttempts = 0;
	for (unsigned int i = m_uiNSetLB; i < rcBitMask.m_iNBitSnapshot; i++)
	{
		if (m_vectorLowestSetUB[i] == m_uiWidth + 1)	continue;
		if (!(m_vectorCommonSetBit[i] <= rcBitMask))	continue;
		for (unsigned int j = rcBitMask.m_uiLowestSetBit; j <= m_vectorLowestSetUB[i]; j++)
		{
			if (!m_cStorage[i][j]) continue;
			if (!(m_cCommonSetBit[i][j] <= rcBitMask)) continue;			
			BitMaskSet & rcBitMaskSet = *m_cStorage[i][j];
			//cout << rcBitMaskSet.size() << endl;
			BitMaskSet::iterator iterEnd = rcBitMaskSet.upper_bound(rcBitMask);
			for (BitMaskSet::iterator iter = rcBitMaskSet.begin(); iter != iterEnd; iter++)
			{
				iNAttempts++;
				if ((BitMask &)(*iter) <= rcBitMask)
				{
					m_cSupSubSetQueryCertificate = *iter;
					if (piNAttempts)	*piNAttempts = iNAttempts;
					return true;
				}		
				if (iNAttempts > iLimit)
				{
					if (piNAttempts)	*piNAttempts = iNAttempts;
					return false;
				}
			}
		}
	}
	if (piNAttempts)	*piNAttempts = iNAttempts;
	return false;
}

bool BitMaskStorage_N_LowestSet::ExistSupSet(BitMask & rcBitMask, int * piNAttempts, int iLimit)
{
	if (m_uiSize == 0)
		return false;
	assert(rcBitMask.m_iBitChange == 0);
	if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
		return true;

	int iNAttempts = 0;
	for (int i = m_uiNSetUB; i > rcBitMask.m_iNBitSnapshot; i--)
	{
		if (!(m_vectorTotalSetBit[i] >= rcBitMask))	continue;
		for (int j = m_vectorLowestSetLB[i]; j <= rcBitMask.m_uiLowestSetBit; j++)
		{
			if (!m_cStorage[i][j]) continue;
			if (!(m_cTotalSetBit[i][j] >= rcBitMask)) continue;
			BitMaskSet & rcBitMaskSet = *m_cStorage[i][j];
			BitMaskSet::iterator iter = rcBitMaskSet.upper_bound(rcBitMask);			
			for (; iter != rcBitMaskSet.end(); iter++)
			{		
				iNAttempts++;
				if ((BitMask &)(*iter) >= rcBitMask)
				{
					m_cSupSubSetQueryCertificate = *iter;
					if (piNAttempts)	*piNAttempts = iNAttempts;
					return true;
				}

				if (iNAttempts > iLimit)
				{
					if (piNAttempts)	*piNAttempts = iNAttempts;
					return false;
				}
			}
		}
	}
	if (piNAttempts)	*piNAttempts = iNAttempts;
	return false;
}

bool BitMaskStorage_N_LowestSet::ExistProperSubSet(BitMask & rcBitMask, int * piNAttempts, int iLimit)
{
	if (m_uiSize == 0)
		return false;
	assert(rcBitMask.m_iBitChange == 0);
	//cout << rcBitMask << endl;
	if (m_iZeroBitMask == 1)
	{
		return true;
	}
	else if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
	{
		return false;
	}
	int iNAttempts = 0;
	for (unsigned int i = m_uiNSetLB; i < rcBitMask.m_iNBitSnapshot; i++)
	{
		if (m_vectorLowestSetUB[i] == m_uiWidth + 1)	continue;
		if (!(m_vectorCommonSetBit[i] <= rcBitMask))	continue;
		for (unsigned int j = rcBitMask.m_uiLowestSetBit; j <= m_vectorLowestSetUB[i]; j++)
		{
			if (!m_cStorage[i][j]) continue;
			if (!(m_cCommonSetBit[i][j] <= rcBitMask)) continue;
			BitMaskSet & rcBitMaskSet = *m_cStorage[i][j];
			//cout << rcBitMaskSet.size() << endl;
			BitMaskSet::iterator iterEnd = rcBitMaskSet.upper_bound(rcBitMask);
			for (BitMaskSet::iterator iter = rcBitMaskSet.begin(); iter != iterEnd; iter++)
			{
				iNAttempts++;
				if ((BitMask &)(*iter) < rcBitMask)
				{
					m_cSupSubSetQueryCertificate = *iter;
					if (piNAttempts)	*piNAttempts = iNAttempts;
					return true;
				}
				if (iNAttempts > iLimit)
				{
					if (piNAttempts)	*piNAttempts = iNAttempts;
					return false;
				}
			}
		}
	}
	if (piNAttempts)	*piNAttempts = iNAttempts;
	return false;
}

bool BitMaskStorage_N_LowestSet::ExistProperSupSet(BitMask & rcBitMask, int * piNAttempts, int iLimit)
{
	if (m_uiSize == 0)
		return false;
	assert(rcBitMask.m_iBitChange == 0);
	if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
		return true;

	int iNAttempts = 0;
	for (int i = m_uiNSetUB; i > rcBitMask.m_iNBitSnapshot; i--)
	{
		if (!(m_vectorTotalSetBit[i] >= rcBitMask))	continue;
		for (int j = m_vectorLowestSetLB[i]; j <= rcBitMask.m_uiLowestSetBit; j++)
		{
			if (!m_cStorage[i][j]) continue;
			if (!(m_cTotalSetBit[i][j] >= rcBitMask)) continue;
			BitMaskSet & rcBitMaskSet = *m_cStorage[i][j];
			BitMaskSet::iterator iter = rcBitMaskSet.upper_bound(rcBitMask);
			for (; iter != rcBitMaskSet.end(); iter++)
			{
				iNAttempts++;
				if ((BitMask &)(*iter) > rcBitMask)
				{
					m_cSupSubSetQueryCertificate = *iter;
					if (piNAttempts)	*piNAttempts = iNAttempts;
					return true;
				}

				if (iNAttempts > iLimit)
				{
					if (piNAttempts)	*piNAttempts = iNAttempts;
					return false;
				}
			}
		}
	}
	if (piNAttempts)	*piNAttempts = iNAttempts;
	return false;
}

int BitMaskStorage_N_LowestSet::SubsetHeuristicLimit()
{
	int iRatioSize = ceil((double)m_uiSize * 0.20);
	return max(iRatioSize, 10);
}

int BitMaskStorage_N_LowestSet::SupsetHeuristicLimit()
{
	int iRatioSize = ceil((double)m_uiSize * 0.40);
	return max(iRatioSize, 10);
}


//BitMaskStorage_N_Longest1String_LowestSet

BitMaskStorage_N_Longest1String_LowestSet::BitMaskStorage_N_Longest1String_LowestSet()
{
	m_uiHasFullBitMask = 0;
	m_uiHasZeroBitMask = 0;
	m_uiWidth = 0;
}


BitMaskStorage_N_Longest1String_LowestSet::~BitMaskStorage_N_Longest1String_LowestSet()
{

}

void BitMaskStorage_N_Longest1String_LowestSet::Initialize(unsigned int uiWidth)
{
	m_uiWidth = uiWidth;
	m_cNEntrances.Create(m_uiWidth + 1);
	m_uiSize = 0;
}

void BitMaskStorage_N_Longest1String_LowestSet::Insert(BitMask & rcBitMask)
{	
	//cout << "Insert Start" << endl;
	assert(rcBitMask.m_iBitChange == 0);

	if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
	{
		m_uiHasFullBitMask = 1;
		return;
	}
	else if (rcBitMask.m_iNBitSnapshot == m_uiWidth)
	{
		m_uiHasZeroBitMask = 1;
		return;
	}

	if (!m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances)
	{
		m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].Create(m_uiWidth + 1);
		m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].Create(m_uiWidth + 1);
		m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances[rcBitMask.m_uiLongest1String].Create();
		
	}
	else if (!m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances)
	{
		m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].Create(m_uiWidth + 1);
		m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances[rcBitMask.m_uiLongest1String].Create();
	}
	else if (!m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances[rcBitMask.m_uiLongest1String].pcBitMaskSet)
	{
		m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances[rcBitMask.m_uiLongest1String].Create();
	}
	std::pair<BitMaskSet::iterator, bool > cResult = m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances[rcBitMask.m_uiLongest1String].pcBitMaskSet->insert(rcBitMask);
	if (cResult.second)
		m_uiSize++;
	UpdateElementLink(rcBitMask);
	//cout << "Insert End" << endl;
}

void BitMaskStorage_N_Longest1String_LowestSet::UpdateElementLink(BitMask & rcBitMask)
{
	m_cNEntrances.cElementLink.Insert(rcBitMask.m_iNBitSnapshot);
	m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].cElementLink.Insert(rcBitMask.m_uiLowestSetBit);
	m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].cElementLink.Insert(rcBitMask.m_uiLongest1String);
}


bool BitMaskStorage_N_Longest1String_LowestSet::Find(BitMask & rcBitMask)
{
	//cout << "Find Start" << endl;
	if (!m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances)
	{
		//cout << "Find End" << endl;
		return false;
	}
	else if (!m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances)
	{
		//cout << "Find End" << endl;
		return false;
	}
	else if (!m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances[rcBitMask.m_uiLongest1String].pcBitMaskSet)
	{
		//cout << "Find End" << endl;
		return false;
	}
	//cout << "Find End" << endl;
	return m_cNEntrances.pcEntrances[rcBitMask.m_iNBitSnapshot].pcEntrances[rcBitMask.m_uiLowestSetBit].pcEntrances[rcBitMask.m_uiLongest1String].pcBitMaskSet->count(rcBitMask) == 1;
}

bool BitMaskStorage_N_Longest1String_LowestSet::ExistSubSet(BitMask & rcBitMask, int * piNAttempts)
{
	//cout << "Sub Start" << endl;
	assert(rcBitMask.m_iBitChange == 0);

	if (m_uiHasZeroBitMask)
		return true;

	if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
	{
		return false;
	}
	bool bStatus = false;
	int iNAttempts = 0;
#if 0		
	for (int i = m_cNEntrances.cElementLink.getStart(); i != -1; i = m_cNEntrances.cElementLink.getNext(i))
	{
		if (i >= rcBitMask.m_iNBitSnapshot) break;
		if (!m_cNEntrances.pcEntrances[i].pcEntrances) continue;				
		SparseElementLinkList & cLowestSetElementLink = m_cNEntrances.pcEntrances[i].cElementLink;
		for (int j = cLowestSetElementLink.getNextOrSelf(rcBitMask.m_uiLowestSetBit); j != -1; j = cLowestSetElementLink.getNext(j))
		{
			//if (j < rcBitMask.m_uiLowestSetBit) break;
			if (!m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances)	continue;						
			SparseElementLinkList & cLongest1sElementLink = m_cNEntrances.pcEntrances[i].pcEntrances[j].cElementLink;
			for (int k = cLongest1sElementLink.getStart(); k != -1; k = cLongest1sElementLink.getNext(k))
			{				
				if (k > rcBitMask.m_uiLongest1String) break;
				if (!m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances[k].pcBitMaskSet)	continue;
				BitMaskSet & rcBitMaskSet = *m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances[k].pcBitMaskSet;
				BitMaskSet::iterator iterEnd = rcBitMaskSet.upper_bound(rcBitMask);
				//BitMaskSet::iterator iterEnd = rcBitMaskSet.end();
				for (BitMaskSet::iterator iter = rcBitMaskSet.begin(); iter != iterEnd; iter++)
				{
					iNAttempts++;
					if ((BitMask &)*iter <= rcBitMask)
					{						
						bStatus = true;
						goto END;
					}
				}
			}
		}	
	}
END:	
	//cout << "Sub End" << endl;
#else
	bool bStatusTest = false;
	for (unsigned int i = 0; i < rcBitMask.m_iNBitSnapshot; i++)
	{
		if (!m_cNEntrances.pcEntrances[i].pcEntrances) continue;
		for (unsigned int j = rcBitMask.m_uiLowestSetBit; j <= m_uiWidth; j++)
		{
			if (!m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances)	continue;
			for (unsigned int k = 1; k <= rcBitMask.m_uiLongest1String; k++)
			{
				if (!m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances[k].pcBitMaskSet)	continue;
				BitMaskSet & rcBitMaskSet = *m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances[k].pcBitMaskSet;
				//BitMaskSet::iterator iterEnd = rcBitMaskSet.upper_bound(rcBitMask);
				BitMaskSet::iterator iterEnd = rcBitMaskSet.end();
				for (BitMaskSet::iterator iter = rcBitMaskSet.begin(); iter != iterEnd; iter++)
				{
					iNAttempts++;
					if ((BitMask &)*iter <= rcBitMask)
					{
						bStatusTest = true;
						goto ENDTest;
					}
				}
			}
		}
	}
ENDTest:
	//assert(bStatusTest == bStatus);
	bStatus = bStatusTest;
#endif
	if (piNAttempts)	*piNAttempts = iNAttempts;
	return bStatus;
}

bool BitMaskStorage_N_Longest1String_LowestSet::ExistSupSet(BitMask & rcBitMask, int * piNAttempts)
{
	//cout << "Sup Start" << endl;
	assert(rcBitMask.m_iBitChange == 0);
	if (m_uiHasFullBitMask)
		return true;
	if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
		return true;
	if (rcBitMask.m_iNBitSnapshot == m_uiWidth)
		return false;
	bool bStatus = false;
	int iNAttempts = 0;
#if 0	
	for (int i = m_cNEntrances.cElementLink.getStartReverse(); i != -1; i = m_cNEntrances.cElementLink.getNextReverse(i))
	{		
		if (i < rcBitMask.m_iNBitSnapshot) break;
		if (!m_cNEntrances.pcEntrances[i].pcEntrances) continue;						
		SparseElementLinkList & cLowestSetElementLink = m_cNEntrances.pcEntrances[i].cElementLink;
		for (int j = cLowestSetElementLink.getStart(); j != -1; j = cLowestSetElementLink.getNext(j))		
		{
			if (j > rcBitMask.m_uiLowestSetBit)	break;
			if (!m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances)	continue;						
			SparseElementLinkList & cLongest1sElementLink = m_cNEntrances.pcEntrances[i].pcEntrances[j].cElementLink;
			for (int k = cLongest1sElementLink.getNextOrSelf(rcBitMask.m_uiLongest1String); k != -1; k = cLongest1sElementLink.getNext(k))
			{
				if (!m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances[k].pcBitMaskSet)	continue;
				BitMaskSet & rcBitMaskSet = *m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances[k].pcBitMaskSet;
				//BitMaskSet::iterator iterBegin = rcBitMaskSet.begin();
				BitMaskSet::iterator iterBegin = rcBitMaskSet.upper_bound(rcBitMask);
				BitMaskSet::iterator iterEnd = rcBitMaskSet.end();
				for (BitMaskSet::iterator iter = iterBegin; iter != iterEnd; iter++)
				{
					iNAttempts++;
					if ((BitMask &)*iter >= rcBitMask)
					{						
						bStatus = true;
						goto END;
					}
				}
			}
		}
	}
END:	
#else
	bool bStatusTest = false;
	for (unsigned int i = m_uiWidth; i > rcBitMask.m_iNBitSnapshot; i--)
	{
		if (!m_cNEntrances.pcEntrances[i].pcEntrances) continue;
		for (unsigned int j = 0; j <= rcBitMask.m_uiLowestSetBit; j++)
		{
			if (!m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances)	continue;
			for (int k = rcBitMask.m_uiLongest1String; k <= m_uiWidth; k++)
			{
				if (!m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances[k].pcBitMaskSet)	continue;
				BitMaskSet & rcBitMaskSet = *m_cNEntrances.pcEntrances[i].pcEntrances[j].pcEntrances[k].pcBitMaskSet;
				BitMaskSet::iterator iterBegin = rcBitMaskSet.begin();
				//BitMaskSet::iterator iterBegin = rcBitMaskSet.upper_bound(rcBitMask);
				BitMaskSet::iterator iterEnd = rcBitMaskSet.end();
				for (BitMaskSet::iterator iter = iterBegin; iter != iterEnd; iter++)
				{
					iNAttempts++;
					if ((BitMask &)*iter >= rcBitMask)
					{
						bStatusTest = true;
						goto ENDTEST;
					}
				}
			}
		}
	}
ENDTEST:
	bStatus = bStatusTest;
	assert(bStatusTest == bStatus);
#endif	
	if (piNAttempts)	*piNAttempts = iNAttempts;
	return bStatus;
}
