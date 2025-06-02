#include <chrono>
#include <cmath>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <set>
#include <vector>
#include <complex>
#include <numeric> // For std::iota, std::accumulate
#include <algorithm> // For std::transform, std::min, std::max
#include <cmath>   // For M_PI, std::abs, std::sin, std::cos, std::polar

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "CycleTimer.h"
#include "itasksys.h"

/*
Sync tests
==========
TestResults pingPongEqualTest(ITaskSystem *t);
TestResults pingPongUnequalTest(ITaskSystem *t);
TestResults superLightTest(ITaskSystem *t);
TestResults superSuperLightTest(ITaskSystem *t);
TestResults recursiveFibonacciTest(ITaskSystem* t);
TestResults mathOperationsInTightForLoopTest(ITaskSystem* t);
TestResults mathOperationsInTightForLoopFanInTest(ITaskSystem* t);
TestResults mathOperationsInTightForLoopReductionTreeTest(ITaskSystem* t);
TestResults spinBetweenRunCallsTest(ITaskSystem *t);
TestResults mandelbrotChunkedTest(ITaskSystem* t);
TestResults fft2dByTransposeSyncTest(ITaskSystem* t);
TestResults matrixTransposeSyncTest(ITaskSystem* t);
TestResults arraySumSyncTest(ITaskSystem* t);
TestResults dotProductSyncTest(ITaskSystem* t);
TestResults fft2dByRowsSyncTest(ITaskSystem* t);

Async with dependencies tests
=============================
TestResults pingPongEqualAsyncTest(ITaskSystem *t);
TestResults pingPongUnequalAsyncTest(ITaskSystem *t);
TestResults superLightAsyncTest(ITaskSystem *t);
TestResults superSuperLightAsyncTest(ITaskSystem *t);
TestResults recursiveFibonacciAsyncTest(ITaskSystem* t);
TestResults mathOperationsInTightForLoopAsyncTest(ITaskSystem* t);
TestResults mathOperationsInTightForLoopFanInAsyncTest(ITaskSystem* t);
TestResults mathOperationsInTightForLoopReductionTreeAsyncTest(ITaskSystem* t);
TestResults spinBetweenRunCallsAsyncTest(ITaskSystem *t);
TestResults mandelbrotChunkedAsyncTest(ITaskSystem* t);
TestResults simpleRunDepsTest(ITaskSystem *t);
TestResults fft2dByRowsAsyncTest(ITaskSystem* t);
TestResults fft2dByTransposeAsyncTest(ITaskSystem* t);
TestResults matrixTransposeAsyncTest(ITaskSystem* t);
TestResults arraySumAsyncTest(ITaskSystem* t);
TestResults dotProductAsyncTest(ITaskSystem* t);
*/

/*
 * Structure to hold results of performance tests
 */
typedef struct {
    bool passed;
    double time;
} TestResults;

/*
 * ==================================================================
 *  Skeleton task definition and test definition. Use this to create
 *  your own test, but feel free to modify or delete existing parts of
 *  the skeleton as needed. Look at some of the below task definitions
 *  and the corresponding test definitions for inspiration.
 *  `class SimpleMultiplyTask` and `simpleTest` are a good simple
 *  example.
 * ==================================================================
*/
/*
 * Implement your task here
*/
class YourTask : public IRunnable {
    public:
        YourTask() {}
        ~YourTask() {}
        void runTask(int task_id, int num_total_tasks) {}
};
/*
 * Implement your test here. Call this function from a wrapper that passes in
 * do_async and num_elements. See `simpleTest`, `simpleTestSync`, and
 * `simpleTestAsync` as an example.
 */
TestResults yourTest(ITaskSystem* t, bool do_async, int num_elements, int num_bulk_task_launches) {
    // TODO: initialize your input and output buffers
    int* output = new int[num_elements];

    // TODO: instantiate your bulk task launches

    // Run the test
    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        // TODO:
        // initialize dependency vector
        // make calls to t->runAsyncWithDeps and push TaskID to dependency vector
        // t->sync() at end
    } else {
        // TODO: make calls to t->run
    }
    double end_time = CycleTimer::currentSeconds();

    // Correctness validation
    TestResults results;
    results.passed = true;

    for (int i=0; i<num_elements; i++) {
        int value = 0; // TODO: initialize value
        for (int j=0; j<num_bulk_task_launches; j++) {
            // TODO: update value as expected
        }

        int expected = value;
        if (output[i] != expected) {
            results.passed = false;
            printf("%d: %d expected=%d\n", i, output[i], expected);
            break;
        }
    }
    results.time = end_time - start_time;

    delete [] output;

    return results;
}

/*
 * ==================================================================
 *   Begin task definitions used in tests
 * ==================================================================
 */

/*
 * Each task performs a number of multiplies and divides in-place on a partial
 * input array. This is designed to be used as a basic correctness test.
*/
class SimpleMultiplyTask : public IRunnable {
    public:
        int num_elements_;
        int* array_;

        SimpleMultiplyTask(int num_elements, int* array)
            : num_elements_(num_elements), array_(array) {}
        ~SimpleMultiplyTask() {}

        static inline int multiply_task(int iters, int input) {
            int accumulator = 1;
            for (int i = 0; i < iters; ++i) {
                accumulator *= input;
            }
            return accumulator;
        }

        void runTask(int task_id, int num_total_tasks) {
            // handle case where num_elements is not evenly divisible by num_total_tasks
            int elements_per_task = (num_elements_ + num_total_tasks-1) / num_total_tasks;
            int start_el = elements_per_task * task_id;
            int end_el = std::min(start_el + elements_per_task, num_elements_);

            for (int i=start_el; i<end_el; i++)
                array_[i] = multiply_task(3, array_[i]);
        }
};

/*
 * Each task computes an output list of accumulated counters. Each counter
 * is incremented in a tight for loop. The `equal_work_` field ensures that
 * each element of the output array requires a different amount of computation.
 */
class PingPongTask : public IRunnable {
    public:
        int num_elements_;
        int* input_array_;
        int* output_array_;
        bool equal_work_;
        int iters_;

        PingPongTask(int num_elements, int* input_array, int* output_array,
                     bool equal_work, int iters)
            : num_elements_(num_elements), input_array_(input_array),
              output_array_(output_array), equal_work_(equal_work),
              iters_(iters) {}
        ~PingPongTask() {}

        static inline int ping_pong_iters(int i, int num_elements, int iters) {
            int max_iters = 2 * iters;
            return std::floor(
                (static_cast<float>(num_elements - i) / num_elements) * max_iters);
        }

        static inline int ping_pong_work(int iters, int input) {
            int accum = input;
            for (int j=0; j<iters; j++) {
                if (j%2==0)
                    accum++;
            }
            return accum;
        }

        void runTask(int task_id, int num_total_tasks) {

            // handle case where num_elements is not evenly divisible by num_total_tasks
            int elements_per_task = (num_elements_ + num_total_tasks-1) / num_total_tasks;
            int start_el = elements_per_task * task_id;
            int end_el = std::min(start_el + elements_per_task, num_elements_);

            if (equal_work_) {
                for (int i=start_el; i<end_el; i++)
                    output_array_[i] = ping_pong_work(iters_, input_array_[i]);
            } else {
                for (int i=start_el; i<end_el; i++) {
                    int el_iters = ping_pong_iters(i, num_elements_, iters_);
                    output_array_[i] = ping_pong_work(el_iters, input_array_[i]);
                }
            }
        }
};

/*
 * Each task computes and writes the idx-th fibonacci number into the
 * position output[task_id].
 */
class RecursiveFibonacciTask: public IRunnable {
    public:
        int idx_;
        int *output_;
        RecursiveFibonacciTask(int idx, int *output) : idx_(idx), output_(output) {}
        ~RecursiveFibonacciTask() {}

        // very slow recursive implementation of the nth fibbonacci number
        int slowFn(int n) {
            if (n < 2) return 1;
            return slowFn(n-1) + slowFn(n-2);
        }

        void runTask(int task_id, int num_total_tasks) {
            output_[task_id] = slowFn(idx_);
        }
};

