
# PARALLEL AND DISTRIBUTED COMPUTING PROJECT: TASK EXECUTION LIBRARY #

## Team Members

<div align="center">
<table>
<tr>
<td align="center" width="50%">
<div>
<img src="1.jpg" width="120px" height="120px" style="border-radius: 50%; border: 4px solid #6366f1; object-fit: cover;" alt="Abdul Wasay Imran"/>
<h2><b>Abdul Wasay Imran</b></h2>
<h3><b>ERP: 27126</b></h3>
<br/>
</div>
</td>
<td align="center" width="50%">
<div>
<img src="2.png" width="120px" height="120px" style="border-radius: 50%; border: 4px solid #6366f1; object-fit: cover;" alt="Syed Muhammad Deebaj Haider Kazmi"/>
<h2><b>Syed Muhammad Deebaj Haider Kazmi</b></h2>
<h3><b>ERP: 26012</b></h3>
<br/>
</div>
</td>
</tr>
</table>
</div>

## Instructor
- Dr. Muhammad Saeed
- Shayan Shamsi (Teacher Assistant)


## Overview
This project implements a C++ library for efficient parallel task execution on multi-core CPUs. The library supports two main modes of operation:
1.  **Synchronous Bulk Task Launch (Part A):** Executes a given number of instances of the same task in parallel, with the `run()` call blocking until all instances complete.
2.  **Asynchronous Task Graph Execution (Part B):** Executes complex task graphs where tasks can have dependencies on others. Tasks are launched asynchronously via `runAsyncWithDeps()`, and a `sync()` call ensures all previously launched tasks are complete.

The goal was to explore different parallelization strategies, understand their trade-offs, and optimize for performance using thread pools and synchronization primitives.

## Part A: Synchronous Bulk Task Launch Implementations

We implemented three parallel versions for synchronous bulk task launches, in addition to the provided serial baseline:

### 1. `TaskSystemParallelSpawn`
*   **Concept:** For each call to `run()`, this system spawns a new set of worker threads (up to the configured `num_threads`).
*   **Implementation Details:**
    *   A `std::vector<std::thread>` is created within the `run()` method.
    *   Tasks are distributed dynamically among these worker threads using a shared `std::atomic<int> next_task_id_atomic` to ensure each task instance (from 0 to `num_total_tasks - 1`) is processed exactly once. Each thread fetches and increments this atomic counter to claim a task ID.
    *   The main thread waits for all spawned worker threads to complete using `thread.join()` before `run()` returns.
*   **Trade-offs:**
    *   Pros: Simple to implement synchronous behavior; isolates thread management to each `run()` call.
    *   Cons: High overhead due to repeated thread creation and destruction, especially for frequent calls to `run()` or short-lived tasks.

### 2. `TaskSystemParallelThreadPoolSpinning`
*   **Concept:** A fixed-size pool of worker threads is created once during system construction. These threads continuously spin, checking a shared job queue for tasks.
*   **Implementation Details:**
    *   Worker threads are created in the constructor and stored in `std::vector<std::thread> thread_pool`.
    *   A `std::queue<JobItem> job_queue` stores individual task instances. `JobItem` includes the `IRunnable*`, task ID, total tasks, and a pointer to a shared `std::atomic<int> completion_counter` for the bulk launch.
    *   Access to `job_queue` is protected by `std::mutex queuelock`.
    *   Worker threads loop, acquiring the lock, checking if the queue is non-empty, and popping a job if available. If the queue is empty, they `std::this_thread::yield()` and retry (spin-wait).
    *   The `run()` method enqueues all `num_total_tasks` as `JobItem`s. It then spin-waits (using `std::this_thread::yield()`) on the `completion_counter` until it reaches zero.
    *   A `std::atomic<bool> stop_flag` is used to signal worker threads to terminate during destruction.
*   **Trade-offs:**
    *   Pros: Reduces thread creation overhead significantly compared to `TaskSystemParallelSpawn`.
    *   Cons: Worker threads and the main thread consume CPU cycles while spin-waiting, which can be wasteful if no tasks are available or if workers are waiting for tasks that the main thread is trying to submit.

### 3. `TaskSystemParallelThreadPoolSleeping`
*   **Concept:** Similar to the spinning thread pool, but worker threads sleep when no work is available, and the main thread sleeps while waiting for task completion, reducing CPU wastage.
*   **Implementation Details:**
    *   Introduces `std::condition_variable task_available_cv` for worker threads to wait on when the `job_queue` is empty.
    *   Worker threads:
        *   Acquire `queuelock`.
        *   Wait on `task_available_cv` using `cv.wait(lock, [&]{ return !job_queue.empty() || stop_flag.load(); });`.
        *   If woken and `job_queue` has tasks, they pop and execute.
        *   After executing a task, they decrement the `completion_counter`. If it becomes zero, they notify `all_done_cv`.
    *   The `run()` method:
        *   Enqueues all `num_total_tasks` as `JobItemSleep`s. Each `JobItemSleep` now also carries pointers to a bulk-launch-specific `std::condition_variable all_done_cv` and `std::mutex all_done_mutex`.
        *   Notifies worker threads via `task_available_cv.notify_all()` after enqueuing jobs.
        *   Waits on `all_done_cv` until the `completion_counter` (for that specific bulk launch) reaches zero.
    *   `stop_flag` and `task_available_cv.notify_all()` are used in the destructor to cleanly shut down worker threads.
