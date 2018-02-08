#include "stdafx.h"
#include "BitMask.h"
#include <cmath>
#include <assert.h>
#include <algorithm>
#include <assert.h>
#include "Util.h"

const unsigned int aiBitMaskSingleBitMask[] = {
	0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 
	0x4000, 0x8000, 0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000, 
	0x1000000, 0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000,
};

BitMask::BitMask()
{
	m_uiWidth = 0;
}

BitMask::BitMask(int iWidth)
{
	int iSize = iWidth / (sizeof(DataType) * 8) + 1;
	m_vectorBitArray.reserve(iSize);
	m_vectorNSetInEachVector.reserve(iSize);
	for (int i = 0; i < iSize; i++)
	{
		m_vectorBitArray.push_back(0);
		m_vectorNSetInEachVector.push_back(0);
	}
	m_uiWidth = iWidth;

	int iLeft = m_uiWidth % (sizeof(DataType) * 8);
	if (iLeft == 0)
	{
		m_uiRemMask = 0;
	}
	else
	{
		m_uiRemMask = 1;
		for (int i = 0; i < iLeft - 1; i++)
		{
			m_uiRemMask <<= 1;
			m_uiRemMask |= 1;
		}
	}

	m_uiHighestSetBit = -1;
	m_uiLowestSetBit = -1;
	
}


BitMask::~BitMask()
{

}

int BitMask::getBit(int iBitIndex)
{
	assert(iBitIndex < m_uiWidth);
	DataType uiArrayIndex = 0;
	DataType uiOffset = 0;
	ToPosInArray(iBitIndex, uiArrayIndex, uiOffset);
	DataType uiWord = m_vectorBitArray[uiArrayIndex];
	return (uiWord >> uiOffset) & (DataType)1;
}

void BitMask::setBit(int iBitIndex, int iBit)
{
	assert(iBitIndex < m_uiWidth);
	assert((iBit == 1) || (iBit == 0));
	DataType uiArrayIndex = 0;
	DataType uiOffset = 0;
	ToPosInArray(iBitIndex, uiArrayIndex, uiOffset);
	if (iBit == 1)
		m_vectorBitArray[uiArrayIndex] |= (DataType)1 << uiOffset;
	else
		m_vectorBitArray[uiArrayIndex] &= ~((DataType)1 << uiOffset);

	m_iBitChange = 1;
}

