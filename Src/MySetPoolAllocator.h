#pragma once

#include <memory>
#include <iostream>
#include <assert.h>
using namespace std;

#include <cmath>

template<class T>
class MySetPoolAllocator
{
#define MYPOOLALLOC_SETOVERHEAD	32
public:
	typedef T                 value_type;
	typedef value_type*       pointer;
	typedef const value_type* const_pointer;
	typedef value_type&       reference;
	typedef const value_type& const_reference;
	typedef std::size_t       size_type;
	typedef std::ptrdiff_t    difference_type;
public:
	struct MyPoolChunkWrapper
	{
		void * pvChunk;
	};
	
public:
	//data member
	unsigned long m_iChunkSize;
	unsigned long m_iChunkNum;
	unsigned long m_iPoolSizeByte;
	void * m_pvPoolMemory;
	struct MyPoolChunkWrapper * m_ptagAvailableChunkTable;
	int * m_piAvailableTableIndex;

	//statistic of usage
	int *m_piPeakChunkUse;
	int *m_piCurrentChunksInUse;	
public:
	//rebind structure
	template <class U>
	struct rebind
	{
		typedef	MySetPoolAllocator<U> other;
	};

public:
	//constructor
	MySetPoolAllocator()
	{
		m_iChunkNum = 0;
		m_iChunkSize = 0;
		m_iPoolSizeByte = 0;		
		m_ptagAvailableChunkTable = NULL;
		m_piAvailableTableIndex = NULL;
		m_pvPoolMemory = NULL;
		m_piPeakChunkUse = NULL;
		m_piCurrentChunksInUse = NULL;		
	}

	~MySetPoolAllocator()
	{
#if 0
		if (m_piNReferences)
		{
			(*m_piNReferences)--;
			if ((*m_piNReferences) == 0)
			{
				Destroy();
		
				m_piNReferences = 0;
			}
		}
#endif
	}

	template<class U>
	MySetPoolAllocator(const MySetPoolAllocator<U> & rcAllocator)
	{
		this->m_iChunkNum = rcAllocator.m_iChunkNum;
		this->m_iChunkSize = rcAllocator.m_iChunkSize;
		this->m_ptagAvailableChunkTable = (struct MyPoolChunkWrapper *)rcAllocator.m_ptagAvailableChunkTable;
		this->m_piAvailableTableIndex = rcAllocator.m_piAvailableTableIndex;
		this->m_pvPoolMemory  = rcAllocator.m_pvPoolMemory;
		this->m_iPoolSizeByte = rcAllocator.m_iPoolSizeByte;

		this->m_piPeakChunkUse = rcAllocator.m_piPeakChunkUse;
		this->m_piCurrentChunksInUse = rcAllocator.m_piCurrentChunksInUse;		
	}

	MySetPoolAllocator & operator = (const MySetPoolAllocator & rcAllocator)
	{
		this->m_iChunkNum = rcAllocator.m_iChunkNum;
		this->m_iChunkSize = rcAllocator.m_iChunkSize;
		this->m_ptagAvailableChunkTable = (struct MyPoolChunkWrapper *)rcAllocator.m_ptagAvailableChunkTable;
		this->m_piAvailableTableIndex = rcAllocator.m_piAvailableTableIndex;
		this->m_pvPoolMemory = rcAllocator.m_pvPoolMemory;
		this->m_iPoolSizeByte = rcAllocator.m_iPoolSizeByte;

		this->m_piPeakChunkUse = rcAllocator.m_piPeakChunkUse;
		this->m_piCurrentChunksInUse = rcAllocator.m_piCurrentChunksInUse;
		return *this;
	}

	MySetPoolAllocator(int iChunkNum)
	{
		InitializePool(iChunkNum);
	}

	pointer address(reference x) const 
	{ 
		return &x; 	
	}

	const_pointer address(const_reference x) const { 
		return &x; 
	}

	pointer allocate(size_type n, const_pointer = 0)
	{		
		if(n != 1)
		{
			cout<<"Can't Allocate More Than One Unit"<<endl;
			while(1);
		}
		const int iSizeofT = sizeof(T);
		if (sizeof(T) > m_iChunkSize)
		{
			cout << "Actual Size Greater Than Chunk Size" << endl;
			cout << sizeof(T) << " " << m_iChunkSize << endl;
			assert(0);
		}

		if((*m_piAvailableTableIndex) < 0)
		{
			cout<<"No More Block In The Pool !"<<endl;
			throw std::bad_alloc();
		}		
		void * pvChunk = m_ptagAvailableChunkTable[*m_piAvailableTableIndex].pvChunk;
		//post processing 
		(*m_piAvailableTableIndex) -- ;
		(*m_piCurrentChunksInUse) ++ ;
		
		if(*m_piPeakChunkUse < *m_piCurrentChunksInUse)
		{
			*m_piPeakChunkUse = *m_piCurrentChunksInUse;
		}		
		return static_cast<pointer> (pvChunk);		
	}

	void deallocate(pointer p, size_type n) 
	{		
		(*m_piAvailableTableIndex) ++;
		(*m_piCurrentChunksInUse) --;
		m_ptagAvailableChunkTable[*m_piAvailableTableIndex].pvChunk = static_cast<void *>(p);		
	}

