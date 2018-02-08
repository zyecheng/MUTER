#pragma once
#include "BitMask.h"
#include "MySet.h"
#include "Util.h"
class BitMaskStorage_N_LowestSet
{
public:
	typedef BitMask::DataType DataType;
	typedef MySet<BitMask, BitMask::ValueComparator > BitMaskSet;
	//typedef MultiDimensionObjectArray<BitMaskSet, 2> BitMaskSet2D;

	typedef BitMaskSet* BitMaskSetPtr;
	typedef BitMaskSetPtr* BitMaskSetPtr1D;
	typedef BitMaskSetPtr1D* BitMaskSetPtr2D;

	typedef BitMask * BitMask1D;
	typedef BitMask1D * BitMask2D;
	BitMask m_cSupSubSetQueryCertificate;
protected:
	unsigned int m_uiWidth;
	BitMaskSetPtr2D m_cStorage;
	BitMaskSet m_cSubSetResultCache;
	BitMaskSet m_cSupSetResultCache;
	BitMask2D m_cCommonSetBit;
	BitMask2D m_cTotalSetBit;
	int m_iZeroBitMask;
	vector<unsigned int> m_vectorHighestSetLB;
	vector<unsigned int> m_vectorHighestSetUB;
	vector<unsigned int> m_vectorLowestSetLB;
	vector<unsigned int> m_vectorLowestSetUB;
	vector<BitMask> m_vectorCommonSetBit;
	vector<BitMask> m_vectorTotalSetBit;
	unsigned int m_uiSize;
	unsigned int m_uiNSetLB;
	unsigned int m_uiNSetUB;


public:
	BitMaskStorage_N_LowestSet();
	~BitMaskStorage_N_LowestSet();
	void Insert(BitMask & rcBitMask);
	bool Find(BitMask & rcBitMask);
	bool ExistSubSet(BitMask & rcBitMask, int * piNAttempts = NULL, int iLimit = 0x7fffffff);
	bool ExistSupSet(BitMask & rcBitMask, int * piNAttempts = NULL, int iLimit = 0x7fffffff);
	bool ExistProperSubSet(BitMask & rcBitMask, int * piNAttempts = NULL, int iLimit = 0x7fffffff);
	bool ExistProperSupSet(BitMask & rcBitMask, int * piNAttempts = NULL, int iLimit = 0x7fffffff);
	void Initialize(unsigned int uiWidth);
	bool ExistSubSupSetMT(BitMask & rcBitMask, int iQueryType);
	int getSize()	{ return m_uiSize; }
	int SubsetHeuristicLimit();
	int SupsetHeuristicLimit();
protected:
	void CreateBitMaskSet(unsigned int iNBit, unsigned int iLowestSetBit);
	void UpdateTotalCommonSetBit(BitMask & rcBitMask);
	void UpdateBoundData(BitMask & rcBitMask);	
	
};

template < class ExtraDataT >
class BitMaskStorageData_N_LowestSet
{
public:	
	typedef ExtraDataT ExtraData;
	typedef BitMask::DataType DataType;
	typedef map<BitMask, ExtraData, BitMask::ValueComparator > BitMaskSet;
	//typedef MultiDimensionObjectArray<BitMaskSet, 2> BitMaskSet2D;

	typedef BitMaskSet* BitMaskSetPtr;
	typedef BitMaskSetPtr* BitMaskSetPtr1D;
	typedef BitMaskSetPtr1D* BitMaskSetPtr2D;

	typedef BitMask * BitMask1D;
	typedef BitMask1D * BitMask2D;
	BitMask m_cSupSubSetQueryCertificate;
protected:
	unsigned int m_uiWidth;
	BitMaskSetPtr2D m_cStorage;
	BitMaskSet m_cSubSetResultCache;
	BitMaskSet m_cSupSetResultCache;
	BitMask2D m_cCommonSetBit;
	BitMask2D m_cTotalSetBit;
	int m_iZeroBitMask;
	vector<unsigned int> m_vectorHighestSetLB;
	vector<unsigned int> m_vectorHighestSetUB;
	vector<unsigned int> m_vectorLowestSetLB;
	vector<unsigned int> m_vectorLowestSetUB;
	vector<BitMask> m_vectorCommonSetBit;
	vector<BitMask> m_vectorTotalSetBit;
	unsigned int m_uiSize;
	unsigned int m_uiNSetLB;
	unsigned int m_uiNSetUB;


public:
	BitMaskStorageData_N_LowestSet()
	{
		m_cStorage = NULL;
		m_cCommonSetBit = NULL;
		m_cTotalSetBit = NULL;
	}

