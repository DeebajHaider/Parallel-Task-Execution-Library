#include "tasksys.h"
#include <vector>
#include <iostream>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads), num_threads(num_threads) { 
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

    static void worker_thread_main(IRunnable* runnable, int num_total_tasks, std::atomic<int>* next_task_id_atomic) {
        int task_id;
        while (true) {
            // Atomically fetch the current task ID and then increment it, each thread will get a unique task ID
            task_id = next_task_id_atomic->fetch_add(1, std::memory_order_relaxed);

            if (task_id >= num_total_tasks) {
                break;
            }

            runnable->runTask(task_id, num_total_tasks);
        }
    }


void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
        if (num_total_tasks <= 0) { //check
            return;
        }

        // Determine the number of threads to actually use
        const int threads_to_use = std::min(this->num_threads, num_total_tasks);

        if (threads_to_use == 0) {  // extra check
            if (this->num_threads == 0 && num_total_tasks > 0) {
                for (int i = 0; i < num_total_tasks; ++i) {
                    runnable->runTask(i, num_total_tasks);
                }
            }
            return;
        }
        
        std::vector<std::thread> worker_threads;
        worker_threads.reserve(threads_to_use);

        // Shared atomic counter for the next task ID. To allow for the better task allocation
        std::atomic<int> next_task_id_atomic(0);

        // Create threads
        for (int i = 0; i < threads_to_use; ++i) {
            worker_threads.emplace_back(worker_thread_main, runnable, num_total_tasks, &next_task_id_atomic);
        }

        // Wait for all worker threads to complete
        for (auto& t : worker_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

static void worker_thread_spinning(std::queue<TaskSystemParallelThreadPoolSpinning::JobItem>* job_queue, std::mutex& queuelock, 
    std::atomic<bool>* stop_flag) {

    while(!stop_flag->load()) {  // keep thread alive  until destructor signals stop
        TaskSystemParallelThreadPoolSpinning::JobItem job_item(nullptr, 0, 0, nullptr);
        {
            std::lock_guard<std::mutex> lock(queuelock);
            if (!job_queue->empty()) {
                job_item = job_queue->front();
                job_queue->pop();
            }
        }

        if (job_item.runnable != nullptr) {
            job_item.runnable->runTask(job_item.current_task_id, job_item.num_total_tasks_ji);

            if (job_item.completion_counter) {
                job_item.completion_counter->fetch_sub(1, std::memory_order_relaxed);
            }
        } else {
            // If no job is available, just wait, spinnning
            std::this_thread::yield(); // Yield to allow other threads to run
        }
    }
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads), stop_flag(false) {
    thread_pool.reserve(num_threads);
    // create worker threads once
    for (int i = 0; i < num_threads; ++i) {
        thread_pool.emplace_back(worker_thread_spinning,&job_queue, std::ref(queuelock), &stop_flag);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    stop_flag.store(true, std::memory_order_release); // Signal threads to stop
    for (auto& thread : thread_pool) {
        if (thread.joinable()) {
            thread.join(); // Wait for all threads to finish
        }
    }
}



void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // make a atomic couter equal to the number of tasks
    std::atomic<int> completion_counter(num_total_tasks);
    // also make numtasks JobItems and push them to the queue

    for (int i = 0; i < num_total_tasks; i++) {
        JobItem job_item(runnable, num_total_tasks, i, &completion_counter);
        {
            std::lock_guard<std::mutex> lock(queuelock);
            job_queue.push(job_item);
        }
    }

    // Wait for all tasks to complete
    while (completion_counter.load(std::memory_order_relaxed) > 0) {
        // Spin-wait until all tasks are completed
        std::this_thread::yield(); //spin wait
    }

}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}