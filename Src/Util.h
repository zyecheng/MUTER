#pragma once 
#include <deque>
#include <string>
#include <set>
#include <map>
#include <inttypes.h>
#define __int64 long
#define DOUBLE_EQUAL(a,b,tol)	\
	(fabs(double(a) - double(b)) <= tol)

#define DOUBLE_GE(a, b, tol)	\
	(((a) - (b)) >= -tol)

#define MY_MAX(a,b) (((a)>(b)) ? (a) : (b))
#define printf_s  printf
#define sprintf_s sprintf

void my_assert(bool bCondition, char axInfo[]);
bool IsFileExist(char axFileName[]);
int ExecuteSelf(char axCmd[]);
void GetTaskSetFileName(deque<string> & rdequeFileNames, char axFolderName[]);
void MakePath(char axPath[]);
bool IsPathExist(char axPath[]);
bool StartWith(char axString[], char axComparison[]);
signed long long getCPUTimeStamp();
double getWallTimeSecond();
int gcd(int a, int b);
int lcm(int a, int b);
void EraseString(string & rcString, const char axTarget[]);
//void printf_s(const char * fmt, ...);
//void sprintf_s(const char * fmt, ...);
class StringCompObj
{
public:
	bool operator () (const string & stringLhs, const string & stringRhs) const
	{
		return std::lexicographical_compare(stringLhs.begin(), stringLhs.end(), stringRhs.begin(), stringRhs.end());
	}
};

template<class T>
using Dictionary = map < std::string, T, StringCompObj > ;




template<typename T, typename CMP = less<T>, typename ALLOC = allocator<T>  > 
inline void PrintSetContent(set<T, CMP, ALLOC> & rsetSet)
{
	for (typename set<T, CMP, ALLOC>::iterator iter = rsetSet.begin(); iter != rsetSet.end(); iter++)
	{
		cout << *iter << ", ";
	}
	cout << endl;
}

template<typename K, typename T, typename CMP = less<T>, typename ALLOC = allocator<T>  >
inline void PrintSetContent(map<K, T, CMP, ALLOC> & rmapMap)
{
	for (typename map<K, T, CMP, ALLOC>::iterator iter = rmapMap.begin();
		iter != rmapMap.end(); iter++)
	{
		cout << "(" << iter->first << ", " << iter->second << "), ";
	}
	cout << endl;
}

void RedirectCoutBuf(char axFileName[]);

int _BitScanForward(unsigned long*, unsigned int);
int _BitScanReverse(unsigned long*, unsigned int);



class SparseElementLinkList
{
public:
	struct ElementIndex
	{
		int iNext;
		int iBack;
		int iExist;
	} * m_ptagElementIndices;	
	int m_iStart;	
	int m_iStartReverse;
	int m_iEnd;
	int m_iSize;
	int m_iNElement;

	SparseElementLinkList()
	{
		m_iStart = -1;
		m_iStartReverse = -1;
		m_ptagElementIndices = NULL;
		m_iSize = 0;
		m_iEnd = -1;
		m_iNElement = 0;		
	}

	~SparseElementLinkList()
	{
		if (m_ptagElementIndices)
			delete[] m_ptagElementIndices;
	}

	void Create(int iSize)
	{
		if (m_ptagElementIndices)
			delete [] m_ptagElementIndices;

		m_ptagElementIndices = new ElementIndex[iSize];
		for (int i = 0; i < iSize; i++)
		{
			m_ptagElementIndices[i].iNext = -1;
			m_ptagElementIndices[i].iBack = -1;
			m_ptagElementIndices[i].iExist = 0;
		}
	
		m_iSize = iSize;
		m_iEnd = -1;
		m_iStart = m_iEnd;
		m_iStartReverse = -1;
		m_iNElement = 0;

	}
	
	void Insert(int iNewElement)
	{		
		{
			int iNextElement = m_iEnd;
			m_ptagElementIndices[iNewElement].iExist = 1;
			for (int i = m_iSize - 1; i >= 0; i--)
			{
				m_ptagElementIndices[i].iNext = iNextElement;
				if (m_ptagElementIndices[i].iExist == 1)
				{
					iNextElement = i;
				}
			}
			m_iStart = iNextElement;
			m_iNElement++;
		}

		{
			int iNextElement = m_iEnd;			
			for (int i = 0; i < m_iSize; i++)
			{
				m_ptagElementIndices[i].iBack = iNextElement;
				if (m_ptagElementIndices[i].iExist == 1)
				{
					iNextElement = i;
				}
			}
			m_iStartReverse = iNextElement;
		}
		
	}

	inline int getNext(int iElementIndex)
	{
		return m_ptagElementIndices[iElementIndex].iNext;
	}

	inline int getNextReverse(int iElementIndex)
	{
		return m_ptagElementIndices[iElementIndex].iBack;
	}

	inline int getNextOrSelf(int iElementIndex)
	{
		if (m_ptagElementIndices[iElementIndex].iExist == 1)
			return iElementIndex;
		else
			return m_ptagElementIndices[iElementIndex].iNext;
	}

	inline int getNextOrSelfReverse(int iElementIndex)
	{
		if (m_ptagElementIndices[iElementIndex].iExist == 1)
			return iElementIndex;
		else
			return m_ptagElementIndices[iElementIndex].iBack;
	}

	inline int getStart()
	{
		return m_iStart;
	}

	inline int getStartReverse()
	{
		return m_iStartReverse;
	}

	inline int end()
	{
		return m_iEnd;
	}
};

const std::string currentDateTime();

unsigned long gcd_long(unsigned long a, unsigned long b);

double ceil_tol(double dValue, double dTol=1e-5);

void Sleep(double dMS);
