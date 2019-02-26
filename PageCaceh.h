#pragma once

#include "Common.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_inst;
	}

	Span* NewSpan(size_t npage);
	Span* _NewSpan(size_t npage);
	// ��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);
	// �ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
	void ReleaseSpanToPageCahce(Span* span);

private:
	SpanList _pagelist[NPAGES];
private:
	PageCache() = default;
	PageCache(const PageCache&) = delete;
	static PageCache _inst;
	std::mutex _mtx;
	//std::unordered_map<PageID, Span*> _id_span_map;
	std::unordered_map<PageID, Span*> _id_span_map;
};