/*
 * Each task copies its task id into the output.
 */
class LightTask: public IRunnable {
    public:
        int *output_;
        LightTask(int *output) : output_(output) {}
        ~LightTask() {}

        void runTask(int task_id, int num_total_tasks) {
            output_[task_id] = task_id;
        }
};

/*
 * Each task performs a sequence of exp, log, and multiplication
 * operations in a tight for loop.
 */
class MathOperationsInTightForLoopTask: public IRunnable {
    public:
        float* output_;
        int array_size_;
        MathOperationsInTightForLoopTask(int array_size, float* output) {
            array_size_ = array_size;
            output_ = output; 
        }
        ~MathOperationsInTightForLoopTask() {}

        void runTask(int task_id, int num_total_tasks) {
            int elements_per_task = array_size_ / num_total_tasks;
            int start = task_id * elements_per_task;
            int end = std::min(start + elements_per_task, array_size_);
            if (array_size_ - end < elements_per_task) {
                end = array_size_;
            }

            for (int i = start; i < end; i++) {
                output_[i] = 0.0;
            }

            for (int i = start; i < end; i++) {
                for (int j = 1; j < 151; j++) {
                    float val;
                    if (i % 3 == 0) {
                        val = exp(j / 100.);
                    } else if (i % 3 == 1) {
                        val = log(j * 2.);
                    } else {
                        val = j * 6;
                    }
                    output_[i] += val;
                }
            }
        }
};

/*
 * Each task computes the sum of `num_to_reduce_` input arrays.
 */
class ReduceTask: public IRunnable {
    public:
        float* input_;
        float* output_;
        int array_size_;
        int num_to_reduce_;
        ReduceTask(int array_size, int num_to_reduce,
                   float* input, float* output) {
            array_size_ = array_size;
            num_to_reduce_ = num_to_reduce;
            input_ = input;
            output_ = output;
        }

        void runTask(int task_id, int num_total_tasks) {
            for (int i = 0; i < array_size_; i++) {
                output_[i] = 0.0;
                for (int j = 0; j < num_to_reduce_; j++) {
                    output_[i] += input_[(j*array_size_) + i];
                }
            }
        }
};

/*
 * Each task computes a number of rows of the output Mandelbrot image.  
 * These rows either form a contiguous chunk of the image (if
 * interleave is false) or are interleaved throughout the image.
 */
class MandelbrotTask: public IRunnable {
    public:
        typedef struct {
            float x0, x1;
            float y0, y1;
            int width;
            int height;
            int max_iterations;
            int* output;
        } MandelArgs;

        MandelArgs *args_;
		int interleave_;

        MandelbrotTask(MandelArgs *args, int interleave) 
          : args_(args), interleave_(interleave) {}
        ~MandelbrotTask() {}

        // helper function used by Mandelbrot computations
        inline int mandel(float c_re, float c_im, int count) {
            float z_re = c_re, z_im = c_im;
            int i;
            for (i = 0; i < count; ++i) {

                if (z_re * z_re + z_im * z_im > 4.f)
                    break;

                float new_re = z_re*z_re - z_im*z_im;
                float new_im = 2.f * z_re * z_im;
                z_re = c_re + new_re;
                z_im = c_im + new_im;
            }

            return i;
        }

        void mandelbrotSerial(
            float x0, float y0, float x1, float y1,
            int width, int height,
            int startRow, int totalRows,
            int max_iterations,
            int output[])
        {
            float dx = (x1 - x0) / width;
            float dy = (y1 - y0) / height;

            int endRow = startRow + totalRows;

            for (int j = startRow; j < endRow; j++) {
                for (int i = 0; i < width; ++i) {
                    float x = x0 + i * dx;
                    float y = y0 + j * dy;

                    int index = (j * width + i);
                    output[index] = mandel(x, y, max_iterations);
                }
            }
        }

        void mandelbrotSerial_interleaved(
            float x0, float y0, float x1, float y1,
            int width, int height,
            int startRow, int totalRows,
            int interleaving,
            int max_iterations,
            int output[]) {
            float dx = (x1 - x0) / width;
            float dy = (y1 - y0) / height;

            int endRow = startRow + totalRows;

            for (int j = startRow; j < endRow; j += interleaving) {
                for (int i = 0; i < width; ++i) {
                    float x = x0 + i * dx;
                    float y = y0 + j * dy;

                    int index = (j * width + i);
                    output[index] = mandel(x, y, max_iterations);
                }
            }
        }
    
        void runTask(int task_id, int num_total_tasks) {
            int rowsPerTask = args_->height / num_total_tasks;

            if (interleave_ == 1) {
                mandelbrotSerial_interleaved(args_->x0, args_->y0, args_->x1, args_->y1,
                                             args_->width, args_->height,
                                             task_id, args_->height - task_id,
                                             num_total_tasks, args_->max_iterations,
                                             args_->output);
            } else {
                mandelbrotSerial(args_->x0, args_->y0, args_->x1, args_->y1,
                                 args_->width, args_->height,
                                 task_id * rowsPerTask, rowsPerTask,
                                 args_->max_iterations, args_->output);
            }
        }
};

/*
 * Each task sleeps for the prescribed amount of time, and then
 * print a message to stdout.
 */
class SleepTask: public IRunnable {
    public:
        int sleep_period_;
        SleepTask(int sleep_period) {
            sleep_period_ = sleep_period;
        }
        ~SleepTask() {}

        void runTask(int task_id, int num_total_tasks) {
            std::this_thread::sleep_for(
                std::chrono::seconds(sleep_period_));
            printf("Running SleepTask (%ds), task %d\n",
                sleep_period_, task_id);
        }
};

/*
 * This task sets its "done" flag when the following conditions are met:
 *  - All dependencies have their "done" flag set prior to the first
      task of this bulk task running.
 *  - The last task of this bulk task has finished running.
 *
 * Intended for building correctness tests of arbitrary task graphs.
 */
class StrictDependencyTask: public IRunnable {
    private:
        const std::vector<bool*>& in_flags_;
        bool *out_flag_;
        std::atomic<int> tasks_started_;
        std::atomic<int> tasks_ended_;
        bool satisfied_;

    public:
        StrictDependencyTask(const std::vector<bool*>& in_flags, bool *out_flag)
          : in_flags_(in_flags), out_flag_(out_flag), tasks_started_(0),
            tasks_ended_(0), satisfied_(false) {}

        void runTask(int task_id, int num_total_tasks) {
            int entry_id = tasks_started_++;
            // First task to be run: check if all dependencies are met.
            if (entry_id == 0) {
                satisfied_ = depsMet();
            }

            doWork(task_id, num_total_tasks);

            int exit_id = ++tasks_ended_;
            // Last task to finish running: mark this entire task launch as done.
            if (exit_id == num_total_tasks) {
                *out_flag_ = satisfied_;
            }
        }

        bool depsMet() {
            for (bool *b : in_flags_) {
                if (*b == false) {
                    return false;
                }
            }
            return true;
        }

        void doWork(int task_id, int num_total_tasks) {
            // Using this as a proxy for actual work.
            std::this_thread::sleep_for (std::chrono::microseconds((1 + (task_id % 10))));
        }
        ~StrictDependencyTask() {}
};

/* 
 * ==================================================================
 *   Begin test definitions
 * ==================================================================
 */

/*
 * Computation: simpleTest is designed as solely a correctness test to be used
 * for early debugging. It is a small test - it launches 2 bulk task launches
 * with 3 tasks each. The computation done by each bulk task launch is a small
 * number of multiplies on each element of the input array. The array
 * modification is done in-place. The array has 6 total elements, divided into
 * 2 elements per task.
 *
 * Debug information for students: to debug, consider setting breakpoints or
 * adding print statements in this function.
 */
