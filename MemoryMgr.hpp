#pragma once
#include <stdlib.h>
#include <assert.h>
#include <mutex>

#ifdef _DEBUG
#include<stdio.h>
#define xPrintf(...) printf(__VA_ARGS__)
#else
#define xPrintf(...)
#endif // _DEBUG




#define MAX_MEMORY_SZIE 1024
class MemoryAlloc;
//�ڴ��
class MemoryBlock
{
public:
	//�ڴ����
	int nID;
	//���ô���
	int nRef;
	//�����ڴ��
	MemoryAlloc* pAlloc;
	//��һ���ڴ��λ��
	MemoryBlock* pNext;
	//�Ƿ��ڳ���
	bool bPool;
private:
	//�ڴ油��Ԥ��
	char c1;
	char c2;
	char c3;
};
const int a = sizeof(MemoryBlock);

//�ڴ��
class MemoryAlloc
{
public:
	MemoryAlloc()
	{
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nBlockSize = 0;
		_nBlocknum = 0; 
		xPrintf("MemoryAlloc\n");
	}
	~MemoryAlloc()
	{
		if (_pBuf)
			free(_pBuf);
	}
	//�����ڴ�
	void* Memalloc(size_t size)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuf)
		{
			initMemory();
		}
		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader)
		{
			pReturn = (MemoryBlock*)malloc(_nBlockSize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			printf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, size);

		}
		else
		{
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}
		xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, size);
		return ((char*)pReturn +sizeof(MemoryBlock));
	}
	//�ͷ��ڴ�
	void Memfree(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		if (pBlock->bPool)
		{//�����ڴ��
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else
		{//�����ͷŵ�
			if (--pBlock->nRef != 0)
			{
				return;
			}
			free(pBlock);
		}
	}
	//��ʼ���ڴ��
	void initMemory()
	{
		xPrintf("initMemory:_nBlockSize=%d,_nBlocknum=%d\n", _nBlockSize, _nBlocknum);
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//�����ڴ�ش�С
		size_t bufsize = _nBlockSize + sizeof(MemoryBlock);
		size_t realSzie = bufsize * _nBlocknum;
		//��ϵͳ����ص��ڴ�
		_pBuf = (char*)malloc(realSzie);

		//��ʼ���ڴ��
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;
		//�����ڴ����г�ʼ��
		MemoryBlock* pTemp1 = _pHeader;

		for (size_t n = 1; n < _nBlocknum; n++)
		{
			MemoryBlock* pTemp2 = (MemoryBlock*)(_pBuf + (n * bufsize));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pAlloc = this;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
protected:
	//�ڴ�ص�ַ
	char* _pBuf;
	//ͷ���ڴ浥ԪHead
	MemoryBlock* _pHeader;
	//�ڴ浥Ԫ���С
	size_t _nBlockSize;
	//�ڴ浥Ԫ������
	size_t _nBlocknum;
	std::mutex _mutex;

};

template<size_t nSize,size_t nBlocksize>
class MemoryAlloctor:public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		const size_t n = sizeof(void*);
		_nBlockSize = (nSize / n) * n + (nSize % n ? n : 0);
		_nBlocknum = nBlocksize;
	}
};

//�ڴ������
class MemoryMgr
{
private:
	MemoryMgr()
	{
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		init_szAlloc(129, 256, &_mem256);
		init_szAlloc(257, 512, &_mem512);
		init_szAlloc(513, 1024, &_mem1024);
		xPrintf("MemoryMgr\n");
	}
	~MemoryMgr()
	{

	}
public:
	static MemoryMgr& Instance()
	{//����ģʽ
		static MemoryMgr mgr;
		return mgr;
	}
	//�����ڴ�
	void* Memalloc(size_t nSize)
	{
		if (nSize <= MAX_MEMORY_SZIE)
		{
			return _szAlloc[nSize]->Memalloc(nSize);
		}
		else
		{
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
			return ((char*)pReturn + sizeof(MemoryBlock));
		}
	}
	//�ͷ��ڴ�
	void Memfree(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		xPrintf("freeMem: %llx, id=%d\n", pBlock, pBlock->nID);
		if (pBlock->bPool)
		{
			pBlock->pAlloc->Memfree(pMem);
		}
		else
		{
			if (--pBlock->nRef == 0)
				free(pBlock);
		}
	}
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		++pBlock->nRef;
	}
private:
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA)
	{
		for (int n = nBegin; n <= nEnd; n++)
		{
			_szAlloc[n] = pMemA;
		}
	}

	MemoryAlloctor<64, 100000> _mem64;
	MemoryAlloctor<128, 100000> _mem128;
	MemoryAlloctor<256, 100000> _mem256;
	MemoryAlloctor<512, 100000> _mem512;
	MemoryAlloctor<1024, 100000> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SZIE + 1];
};
 