*   **Trade-offs:**
    *   Pros: Maximizes CPU efficiency by putting idle threads to sleep. Reduces contention if the main thread was previously spinning.
    *   Cons: Slightly more complex due to condition variable management. Potential for minor overhead from context switching if tasks are extremely short and arrive very rapidly.

## Part B: Asynchronous Task Graph Execution (`TaskSystemParallelThreadPoolSleeping`)

The `TaskSystemParallelThreadPoolSleeping` class was extended to support asynchronous task launches with dependencies.

### Key Concepts & Data Structures:
*   **`TaskID`**: A unique identifier for each bulk task launch, generated by `_next_bulk_task_id_counter`.
*   **`SubTask`**: Represents a single instance of a task within a bulk launch. Stored in `_ready_sub_task_queue`. Fields: `IRunnable*`, `task_idx`, `total_tasks_in_bulk`, `bulk_task_id`.
*   **`BulkTaskNode`**: Represents a bulk task launch in the dependency graph. Stored in `std::map<TaskID, BulkTaskNode> _task_graph_nodes`. Key fields:
    *   `id`: The `TaskID` of this bulk launch.
    *   `runnable_ptr`: The `IRunnable` object for this bulk task.
    *   `num_total_sub_tasks`: Total sub-tasks in this bulk launch.
    *   `remaining_sub_tasks_count` (atomic): Number of sub-tasks not yet completed by workers for this bulk task.
    *   `unsatisfied_dependencies_count` (atomic): Number of prerequisite bulk tasks that have not yet completed.
    *   `successor_bulk_task_ids`: A `std::vector<TaskID>` of bulk tasks that depend on this one.
    *   `sub_tasks_scheduled`: A boolean flag to prevent scheduling sub-tasks multiple times.
*   **`_ready_sub_task_queue` (std::queue<SubTask>)**: A queue of individual sub-tasks that are ready to be executed by worker threads. Protected by `_ready_queue_mutex` and signaled by `_ready_queue_cv`.
*   **`_task_graph_nodes` (std::map<TaskID, BulkTaskNode>)**: Stores all bulk task nodes, representing the task graph. Protected by `_task_graph_mutex`.
*   **`_active_bulk_tasks_count` (atomic)**: Counts the number of bulk tasks that have been submitted via `runAsyncWithDeps` but have not yet fully completed (all their sub-tasks finished and their completion processed). Used by `sync()`.
*   **`_sync_op_cv` and `_sync_op_mutex`**: Used by the `sync()` method to wait until `_active_bulk_tasks_count` becomes zero.

### Workflow:
1.  **`runAsyncWithDeps(runnable, num_total_tasks, deps)`**:
    *   Generates a new unique `TaskID`.
    *   Increments `_active_bulk_tasks_count`.
    *   Creates a `BulkTaskNode` for this new bulk launch.
    *   Populates its `unsatisfied_dependencies_count` by checking the status (`remaining_sub_tasks_count > 0`) of each `TaskID` in the `deps` vector. For each unsatisfied dependency, it adds the new `TaskID` to the dependency's `successor_bulk_task_ids` list.
    *   If `unsatisfied_dependencies_count` is 0 and `num_total_tasks > 0`:
        *   The bulk task is ready. Its individual sub-tasks (as `SubTask` objects) are enqueued into `_ready_sub_task_queue`.
        *   `_ready_queue_cv` is notified.
        *   The `sub_tasks_scheduled` flag in the `BulkTaskNode` is set.
    *   If `unsatisfied_dependencies_count` is 0 and `num_total_tasks == 0`:
        *   The 0-task bulk launch is immediately "completed". `handle_bulk_task_completion()` is called for it.
    *   Returns the new `TaskID`.

2.  **Worker Thread Loop (`worker_thread_loop`)**:
    *   Waits on `_ready_queue_cv` until `_ready_sub_task_queue` is not empty or the system is being killed.
    *   Dequeues a `SubTask`.
    *   Executes `sub_task.runnable->runTask(...)`.
    *   Atomically decrements `remaining_sub_tasks_count` for the `BulkTaskNode` corresponding to `sub_task.bulk_task_id`.
    *   If `remaining_sub_tasks_count` for that bulk task becomes 0, it calls `handle_bulk_task_completion(sub_task.bulk_task_id)`.