TestResults simpleTest(ITaskSystem* t, bool do_async) {
    int num_elements_per_task = 2;
    int num_tasks = 3;
    int num_elements = num_elements_per_task * num_tasks;
    int num_bulk_task_launches = 2;

    int* array = new int[num_elements];

    for (int i=0; i<num_elements; i++) {
        array[i] = i + 1;
    }

    SimpleMultiplyTask first = SimpleMultiplyTask(num_elements, array);
    SimpleMultiplyTask second = SimpleMultiplyTask(num_elements, array);

    // Run the test
    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        std::vector<TaskID> firstDeps;
        TaskID first_task_id = t->runAsyncWithDeps(&first, num_tasks, firstDeps);
        std::vector<TaskID> secondDeps;
        secondDeps.push_back(first_task_id);
        t->runAsyncWithDeps(&second, num_tasks, secondDeps);
        t->sync();
    } else {
        t->run(&first, num_tasks);
        t->run(&second, num_tasks);
    }
    double end_time = CycleTimer::currentSeconds();

    // Correctness validation
    TestResults results;
    results.passed = true;

    for (int i=0; i<num_elements; i++) {
        int value = i+1;

        for (int j=0; j<num_bulk_task_launches; j++)
            value = SimpleMultiplyTask::multiply_task(3, value);

        int expected = value;
        if (array[i] != expected) {
            results.passed = false;
            printf("%d: %d expected=%d\n", i, array[i], expected);
            break;
        }
    }
    results.time = end_time - start_time;

    delete [] array;

    return results;
}

TestResults simpleTestSync(ITaskSystem* t) {
    return simpleTest(t, false);
}

TestResults simpleTestAsync(ITaskSystem* t) {
    return simpleTest(t, true);
}

/*
 * Computation: pingPongTest launches 400 bulk task launches with 64 tasks each.
 * The computation done by each bulk task launch takes as input a buffer of size
 * `num_elements` as input, performs an elementwise computation, and writes to
 * an equal-sized output buffer. By default, the elementwise computation defined
 * by class PingPongTask increments the input element `base_iters` times and
 * writes the result to the corresponding location in the output buffer. If
 * `equal_work` is true, each element requires the same amount of work to
 * compute. Otherwise, the lower index elements require more work to compute,
 * and each task therefore does a different amount of work. Each task takes as
 * input the output buffer of the previous task, so each task depends on the
 * task before it. When base_iters is sufficiently small, the overhead of
 * launching threads is non-trival and so there are benefits to a thread pool.
 * The amount of computation per task is controlled using `num_elements` and
 * `base_iters`, because each task gets `num_elements` / `num_tasks` elements
 * and does O(base_iters) work per element.
 */
TestResults pingPongTest(ITaskSystem* t, bool equal_work, bool do_async,
                         int num_elements, int base_iters) {

    int num_tasks = 64;
    int num_bulk_task_launches = 400;   

    int* input = new int[num_elements];
    int* output = new int[num_elements];

    // Init input
    for (int i=0; i<num_elements; i++) {
        input[i] = i;
        output[i] = 0;
    }

    // Ping-pong input and output buffers with all the
    // back-to-back task launches
    std::vector<PingPongTask*> runnables(
        num_bulk_task_launches);
    for (int i=0; i<num_bulk_task_launches; i++) {
        if (i % 2 == 0)
            runnables[i] = new PingPongTask(
                num_elements, input, output,
                equal_work, base_iters);
        else
            runnables[i] = new PingPongTask(
                num_elements, output, input,
                equal_work, base_iters);
    }

    // Run the test
    double start_time = CycleTimer::currentSeconds();
    TaskID prev_task_id;
    for (int i=0; i<num_bulk_task_launches; i++) {
        if (do_async) {
            std::vector<TaskID> deps;
            if (i > 0) {
                deps.push_back(prev_task_id);
            }
            prev_task_id = t->runAsyncWithDeps(
                runnables[i], num_tasks, deps);
        } else {
            t->run(runnables[i], num_tasks);
        }
    }
    if (do_async)
        t->sync();
    double end_time = CycleTimer::currentSeconds();

    // Correctness validation
    TestResults results;
    results.passed = true;

    // Number of ping-pongs determines which buffer to look at for the results
    int* buffer = (num_bulk_task_launches % 2 == 1) ? output : input; 

    for (int i=0; i<num_elements; i++) {
        int value = i;
        for (int j=0; j<num_bulk_task_launches; j++) {
            int iters = (!equal_work) ? PingPongTask::ping_pong_iters(
                i, num_elements, base_iters) : base_iters;
            value = PingPongTask::ping_pong_work(iters, value);
        }

        int expected = value;
        if (buffer[i] != expected) {
            results.passed = false;
            printf("%d: %d expected=%d\n", i, buffer[i], expected);
            break;
        }
    }
    results.time = end_time - start_time;

    delete [] input;
    delete [] output;
    for (int i=0; i<num_bulk_task_launches; i++)
        delete runnables[i];
    
    return results;
}

TestResults superSuperLightTest(ITaskSystem* t) {
    int num_elements = 32 * 1024;
    int base_iters = 0;
    return pingPongTest(t, true, false, num_elements, base_iters);
}

TestResults superSuperLightAsyncTest(ITaskSystem* t) {
    int num_elements = 32 * 1024;
    int base_iters = 0;
    return pingPongTest(t, true, true, num_elements, base_iters);
}

TestResults superLightTest(ITaskSystem* t) {
    int num_elements = 32 * 1024;
    int base_iters = 32;
    return pingPongTest(t, true, false, num_elements, base_iters);
}

TestResults superLightAsyncTest(ITaskSystem* t) {
    int num_elements = 32 * 1024;
    int base_iters = 32;
    return pingPongTest(t, true, true, num_elements, base_iters);
}

TestResults pingPongEqualTest(ITaskSystem* t) {
    int num_elements = 512 * 1024;
    int base_iters = 32;
    return pingPongTest(t, true, false, num_elements, base_iters);
}

TestResults pingPongUnequalTest(ITaskSystem* t) {
    int num_elements = 512 * 1024;
    int base_iters = 32;
    return pingPongTest(t, false, false, num_elements, base_iters);
}

TestResults pingPongEqualAsyncTest(ITaskSystem* t) {
    int num_elements = 512 * 1024;
    int base_iters = 32;
    return pingPongTest(t, true, true, num_elements, base_iters);
}

TestResults pingPongUnequalAsyncTest(ITaskSystem* t) {
    int num_elements = 512 * 1024;
    int base_iters = 32;
    return pingPongTest(t, false, true, num_elements, base_iters);
}

/*
 * Computation: The following tests compute Fibonacci numbers using
 * recursion. Since the tasks are compute intensive, the tests show
 * the benefit of parallelism.
 */
TestResults recursiveFibonacciTestBase(ITaskSystem* t, bool do_async) {

    int num_tasks = 256;
    int num_bulk_task_launches = 30;
    int fib_index = 25;

    int* task_output = new int[num_tasks];
    for (int i = 0; i < num_tasks; i++) {
        task_output[i] = 0;
    }

    std::vector<RecursiveFibonacciTask*> fib_tasks(num_bulk_task_launches);
    for (int i = 0; i < num_bulk_task_launches; i++) {
        fib_tasks[i] = new RecursiveFibonacciTask(fib_index, task_output);
    }

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        std::vector<TaskID> deps; // Call runAsyncWithDeps without dependencies
        for (int i = 0; i < num_bulk_task_launches; i++) {
            t->runAsyncWithDeps(fib_tasks[i], num_tasks, deps);
        }
        t->sync();
    } else {
        for (int i = 0; i < num_bulk_task_launches; i++) {
            t->run(fib_tasks[i], num_tasks);
        }
    }
    double end_time = CycleTimer::currentSeconds();

    // Validate correctness 
    TestResults result;
    result.passed = true;
    for (int i = 0; i < num_tasks; i++) {
        if (task_output[i] != 121393) {
            printf("%d\n", task_output[i]);
            result.passed = false;
            break;
        }
    }
    result.time = end_time - start_time;

    delete [] task_output;
    for (int i = 0; i < num_bulk_task_launches; i++) {
        delete fib_tasks[i];
    }

    return result;
}

TestResults recursiveFibonacciTest(ITaskSystem* t) {
    return recursiveFibonacciTestBase(t, false);
}

