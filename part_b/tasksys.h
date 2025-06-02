#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <atomic>
#include <algorithm>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */

// Helper struct for individual sub-tasks placed in the ready queue
struct SubTask {
    IRunnable* runnable;
    int task_idx;
    int total_tasks_in_bulk;
    TaskID bulk_task_id;
};

class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
public:
    TaskSystemParallelThreadPoolSleeping(int num_threads);
    ~TaskSystemParallelThreadPoolSleeping();
    const char* name();
    void run(IRunnable* runnable, int num_total_tasks);
    TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                            const std::vector<TaskID>& deps);
    void sync();

private:
    void worker_thread_loop();
    
    // Helper to enqueue sub-tasks of a ready bulk task
    // Takes runnable and num_total_tasks as arguments to avoid re-locking _task_graph_mutex
    void schedule_bulk_task(TaskID bulk_id, IRunnable* runnable, int num_total_sub_tasks);
    
    void handle_bulk_task_completion(TaskID bulk_id);

    int _num_threads;
    std::vector<std::thread> _threads_pool;
    std::atomic<bool> _killed; // Signal to stop threads

    // Ready queue for individual sub-tasks
    std::queue<SubTask> _ready_sub_task_queue;
    std::mutex _ready_queue_mutex;
    std::condition_variable _ready_queue_cv;

    // Task graph representation
    // Each node represents a bulk task launch
    struct BulkTaskNode {
        TaskID id;
        IRunnable* runnable_ptr; // Pointer to the runnable object for this bulk task
        int num_total_sub_tasks; // Total sub-tasks in this bulk launch
        
        std::atomic<int> remaining_sub_tasks_count; // Sub-tasks yet to be executed by workers
        std::atomic<int> unsatisfied_dependencies_count; // Prerequisite bulk tasks not yet complete
        
        std::vector<TaskID> successor_bulk_task_ids; // Bulk tasks that depend on this one
        bool sub_tasks_scheduled; // Flag to prevent scheduling sub-tasks multiple times

        BulkTaskNode(TaskID tid, IRunnable* r, int n_tasks)
            : id(tid), runnable_ptr(r), num_total_sub_tasks(n_tasks),
              remaining_sub_tasks_count(n_tasks), unsatisfied_dependencies_count(0), // Initially 0, calculated later
              sub_tasks_scheduled(false) {}
        
        // Default constructor for map compatibility (e.g. using .at() or [])
        BulkTaskNode() : BulkTaskNode(-1, nullptr, 0) {} 
    };
    std::map<TaskID, BulkTaskNode> _task_graph_nodes;
    std::mutex _task_graph_mutex; // Protects _task_graph_nodes map structure and node's successor_list

    std::atomic<TaskID> _next_bulk_task_id_counter; // Generates unique TaskIDs for bulk launches
    std::atomic<int> _active_bulk_tasks_count;   // Counts bulk tasks submitted but not yet fully completed (for sync())

    // Sync mechanism for the sync() method
    std::mutex _sync_op_mutex;
    std::condition_variable _sync_op_cv; // CV for sync() to wait on
};

#endif