3.  **`handle_bulk_task_completion(completed_bulk_id)`**:
    *   This function is called when all sub-tasks of `completed_bulk_id` are done.
    *   It iterates through `successor_bulk_task_ids` of the completed bulk task.
    *   For each successor:
        *   Atomically decrements its `unsatisfied_dependencies_count`.
        *   If a successor's `unsatisfied_dependencies_count` becomes 0:
            *   If it has `num_total_sub_tasks > 0` and `!sub_tasks_scheduled`: Its sub-tasks are enqueued to `_ready_sub_task_queue`, `_ready_queue_cv` is notified, and `sub_tasks_scheduled` is set.
            *   If it has `num_total_sub_tasks == 0`: It's a 0-task bulk launch whose dependencies are now met. Recursively call `handle_bulk_task_completion()` for this successor.
    *   Atomically decrements `_active_bulk_tasks_count`.
    *   If `_active_bulk_tasks_count` becomes 0, it notifies `_sync_op_cv`.

4.  **`sync()`**:
    *   Waits on `_sync_op_cv` until `_active_bulk_tasks_count` becomes 0.

5.  **`run(runnable, num_total_tasks)` (for Part B context)**:
    *   Implemented by calling `runAsyncWithDeps(runnable, num_total_tasks, {})` followed by `sync()`.

## Custom Tests
We developed the following custom tests to evaluate correctness and performance on diverse workloads:
1.  **`fft2dByRows` (Sync/Async):** Computes 2D FFT by performing 1D FFTs on all rows of a matrix. Tests parallelization of independent row operations. Each task handles a subset of rows.
2.  **`fft2dByTranspose` (Sync/Async):** Computes full 2D FFT using the transpose method: Row FFTs -> Transpose -> "Row" FFTs (on columns) -> Transpose. Tests a pipeline of dependent bulk launches in the async version.
3.  **`matrixTranspose` (Sync/Async):** Transposes a large matrix. Each task transposes a block of rows from the source to columns in the destination.
4.  **`arraySum` (Sync/Async):** Computes the sum of elements in a large array. Tasks compute partial sums for chunks of the array, which are then aggregated.
5.  **`dotProduct` (Sync/Async):** Computes the dot product of two large vectors. Tasks compute partial dot products for chunks.

These tests cover scenarios with:
*   Independent tasks (e.g., row FFTs, initial stages of sum/dot product).
*   Dependent task stages (e.g., full 2D FFT by transpose).
*   Varying computational intensity and data access patterns.

## Performance Observations

### Part A (Synchronous Bulk Launch)
*   **`TaskSystemSerial`**: Serves as the baseline. Performance scales linearly with `num_total_tasks`.
*   **`TaskSystemParallelSpawn`**: Shows speedup over serial due to parallelism. However, for tests with many small/fast bulk launches (e.g., `super_super_light`), the overhead of thread creation/joining per `run()` call can make it slower than thread pool implementations or even approach serial times if tasks are extremely light. For our custom tests with fewer, larger bulk launches, it performs better than serial but generally worse than thread pool versions.
*   **`TaskSystemParallelThreadPoolSpinning`**: Significantly outperforms `ParallelSpawn` on tests with frequent, short tasks by avoiding thread creation overhead. It shows good speedups on most workloads. Its performance can be slightly better than `ThreadPoolSleeping` for very rapid task dispatches where CV overhead might be noticeable, but can be worse if threads spin wastefully.
*   **`TaskSystemParallelThreadPoolSleeping`**: Generally the most robust performer. It matches or slightly trails `ThreadPoolSpinning` when tasks are abundant and rapidly processed, but excels when there are lulls in task availability or when not all cores are utilized by tasks (e.g., `spin_between_run_calls`), as it avoids wasteful CPU spinning. Our custom tests show it providing strong, consistent speedups.

**Example for `fft2d_by_rows_sync` (1024x1024 matrix, 10 cores):**
*   Serial: ~106 ms
*   Spawn: ~32 ms
*   Spin: ~21 ms
*   Sleep: ~24 ms
This shows the thread pool advantage, with Spin slightly edging out Sleep likely due to the continuous stream of row tasks.

### Part B (Asynchronous Task Graph Execution - `ThreadPoolSleeping`)
*   The `ThreadPoolSleeping` implementation with dependency management shows significant speedups for asynchronous workloads, especially those with inherent parallelism that can be exposed by the task graph (e.g., the transpose-based 2D FFT).
*   **`fft2dByTransposeAsync` (512x512 matrix, 10 cores):**
    *   Serial (simulated async): ~48 ms
    *   ThreadPoolSleeping (Async): ~12 ms
    This demonstrates a ~4x speedup, effectively utilizing parallelism across dependent stages.
*   The custom tests highlight the system's ability to manage dependencies and schedule tasks efficiently, achieving good parallel speedup.

## Build and Run
The project is built using the provided `Makefile`. Do not modify it.
To run tests:
```bash
./runtasks -n <num_threads> <test_name>
# Example: ./runtasks -n 10 mandelbrot_chunked