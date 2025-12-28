#ifndef WGAC_ASYNCSERVICE_H
#define WGAC_ASYNCSERVICE_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacAsyncService : public INativeAsyncService
{
protected:
    struct TaskItem
    {
        Semaphore* semaphore;
        Func<void()> proc;

        TaskItem();
        TaskItem(Semaphore* _semaphore, const Func<void()>& _proc);
        ~TaskItem();
    };

    class DelayItem : public Object, public INativeDelay
    {
    public:
        DelayItem(WGacAsyncService* _service, const Func<void()>& _proc, bool _executeInMainThread, vint milliseconds);
        ~DelayItem();

        WGacAsyncService* service;
        Func<void()> proc;
        ExecuteStatus status;
        DateTime executeTime;
        bool executeInMainThread;

        ExecuteStatus GetStatus() override;
        bool Delay(vint milliseconds) override;
        bool Cancel() override;
    };

    collections::List<TaskItem> taskItems;
    collections::List<Ptr<DelayItem>> delayItems;
    SpinLock taskListLock;
    vint mainThreadId;

public:
    WGacAsyncService();
    ~WGacAsyncService();

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