TestResults recursiveFibonacciAsyncTest(ITaskSystem* t) {
    return recursiveFibonacciTestBase(t, true);
}

/*
 * Computation: The following tests perform exps, logs, and multiplications
 * in a tight for loop. Tasks are sufficiently compute-intensive and lightweight:
 * the threadpool implementation should perform better than the one that spawns
 * new threads with every bulk task launch.
 */
TestResults mathOperationsInTightForLoopTestBase(ITaskSystem* t, int num_tasks,
                                                 bool run_with_dependencies, bool do_async) {

    int num_bulk_task_launches = 2000;

    int array_size = 512;
    float* task_output = new float[num_bulk_task_launches * array_size];

    for (int i = 0; i < (num_bulk_task_launches * array_size); i++) {
        task_output[i] = 0.0;
    }

    std::vector<MathOperationsInTightForLoopTask> medium_tasks;
    for (int i = 0; i < num_bulk_task_launches; i++) {
        medium_tasks.push_back(MathOperationsInTightForLoopTask(
            array_size, &task_output[i*array_size]));
    }

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        if (run_with_dependencies) {
            TaskID prev_task_id;
            for (int i = 0; i < num_bulk_task_launches; i++) {
                std::vector<TaskID> deps;
                if (i > 0) {
                    deps.push_back(prev_task_id);
                }
                prev_task_id = t->runAsyncWithDeps(&medium_tasks[i], num_tasks, deps);
            }
        } else {
            std::vector<TaskID> deps;
            for (int i = 0; i < num_bulk_task_launches; i++) {
                t->runAsyncWithDeps(&medium_tasks[i], num_tasks, deps);
            }
        }
        t->sync();
    } else {
        for (int i = 0; i < num_bulk_task_launches; i++) {
            t->run(&medium_tasks[i], num_tasks);
        }
    }
    double end_time = CycleTimer::currentSeconds();

    TestResults result;
    result.passed = true;
    for (int i = 0; i < array_size; i++) {
        if (i % 3 == 0) {
            if (std::floor(task_output[i]) != 349) {
                printf("%d: %f expected=%d\n", i, std::floor(task_output[i]), 349);
                result.passed = false;
            }
        } else if (i % 3 == 1) {
            if (std::floor(task_output[i]) != 708) {
                printf("%d: %f expected=%d\n", i, std::floor(task_output[i]), 708);
                result.passed = false;
            }
        } else {
            if (std::floor(task_output[i]) != 67950) {
                printf("%d: %f expected=%d\n", i, std::floor(task_output[i]), 67950);
                result.passed = false;
            }
        }
    }
    result.time = end_time - start_time;

    delete [] task_output;

    return result;
}

TestResults mathOperationsInTightForLoopTest(ITaskSystem* t) {
    return mathOperationsInTightForLoopTestBase(t, 16, true, false);
}


TestResults mathOperationsInTightForLoopAsyncTest(ITaskSystem* t) {
    return mathOperationsInTightForLoopTestBase(t, 16, true, true);
}

TestResults mathOperationsInTightForLoopFewerTasksTest(ITaskSystem* t) {
    return mathOperationsInTightForLoopTestBase(t, 9, false, false);
}

TestResults mathOperationsInTightForLoopFewerTasksAsyncTest(ITaskSystem* t) {
    return mathOperationsInTightForLoopTestBase(t, 9, false, true);
}

/*
 * Computation: The following tests perform exps, logs, and multiplications
 * in a tight for loop, then sum the outputs of the different tasks using
 * a single reduce task. The async version of this test features a computation
 * DAG with fan-in dependencies.
 */
TestResults mathOperationsInTightForLoopFanInTestBase(ITaskSystem* t, bool do_async) {

    int num_tasks = 64;
    int num_bulk_task_launches = 256;

    int array_size = 2048;
    float* task_output = new float[num_bulk_task_launches*array_size];
    float* final_task_output = new float[array_size];

    for (int i = 0; i < array_size; i++) {
        final_task_output[i] = 0.0;
    }

    std::vector<MathOperationsInTightForLoopTask> medium_tasks;
    for (int i = 0; i < num_bulk_task_launches; i++) {
        medium_tasks.push_back(MathOperationsInTightForLoopTask(
            array_size, &task_output[i*array_size]));
    }
    ReduceTask reduce_task(array_size, num_bulk_task_launches, task_output,
                           final_task_output);

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        std::vector<TaskID> no_deps;
        std::vector<TaskID> deps;
        for (int i = 0; i < num_bulk_task_launches; i++) {
            TaskID task_id = t->runAsyncWithDeps(&medium_tasks[i], num_tasks, no_deps);
            deps.push_back(task_id);
        }
        t->runAsyncWithDeps(&reduce_task, 1, deps);
        t->sync();
    } else {
        for (int i = 0; i < num_bulk_task_launches; i++) {
            t->run(&medium_tasks[i], num_tasks);
        }
        t->run(&reduce_task, 1);
    }
    double end_time = CycleTimer::currentSeconds();

    TestResults result;
    result.passed = true;
    for (int i = 0; i < array_size; i++) {
        if (i % 3 == 0) {
            if (std::floor(final_task_output[i]) != 89577) {
                printf("%d: %f expected=%d\n", i, std::floor(final_task_output[i]), 69982);
                result.passed = false;
            }
        } else if (i % 3 == 1) {
            if (std::floor(final_task_output[i]) != 181502) {
                printf("%d: %f expected=%d\n", i, std::floor(final_task_output[i]), 141798);
                result.passed = false;
            }
        } else {
            if (std::floor(final_task_output[i]) != (67950 * num_bulk_task_launches)) {
                printf("%d: %f expected=%d\n", i,
                       std::floor(final_task_output[i]),
                       67950 * num_bulk_task_launches);
                result.passed = false;
            }
        }
    }
    result.time = end_time - start_time;

    delete [] task_output;
    delete [] final_task_output;

    return result;
}

TestResults mathOperationsInTightForLoopFanInTest(ITaskSystem* t) {
    return mathOperationsInTightForLoopFanInTestBase(t, false);
}

TestResults mathOperationsInTightForLoopFanInAsyncTest(ITaskSystem* t) {
    return mathOperationsInTightForLoopFanInTestBase(t, true);
}

/*
 * Computation: The following tests perform exps, logs, and multiplications
 * in a tight for loop, then sum the outputs of the different tasks using
 * multiple reduce tasks in a binary tree structure. The async version of this
 * test features a binary tree computation DAG.
 */
