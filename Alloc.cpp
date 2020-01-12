#include "Alloc.h"
#include "MemoryMgr.hpp"

void* operator new(size_t size)
{
	return MemoryMgr::Instance().Memalloc(size);
}

void operator delete(void* p)
{
	MemoryMgr::Instance().Memfree(p);
}
void* operator new[](size_t size)
{
	return MemoryMgr::Instance().Memalloc(size);
}
void operator delete[](void* p)
{
	MemoryMgr::Instance().Memfree(p);
}
void* mem_alloc(size_t size)
{
	return malloc(size);
}

void mem_free(void* p)
{
	free(p);
}