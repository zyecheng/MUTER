#pragma once
#include <vector>
#include <fstream>
#include <unordered_map>

//interesting tool class
class BitMaskStorage_N_LowestSet;
class BitMask
{
public:
	typedef unsigned int DataType;//recommended no more than 32	
public:
	vector<DataType> m_vectorBitArray;
	vector<DataType> m_vectorNSetInEachVector;
	vector<bool> m_vectorSingleBitArray;
	unsigned int m_uiHighestSetBit;
	unsigned int m_uiLowestSetBit;
	DataType m_uiWidth;
	DataType m_uiRemMask;
	unsigned int m_iNBitSnapshot;
	int m_iBitChange;
	unsigned int m_uiLongest1String;
public:
	BitMask();
	BitMask(int iWidth);
	~BitMask();
	int getBit(int iBitIndex);
	void setBit(int iIndex, int iBit);
	vector<DataType> & getDataVector()	{ return m_vectorBitArray; }
	inline DataType getWidth() const	{ return m_uiWidth; }
	inline int getNBitSnapshot()	{ return m_iNBitSnapshot; }
	int countOnes();
	void setAllBitToOne();
	void setAllBitToZero();
	void FlipAllBits();
	void WriteImage(ofstream & ofstreamOut);
	void ReadImage(ifstream & ofstreamIn);
	unsigned int NumberOfBits(unsigned int i) const ;
	unsigned int HighestSetBit();
	unsigned int LowestSetBit();
	int NBitSnapshot();
	void ConstructBitMaskData();
	unsigned int CountLongest1String();
	size_t hash() const;
private:
	void ToPosInArray(int iBitIndex, DataType & uiIndexInArray, DataType & uiOffset);
	void ConstructSingleBitArray();	
	friend BitMask operator | (BitMask & cLhs, BitMask & cRhs);
	friend BitMask operator & (BitMask & cLhs, BitMask & cRhs);
	friend BitMask operator ^ (BitMask & cLhs, BitMask & cRhs);

	friend bool operator >= (BitMask & cLhs, BitMask & cRhs);
	friend bool operator > (BitMask & cLhs, BitMask & cRhs);
	friend bool operator <= (BitMask & cLhs, BitMask & cRhs);
	friend bool operator < (BitMask & cLhs, BitMask & cRhs);
	friend bool operator == (const BitMask & cLhs, const BitMask & cRhs);

	friend ostream & operator << (ostream & cStream, const BitMask & cLhs);

public:
	class NBitValueComparator
	{
	public:
		bool operator() (const BitMask & cLhs, const BitMask & cRhs);
	};

	class ValueComparator
	{
	public:
		bool operator() (const BitMask & cLhs, const BitMask & cRhs);
	};

	class EqualityComparator
	{
		bool operator() (const BitMask & cLhs, const BitMask & cRhs);
	};

	class Hasher
	{
	public:
		size_t operator() (const BitMask & cLhs) const;
	};

	friend class BitMaskStorage_N_LowestSet;		
};

template<class Data>
class BitMaskHashMap
{
public:
	
	unordered_map<BitMask, Data, BitMask::Hasher> m_cHashMap;
public:
	BitMaskHashMap()	{}
	~BitMaskHashMap()	{}
	Data & getData(BitMask & rcBitMask)	{ return m_cHashMap[rcBitMask]; }
	Data & operator [] (const BitMask & rcBitMask)	{ return m_cHashMap[rcBitMask]; }
	
};