TestResults mathOperationsInTightForLoopReductionTreeTestBase(ITaskSystem* t, bool do_async) {

    int num_tasks = 64;
    int num_bulk_task_launches = 32;

    int array_size = 16384;
    float* buffer1 = new float[num_bulk_task_launches*array_size];
    float* buffer2 = new float[(num_bulk_task_launches/2)*array_size];
    float* buffer3 = new float[(num_bulk_task_launches/4)*array_size];
    float* buffer4 = new float[(num_bulk_task_launches/8)*array_size];
    float* buffer5 = new float[(num_bulk_task_launches/16)*array_size];
    float* buffer6 = new float[(num_bulk_task_launches/32)*array_size];
    std::vector<float*> buffers = std::vector<float*>();
    buffers.push_back(buffer1); buffers.push_back(buffer2);
    buffers.push_back(buffer3); buffers.push_back(buffer4);
    buffers.push_back(buffer5); buffers.push_back(buffer6);

    for (int i = 0; i < (num_bulk_task_launches/32)*array_size; i++) {
        buffer6[i] = 0.0;
    }

    // First, run several MathOperationsInTightForLoopTasks
    std::vector<MathOperationsInTightForLoopTask> medium_tasks;
    for (int i = 0; i < num_bulk_task_launches; i++) {
        medium_tasks.push_back(MathOperationsInTightForLoopTask(
            array_size, &buffer1[i*array_size]));
    }
    // Now, compute the sum of each of the outputs of each of these
    // bulk task launches
    std::vector<ReduceTask> reduce_tasks;
    int num_reduce_tasks = num_bulk_task_launches / 2;
    int i = 0;
    while (num_reduce_tasks >= 1) {
        float* input = buffers[i];
        float* output = buffers[i+1];;
        for (int i = 0; i < num_reduce_tasks; i++) {
            ReduceTask reduce_task(array_size, 2, input,
                                   output);
            reduce_tasks.emplace_back(reduce_task);
            input += (2 * array_size);
            output += (array_size);
        }
        i++;
        num_reduce_tasks /= 2;
    }

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        std::vector<TaskID> no_deps;
        std::vector<std::vector<TaskID>> all_deps;
        std::vector<std::vector<TaskID>> new_all_deps;
        std::vector<TaskID> cur_deps;
        for (int i = 0; i < num_bulk_task_launches; i++) {
            TaskID task_id = t->runAsyncWithDeps(&medium_tasks[i], num_tasks, no_deps);
            cur_deps.push_back(task_id);
            if (cur_deps.size() == 2) {
                all_deps.emplace_back(cur_deps);
                cur_deps = std::vector<TaskID>();
            }
        }
        // Make sure runAsyncWithDeps() is called with the right dependencies
        cur_deps = std::vector<TaskID>();
        int num_reduce_tasks = num_bulk_task_launches / 2;
        int reduce_idx = 0;
        while (num_reduce_tasks >= 1) {
            for (int i = 0; i < num_reduce_tasks; i++) {
                TaskID task_id = t->runAsyncWithDeps(
                    &reduce_tasks[reduce_idx+i], 1, all_deps[i]);
                cur_deps.push_back(task_id);
                if (cur_deps.size() == 2) {
                    new_all_deps.emplace_back(cur_deps);
                    cur_deps = std::vector<TaskID>();
                }
            }
            reduce_idx += num_reduce_tasks;
            all_deps.clear();
            for (std::vector<TaskID> deps: new_all_deps) {
                all_deps.emplace_back(deps);
            }
            new_all_deps.clear();
            num_reduce_tasks /= 2;
        }
        t->sync();
    } else {
        for (int i = 0; i < num_bulk_task_launches; i++) {
            t->run(&medium_tasks[i], num_tasks);
        }
        for (size_t i = 0; i < reduce_tasks.size(); i++) {
            t->run(&reduce_tasks[i], 1);
        }
    }
    double end_time = CycleTimer::currentSeconds();

    TestResults result;
    result.passed = true;
    for (int i = 0; i < array_size; i++) {
        if (i % 3 == 0) {
            if (std::floor(buffer6[i]) != 11197) {
                printf("%d: %f expected=%d\n", i,
                    std::floor(buffer6[i]), 11197);
                result.passed = false;
            }
        } else if (i % 3 == 1) {
            if (std::floor(buffer6[i]) != 22687) {
                printf("%d: %f expected=%d\n", i,
                    std::floor(buffer6[i]), 22687);
                result.passed = false;
            }
        } else {
            if (std::floor(buffer6[i]) != (
                    67950 * num_bulk_task_launches)) {
                printf("%d: %f expected=%d\n", i,
                       std::floor(buffer6[i]),
                       67950 * num_bulk_task_launches);
                result.passed = false;
            }
        }
    }
    result.time = end_time - start_time;

    delete [] buffer1;
    delete [] buffer2;
    delete [] buffer3;
    delete [] buffer4;
    delete [] buffer5;
    delete [] buffer6;

    return result;
}

TestResults mathOperationsInTightForLoopReductionTreeTest(ITaskSystem* t) {
    return mathOperationsInTightForLoopReductionTreeTestBase(t, false);
}

TestResults mathOperationsInTightForLoopReductionTreeAsyncTest(ITaskSystem* t) {
    return mathOperationsInTightForLoopReductionTreeTestBase(t, true);
}

/*
 * Computation: In between two calls to a light weight task, these tests spawn
 * a medium weight bulk task launch that only has enough enough tasks to
 * use a subset of the available hardware threads.
 * If the main thread and unused threads spin, they will compete with
 * the useful work, causing slowdowns. In contrast, if unused threads
 * sleep until they are required to do work, the threads doing useful work
 * will not be crowded out.
 */
TestResults spinBetweenRunCallsTestBase(ITaskSystem *t, bool do_async) {

    int num_light_tasks = 1;
    int num_med_tasks = 2;

    int *light_task_output = new int[num_light_tasks];
    int *med_task_output = new int[num_med_tasks];

    for (int i = 0; i < num_light_tasks; i++) {
        light_task_output[i] = 0;
    }

    for (int i = 0; i < num_med_tasks; i++) {
        med_task_output[i] = 0;
    }

    LightTask light_task(light_task_output);
    RecursiveFibonacciTask medium_task(40, med_task_output);

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        std::vector<TaskID> deps;
        TaskID light_task_id = t->runAsyncWithDeps(
            &light_task, num_light_tasks, deps);
        // A subset of available threads performs work
        // here, while the rest spin
        TaskID medium_task_id = t->runAsyncWithDeps(
            &medium_task, num_med_tasks, deps);
        deps.push_back(light_task_id); deps.push_back(
            medium_task_id);
        t->runAsyncWithDeps(&light_task, num_light_tasks, deps);
        t->sync();
    } else {
        t->run(&light_task, num_light_tasks);
        // Notice that since the number of tasks is small, only a small
        // number of available threads can perform work here, while the
        // rest spin.
        t->run(&medium_task, num_med_tasks);
        t->run(&light_task, num_light_tasks);
    }
    double end_time = CycleTimer::currentSeconds();

    // Validate correctness
    TestResults result;
    result.passed = true;
    for (int i = 0; i < num_med_tasks; i++) {
        if (med_task_output[i] != 165580141) {
            result.passed = false;
        }
    }
    result.time = end_time - start_time;

    delete [] light_task_output;
    delete [] med_task_output;
    return result;
}

TestResults spinBetweenRunCallsTest(ITaskSystem *t) {
    return spinBetweenRunCallsTestBase(t, false);
}

TestResults spinBetweenRunCallsAsyncTest(ITaskSystem *t) {
    return spinBetweenRunCallsTestBase(t, true);
}

/*
 * Computation: This test computes a Mandelbrot fractal image by
 * decomposing the problem into tasks that produce contiguous chunks of
 * output image rows. Note that only one bulk task launch is performed,
 * which means thread pool and spawning threads each run() should have
 * similar performance.
 */
TestResults mandelbrotChunkedTestBase(ITaskSystem* t, bool do_async) {

    int num_tasks = 128;
    
    MandelbrotTask::MandelArgs ma;
    ma.x0 = -2;
    ma.x1 = 1;
    ma.y0 = -1;
    ma.y1 = 1;
    ma.width = 1600;
    ma.height = 1200;
    ma.max_iterations = 256;
    ma.output = new int[ma.width * ma.height];
    for (int i = 0; i < (ma.width * ma.height); i++) {
        ma.output[i] = 0;
    }

    MandelbrotTask mandel_task(&ma, true);  // No interleaving

    // time task-based implementation
    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        std::vector<TaskID> deps; // Call runAsyncWithDeps without dependencies.
        t->runAsyncWithDeps(&mandel_task, num_tasks, deps);
        t->sync();
    } else {
        t->run(&mandel_task, num_tasks);
    }
    double end_time = CycleTimer::currentSeconds();

    // Validate correctness of the task-based implementation
    // against sequential implementation
    int *golden = new int[ma.width * ma.height];
    mandel_task.mandelbrotSerial(ma.x0, ma.y0, ma.x1, ma.y1,
                                 ma.width, ma.height,
                                 0, ma.height,
                                 ma.max_iterations,
                                 golden);

    TestResults result;
    result.passed = true;
    for (int i = 0; i < ma.width * ma.height; i++) {
        if (golden[i] != ma.output[i]) {
            result.passed = false;
        }
    }
    
    result.time = end_time - start_time;

    delete [] golden;
    delete [] ma.output;

    return result;
}

TestResults mandelbrotChunkedTest(ITaskSystem* t) {
    return mandelbrotChunkedTestBase(t, false);
}

