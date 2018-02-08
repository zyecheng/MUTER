#pragma once
#include "stdafx.h"
#include <set>
#include "stdafx.h"
#include <set>
#include <vector>
#include <algorithm>
#include "MySetPoolAllocator.h"
#include <stdarg.h>



#if 1
template<class T1,
class T2 = less<T1>,
//class T3 = MySetPoolAllocator<T1>
class T3 = allocator<T1>
>
class MySet	
{
public:
	typedef  typename set<T1, T2, T3>::iterator iterator;
	typedef  typename set<T1, T2, T3>::const_reverse_iterator const_reverse_iterator;	
	typedef   set<T1, T2, T3>	SetContainer;
	typedef   MySet<T1, T2, T3> MySetType;
private:
	set< T1,T2,T3 > m_cContent;
public:
	MySet(void){}		
	~MySet(void)
	{
		m_cContent.clear();
	}

	MySet(T3 cAllocator)
		:m_cContent(T2(),cAllocator)
	{

	}

	MySet(T2 cComparator)
		:m_cContent(cComparator)
	{

	}

	MySet(set<T1, T2, T3> & rcRawSet)
	{
		m_cContent = rcRawSet;
	}

	void clear()
	{		
		m_cContent.clear();
	}

	typename std::pair<typename set<T1, T2, T3 >::iterator,bool> insert(const T1 & rcNewContent)
	{
		return m_cContent.insert(rcNewContent);
	}

	void insert(iterator first, iterator last)
	{
		return m_cContent.insert(first, last);			
	}

	int size() const
	{
		return m_cContent.size();
	}

	void erase(typename MySet<T1, T2, T3>::iterator iterErase)
	{
		m_cContent.erase(iterErase);
	}

	void erase(T1 cElement)
	{		
		m_cContent.erase(cElement);
	}

	int count(const T1 & cElement) const
	{
		return m_cContent.count(cElement);
	}

	SetContainer & getSetContent()
	{
		return m_cContent;
	}

	typename set<T1, T2, T3 >::iterator begin() const
	{
		return m_cContent.begin();
	}

	typename set<T1, T2, T3 >::iterator end() const 
	{
		return m_cContent.end();
	}

	typename set<T1, T2, T3 >::const_reverse_iterator crbegin() const
	{
		return m_cContent.crbegin();
	}

	typename set<T1, T2, T3 >::const_reverse_iterator crend() const
	{
		return m_cContent.crend();
	}

	typename set<T1, T2, T3 >::iterator find(const T1 & rcTarget)
	{
		return m_cContent.find(rcTarget);
	}

	typename set<T1, T2, T3 >::iterator lower_bound(const T1 & rcTarget)
	{
		return m_cContent.lower_bound(rcTarget);
	}

	typename set<T1, T2, T3 >::iterator upper_bound(const T1 & rcTarget)
	{
		return m_cContent.upper_bound(rcTarget);
	}


	bool empty()
	{
		return m_cContent.empty();
	}

	bool IsIntersect(MySetType & rcSet)
	{		
		set<T1, T2, T3 > * pcSmaller = &this->m_cContent;
		set<T1, T2, T3 > * pcBigger = (set<T1, T2, T3 > *)&rcSet.m_cContent;
		if (this->m_cContent.empty() || rcSet.m_cContent.empty())
		{
			return false;
		}
		else if (this->m_cContent.size() > rcSet.m_cContent.size())
		{
			pcSmaller = (set<T1, T2, T3 > *)&rcSet.m_cContent;
			pcBigger = (set<T1, T2, T3 > *)&this->m_cContent;
		}

		for (typename set<T1, T2, MySetPoolAllocator<T1> >::iterator iter = pcSmaller->begin(); iter != pcSmaller->end(); iter++)
		{
			if (pcBigger->find((*iter)) != pcBigger->end())
				return true;
		}
		return false;
	}

	friend MySet<T1, T2, T3> operator + (const MySet<T1, T2, T3> & cSetA, const MySet<T1, T2, T3> & cSetB)
	{
		MySet<T1,T2> cUnion(cSetB.m_cContent.get_allocator());		
		set<T1,T2, T3 > * pcSmaller = (set<T1,T2, T3 > *)&cSetA.m_cContent;
		set<T1,T2, T3 > * pcBigger = (set<T1,T2, T3 > *)&cSetB.m_cContent; 

		if(cSetA.m_cContent.empty())
		{
			cUnion = cSetB;		
			return cUnion;
		}
		else if(cSetB.m_cContent.empty())
		{
			cUnion = cSetA;
			return cUnion;
		}
		else if(cSetA.m_cContent.size()>cSetB.m_cContent.size())
		{
			pcSmaller = (set<T1,T2, T3 > *)&cSetB.m_cContent;
			pcBigger = (set<T1,T2, T3 > *)&cSetA.m_cContent;		
		}
		cUnion.m_cContent = *pcBigger;
		for(typename set<T1,T2,MySetPoolAllocator<T1> >::iterator iter = pcSmaller->begin();iter != pcSmaller->end();iter ++)
		{
			cUnion.m_cContent.insert((*iter));
		}
		return cUnion;
	}

