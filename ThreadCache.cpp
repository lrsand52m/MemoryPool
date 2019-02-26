#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t index, size_t byte)
{
	FreeList* freelist = &_freelist[index];
	size_t num_to_move = min(ClassSize::NumMoveSize(byte), freelist->MaxSize());
	void* start, *end;
	size_t fetchnum = CentralCache::GetInstance()->FetchRangeObj(start, end, num_to_move, byte);
	if (fetchnum > 1)
		freelist->PushRange(NEXT_OBJ(start), end, fetchnum-1);

	if (num_to_move == freelist->MaxSize())
	{
		freelist->SetMaxSize(num_to_move + 1);
	}

	return start;
}

void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAXBYTES);

	// ����ȡ��
	size = ClassSize::Roundup(size);
	size_t index = ClassSize::Index(size);
	FreeList* freelist = &_freelist[index];
	if (!freelist->Empty())
	{
		return freelist->Pop();
	}
	else
	{
		return FetchFromCentralCache(index, size);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t byte)
{
	assert(byte <= MAXBYTES);
	size_t index = ClassSize::Index(byte);
	FreeList* freelist = &_freelist[index];
	freelist->Push(ptr);

	// ���������������������һ�����������Ļ����ƶ�������ʱ
	// ��ʼ���ն������Ļ���
	if (freelist->Size() >= freelist->MaxSize())
	{
		ListTooLong(freelist, byte);
	}

	// thread cache�ܵ��ֽ�������2M����ʼ�ͷ�
}

void ThreadCache::ListTooLong(FreeList* freelist, size_t byte)
{
	void* start = freelist->Clear();
	CentralCache::GetInstance()->ReleaseListToSpans(start, byte);
}