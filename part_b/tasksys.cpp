#include "tasksys.h"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <atomic>
#include <algorithm>


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
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads)
    : ITaskSystem(num_threads),
      _num_threads(num_threads),
      _killed(false),
      _next_bulk_task_id_counter(0),
      _active_bulk_tasks_count(0) {
    _threads_pool.reserve(num_threads);
    for (int i = 0; i < _num_threads; ++i) {
        _threads_pool.emplace_back(&TaskSystemParallelThreadPoolSleeping::worker_thread_loop, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    _killed = true;
    _ready_queue_cv.notify_all(); // Wake up all threads
    for (int i = 0; i < _num_threads; ++i) {
        if (_threads_pool[i].joinable()) {
            _threads_pool[i].join();
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::worker_thread_loop() {
    while (true) {
        SubTask current_sub_task;
        bool task_dequeued = false;

        { // Lock scope for ready queue
            std::unique_lock<std::mutex> lock(_ready_queue_mutex);
            _ready_queue_cv.wait(lock, [this] {
                return _killed.load() || !_ready_sub_task_queue.empty();
            });

            if (_killed.load() && _ready_sub_task_queue.empty()) {
                return; // Exit thread
            }

            if (!_ready_sub_task_queue.empty()) {
                current_sub_task = _ready_sub_task_queue.front();
                _ready_sub_task_queue.pop();
                task_dequeued = true;
            }
        } // Unlock _ready_queue_mutex

        if (task_dequeued) {
            current_sub_task.runnable->runTask(current_sub_task.task_idx, current_sub_task.total_tasks_in_bulk);

            // This sub-task is done. Update the count for its bulk task.
            TaskID completed_bulk_id = current_sub_task.bulk_task_id;
            bool all_sub_tasks_of_bulk_done = false;

            { // Lock scope for task graph
                std::lock_guard<std::mutex> graph_lock(_task_graph_mutex);
                // Check if node exists, critical if tasks can be removed, but here we assume they stay until sync.
                auto it = _task_graph_nodes.find(completed_bulk_id);
                if (it != _task_graph_nodes.end()) {
                     // Decrement and check if this was the last sub-task for this bulk task
                    if (--(it->second.remaining_sub_tasks_count) == 0) {
                        all_sub_tasks_of_bulk_done = true;
                    }
                }
            } // Unlock _task_graph_mutex

            if (all_sub_tasks_of_bulk_done) {
                handle_bulk_task_completion(completed_bulk_id);
            }
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::schedule_bulk_task(TaskID bulk_id, IRunnable* runnable, int num_total_sub_tasks) {
    { // Lock scope for ready queue
        std::lock_guard<std::mutex> lock(_ready_queue_mutex);
        for (int i = 0; i < num_total_sub_tasks; ++i) {
            _ready_sub_task_queue.push({runnable, i, num_total_sub_tasks, bulk_id});
        }
    }
    _ready_queue_cv.notify_all(); // Notify one or all; all is safer/simpler
}

void TaskSystemParallelThreadPoolSleeping::handle_bulk_task_completion(TaskID completed_bulk_id) {
    std::vector<std::tuple<TaskID, IRunnable*, int>> newly_runnable_tasks_info;
    std::vector<TaskID> newly_completed_zero_sub_task_tasks;

    { // Lock scope for task graph
        std::lock_guard<std::mutex> graph_lock(_task_graph_mutex);
        auto it_completed = _task_graph_nodes.find(completed_bulk_id);
        if (it_completed == _task_graph_nodes.end()) {
            // Should not happen if logic is correct
            return; 
        }

        // This bulk task is complete. Process its successors.
        for (TaskID successor_id : it_completed->second.successor_bulk_task_ids) {
            auto it_successor = _task_graph_nodes.find(successor_id);
            if (it_successor != _task_graph_nodes.end()) {
                // Decrement dependency count for the successor
                if (--(it_successor->second.unsatisfied_dependencies_count) == 0) {
                    // Successor's dependencies are now met.
                    if (it_successor->second.num_total_sub_tasks > 0) {
                        if (!it_successor->second.sub_tasks_scheduled) { // Check before adding
                             it_successor->second.sub_tasks_scheduled = true;
                             newly_runnable_tasks_info.emplace_back(successor_id, it_successor->second.runnable_ptr, it_successor->second.num_total_sub_tasks);
                        }
                    } else {
                        // This successor is a 0-sub-task bulk task and its dependencies are met. its done
                        newly_completed_zero_sub_task_tasks.push_back(successor_id);
                    }
                }
            }
        }
    } // Unlock _task_graph_mutex

    // Schedule newly runnable tasks (those with >0 sub-tasks)
    for (const auto& task_info_tuple : newly_runnable_tasks_info) {
        schedule_bulk_task(std::get<0>(task_info_tuple), std::get<1>(task_info_tuple), std::get<2>(task_info_tuple));
    }

    // Recursively handle completion for 0-sub-task bulk tasks that just became "complete"
    for (TaskID zero_task_id : newly_completed_zero_sub_task_tasks) {
        handle_bulk_task_completion(zero_task_id); // This will decrement _active_bulk_tasks_count for it
    }

    // Decrement count of active bulk tasks. If it reaches zero, notify sync().
    if (--_active_bulk_tasks_count == 0) {
        std::lock_guard<std::mutex> sync_lock(_sync_op_mutex);
        _sync_op_cv.notify_all();
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    if (num_total_tasks <= 0) { // Handle edge case of 0 or negative tasks
        return;
    }
    // According to problem: "You can asume all programs will either call only run() 
    // or only runAsyncWithDeps()." This simplifies run() to be a synchronous version of runAsync.
    TaskID task_id = runAsyncWithDeps(runnable, num_total_tasks, {});
    sync(); // Waits for ALL tasks, which in this scenario (only run() calls) means this one.
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    if (num_total_tasks < 0) num_total_tasks = 0; // Treat negative as zero tasks

    TaskID new_bulk_task_id = _next_bulk_task_id_counter++;
    _active_bulk_tasks_count++; // Increment count of tasks for sync()

    bool ready_to_schedule_immediately = false;
    bool is_zero_sub_task_and_deps_met = false;

    { // Lock scope for task graph
        std::lock_guard<std::mutex> graph_lock(_task_graph_mutex);
        
        // Create the node for this new bulk task
        // Use emplace to construct in-place to avoid copy assignment issues with std::atomic.
        auto emplace_result = _task_graph_nodes.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(new_bulk_task_id),
            std::forward_as_tuple(new_bulk_task_id, runnable, num_total_tasks)
        );
        BulkTaskNode& new_node = emplace_result.first->second; // Get a reference

        int initial_unsatisfied_deps = 0;
        if (!deps.empty()) {
            for (TaskID dep_id : deps) {
                auto it_dep = _task_graph_nodes.find(dep_id);
                if (it_dep != _task_graph_nodes.end()) {
                    // If the dependency bulk task is not yet complete (still has sub-tasks or is a 0-task dep not yet "handled")
                    // A bulk task is complete if its remaining_sub_tasks_count is 0 AND it has been processed by handle_bulk_task_completion.
                    if (it_dep->second.remaining_sub_tasks_count > 0) {
                        initial_unsatisfied_deps++;
                        it_dep->second.successor_bulk_task_ids.push_back(new_bulk_task_id);
                    }
                    // If dependency is already 0 remaining_sub_tasks, it's considered met.
                } else {
                    //This shoudl neber happen. ignrnign for error 
                }
            }
        }
        new_node.unsatisfied_dependencies_count = initial_unsatisfied_deps;

        if (initial_unsatisfied_deps == 0) {
            if (num_total_tasks > 0) {
                 if (!new_node.sub_tasks_scheduled) { // Check before scheduling
                    new_node.sub_tasks_scheduled = true;
                    ready_to_schedule_immediately = true;
                 }
            } else {
                // 0-sub-task bulk task with all dependencies (0 in this case) met.
                is_zero_sub_task_and_deps_met = true;
            }
        }
    }

    if (ready_to_schedule_immediately) {
        schedule_bulk_task(new_bulk_task_id, runnable, num_total_tasks);
    } else if (is_zero_sub_task_and_deps_met) {
        // This 0-sub-task bulk task is "complete" as soon as it's created.
        // (since its remaining_sub_tasks_count is already 0 from constructor)
        handle_bulk_task_completion(new_bulk_task_id);
    }
    
    return new_bulk_task_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {
    std::unique_lock<std::mutex> lock(_sync_op_mutex);
    _sync_op_cv.wait(lock, [this] {
        return _active_bulk_tasks_count.load() == 0;
    });
}