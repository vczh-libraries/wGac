#include "WGacAsyncService.h"

namespace vl {
namespace presentation {
namespace wayland {

using namespace collections;

WGacAsyncService::TaskItem::TaskItem(const Func<void()>& _proc, bool _waitable)
    : proc(_proc)
    , waitable(_waitable)
{
    if (waitable)
    {
        CHECK_ERROR(
            completedEvent.CreateManualUnsignal(false),
            L"vl::presentation::wayland::WGacAsyncService::TaskItem::TaskItem(const Func<void()>&, bool)#Failed to create the completion event."
            );
    }
}

WGacAsyncService::TaskItem::~TaskItem()
{
}

void WGacAsyncService::TaskItem::Execute()
{
    SPIN_LOCK(stateLock)
    {
        if (status != Status::Pending)
        {
            return;
        }
        status = Status::Executing;
    }

    try
    {
        proc();
    }
    catch (...)
    {
        SPIN_LOCK(stateLock)
        {
            status = Status::Failed;
        }
        if (waitable)
        {
            completedEvent.Signal();
        }
        throw;
    }

    SPIN_LOCK(stateLock)
    {
        status = Status::Executed;
    }
    if (waitable)
    {
        completedEvent.Signal();
    }
}

bool WGacAsyncService::TaskItem::Cancel()
{
    bool canceled = false;
    SPIN_LOCK(stateLock)
    {
        if (status == Status::Pending)
        {
            status = Status::Canceled;
            canceled = true;
        }
    }
    if (canceled && waitable)
    {
        completedEvent.Signal();
    }
    return canceled;
}

bool WGacAsyncService::TaskItem::Wait(vint milliseconds)
{
    CHECK_ERROR(
        waitable,
        L"vl::presentation::wayland::WGacAsyncService::TaskItem::Wait(vint)#Only a waitable task can be waited for."
        );

    bool signaled = milliseconds < 0
        ? completedEvent.Wait()
        : completedEvent.WaitForTime(milliseconds);
    if (!signaled)
    {
        if (Cancel())
        {
            return false;
        }

        // The task started while the timeout was expiring. Its callback can
        // reference the waiting caller's stack, so keep waiting for completion.
        if (!completedEvent.Wait())
        {
            return false;
        }
    }

    bool executed = false;
    SPIN_LOCK(stateLock)
    {
        executed = status == Status::Executed;
    }
    return executed;
}

WGacAsyncService::DelayItem::DelayItem(WGacAsyncService* _service, const Func<void()>& _proc, bool _executeInMainThread, vint milliseconds)
    : service(_service)
    , proc(_proc)
    , status(INativeDelay::Pending)
    , executeTime(DateTime::LocalTime().Forward(milliseconds))
    , executeInMainThread(_executeInMainThread)
{
}

WGacAsyncService::DelayItem::~DelayItem()
{
}

INativeDelay::ExecuteStatus WGacAsyncService::DelayItem::GetStatus()
{
    ExecuteStatus result;
    SPIN_LOCK(stateLock)
    {
        result = status;
    }
    return result;
}

bool WGacAsyncService::DelayItem::Delay(vint milliseconds)
{
    SPIN_LOCK(service->taskListLock)
    {
        SPIN_LOCK(stateLock)
        {
            if (!service->stopped && status == INativeDelay::Pending)
            {
                executeTime = DateTime::LocalTime().Forward(milliseconds);
                return true;
            }
        }
    }
    return false;
}

bool WGacAsyncService::DelayItem::Cancel()
{
    SPIN_LOCK(service->taskListLock)
    {
        SPIN_LOCK(stateLock)
        {
            if (status == INativeDelay::Pending)
            {
                if (service->delayItems.Remove(this))
                {
                    status = INativeDelay::Canceled;
                    return true;
                }
            }
        }
    }
    return false;
}

WGacAsyncService::WGacAsyncService()
    : mainThreadId(Thread::GetCurrentThreadId())
{
}

WGacAsyncService::~WGacAsyncService()
{
    Stop();
}

void WGacAsyncService::Stop()
{
    List<Ptr<TaskItem>> canceledTasks;
    SPIN_LOCK(taskListLock)
    {
        if (stopped)
        {
            return;
        }

        stopped = true;
        CopyFrom(canceledTasks, taskItems);
        taskItems.Clear();
        for (auto delay : delayItems)
        {
            SPIN_LOCK(delay->stateLock)
            {
                if (delay->status == INativeDelay::Pending)
                {
                    delay->status = INativeDelay::Canceled;
                }
            }
        }
        delayItems.Clear();
    }

    for (auto task : canceledTasks)
    {
        task->Cancel();
    }
}

void WGacAsyncService::ExecuteAsyncTasks()
{
    DateTime now = DateTime::LocalTime();
    List<Ptr<TaskItem>> items;
    List<Ptr<DelayItem>> executableDelayItems;

    SPIN_LOCK(taskListLock)
    {
        if (stopped)
        {
            return;
        }
        CopyFrom(items, taskItems);
        taskItems.Clear();
        for (vint i = delayItems.Count() - 1; i >= 0; i--)
        {
            Ptr<DelayItem> item = delayItems[i];
            if (now >= item->executeTime)
            {
                SPIN_LOCK(item->stateLock)
                {
                    item->status = INativeDelay::Executing;
                }
                executableDelayItems.Add(item);
                delayItems.RemoveAt(i);
            }
        }
    }

    for (vint i = 0; i < items.Count(); i++)
    {
        try
        {
            items[i]->Execute();
        }
        catch (...)
        {
            for (vint j = i + 1; j < items.Count(); j++)
            {
                items[j]->Cancel();
            }
            throw;
        }
    }

    for (auto item : executableDelayItems)
    {
        if (item->executeInMainThread)
        {
            item->proc();
            SPIN_LOCK(item->stateLock)
            {
                item->status = INativeDelay::Executed;
            }
        }
        else
        {
            InvokeAsync([=]()
            {
                item->proc();
                SPIN_LOCK(item->stateLock)
                {
                    item->status = INativeDelay::Executed;
                }
            });
        }
    }
}

bool WGacAsyncService::IsInMainThread(INativeWindow* window)
{
    return Thread::GetCurrentThreadId() == mainThreadId;
}

void WGacAsyncService::InvokeAsync(const Func<void()>& proc)
{
    ThreadPoolLite::Queue(proc);
}

void WGacAsyncService::InvokeInMainThread(INativeWindow* window, const Func<void()>& proc)
{
    auto item = Ptr(new TaskItem(proc, false));
    SPIN_LOCK(taskListLock)
    {
        if (!stopped)
        {
            taskItems.Add(item);
        }
    }
}

bool WGacAsyncService::InvokeInMainThreadAndWait(INativeWindow* window, const Func<void()>& proc, vint milliseconds)
{
    auto item = Ptr(new TaskItem(proc, true));
    bool accepted = false;
    SPIN_LOCK(taskListLock)
    {
        if (!stopped)
        {
            taskItems.Add(item);
            accepted = true;
        }
    }

    if (!accepted)
    {
        item->Cancel();
        return false;
    }
    return item->Wait(milliseconds);
}

Ptr<INativeDelay> WGacAsyncService::DelayExecute(const Func<void()>& proc, vint milliseconds)
{
    Ptr<DelayItem> delay;
    SPIN_LOCK(taskListLock)
    {
        delay = Ptr(new DelayItem(this, proc, false, milliseconds));
        if (stopped)
        {
            SPIN_LOCK(delay->stateLock)
            {
                delay->status = INativeDelay::Canceled;
            }
        }
        else
        {
            delayItems.Add(delay);
        }
    }
    return delay;
}

Ptr<INativeDelay> WGacAsyncService::DelayExecuteInMainThread(const Func<void()>& proc, vint milliseconds)
{
    Ptr<DelayItem> delay;
    SPIN_LOCK(taskListLock)
    {
        delay = Ptr(new DelayItem(this, proc, true, milliseconds));
        if (stopped)
        {
            SPIN_LOCK(delay->stateLock)
            {
                delay->status = INativeDelay::Canceled;
            }
        }
        else
        {
            delayItems.Add(delay);
        }
    }
    return delay;
}

}
}
}
