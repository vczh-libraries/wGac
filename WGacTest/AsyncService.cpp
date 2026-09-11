#include "Services/WGacAsyncService.h"

using namespace vl;
using namespace vl::presentation::wayland;

TEST_FILE
{
	TEST_CASE(L"Nested modal pumping executes work already queued behind the modal")
	{
		WGacAsyncService service;
		vint step = 0;
		service.InvokeInMainThread(nullptr, [&]()
		{
			TEST_ASSERT(step == 0);
			step = 1;
			service.ExecuteAsyncTasks();
			TEST_ASSERT(step == 2);
			step = 3;
		});
		service.InvokeInMainThread(nullptr, [&]()
		{
			TEST_ASSERT(step == 1);
			step = 2;
		});
		service.ExecuteAsyncTasks();
		TEST_ASSERT(step == 3);
		service.ExecuteAsyncTasks();
		TEST_ASSERT(step == 3);
	});

	TEST_CASE(L"Stopping from a callback cancels the rest of the pending batch")
	{
		WGacAsyncService service;
		bool executed = false;
		service.InvokeInMainThread(nullptr, [&]() { service.Stop(); });
		service.InvokeInMainThread(nullptr, [&]() { executed = true; });
		service.ExecuteAsyncTasks();
		TEST_ASSERT(!executed);
	});
}

int main(int argc, char* argv[])
{
	auto result = unittest::UnitTest::RunAndDisposeTests(argc, argv);
	FinalizeGlobalStorage();
	return result;
}
