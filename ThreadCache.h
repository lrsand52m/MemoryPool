#pragma once

#include "Common.h"

class ThreadCache
{
public:
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	// 从中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t size);
	// 链表中对象太多，开始回收。
	void ListTooLong(FreeList* freelist, size_t byte);
private:
	FreeList _freelist[NLISTS];
};
//TLS技术，不是所有平台都支持
static _declspec(thread) ThreadCache* tls_threadcache = nullptr;