TestResults mandelbrotChunkedAsyncTest(ITaskSystem* t) {
    return mandelbrotChunkedTestBase(t, true);
}

/*
 * Computation: Simple correctness test for runAsyncWithDeps.
 * Tasks sleep for a prescribed amount of time and then print
 * a message to stdout.
 */
TestResults simpleRunDepsTest(ITaskSystem *t) {

    IRunnable* a = new SleepTask(1);
    IRunnable* b = new SleepTask(5);
    IRunnable* c = new SleepTask(10);

    std::vector<TaskID> a_deps;
    std::vector<TaskID> b_deps;
    std::vector<TaskID> c_deps;

    
    double start_time = CycleTimer::currentSeconds();
    auto a_taskid = t->runAsyncWithDeps(a, 10, a_deps);

    b_deps.push_back(a_taskid);
    auto b_task_id = t->runAsyncWithDeps(b, 6, b_deps);

    c_deps.push_back(b_task_id);
    t->runAsyncWithDeps(c, 12, c_deps);

    t->sync();
    double end_time = CycleTimer::currentSeconds();

    TestResults result;
    result.passed = true;
    result.time = end_time - start_time;

    delete a;
    delete b;
    delete c;

    return result;
}

/*
 * This test makes dependencies in a diamond topology are satisfied.
 */
TestResults strictDiamondDepsTest(ITaskSystem *t) {
    // Just four tasks in a diamond.
    bool *done = new bool[4]();

    // When these strict tasks run, they check their dependencies.
    std::vector<bool*> a_dep_fs;
    IRunnable* a = new StrictDependencyTask(a_dep_fs, done);

    std::vector<bool*> b_dep_fs = {done};
    IRunnable* b = new StrictDependencyTask(b_dep_fs, done+1);

    std::vector<bool*> c_dep_fs = {done};
    IRunnable* c = new StrictDependencyTask(c_dep_fs, done+2);

    std::vector<bool*> d_dep_fs = {done+1,done+2};
    IRunnable* d = new StrictDependencyTask(d_dep_fs, done+3);

    std::vector<TaskID> a_deps;
    std::vector<TaskID> b_deps;
    std::vector<TaskID> c_deps;
    std::vector<TaskID> d_deps;

    double start_time = CycleTimer::currentSeconds();
    auto a_taskid = t->runAsyncWithDeps(a, 1, a_deps);

    b_deps.push_back(a_taskid);
    auto b_task_id = t->runAsyncWithDeps(b, 6, b_deps);

    c_deps.push_back(b_task_id);
    auto c_task_id = t->runAsyncWithDeps(c, 12, c_deps);

    d_deps.push_back(b_task_id);
    d_deps.push_back(c_task_id);
    t->runAsyncWithDeps(d,4,d_deps);

    t->sync();
    double end_time = CycleTimer::currentSeconds();
    
    TestResults result;
    result.passed = done[3];
    result.time = end_time - start_time;

    delete[] done;
    delete a;
    delete b;
    delete c;
    delete d;

    return result;
};

/*
 * These tests generates and run a random DAG of n tasks and at most m edges,
 * and make all dependencies are satisfied.
 */
TestResults strictGraphDepsTestBase(ITaskSystem*t, int n, int m, unsigned int seed) {
    // For repeatability.
    srand(seed);

    // Each StrictDependencyTask sets this when it is complete.
    bool *done = new bool[n]();

    std::vector<int> idx_deps[n];
    std::vector<bool*> flag_deps[n];
    std::vector<TaskID> task_deps[n];

    // For keeping track of the TaskIDs returned by the task system.
    TaskID *task_ids = new TaskID[n];

    // Used to avoid duplicate edges
    std::set<std::pair<int,int> > eset;

    // Generate random graph.
    for (int i = 0; i < m; i++) {
        int s = rand() % n;
        int t = rand() % n;
        if (s > t) {
            std::swap(s,t);
        }
        if (s == t || eset.count({s,t})) {
            continue;
        }
        idx_deps[t].push_back(s);
        flag_deps[t].push_back(done + s);
        eset.insert({s,t});
    }

    std::vector<IRunnable*> tasks;
    for (int i = 0; i < n; i++) {
        tasks.push_back(new StrictDependencyTask(flag_deps[i], done + i));
    }

    double start_time = CycleTimer::currentSeconds();
    for (int i = 0; i < n; i++) {
        // Populate TaskID deps.
        for (int idx : idx_deps[i]) {
            task_deps[i].push_back(task_ids[idx]);
        }
        // Launch async and record this task's id.
        task_ids[i] = t->runAsyncWithDeps(tasks[i], (rand() % 15) + 1, task_deps[i]);
    }
    t->sync();
    double end_time = CycleTimer::currentSeconds();
    
    TestResults result;
    result.passed = done[n-1];
    result.time = end_time - start_time;
    return result;
}

TestResults strictGraphDepsSmall(ITaskSystem* t) {
    return strictGraphDepsTestBase(t,4,2,0);
}

TestResults strictGraphDepsMedium(ITaskSystem* t) {
    return strictGraphDepsTestBase(t,100,1000,0);
}

TestResults strictGraphDepsLarge(ITaskSystem* t) {
    return strictGraphDepsTestBase(t,1000,20000,0);
}

// Helper function for 1D FFT (Recursive Cooley-Tukey)
// Operates in-place. Input size N must be a power of 2.
void fft_1d_recursive(std::vector<std::complex<double>>& x) {
    const size_t N = x.size();
    if (N <= 1) return;

    if ((N & (N - 1)) != 0) {
        // For simplicity in this example, we'll just print an error if not power of 2
        fprintf(stderr, "FFT size is not a power of 2. Size: %zu\n", N);
    }


    // Divide
    std::vector<std::complex<double>> even(N / 2);
    std::vector<std::complex<double>> odd(N / 2);
    for (size_t i = 0; i < N / 2; ++i) {
        even[i] = x[2 * i];
        odd[i] = x[2 * i + 1];
    }

    // Conquer
    fft_1d_recursive(even);
    fft_1d_recursive(odd);

    // Combine
    for (size_t k = 0; k < N / 2; ++k) {
        std::complex<double> t = std::polar(1.0, -2.0 * M_PI * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k + N / 2] = even[k] - t;
    }
}

// Helper function for comparing complex vectors (for verification)
bool compare_complex_vectors(const std::vector<std::complex<double>>& v1, const std::vector<std::complex<double>>& v2, double tolerance = 1e-7) {
    if (v1.size() != v2.size()) return false;
    for (size_t i = 0; i < v1.size(); ++i) {
        if (std::abs(v1[i].real() - v2[i].real()) > tolerance || std::abs(v1[i].imag() - v2[i].imag()) > tolerance) {
            printf("Verification mismatch at index %zu: v1=(%.5f, %.5f), v2=(%.5f, %.5f)\n", 
                   i, v1[i].real(), v1[i].imag(), v2[i].real(), v2[i].imag());
            return false;
        }
    }
    return true;
}

// Helper function for comparing complex matrices (for verification)
bool compare_complex_matrices(const std::vector<std::vector<std::complex<double>>>& m1, 
                              const std::vector<std::vector<std::complex<double>>>& m2, 
                              double tolerance = 1e-7) {
    if (m1.size() != m2.size()) {
        printf("Verification matrix row count mismatch: m1.size()=%zu, m2.size()=%zu\n", m1.size(), m2.size());
        return false;
    }
    for (size_t i = 0; i < m1.size(); ++i) {
        if (m1[i].size() != m2[i].size()) {
             printf("Verification matrix col count mismatch for row %zu: m1[i].size()=%zu, m2[i].size()=%zu\n", i, m1[i].size(), m2[i].size());
            return false;
        }
        if (!compare_complex_vectors(m1[i], m2[i], tolerance)) {
            printf("Verification mismatch in matrix row %zu\n", i);
            return false;
        }
    }
    return true;
}

