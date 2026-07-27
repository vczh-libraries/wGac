#ifndef WGAC_ASYNCSERVICE_H
#define WGAC_ASYNCSERVICE_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacAsyncService : public INativeAsyncService
{
protected:
    class TaskItem : public Object
    {
    protected:
        enum class Status
        {
            Pending,
            Executing,
            Executed,
            Failed,
            Canceled,
        };

        SpinLock stateLock;
        EventObject completedEvent;
        Func<void()> proc;
        Status status = Status::Pending;
        bool waitable;

    public:
        TaskItem(const Func<void()>& _proc, bool _waitable);
        ~TaskItem();

        void Execute();
        bool Cancel();
        bool Wait(vint milliseconds);
    };

    class DelayItem : public Object, public INativeDelay
    {
    public:
        DelayItem(WGacAsyncService* _service, const Func<void()>& _proc, bool _executeInMainThread, vint milliseconds);
        ~DelayItem();

        WGacAsyncService* service;
        Func<void()> proc;
        SpinLock stateLock;
        ExecuteStatus status;
        DateTime executeTime;
        bool executeInMainThread;

        ExecuteStatus GetStatus() override;
        bool Delay(vint milliseconds) override;
        bool Cancel() override;
    };

    collections::List<Ptr<TaskItem>> taskItems;
    collections::List<Ptr<DelayItem>> delayItems;
    SpinLock taskListLock;
    vint mainThreadId;
    bool stopped = false;

public:
    WGacAsyncService();
    ~WGacAsyncService();

    void Stop();
    void ExecuteAsyncTasks();
    bool IsInMainThread(INativeWindow* window) override;
    void InvokeAsync(const Func<void()>& proc) override;
    void InvokeInMainThread(INativeWindow* window, const Func<void()>& proc) override;
    bool InvokeInMainThreadAndWait(INativeWindow* window, const Func<void()>& proc, vint milliseconds) override;
    Ptr<INativeDelay> DelayExecute(const Func<void()>& proc, vint milliseconds) override;
    Ptr<INativeDelay> DelayExecuteInMainThread(const Func<void()>& proc, vint milliseconds) override;
};

}
}
}

#endif // WGAC_ASYNCSERVICE_H
