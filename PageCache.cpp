#include "PageCaceh.h"

PageCache PageCache::_inst;

Span* PageCache::_NewSpan(size_t npage)
{
	if (!_pagelist[npage].Empty())
	{
		return _pagelist[npage].PopFront();
	}
	for (size_t i = npage+1; i < NPAGES; ++i)
	{
		SpanList* pagelist = &_pagelist[i];
		if (!pagelist->Empty())
		{
			Span* span = pagelist->PopFront();
			Span* split = new Span;
			split->_pageid = span->_pageid + span->_npage - npage;
			split->_npage = npage;
			span->_npage -= npage;
			_pagelist[span->_npage].PushFront(span);

			for (size_t i = 0; i < split->_npage; ++i)
			{
				_id_span_map[split->_pageid + i] = split;
			}

			return split;
		}
	}

	// 需要向系统申请内存
	void* ptr = SystemAlloc(128);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	Span* largespan = new Span;
	largespan->_pageid = (PageID)ptr >> PAGE_SHIFT;
	largespan->_npage = NPAGES - 1;
	_pagelist[NPAGES - 1].PushFront(largespan);

	for (size_t i = 0; i < largespan->_npage; ++i)
	{
		_id_span_map[largespan->_pageid+i] = largespan;
	}

	return _NewSpan(npage);
}
Span* PageCache::NewSpan(size_t npage)
{
	std::unique_lock<std::mutex>lock(_mtx);
	if (npage >= NPAGES)
	{
		void* ptr = SystemAlloc(npage);
		Span* span = new Span;
		span->_pageid = (PageID)ptr >> PAGE_SHIFT;
		span->_npage = npage;
		span->_objsize = npage << PAGE_SHIFT;
		_id_span_map[span->_pageid] = span;
		return span;
	}
	Span* span = _NewSpan(npage);
	span->_objsize = npage << PAGE_SHIFT;
	return span;
}

// 获取从对象到span的映射
Span* PageCache::MapObjectToSpan(void* obj)
{
	PageID pageid = (PageID)obj >> PAGE_SHIFT;
	auto it = _id_span_map.find(pageid);
	assert(it != _id_span_map.end());

	return it->second;
}

// 释放空闲span回到Pagecache，并合并相邻的span
void PageCache::ReleaseSpanToPageCahce(Span* span)
{
	std::unique_lock<std::mutex>lock(_mtx);
	if (span->_npage>=NPAGES)
	{
		void* ptr = (void*)(span->_npage<<PAGE_SHIFT);
		_id_span_map.erase(span->_pageid);
		SystemFree(ptr);
		delete span;
		return;
	}
	auto previt = _id_span_map.find(span->_pageid - 1);
	while (previt != _id_span_map.end())
	{
		Span* prevspan = previt->second;
		// 不是空闲，则直接跳出
		if (prevspan->_usecount != 0)
			break;

		// 如果合并出超过NPAGES页的span，则不合并，否则没办法管理
		if (prevspan->_npage + span->_npage >= NPAGES)
			break;

		_pagelist[prevspan->_npage].Erase(prevspan);
		prevspan->_npage += span->_npage;
		delete span;
		span = prevspan;

		previt = _id_span_map.find(span->_pageid - 1);
	}

	auto nextit = _id_span_map.find(span->_pageid + span->_npage);
	while (nextit != _id_span_map.end())
	{
		Span* nextspan = nextit->second;
		if (nextspan->_usecount != 0)
			break;

		if (span->_npage + nextspan->_npage >= NPAGES)
			break;

		_pagelist[nextspan->_npage].Erase(nextspan);
		span->_npage += nextspan->_npage;
		delete nextspan;

		nextit = _id_span_map.find(span->_pageid + span->_npage);
	}

	for (size_t i = 0; i < span->_npage; ++i)
	{
		_id_span_map[span->_pageid + i] = span;
	}
	_pagelist[span->_npage].PushFront(span);
}