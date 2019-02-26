#include "ConcurrentAlloc.h"

void TestThreadCache()
{
	/*std::thread t1(ConcurrentAlloc, 10);
	std::thread t2(ConcurrentAlloc, 10);

	t1.join();
	t2.join();*/

	std::vector<void*> v;
	for (size_t i = 0; i < 10; ++i)
	{
		v.push_back(ConcurrentAlloc(65*1024));
		cout << v.back() << endl;
	}
	cout << endl << endl;

	for (size_t i = 0; i < 10; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();

	for (size_t i = 0; i < 10; ++i)
	{
		v.push_back(ConcurrentAlloc(10));
		cout << v.back() << endl;
	}

	for (size_t i = 0; i < 10; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();
}

void TestPageCache()
{
	//void* ptr = VirtualAlloc(NULL, (NPAGES - 1) << PAGE_SHIFT, MEM_RESERVE, PAGE_READWRITE);
	void* ptr = malloc((NPAGES - 1) << PAGE_SHIFT);
	cout << ptr << endl;
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	PageID pageid = (PageID)ptr >> PAGE_SHIFT;
	cout << pageid << endl;

	void* shiftptr = (void*)(pageid << PAGE_SHIFT);
	cout << shiftptr << endl;
}

void TestConcurrentAlloc()
{
	size_t n = 200;
	std::vector<void*> v;
	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(65*1024));
		cout << v.back() << endl;
	}
	cout << endl << endl;

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();


	for (size_t i = 0; i < n; ++i)
	{
		v.push_back(ConcurrentAlloc(100));
		//cout << v.back() << endl;
	}

	for (size_t i = 0; i < n; ++i)
	{
		ConcurrentFree(v[i]);
	}
	v.clear();
}

int main()
{
	//TestPageCache();
	TestConcurrentAlloc();
	//TestThreadCache();
	system("pause");
	return 0;
}