	friend MySet<T1, T2, T3> operator * (const MySet<T1, T2, T3> & cSetA, const MySet<T1, T2, T3> & cSetB)
	{
		MySet<T1,T2> cIntersection(cSetB.m_cContent.get_allocator());
		set<T1,T2, T3 > * pcSmaller = (set<T1,T2, T3 > *)&cSetA.m_cContent;
		set<T1,T2, T3 > * pcBigger = (set<T1,T2, T3 > *)&cSetB.m_cContent; 
		if(cSetA.m_cContent.empty()||cSetB.m_cContent.empty())
		{
			return cIntersection;
		}
		else if(cSetA.m_cContent.size()>cSetB.m_cContent.size())
		{
			pcSmaller = (set<T1,T2, T3 > *)&cSetB.m_cContent;
			pcBigger = (set<T1,T2, T3 > *)&cSetA.m_cContent;		
		}		
		for(typename set<T1,T2,MySetPoolAllocator<T1> >::iterator iter = pcSmaller->begin();iter != pcSmaller->end();iter ++)
		{
			if(pcBigger->find((*iter))!=pcBigger->end())
				cIntersection.m_cContent.insert((*iter));
		}
		return cIntersection;
	}

	MySet<T1, T2, T3> & operator *= (const MySet<T1, T2, T3> & cSet)
	{
		if(this->m_cContent.empty()||cSet.m_cContent.empty())
		{
			this->m_cContent.clear();
		}
		typename set<T1,T2,MySetPoolAllocator<T1> >::iterator iter = m_cContent.begin();
		for(;iter != m_cContent.end();)
		{
			if(cSet.m_cContent.find((*iter))==cSet.m_cContent.end())
			{
				typename set<T1,T2,MySetPoolAllocator<T1> >::iterator iterTmp = iter++;
				m_cContent.erase((*iterTmp));			
			}
			else
				iter ++;
		}
		return *this;
	}

	MySet<T1, T2, T3> & operator += (const MySet<T1, T2, T3> & cSet)
	{
		if(this->m_cContent.empty())
		{
			this->m_cContent = cSet.m_cContent;
			return (*this);
		}
		else if(cSet.m_cContent.empty())
		{
			return (*this);
		}
		for(typename set<T1,T2,MySetPoolAllocator<T1> >::iterator iter  = cSet.m_cContent.begin();iter != cSet.m_cContent.end();iter ++)
		{
			m_cContent.insert((*iter));
		}
		return (*this);
	}

	MySet<T1, T2, T3> & operator = (const MySet<T1, T2, T3> & cSet)
	{
		m_cContent = cSet.m_cContent;
		return *this;
	}

	MySet<T1, T2, T3> & operator = (const set<T1, T2, T3> & cSet)
	{
		m_cContent = cSet;
		return *this;
	}


	friend bool operator >= (const MySet<T1, T2, T3> & cSetA, const MySet<T1, T2, T3> & cSetB)
	{
		//superseteq test
		int iSizeA = cSetA.size();
		int iSizeB = cSetB.size();
		if (iSizeA < iSizeB)
		{
			return false;
		}
		for (typename SetContainer::iterator iter = cSetB.begin(); iter != cSetB.end(); iter++)
		{
			if (cSetA.count(*iter) == 0)
				return false;
		}
		return true;
	}

	friend bool operator > (const MySet<T1, T2, T3> & cSetA, const MySet<T1, T2, T3> & cSetB)
	{
		//superset test
		int iSizeA = cSetA.size();
		int iSizeB = cSetB.size();
		if (iSizeA <= iSizeB)
		{
			return false;
		}
		for (typename SetContainer::iterator iter = cSetB.begin(); iter != cSetB.end(); iter++)
		{
			if (cSetA.count(*iter) == 0)
				return false;
		}
		return true;
	}

	friend bool operator <= (const MySet<T1, T2, T3> & cSetA, const MySet<T1, T2, T3> & cSetB)
	{
		//subseteq test
		int iSizeA = cSetA.size();
		int iSizeB = cSetB.size();
		if (iSizeA > iSizeB)
		{
			return false;
		}
		for (typename SetContainer::iterator iter = cSetA.begin(); iter != cSetA.end(); iter++)
		{
			if (cSetB.count(*iter) == 0)
				return false;
		}
		return true;
	}

	friend bool operator < (const MySet<T1, T2, T3> & cSetA, const MySet<T1, T2, T3> & cSetB)
	{
		//subset test
		int iSizeA = cSetA.size();
		int iSizeB = cSetB.size();
		if (iSizeA >= iSizeB)
		{
			return false;
		}
		for (typename SetContainer::iterator iter = cSetA.begin(); iter != cSetA.end(); iter++)
		{
			if (cSetB.count(*iter) == 0)
				return false;
		}
		return true;
	}

	friend bool operator == (const MySet<T1, T2, T3> & cSetA, const MySet<T1, T2, T3> & cSetB)
	{
		if (cSetA.size() != cSetB.size())
		{
			return false;
		}
		MySet<T1, T2, T3> ::iterator iterA = cSetA.begin();
		MySet<T1, T2, T3> ::iterator iterB = cSetB.begin();
		for (; iterA != cSetA.end(); iterA++, iterB++)
		{
			T2 & rcComp = cSetA.m_cContent.key_comp();
			if (rcComp(*iterA, *iterB) || rcComp(*iterB, *iterA))
			{
				return false;
			}
		}
		return true;
	}

	friend std::ostream & operator << (std::ostream & os, const MySet<T1,T2, T3> & cSet)
	{
		for(typename set<T1,T2,T3 >::iterator iter = cSet.m_cContent.begin();iter != cSet.m_cContent.end();iter ++)
		{
			os<<(*iter)<<endl;
		}
		return os;
	}
};

#endif