// Helper for serial 2D FFT (for verification)
void fft_2d_serial(std::vector<std::vector<std::complex<double>>>& matrix) {
    int R = matrix.size();
    if (R == 0) return;
    int C = matrix[0].size();

    // FFT on all rows
    for (int i = 0; i < R; ++i) {
        fft_1d_recursive(matrix[i]);
    }

    // Transpose
    std::vector<std::vector<std::complex<double>>> temp_matrix(C, std::vector<std::complex<double>>(R));
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            temp_matrix[j][i] = matrix[i][j];
        }
    }

    // FFT on rows of transposed matrix (effectively columns of original)
    for (int i = 0; i < C; ++i) {
        fft_1d_recursive(temp_matrix[i]);
    }

    // Transpose back
    for (int i = 0; i < C; ++i) {
        for (int j = 0; j < R; ++j) {
            matrix[j][i] = temp_matrix[i][j];
        }
    }
}

// Task for 1D FFT on rows of a matrix
class OneDFFTRowTask : public IRunnable {
public:
    std::vector<std::vector<std::complex<double>>>* matrix_ptr_;
    int num_total_rows_; // Total rows in the matrix this task might operate on

    OneDFFTRowTask(std::vector<std::vector<std::complex<double>>>* matrix)
        : matrix_ptr_(matrix) {
        num_total_rows_ = matrix ? matrix->size() : 0;
    }

    void runTask(int task_id, int num_overall_tasks) override {
        int rows_per_task_instance = (num_total_rows_ + num_overall_tasks - 1) / num_overall_tasks;
        int start_row = task_id * rows_per_task_instance;
        int end_row = std::min(start_row + rows_per_task_instance, num_total_rows_);

        for (int r = start_row; r < end_row; ++r) {
            if (r < num_total_rows_) { // Ensure row index is valid
                 fft_1d_recursive((*matrix_ptr_)[r]);
            }
        }
    }
};

// Task for Matrix Transpose (Templated for different data types)
template<typename T>
class MatrixTransposeTaskT : public IRunnable {
public:
    const std::vector<std::vector<T>>* src_matrix_;
    std::vector<std::vector<T>>* dst_matrix_; // Must be pre-sized to C_src x R_src
    int R_src_, C_src_;

    MatrixTransposeTaskT(const std::vector<std::vector<T>>* src,
                         std::vector<std::vector<T>>* dst,
                         int R, int C) // R, C are dimensions of src
        : src_matrix_(src), dst_matrix_(dst), R_src_(R), C_src_(C) {}

    void runTask(int task_id, int num_total_tasks) override {
        // Each task transposes a block of rows from source to columns in destination
        int rows_per_task_instance = (R_src_ + num_total_tasks - 1) / num_total_tasks;
        int start_row_src = task_id * rows_per_task_instance;
        int end_row_src = std::min(start_row_src + rows_per_task_instance, R_src_);

        for (int i = start_row_src; i < end_row_src; ++i) {
            for (int j = 0; j < C_src_; ++j) {
                (*dst_matrix_)[j][i] = (*src_matrix_)[i][j];
            }
        }
    }
};

// Task for Array Sum (calculates partial sum for a chunk)
class ArraySumChunkTask : public IRunnable {
public:
    const double* array_ptr_;
    int total_elements_;
    double* partial_sums_ptr_; // Array to store partial sums, indexed by task_id

    ArraySumChunkTask(const double* array, int N, double* partial_sums)
        : array_ptr_(array), total_elements_(N), partial_sums_ptr_(partial_sums) {}

    void runTask(int task_id, int num_total_tasks) override {
        int elements_per_task = (total_elements_ + num_total_tasks - 1) / num_total_tasks;
        int start_el = task_id * elements_per_task;
        int end_el = std::min(start_el + elements_per_task, total_elements_);

        double sum = 0.0;
        for (int i = start_el; i < end_el; ++i) {
            sum += array_ptr_[i];
        }
        partial_sums_ptr_[task_id] = sum;
    }
};

// Task for Dot Product (calculates partial dot product for a chunk)
class DotProductChunkTask : public IRunnable {
public:
    const double* vec_a_ptr_;
    const double* vec_b_ptr_;
    int total_elements_;
    double* partial_dots_ptr_; // Array to store partial dot products

    DotProductChunkTask(const double* vec_a, const double* vec_b, int N, double* partial_dots)
        : vec_a_ptr_(vec_a), vec_b_ptr_(vec_b), total_elements_(N), partial_dots_ptr_(partial_dots) {}

    void runTask(int task_id, int num_total_tasks) override {
        int elements_per_task = (total_elements_ + num_total_tasks - 1) / num_total_tasks;
        int start_el = task_id * elements_per_task;
        int end_el = std::min(start_el + elements_per_task, total_elements_);

        double dot_sum = 0.0;
        for (int i = start_el; i < end_el; ++i) {
            dot_sum += vec_a_ptr_[i] * vec_b_ptr_[i];
        }
        partial_dots_ptr_[task_id] = dot_sum;
    }
};

// --- 2D FFT by Rows Only Test ---
TestResults fft2dByRows_Impl(ITaskSystem* t, bool do_async, int N_dim, int num_fft_tasks) {
    // N_dim must be a power of 2 for fft_1d_recursive
    std::vector<std::vector<std::complex<double>>> matrix(N_dim, std::vector<std::complex<double>>(N_dim));
    std::vector<std::vector<std::complex<double>>> matrix_golden(N_dim, std::vector<std::complex<double>>(N_dim));

    // Initialize matrices
    for (int i = 0; i < N_dim; ++i) {
        for (int j = 0; j < N_dim; ++j) {
            matrix[i][j] = std::complex<double>(rand() % 100, rand() % 100);
            matrix_golden[i][j] = matrix[i][j];
        }
    }

    // Serial computation for golden result (row FFTs only)
    for (int i = 0; i < N_dim; ++i) {
        fft_1d_recursive(matrix_golden[i]);
    }

    OneDFFTRowTask fft_task_runner(&matrix);

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        t->runAsyncWithDeps(&fft_task_runner, num_fft_tasks, {});
        t->sync();
    } else {
        t->run(&fft_task_runner, num_fft_tasks);
    }
    double end_time = CycleTimer::currentSeconds();

    TestResults results;
    results.passed = compare_complex_matrices(matrix, matrix_golden);
    if (!results.passed) printf("ERROR: fft2dByRows_Impl correctness check FAILED!\n");
    results.time = end_time - start_time;
    return results;
}

TestResults fft2dByRowsSyncTest(ITaskSystem* t) {
    return fft2dByRows_Impl(t, false, 1024, 1024); // 512x512 matrix, 512 tasks (1 per row)
}
TestResults fft2dByRowsAsyncTest(ITaskSystem* t) {
    return fft2dByRows_Impl(t, true, 1024, 1024);
}


