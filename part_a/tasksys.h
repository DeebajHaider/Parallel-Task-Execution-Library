#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

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

    private:
        int num_threads;
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

        struct JobItem {
            IRunnable* runnable;       
            int num_total_tasks_ji; 
            int current_task_id;      
            std::atomic<int>* completion_counter; 

            JobItem(IRunnable* task_runnable,
                    int total_tasks_for_group,
                    int task_id_for_this_item,
                    std::atomic<int>* counter_for_group)
                : runnable(task_runnable),
                num_total_tasks_ji(total_tasks_for_group),
                current_task_id(task_id_for_this_item),
                completion_counter(counter_for_group) {
            }
        };

    private:
        std::vector<std::thread> thread_pool;
        std::queue<JobItem> job_queue; 
        std::mutex queuelock; 
        std::atomic<bool> stop_flag; // Flag to signal threads to stop on destruction
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();

        struct JobItemSleep {
            IRunnable* runnable;       
            int num_total_tasks_ji; 
            int current_task_id;      
            std::atomic<int>* completion_counter; 

            JobItemSleep(IRunnable* task_runnable,
                    int total_tasks_for_group,
                    int task_id_for_this_item,
                    std::atomic<int>* counter_for_group
                    )
                : runnable(task_runnable),
                num_total_tasks_ji(total_tasks_for_group),
                current_task_id(task_id_for_this_item),
                completion_counter(counter_for_group){
            }
        };


    private:
        std::vector<std::thread> thread_pool;
        std::queue<JobItemSleep> job_queue; 
        std::mutex queuelock; 
        std::atomic<bool> stop_flag; // Flag to signal threads to stop on destruction
        std::condition_variable task_available_cv; // Condition variable to signal available tasks
};

#endif