	~BitMaskStorageData_N_LowestSet()
	{
		if (m_cStorage)
		{
			for (int i = 0; i < m_uiWidth + 1; i++)
			{
				for (int j = 0; j < m_uiWidth; j++)
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
			for (int i = 0; i < m_uiWidth + 1; i++)
			{
				delete[] m_cCommonSetBit[i];
			}
			delete m_cCommonSetBit;
			m_cCommonSetBit = NULL;
		}

		if (m_cTotalSetBit)
		{
			for (int i = 0; i < m_uiWidth + 1; i++)
			{
				delete[] m_cTotalSetBit[i];
			}
			delete m_cTotalSetBit;
			m_cTotalSetBit = NULL;
		}
	}

	void Insert(BitMask & rcBitMask, ExtraData & rcNewData)
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

		//std::pair<typename BitMaskSet::iterator, bool> cStatus = m_cStorage[iFirstIndex][iSecondIndex]->insert(rcBitMask);
		std::pair<typename BitMaskSet::iterator, bool> cStatus = m_cStorage[iFirstIndex][iSecondIndex]->insert({rcBitMask, rcNewData});
		if (!cStatus.second)	return;

		UpdateTotalCommonSetBit(rcBitMask);
		UpdateBoundData(rcBitMask);
	}

	bool Find(BitMask & rcBitMask, ExtraData & rcFoundData)
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
		typename BitMaskSet::iterator iterFind = m_cStorage[iFirstIndex][iSecondIndex]->find(rcBitMask);
		if (iterFind != m_cStorage[iFirstIndex][iSecondIndex]->end())
		{
			rcFoundData = iterFind->second;
			return true;
		}
		else
		{
			return false;
		}		
	}

	bool ExistSubSet(BitMask & rcBitMask, ExtraData & rcRefData, int * piNAttempts = NULL, int iLimit = 0x7fffffff)
	{
		if (m_uiSize == 0)
			return false;
		assert(rcBitMask.m_iBitChange == 0);
		//cout << rcBitMask << endl;
#if 0
		if (m_iZeroBitMask == 1)
		{
			return true;
		}
		else if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
		{
			return false;
		}
#endif
		int iNAttempts = 0;
		for (int i = m_uiNSetLB; i < rcBitMask.m_iNBitSnapshot; i++)
		{
			if (m_vectorLowestSetUB[i] == m_uiWidth + 1)	continue;
			if (!(m_vectorCommonSetBit[i] <= rcBitMask))	continue;
			for (int j = rcBitMask.m_uiLowestSetBit; j <= m_vectorLowestSetUB[i]; j++)
			{
				if (!m_cStorage[i][j]) continue;
				if (!(m_cCommonSetBit[i][j] <= rcBitMask)) continue;
				BitMaskSet & rcBitMaskSet = *m_cStorage[i][j];
				//cout << rcBitMaskSet.size() << endl;
				typename BitMaskSet::iterator iterEnd = rcBitMaskSet.upper_bound(rcBitMask);
				for (typename BitMaskSet::iterator iter = rcBitMaskSet.begin(); iter != iterEnd; iter++)
				{
					iNAttempts++;
					if ((BitMask &)(iter->first) <= rcBitMask)
					{
						ExtraData & rcExtraDataCand = iter->second;
						if (ExtraDataTestSubset(rcRefData, rcExtraDataCand))
						{
							m_cSupSubSetQueryCertificate = iter->first;
							if (piNAttempts)	*piNAttempts = iNAttempts;
							return true;
						}						
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

	bool ExistSupSet(BitMask & rcBitMask, ExtraData & rcRefData, int * piNAttempts = NULL, int iLimit = 0x7fffffff)
	{
		if (m_uiSize == 0)
			return false;
		assert(rcBitMask.m_iBitChange == 0);
#if 0
		if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
			return true;
#endif

		int iNAttempts = 0;
		for (int i = m_uiNSetUB; i > rcBitMask.m_iNBitSnapshot; i--)
		{
			if (!(m_vectorTotalSetBit[i] >= rcBitMask))	continue;
			for (int j = m_vectorLowestSetLB[i]; j <= rcBitMask.m_uiLowestSetBit; j++)
			{
				if (!m_cStorage[i][j]) continue;
				if (!(m_cTotalSetBit[i][j] >= rcBitMask)) continue;
				BitMaskSet & rcBitMaskSet = *m_cStorage[i][j];
				typename BitMaskSet::iterator iter = rcBitMaskSet.upper_bound(rcBitMask);
				for (; iter != rcBitMaskSet.end(); iter++)
				{
					iNAttempts++;
					if ((BitMask &)(iter->first) >= rcBitMask)
					{
						ExtraData & rcExtraDataCand = iter->second;
						if (ExtraDataTestSupset(rcRefData, rcExtraDataCand))
						{
							m_cSupSubSetQueryCertificate = iter->first;
							if (piNAttempts)	*piNAttempts = iNAttempts;
							return true;
						}
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

	bool ExistProperSubSet(BitMask & rcBitMask, ExtraData & rcRefData, int * piNAttempts = NULL, int iLimit = 0x7fffffff)
	{
		if (m_uiSize == 0)
			return false;
		assert(rcBitMask.m_iBitChange == 0);
		//cout << rcBitMask << endl;
#if 0
		if (m_iZeroBitMask == 1)
		{
			return true;
		}
		else if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
		{
			return false;
		}
#endif
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
				typename BitMaskSet::iterator iterEnd = rcBitMaskSet.upper_bound(rcBitMask);
				for (typename BitMaskSet::iterator iter = rcBitMaskSet.begin(); iter != iterEnd; iter++)
				{
					iNAttempts++;
					if ((BitMask &)(iter->first) < rcBitMask)
					{
						ExtraData & rcExtraDataCand = iter->second;
						if (ExtraDataTestSubset(rcRefData, rcExtraDataCand))
						{
							m_cSupSubSetQueryCertificate = iter->first;
							if (piNAttempts)	*piNAttempts = iNAttempts;
							return true;
						}
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

	bool ExistProperSupSet(BitMask & rcBitMask, ExtraData & rcRefData, int * piNAttempts = NULL, int iLimit = 0x7fffffff)
	{
		if (m_uiSize == 0)
			return false;
		assert(rcBitMask.m_iBitChange == 0);
#if 0
		if (rcBitMask.m_uiLowestSetBit == m_uiWidth + 1)
			return true;
#endif

		int iNAttempts = 0;
		for (unsigned int i = m_uiNSetUB; i > rcBitMask.m_iNBitSnapshot; i--)
		{
			if (!(m_vectorTotalSetBit[i] >= rcBitMask))	continue;
			for (unsigned int j = m_vectorLowestSetLB[i]; j <= rcBitMask.m_uiLowestSetBit; j++)
			{
				if (!m_cStorage[i][j]) continue;
				if (!(m_cTotalSetBit[i][j] >= rcBitMask)) continue;
				BitMaskSet & rcBitMaskSet = *m_cStorage[i][j];
				typename BitMaskSet::iterator iter = rcBitMaskSet.upper_bound(rcBitMask);
				for (; iter != rcBitMaskSet.end(); iter++)
				{
					iNAttempts++;
					if ((BitMask &)(iter->first) > rcBitMask)
					{
						ExtraData & rcExtraDataCand = iter->second;
						if (ExtraDataTestSupset(rcRefData, rcExtraDataCand))
						{
							m_cSupSubSetQueryCertificate = iter->first;
							if (piNAttempts)	*piNAttempts = iNAttempts;
							return true;
						}
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

	void Initialize(unsigned int uiWidth)
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
		for (unsigned int i = 0; i < m_uiWidth + 1; i++)
		{
			m_cStorage[i] = new BitMaskSetPtr[m_uiWidth + 1];
			for (int j = 0; j < m_uiWidth + 1; j++)
			{
				m_cStorage[i][j] = NULL;
			}
		}

		m_cCommonSetBit = new BitMask1D[m_uiWidth + 1];
		m_cTotalSetBit = new BitMask1D[m_uiWidth + 1];
		for (unsigned int i = 0; i < m_uiWidth + 1; i++)
		{
			m_cCommonSetBit[i] = new BitMask[m_uiWidth];
			m_cTotalSetBit[i] = new BitMask[m_uiWidth];
		}

		m_uiNSetLB = m_uiWidth + 1;
		m_uiNSetUB = m_uiWidth + 1;

		//InitializeMTMutexHandle();	
	}	

	int getSize()	{ return m_uiSize; }
protected:
	void CreateBitMaskSet(unsigned int iNBit, unsigned int iLowestSetBit)
	{
		m_cStorage[iNBit][iLowestSetBit] = new BitMaskSet;
	}

	void UpdateTotalCommonSetBit(BitMask & rcBitMask)
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

	void UpdateBoundData(BitMask & rcBitMask)
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

	virtual bool ExtraDataTestSubset(ExtraData & rcReference, ExtraData & rcCandidate)
	{
		return false;
	}

	virtual bool ExtraDataTestSupset(ExtraData & rcReference, ExtraData & rcCandidate)
	{
		return false;
	}
};

class BitMaskStorage_N_Longest1String_LowestSet
{
public:

	typedef BitMask::DataType DataType;
	typedef MySet<BitMask, BitMask::ValueComparator > BitMaskSet;
	struct LowestSet_Entrance;
	struct Longest1String_Entrance
	{		
		BitMaskSet * pcBitMaskSet;
		inline Longest1String_Entrance()
		{
			pcBitMaskSet = NULL;
		}	

		~Longest1String_Entrance()
		{
			if (pcBitMaskSet)
				delete pcBitMaskSet;
		}

		inline void Create()
		{
			if (pcBitMaskSet)
				delete pcBitMaskSet;
			pcBitMaskSet = new BitMaskSet();
		}
	};

	struct LowestSet_Entrance
	{
		SparseElementLinkList cElementLink;
		Longest1String_Entrance * pcEntrances;

		inline LowestSet_Entrance()
		{
			pcEntrances = NULL;			
		}

		~LowestSet_Entrance()
		{
			if (pcEntrances)
				delete [] pcEntrances;
		}

		inline void Create(int iSize)
		{
			if (pcEntrances)
				delete [] pcEntrances;
			pcEntrances = new Longest1String_Entrance[iSize];
			cElementLink.Create(iSize);
		}
	};

	struct N_Entrance
	{
		SparseElementLinkList cElementLink;
		LowestSet_Entrance * pcEntrances;
		inline N_Entrance()
		{
			pcEntrances = NULL;
		}		

		~N_Entrance()
		{
			if (pcEntrances)
				delete [] pcEntrances;
		}

		inline void Create(int iSize)
		{
			if (pcEntrances)
				delete [] pcEntrances;
			pcEntrances = new LowestSet_Entrance[iSize];
			cElementLink.Create(iSize);
		}
	};

	struct Main_Entrance
	{
		SparseElementLinkList cElementLink;
		N_Entrance * pcEntrances;
		inline Main_Entrance()
		{
			pcEntrances = NULL;
		}

		~Main_Entrance()
		{
			if (pcEntrances)
				delete [] pcEntrances;
		}

		inline void Create(int iSize)
		{
			if (pcEntrances)
				delete [] pcEntrances;
			pcEntrances = new N_Entrance[iSize];
			cElementLink.Create(iSize);
		}

		inline N_Entrance & operator [] (int iIndex)
		{
			return pcEntrances[iIndex];
		}
	};
protected:
	Main_Entrance m_cNEntrances;
	unsigned int m_uiWidth;
	unsigned int m_uiHasZeroBitMask;
	unsigned int m_uiHasFullBitMask;
	unsigned int m_uiSize;
public:
	BitMaskStorage_N_Longest1String_LowestSet();
	~BitMaskStorage_N_Longest1String_LowestSet();
	void Initialize(unsigned int uiWidth);
	void Insert(BitMask & rcBitMask);
	bool Find(BitMask & rcBitMask);
	bool ExistSupSet(BitMask & rcBitMask, int * piNAttempts = NULL);
	bool ExistSubSet(BitMask & rcBitMask, int * piNAttempts = NULL);
	void UpdateElementLink(BitMask & rcBitMask);
	int getSize()	{ return m_uiSize; }
protected:	
};