unsigned int BitMask::NumberOfBits(unsigned int i) const
{
	// Java: use >>> instead of >>
	// C or C++: use uint32_t
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

unsigned int BitMask::HighestSetBit()
{
	int iSize = m_vectorBitArray.size();
	DataType uiData = 0;
	int i = iSize - 1;
	for (; i >= 0; i--)
	{
		if (m_vectorBitArray[i])
		{
			uiData = m_vectorBitArray[i];
			break;
		}
	}
	unsigned long uiIndex;
	if (uiData)
	{
		_BitScanReverse(&uiIndex, uiData);
		m_uiHighestSetBit = uiIndex + sizeof(DataType) * 8 * i;
	}
	else
	{
		m_uiHighestSetBit = m_uiWidth + 1;
	}
	return m_uiHighestSetBit;
}

unsigned int BitMask::LowestSetBit()
{
	int iSize = m_vectorBitArray.size();
	DataType uiData = 0;
	int i = 0;
	for (; i < iSize; i++)
	{
		if (m_vectorBitArray[i])
		{
			uiData = m_vectorBitArray[i];
			break;
		}
	}

	unsigned long uiIndex;
	if (uiData)
	{
		_BitScanForward(&uiIndex, uiData);
		m_uiLowestSetBit = uiIndex + sizeof(DataType) * 8 * i;
	}
	else
	{
		m_uiLowestSetBit = m_uiWidth + 1;
	}
	return m_uiLowestSetBit;
}

void BitMask::ConstructBitMaskData()
{
	NBitSnapshot();
	HighestSetBit();
	LowestSetBit();
	//CountLongest1String();
	m_iBitChange = 0;
}

void BitMask::ConstructSingleBitArray()
{	
	m_vectorSingleBitArray.clear();
	m_vectorSingleBitArray.reserve(m_uiWidth);
	int iBitIndex = 0;
	unsigned long iSize = m_vectorBitArray.size();
	for (unsigned long i = 0; i < iSize; i++)
	{
		for (unsigned long j = 0; j < sizeof(DataType); j++)
		{
			if (iBitIndex == m_uiWidth)	break;
			if (m_vectorBitArray[i] & aiBitMaskSingleBitMask[j])
			{
				m_vectorSingleBitArray[j] = true;
			}
			else
			{
				m_vectorSingleBitArray[j] = false;
			}
			iBitIndex++;
		}
	}
}

int BitMask::NBitSnapshot()
{
	m_iNBitSnapshot = countOnes();
	return m_iNBitSnapshot;
}

int BitMask::countOnes()
{
	int iSize = m_vectorBitArray.size();
	int iOnes = 0;
	for (int i = 0; i < iSize - 1; i++)
	{
		DataType uiValue = m_vectorBitArray[i];
		m_vectorNSetInEachVector[i] = NumberOfBits(uiValue);
		iOnes += m_vectorNSetInEachVector[i];
	}
	m_vectorNSetInEachVector[iSize - 1] = NumberOfBits(m_vectorBitArray[iSize - 1] & m_uiRemMask);
	iOnes += m_vectorNSetInEachVector[iSize - 1];
	return iOnes;
}

void BitMask::setAllBitToOne()
{
	for (vector<DataType>::iterator iter = m_vectorBitArray.begin(); iter != m_vectorBitArray.end(); iter++)
	{
		*iter = ~0;
	}
	m_vectorBitArray[m_vectorBitArray.size() - 1] &= m_uiRemMask;
}

void BitMask::setAllBitToZero()
{
	for (vector<DataType>::iterator iter = m_vectorBitArray.begin(); iter != m_vectorBitArray.end(); iter++)
	{
		*iter = 0;
	}
}

void BitMask::FlipAllBits()
{
	for (vector<DataType>::iterator iter = m_vectorBitArray.begin(); iter != m_vectorBitArray.end(); iter++)
	{
		*iter = ~(*iter);
	}
}

void BitMask::ToPosInArray(int iBitIndex, DataType & uiIndexInArray, DataType & uiOffset)
{
	assert(iBitIndex <= m_uiWidth);
	uiIndexInArray = iBitIndex / (sizeof(DataType) * 8);
	uiOffset = iBitIndex % (sizeof(DataType) * 8);
}

void BitMask::WriteImage(ofstream & ofstreamOut)
{
	ofstreamOut.write((const char *)&m_uiWidth, sizeof(DataType));
	unsigned long iSize = m_vectorBitArray.size();
	ofstreamOut.write((const char *)&iSize, sizeof(int));
	for (unsigned long i = 0; i < iSize; i++)
	{
		DataType uiValue = m_vectorBitArray[i];
		ofstreamOut.write((const char *)&uiValue, sizeof(DataType));
	}
}

void BitMask::ReadImage(ifstream & ifstreamIn)
{
	ifstreamIn.read((char *)&m_uiWidth, sizeof(DataType));
	int iSize = 0;
	ifstreamIn.read((char *)&iSize, sizeof(int));
	for (int i = 0; i < iSize; i++)
	{
		DataType uiValue = m_vectorBitArray[i];
		ifstreamIn.read((char *)&uiValue, sizeof(DataType));
		m_vectorBitArray.push_back(uiValue);
	}
}

BitMask operator | (BitMask & cLhs, BitMask & cRhs)
{
	assert(cLhs.m_uiWidth == cRhs.m_uiWidth);
	BitMask cRes(cLhs.m_uiWidth);
	unsigned long iSize = cLhs.m_vectorBitArray.size();
	for (unsigned long i = 0; i < iSize; i++)
	{
		cRes.m_vectorBitArray[i] = cLhs.m_vectorBitArray[i] | cRhs.m_vectorBitArray[i];
	}
	return cRes;
}

BitMask operator & (BitMask & cLhs, BitMask & cRhs)
{
	assert(cLhs.m_uiWidth == cRhs.m_uiWidth);
	BitMask cRes(cLhs.m_uiWidth);
	int iSize = cLhs.m_vectorBitArray.size();
	for (int i = 0; i < iSize; i++)
	{
		cRes.m_vectorBitArray[i] = cLhs.m_vectorBitArray[i] & cRhs.m_vectorBitArray[i];
	}
	return cRes;
}

BitMask operator ^ (BitMask & cLhs, BitMask & cRhs)
{
	int iSize = min(cLhs.m_vectorBitArray.size(), cRhs.m_vectorBitArray.size());
	BitMask cRes(iSize);
	for (int i = 0; i < iSize; i++)
	{
		cRes.m_vectorBitArray[i] = cLhs.m_vectorBitArray[i] ^ cRhs.m_vectorBitArray[i];
	}
	return cRes;
}

bool operator >= (BitMask & cLhs, BitMask & cRhs)
{
	typedef BitMask::DataType	DataType;
	int iLhsWidth = cLhs.getWidth();
	int iRhsWidth = cRhs.getWidth();
	if (iLhsWidth < iRhsWidth)
	{
		return false;
	}
	
	//if (cRhs.m_uiLongest1String > cLhs.m_uiLongest1String)	return false;

	vector<DataType> & rvectorLHSData = cLhs.m_vectorBitArray;
	vector<DataType> & rvectorRHSData = cRhs.m_vectorBitArray;
	int iSize = rvectorRHSData.size();
	for (int i = 0; i < iSize - 1; i++)
	{
		DataType uiLHSData = rvectorLHSData[i];
		DataType uiRHSData = rvectorRHSData[i];
		if ((uiLHSData | uiRHSData) != uiLHSData)
			return false;
	}

	DataType uiLHSData = rvectorLHSData[iSize - 1] & cRhs.m_uiRemMask;
	DataType uiRHSData = rvectorRHSData[iSize - 1] & cRhs.m_uiRemMask;

	return ((uiLHSData | uiRHSData) == uiLHSData);
	return true;
}

bool operator > (BitMask & cLhs, BitMask & cRhs)
{
	typedef BitMask::DataType	DataType;
	int iLhsWidth = cLhs.getWidth();
	int iRhsWidth = cRhs.getWidth();
	if (iLhsWidth < iRhsWidth)
	{
		return false;
	}

	//if (cRhs.m_uiLongest1String > cLhs.m_uiLongest1String)	return false;

	vector<DataType> & rvectorLHSData = cLhs.m_vectorBitArray;
	vector<DataType> & rvectorRHSData = cRhs.m_vectorBitArray;
	int iSize = rvectorRHSData.size();
	bool bIsDiff = false;
	for (int i = 0; i < iSize - 1; i++)
	{
		DataType uiLHSData = rvectorLHSData[i];
		DataType uiRHSData = rvectorRHSData[i];
		if ((uiLHSData | uiRHSData) != uiLHSData)
			return false;
		if (uiLHSData != uiRHSData)
			bIsDiff = true;
	}

	DataType uiLHSData = rvectorLHSData[iSize - 1] & cRhs.m_uiRemMask;
	DataType uiRHSData = rvectorRHSData[iSize - 1] & cRhs.m_uiRemMask;
	if (uiLHSData != uiRHSData) bIsDiff = true;
	return (((uiLHSData | uiRHSData) == uiLHSData) & bIsDiff);
	return true;
}

bool operator <= (BitMask & cLhs, BitMask & cRhs)
{
	return cRhs >= cLhs;
}

bool operator < (BitMask & cLhs, BitMask & cRhs)
{
	return cRhs > cLhs;
}

bool operator == (const BitMask & cLhs, const BitMask & cRhs)
{
	typedef BitMask::DataType	DataType;
	int iLhsWidth = cLhs.getWidth();
	int iRhsWidth = cRhs.getWidth();
	if (iLhsWidth != iRhsWidth)
	{
		return false;
	}

	const vector<DataType> & rvectorLHSData = cLhs.m_vectorBitArray;
	const vector<DataType> & rvectorRHSData = cRhs.m_vectorBitArray;
	int iSize = rvectorRHSData.size();
	for (int i = 0; i < iSize - 1; i++)
	{
		DataType uiLHSData = rvectorLHSData[i];
		DataType uiRHSData = rvectorRHSData[i];
		if (uiLHSData != uiRHSData)
			return false;
	}

	DataType uiLHSData = rvectorLHSData[iSize - 1] & cRhs.m_uiRemMask;
	DataType uiRHSData = rvectorRHSData[iSize - 1] & cRhs.m_uiRemMask;
	return ((uiLHSData) == uiRHSData);
}

ostream & operator << (ostream & cStream, const BitMask & cLhs)
{
	typedef BitMask::DataType	DataType;
	int iSize = cLhs.m_vectorBitArray.size();
	int iCurrentBitIndex = 0;
	for (int i = 0; i < iSize; i++)
	{
		DataType uiValue = cLhs.m_vectorBitArray[i];
		for (int i = 0; i < sizeof(DataType) * 8; i++)
		{
			if (uiValue & 1)
			{
				cStream << "1";
			}
			else
			{
				cStream << "0";
			}
			iCurrentBitIndex++;
			if (iCurrentBitIndex >= cLhs.m_uiWidth)
			{
				return cStream;
			}
			uiValue >>= 1;
		}
	}
	return cStream;
}

bool BitMask::NBitValueComparator::operator() (const BitMask & cLhs, const BitMask & cRhs)
{
	assert(cLhs.m_uiWidth == cRhs.m_uiWidth);
	int iLHSOnes = cLhs.m_iNBitSnapshot;
	int iRHSOnes = cRhs.m_iNBitSnapshot;
	if (iLHSOnes == iRHSOnes)
	{
		const vector<DataType> & rvectorLHS = cLhs.m_vectorBitArray;
		const vector<DataType> & rvectorRHS = cRhs.m_vectorBitArray;
		int iSize = rvectorRHS.size();
		for (int i = 0; i < iSize - 1; i++)
		{
			DataType uiDataLHS = rvectorLHS[i];
			DataType uiDataRHS = rvectorRHS[i];
			if (uiDataLHS < uiDataRHS)
			{
				return true;
			}
		}

		DataType uiDataLHS = rvectorLHS[iSize - 1] & cLhs.m_uiRemMask;
		DataType uiDataRHS = rvectorRHS[iSize - 1] & cRhs.m_uiRemMask;
		return uiDataLHS < uiDataRHS;
	}
	else
	{
		return iLHSOnes < iRHSOnes;
	}
	return false;
}

bool BitMask::ValueComparator::operator() (const BitMask & cLhs, const BitMask & cRhs)
{
	
	assert(cLhs.m_uiWidth == cRhs.m_uiWidth);
	const vector<DataType> & rvectorLHS = cLhs.m_vectorBitArray;
	const vector<DataType> & rvectorRHS = cRhs.m_vectorBitArray;
	int iSize = rvectorRHS.size();
#if 0
	if (cLhs.m_uiLongest1String < cRhs.m_uiLongest1String)
		return true;
	else if (cLhs.m_uiLongest1String > cRhs.m_uiLongest1String)
		return false;
#endif
	DataType uiDataLHS = rvectorLHS[iSize - 1] & cLhs.m_uiRemMask;
	DataType uiDataRHS = rvectorRHS[iSize - 1] & cRhs.m_uiRemMask;
	if (uiDataRHS == uiDataLHS)
	{
		//cout << "Compare Start " << endl;
		for (int i = iSize - 2; i >= 0; i--)
		{
			DataType uiDataLHS = rvectorLHS[i];
			DataType uiDataRHS = rvectorRHS[i];
			if (uiDataLHS == uiDataRHS)
				continue;
			else
				return uiDataLHS < uiDataRHS;
		}
		//cout << "Compare End " << endl;
	}
	else
		return uiDataLHS < uiDataRHS;

	return false;
}

bool BitMask::EqualityComparator::operator() (const BitMask & cLhs, const BitMask & cRhs)
{

	assert(cLhs.m_uiWidth == cRhs.m_uiWidth);
	const vector<DataType> & rvectorLHS = cLhs.m_vectorBitArray;
	const vector<DataType> & rvectorRHS = cRhs.m_vectorBitArray;
	int iSize = rvectorRHS.size();
	DataType uiDataLHS = rvectorLHS[iSize - 1] & cLhs.m_uiRemMask;
	DataType uiDataRHS = rvectorRHS[iSize - 1] & cRhs.m_uiRemMask;
	if (uiDataRHS == uiDataLHS)
	{
		for (int i = iSize - 2; i >= 0; i--)
		{
			DataType uiDataLHS = rvectorLHS[i];
			DataType uiDataRHS = rvectorRHS[i];
			if (uiDataLHS == uiDataRHS)
				continue;
			else
				return false;
		}
	}
	else
		return false;
	return true;
}

size_t BitMask::Hasher::operator() (const BitMask & cLhs) const
{
	return cLhs.hash();
}


unsigned int BitMask::CountLongest1String()
{	
	unsigned int iSize = m_vectorBitArray.size();
	unsigned int iCurrentLongest = 0;
	unsigned int iBitIndex = 0;
	unsigned int iCurrentLength = 0;
	for (int i = 0; i < iSize; i++)
	{
		if (m_vectorBitArray[i] == 0)
		{
			if (iCurrentLength > iCurrentLongest)
			{
				iCurrentLongest = iCurrentLength;
			}
			iCurrentLength = 0;
			continue;
		}
		int iUnitLength = sizeof(DataType) * 8;
		for (int j = 0; j < iUnitLength; j++)
		{
			if (m_vectorBitArray[i] & aiBitMaskSingleBitMask[j])
			{
				iCurrentLength++;
			}
			else
			{
				if (iCurrentLength > iCurrentLongest)
				{
					iCurrentLongest = iCurrentLength;
				}
				iCurrentLength = 0;
			}
		}
	}
	m_uiLongest1String = iCurrentLongest;
	return iCurrentLongest;
}

size_t BitMask::hash() const
{
	size_t iHashValue = 0;
	int iSize = m_vectorBitArray.size();
	for (int i = 0; i < iSize; i++)
	{
		iHashValue = (iHashValue + (324723947 + m_vectorBitArray[i])) ^ 93485734985;
	}
	return iHashValue;	
}