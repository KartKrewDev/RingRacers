// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_CORE_THREAD_POOL_H__
#define __SRB2_CORE_THREAD_POOL_H__

#include <stddef.h>

#ifdef __cplusplus

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "spmc_queue.hpp"

namespace srb2
{

class ThreadPool
{
public:
	struct Task
	{
		void (*thunk)(void*);
		void (*deleter)(void*);
		std::shared_ptr<std::atomic<uint32_t>> pseudosema;
		std::array<std::byte, 512 - sizeof(void(*)(void*)) * 2 - sizeof(std::shared_ptr<std::atomic<uint32_t>>)> raw;
	};

	using Queue = SpMcQueue<Task>;

	class Sema
	{
		std::shared_ptr<std::atomic<uint32_t>> pseudosema_;

		explicit Sema(std::shared_ptr<std::atomic<uint32_t>> sema) : pseudosema_(sema) {}

		friend class ThreadPool;
	public:
		Sema() = default;
	};

private:
	std::shared_ptr<std::atomic<bool>> pool_alive_;
	std::vector<std::shared_ptr<std::mutex>> worker_ready_mutexes_;
	std::vector<std::shared_ptr<std::condition_variable>> worker_ready_condvars_;
	std::vector<std::shared_ptr<Queue>> work_queues_;
	std::vector<std::thread> threads_;
	size_t next_queue_index_ = 0;
	std::shared_ptr<std::atomic<uint32_t>> cur_sema_;

	bool immediate_mode_ = false;
	bool sema_begun_ = false;

public:
	ThreadPool();
	explicit ThreadPool(size_t threads);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&);
	~ThreadPool();

	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&);

	void begin_sema();
	ThreadPool::Sema end_sema();

	/// Enqueue but don't notify
	template <typename T> void schedule(T&& thunk);
	/// Notify threads after several schedules
	void notify();
	void notify_sema(const Sema& sema);
	void wait_idle();
	void wait_sema(const Sema& sema);
	void shutdown();
};

extern std::unique_ptr<ThreadPool> g_main_threadpool;

template <typename F>
void callable_caller(F* f)
{
	(*f)();
}

template <typename F>
void callable_destroyer(F* f)
{
	f->~F();
}

template <typename T>
void ThreadPool::schedule(T&& thunk)
{
	static_assert(sizeof(T) <= sizeof(std::declval<Task>().raw));

	if (immediate_mode_)
	{
		(thunk)();
		return;
	}

	if (sema_begun_)
	{
		if (cur_sema_ == nullptr)
		{
			cur_sema_ = std::make_shared<std::atomic<uint32_t>>(0);
		}
		cur_sema_->fetch_add(1, std::memory_order_relaxed);
	}

	size_t qi = next_queue_index_;

	{
		std::shared_ptr<Queue> q = work_queues_[qi];
		Task task;
		task.thunk = reinterpret_cast<void(*)(void*)>(callable_caller<T>);
		task.deleter = reinterpret_cast<void(*)(void*)>(callable_destroyer<T>);
		task.pseudosema = cur_sema_;
		new (reinterpret_cast<T*>(task.raw.data())) T(std::move(thunk));

		q->push(std::move(task));
	}
	// worker_ready_condvars_[qi]->notify_one();

	next_queue_index_ += 1;
	if (next_queue_index_ >= threads_.size())
	{
		next_queue_index_ = 0;
	}
}

} // namespace srb2

extern "C" {

#endif // __cplusplus

typedef void (*srb2cthunk_t)(void*);

void I_ThreadPoolInit(void);
void I_ThreadPoolShutdown(void);
void I_ThreadPoolSubmit(srb2cthunk_t thunk, void* data);
void I_ThreadPoolWaitIdle(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __SRB2_CORE_THREAD_POOL_H__