// --- 2D FFT using Transpose Test (Full 2D FFT) ---
TestResults fft2dByTranspose_Impl(ITaskSystem* t, bool do_async, int N_dim, int num_tasks_per_stage) {
    // N_dim must be a power of 2
    std::vector<std::vector<std::complex<double>>> matrix(N_dim, std::vector<std::complex<double>>(N_dim));
    std::vector<std::vector<std::complex<double>>> matrix_golden(N_dim, std::vector<std::complex<double>>(N_dim));
    
    // Intermediate matrices for the transpose method
    std::vector<std::vector<std::complex<double>>> M1 = matrix; // For row FFTs
    std::vector<std::vector<std::complex<double>>> M2(N_dim, std::vector<std::complex<double>>(N_dim)); // For first transpose
    std::vector<std::vector<std::complex<double>>> M3 = M2; // For col FFTs (as row FFTs on transposed)
    std::vector<std::vector<std::complex<double>>> M_output(N_dim, std::vector<std::complex<double>>(N_dim)); // For final transpose

    // Initialize
    for (int i = 0; i < N_dim; ++i) {
        for (int j = 0; j < N_dim; ++j) {
            M1[i][j] = std::complex<double>(rand() % 100 - 50, rand() % 100 - 50);
            matrix_golden[i][j] = M1[i][j];
        }
    }

    // Serial computation for golden result (full 2D FFT)
    fft_2d_serial(matrix_golden);
    
    OneDFFTRowTask row_fft_runner(&M1);
    MatrixTransposeTaskT<std::complex<double>> transpose1_runner(&M1, &M2, N_dim, N_dim);
    OneDFFTRowTask col_fft_runner(&M2); // Operates on M2 (which is M1 transposed)
    MatrixTransposeTaskT<std::complex<double>> transpose2_runner(&M2, &M_output, N_dim, N_dim);


    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        TaskID task_id_row_fft = t->runAsyncWithDeps(&row_fft_runner, num_tasks_per_stage, {});
        
        std::vector<TaskID> dep_transpose1 = {task_id_row_fft};
        TaskID task_id_transpose1 = t->runAsyncWithDeps(&transpose1_runner, num_tasks_per_stage, dep_transpose1);
        
        std::vector<TaskID> dep_col_fft = {task_id_transpose1};
        TaskID task_id_col_fft = t->runAsyncWithDeps(&col_fft_runner, num_tasks_per_stage, dep_col_fft);
        
        std::vector<TaskID> dep_transpose2 = {task_id_col_fft};
        /* TaskID task_id_transpose2 = */ t->runAsyncWithDeps(&transpose2_runner, num_tasks_per_stage, dep_transpose2);
        
        t->sync();
    } else {
        t->run(&row_fft_runner, num_tasks_per_stage);       // M1 gets row FFTs
        t->run(&transpose1_runner, num_tasks_per_stage);   // M1 transposed into M2
        t->run(&col_fft_runner, num_tasks_per_stage);      // M2 gets "row" FFTs (cols of M1)
        t->run(&transpose2_runner, num_tasks_per_stage);   // M2 transposed into M_output
    }
    double end_time = CycleTimer::currentSeconds();

    TestResults results;
    results.passed = compare_complex_matrices(M_output, matrix_golden);
     if (!results.passed) printf("ERROR: fft2dByTranspose_Impl correctness check FAILED!\n");
    results.time = end_time - start_time;
    return results;
}

TestResults fft2dByTransposeSyncTest(ITaskSystem* t) {
    return fft2dByTranspose_Impl(t, false, 512, 64); 
}
TestResults fft2dByTransposeAsyncTest(ITaskSystem* t) {
    return fft2dByTranspose_Impl(t, true, 512, 64); 
}

// --- Matrix Transpose Test (double) ---
TestResults matrixTranspose_Impl(ITaskSystem* t, bool do_async, int R, int C, int num_transpose_tasks) {
    std::vector<std::vector<double>> matrix_src(R, std::vector<double>(C));
    std::vector<std::vector<double>> matrix_dst(C, std::vector<double>(R)); // Transposed dimensions
    std::vector<std::vector<double>> matrix_golden(C, std::vector<double>(R));

    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            matrix_src[i][j] = static_cast<double>(i * C + j);
        }
    }

    // Serial computation for golden result
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            matrix_golden[j][i] = matrix_src[i][j];
        }
    }

    MatrixTransposeTaskT<double> transpose_runner(&matrix_src, &matrix_dst, R, C);

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        t->runAsyncWithDeps(&transpose_runner, num_transpose_tasks, {});
        t->sync();
    } else {
        t->run(&transpose_runner, num_transpose_tasks);
    }
    double end_time = CycleTimer::currentSeconds();

    TestResults results;
    results.passed = true;
    for(int i=0; i<C; ++i) {
        for(int j=0; j<R; ++j) {
            if (std::abs(matrix_dst[i][j] - matrix_golden[i][j]) > 1e-9) {
                results.passed = false; 
                printf("Verification mismatch matrixTranspose_Impl at dst[%d][%d]: %.5f vs %.5f\n", i,j, matrix_dst[i][j], matrix_golden[i][j]);
                goto end_transpose_check; 
            }
        }
    }
    end_transpose_check:;
    if (!results.passed) printf("ERROR: matrixTranspose_Impl correctness check FAILED!\n");
    results.time = end_time - start_time;
    return results;
}
TestResults matrixTransposeSyncTest(ITaskSystem* t) {
    return matrixTranspose_Impl(t, false, 2048, 4096, 128);
}
TestResults matrixTransposeAsyncTest(ITaskSystem* t) {
    return matrixTranspose_Impl(t, true, 2048, 4096, 128);
}

// --- Array Sum Test ---
TestResults arraySum_Impl(ITaskSystem* t, bool do_async, int array_size, int num_sum_tasks) {
    std::vector<double> array_data(array_size);
    for (int i = 0; i < array_size; ++i) {
        array_data[i] = static_cast<double>(rand() % 10);
    }

    double golden_sum = 0.0;
    for (int i = 0; i < array_size; ++i) {
        golden_sum += array_data[i];
    }

    std::vector<double> partial_sums(num_sum_tasks, 0.0);
    ArraySumChunkTask sum_task_runner(array_data.data(), array_size, partial_sums.data());

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        t->runAsyncWithDeps(&sum_task_runner, num_sum_tasks, {});
        t->sync();
    } else {
        t->run(&sum_task_runner, num_sum_tasks);
    }
    double end_time = CycleTimer::currentSeconds();

    double parallel_sum = 0.0;
    for (int i = 0; i < num_sum_tasks; ++i) {
        parallel_sum += partial_sums[i];
    }
    
    TestResults results;
    results.passed = (std::abs(parallel_sum - golden_sum) < 1e-7 * std::abs(golden_sum) || std::abs(parallel_sum - golden_sum) < 1e-9);
    if (!results.passed) {
         printf("ERROR: arraySum_Impl correctness check FAILED! Parallel: %.5f, Golden: %.5f\n", parallel_sum, golden_sum);
    }
    results.time = end_time - start_time;
    return results;
}
TestResults arraySumSyncTest(ITaskSystem* t) {
    return arraySum_Impl(t, false, 80 * 1024 * 1024, 256);
}
TestResults arraySumAsyncTest(ITaskSystem* t) {
    return arraySum_Impl(t, true, 80 * 1024 * 1024, 256);
}

// --- Dot Product Test ---
TestResults dotProduct_Impl(ITaskSystem* t, bool do_async, int vec_size, int num_dot_tasks) {
    std::vector<double> vec_a(vec_size);
    std::vector<double> vec_b(vec_size);
    for (int i = 0; i < vec_size; ++i) {
        vec_a[i] = static_cast<double>(rand() % 5);
        vec_b[i] = static_cast<double>(rand() % 5);
    }

    double golden_dot_product = 0.0;
    for (int i = 0; i < vec_size; ++i) {
        golden_dot_product += vec_a[i] * vec_b[i];
    }

    std::vector<double> partial_dots(num_dot_tasks, 0.0);
    DotProductChunkTask dot_task_runner(vec_a.data(), vec_b.data(), vec_size, partial_dots.data());

    double start_time = CycleTimer::currentSeconds();
    if (do_async) {
        t->runAsyncWithDeps(&dot_task_runner, num_dot_tasks, {});
        t->sync();
    } else {
        t->run(&dot_task_runner, num_dot_tasks);
    }
    double end_time = CycleTimer::currentSeconds();

    double parallel_dot_product = 0.0;
    for (int i = 0; i < num_dot_tasks; ++i) {
        parallel_dot_product += partial_dots[i];
    }

    TestResults results;
    results.passed = (std::abs(parallel_dot_product - golden_dot_product) < 1e-7 * std::abs(golden_dot_product) || std::abs(parallel_dot_product - golden_dot_product) < 1e-9);
     if (!results.passed) {
         printf("ERROR: dotProduct_Impl correctness check FAILED! Parallel: %.5f, Golden: %.5f\n", parallel_dot_product, golden_dot_product);
    }
    results.time = end_time - start_time;
    return results;
}
TestResults dotProductSyncTest(ITaskSystem* t) {
    return dotProduct_Impl(t, false, 80 * 1024 * 1024, 256);
}
TestResults dotProductAsyncTest(ITaskSystem* t) {
    return dotProduct_Impl(t, true, 80 * 1024 * 1024, 256);
}