	size_type max_size() const { 
		return static_cast<size_type> (m_iChunkNum);
	}

	void construct(pointer p, const value_type& x) { 
		new(p) value_type(x); 
	}

	void destroy(pointer p) 
	{ 
		p->~value_type();
	}

	friend inline bool operator==(const MySetPoolAllocator<T>&,const MySetPoolAllocator<T>&) {
		return true;
	}


	friend inline bool operator!=(const MySetPoolAllocator<T>&, const MySetPoolAllocator<T>&) {
		return false;
	}

	int getPeakUsage()	{return *m_piPeakChunkUse;}

	int getTotalChunk()	{return m_iChunkNum;}

	int getPoolSizeMB()	{ return m_iPoolSizeByte/1e6;}

	void Destroy()
	{
		if (m_piPeakChunkUse)	delete m_piPeakChunkUse;		
		if (m_ptagAvailableChunkTable)	 delete m_ptagAvailableChunkTable;
		if (m_pvPoolMemory)	delete m_pvPoolMemory;
		if (m_piAvailableTableIndex) delete m_piAvailableTableIndex;
	}

protected:
	void InitializePool(int iChunkNum)
	{
		m_iChunkSize = CalculateChunkSize();
		m_iChunkNum = iChunkNum;
		m_iPoolSizeByte = m_iChunkNum * m_iChunkSize;
		m_pvPoolMemory = (void *)malloc(m_iPoolSizeByte);		
		m_piAvailableTableIndex = new int;
		*m_piAvailableTableIndex = m_iChunkNum - 1;
		m_ptagAvailableChunkTable = new struct MyPoolChunkWrapper [m_iChunkNum];

		//reset statistic variable
		m_piPeakChunkUse = new int;
		*m_piPeakChunkUse = 0;
		m_piCurrentChunksInUse = new int;
		*m_piCurrentChunksInUse = 0;

		//create storage for each chunk
		//and set up the available table
		for(int i = 0;i<m_iChunkNum;i++)
		{
			//m_ppvAvailableChunkAddress[i] =(void *) (m_pvPoolMemory + i*m_iChunkSize);
			m_ptagAvailableChunkTable[i].pvChunk = (char *)m_pvPoolMemory + i*m_iChunkSize;
		}
	}	

	virtual int CalculateChunkSize()
	{
		return (sizeof(T)/4 + 1)*4 + MYPOOLALLOC_SETOVERHEAD;
	}

};

template<class T>
class GeneralPoolAllocator : public MySetPoolAllocator < T >
{
protected:
	unsigned int m_uiStorageOverhead;
public:
	//constructor
	GeneralPoolAllocator()
		:MySetPoolAllocator<T>()
	{
		m_uiStorageOverhead = 0;
	}

	~GeneralPoolAllocator()
	{

	}

	template<class U>
	GeneralPoolAllocator(const GeneralPoolAllocator<U> & rcAllocator)
	{
		this->m_iChunkNum = rcAllocator.m_iChunkNum;
		this->m_iChunkSize = rcAllocator.m_iChunkSize;
		this->m_ptagAvailableChunkTable = (struct MyPoolChunkWrapper *)rcAllocator.m_ptagAvailableChunkTable;
		this->m_piAvailableTableIndex = rcAllocator.m_piAvailableTableIndex;
		this->m_pvPoolMemory = rcAllocator.m_pvPoolMemory;
		this->m_iPoolSizeByte = rcAllocator.m_iPoolSizeByte;
		this->m_piPeakChunkUse = rcAllocator.m_piPeakChunkUse;
		this->m_piCurrentChunksInUse = rcAllocator.m_piCurrentChunksInUse;
		this->m_uiStorageOverhead = rcAllocator.m_uiStorageOverhead;
	}

	GeneralPoolAllocator & operator = (const GeneralPoolAllocator & rcAllocator)
	{
		this->m_iChunkNum = rcAllocator.m_iChunkNum;
		this->m_iChunkSize = rcAllocator.m_iChunkSize;
		this->m_ptagAvailableChunkTable = (struct MyPoolChunkWrapper *)rcAllocator.m_ptagAvailableChunkTable;
		this->m_piAvailableTableIndex = rcAllocator.m_piAvailableTableIndex;
		this->m_pvPoolMemory = rcAllocator.m_pvPoolMemory;
		this->m_iPoolSizeByte = rcAllocator.m_iPoolSizeByte;

		this->m_piPeakChunkUse = rcAllocator.m_piPeakChunkUse;
		this->m_piCurrentChunksInUse = rcAllocator.m_piCurrentChunksInUse;
		this->m_piNReferences = rcAllocator.m_piNReferences;		
		this->m_uiStorageOverhead = rcAllocator.m_uiStorageOverhead;
		return *this;
	}

	GeneralPoolAllocator(int iChunkNum, unsigned int uiStorageOverHead)
	{
		m_uiStorageOverhead = uiStorageOverHead;
		InitializePool(iChunkNum);
	}

	virtual int CalculateChunkSize()
	{
		return (sizeof(T) / 4 + 1) * 4 + m_uiStorageOverhead;
	}
};