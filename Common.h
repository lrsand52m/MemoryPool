#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <map>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32


using std::cout;
using std::endl;

#include <assert.h>

// 管理对象自由链表的长度
const size_t NLISTS = 240;  //?
const size_t MAXBYTES = 64 * 1024;
const size_t PAGE_SHIFT = 12;
const size_t NPAGES = 129;

static inline void*& NEXT_OBJ(void* obj)
{
	return *((void**)obj);
}

static inline void* SystemAlloc(size_t npage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(NULL, (npage) << PAGE_SHIFT, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}
#else
	//brk,mmap
#endif
	return ptr;
}
static inline void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	//brk,mmap
#endif
	return;
}
typedef size_t PageID;
struct Span
{
	PageID _pageid = 0;			// 页号
	size_t _npage = 0;		// 页的数量

	Span*  _next = nullptr;
	Span*  _prev = nullptr;

	void*  _objlist = nullptr;	// 对象自由链表
	size_t _objsize = 0;		// 对象大小
	size_t _usecount = 0;		// 使用计数
};

class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* begin()
	{
		return _head->_next;
	}

	Span* end()
	{
		return _head;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}

	void Insert(Span* cur, Span* newspan)
	{
		assert(cur);
		Span* prev = cur->_prev;
		// prev newspan cur

		prev->_next = newspan;
		newspan->_prev = prev;
		newspan->_next = cur;
		cur->_prev = newspan;
	}

	void Erase(Span* cur)
	{
		assert(cur != nullptr && cur != _head);
		Span* prev = cur->_prev;
		Span* next = cur->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	void PushFront(Span* span)
	{
		Insert(begin(), span);
	}
	void PushBack(Span* span)
	{
		Insert(end(), span);
	}

	Span* PopFront()
	{
		Span* span = begin();
		Erase(span);
		return span;
	}
	Span* PopBack()
	{
		Span* span = _head->_prev;
		Erase(span);
		return span;
	}

	std::mutex _mtx;
private:
	Span* _head = nullptr;
};

class FreeList
{
public:
	bool Empty()
	{
		return _list == nullptr;
	}

	void PushRange(void* start, void* end, size_t num)
	{
		NEXT_OBJ(end) = _list;
		_list = start;
		_size += num;
	}

	void* Clear()
	{
		_size = 0;
		void * list = _list;
		_list = nullptr;
		return list;
	}

	void* Pop()
	{
		void* obj = _list;
		_list = NEXT_OBJ(obj);
		--_size;

		return obj;
	}

	void Push(void* obj)
	{
		NEXT_OBJ(obj) = _list;
		_list = obj;
		++_size;
	}
	size_t Size()
	{
		return _size;
	}

	void SetMaxSize(size_t maxsize)
	{
		_maxsize = maxsize;
	}

	size_t MaxSize()
	{
		return _maxsize;
	}
private:
	void* _list = nullptr;
	size_t _size = 0;
	size_t _maxsize = 10;
};

// 管理对齐映射
class ClassSize
{
	// 控制在12%左右的内碎片浪费
	// [1,128]				8byte对齐 freelist[0,16)
	// [129,1024]			16byte对齐 freelist[16,72)
	// [1025,8*1024]		128byte对齐 freelist[72,128)
	// [8*1024+1,64*1024]	512byte对齐 freelist[128,240)
public:
	static inline size_t _Roundup(size_t size, size_t align)
	{
		return (size + (align) - 1) & ~((align) - 1);
	}

	static inline size_t Roundup(size_t size)
	{
		assert(size <= MAXBYTES);

		if (size <= 128) {
			return _Roundup(size, 8);
		}
		else if (size <= 1024){
			return _Roundup(size, 16);
		}
		else if (size <= 8192){
			return _Roundup(size, 128);
		}
		else if (size <= 65536){
			return _Roundup(size, 512);
		}
		else {
			return -1;
		}
	}

	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes < MAXBYTES);

		// 每个区间有多少个自由链表
		static int group_array[3] = { 16, 72, 128 };

		if (bytes <= 128){
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024){
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8192){
			return _Index(bytes - 1024, 7) + group_array[1];
		}
		else if (bytes <= 65536){
			return _Index(bytes - 8192, 9) + group_array[2];
		}
		else{
			return -1;
		}
	}

	static size_t NumMoveSize(size_t size)
	{
		if (size <= 0)
			return 0;

		int num = (int)(MAXBYTES / size);
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}

	// 计算一次向系统获取几个页
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num*size;

		npage >>= 12;
		if (!npage)
			npage = 1;

		return npage;
	}
};
