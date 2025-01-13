// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "thread_pool.h"

#include <algorithm>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <system_error>

#include <fmt/format.h>
#include <tracy/tracy/Tracy.hpp>

#include "../core/string.h"
#include "../core/vector.hpp"
#include "../cxxutil.hpp"
#include "../m_argv.h"

using namespace srb2;

static void do_work(ThreadPool::Task& work)
{
	try
	{
		ZoneScoped;
		(work.thunk)(work.raw.data());
	}
	catch (...)
	{
		// can't do anything
	}

	(work.deleter)(work.raw.data());
	if (work.pseudosema)
	{
		work.pseudosema->fetch_sub(1, std::memory_order_relaxed);
	}
}

static void pool_executor(
	int thread_index,
	std::shared_ptr<std::atomic<bool>> pool_alive,
	std::shared_ptr<std::mutex> worker_ready_mutex,
	std::shared_ptr<std::condition_variable> worker_ready_condvar,
	std::shared_ptr<ThreadPool::Queue> my_wq,
	srb2::Vector<std::shared_ptr<ThreadPool::Queue>> other_wqs
)
{
	{
		srb2::String thread_name = srb2::format("Thread Pool Thread {}", thread_index);
		tracy::SetThreadName(thread_name.c_str());
	}

	int spins = 0;
	while (true)
	{
		std::optional<ThreadPool::Task> work = my_wq->steal();
		bool did_work = false;
		if (work)
		{
			do_work(*work);

			did_work = true;
			spins = 0;
		}
		else
		{
			for (auto& q : other_wqs)
			{
				work = q->steal();
				if (work)
				{
					do_work(*work);

					did_work = true;
					spins = 0;

					// We only want to steal one work item at a time, to prioritize our own queue
					break;
				}
			}
		}

		if (!did_work)
		{
			// Spin a few loops to avoid yielding, then wait for the ready lock
			spins += 1;
			if (spins > 100)
			{
				std::unique_lock<std::mutex> ready_lock {*worker_ready_mutex};
				while (my_wq->empty() && pool_alive->load())
				{
					worker_ready_condvar->wait(ready_lock);
				}

				if (!pool_alive->load())
				{
					break;
				}
			}
		}
	}
}

ThreadPool::ThreadPool()
{
	immediate_mode_ = true;
}

ThreadPool::ThreadPool(size_t threads)
{
	next_queue_index_ = 0;
	pool_alive_ = std::make_shared<std::atomic<bool>>(true);

	for (size_t i = 0; i < threads; i++)
	{
		std::shared_ptr<Queue> wsq = std::make_shared<Queue>(2048);
		work_queues_.push_back(wsq);

		std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
		worker_ready_mutexes_.push_back(std::move(mutex));
		std::shared_ptr<std::condition_variable> condvar = std::make_shared<std::condition_variable>();
		worker_ready_condvars_.push_back(std::move(condvar));
	}

	for (size_t i = 0; i < threads; i++)
	{
		std::shared_ptr<Queue> my_queue = work_queues_[i];
		srb2::Vector<std::shared_ptr<Queue>> other_queues;
		for (size_t j = 0; j < threads; j++)
		{
			// Order the other queues starting from the next adjacent worker
			// i.e. if this is worker 2 of 8, then other queues is 3, 4, 5, 6, 7, 0, 1
			// This tries to balance out work stealing behavior

			size_t other_index = j + i;
			if (other_index >= threads)
			{
				other_index -= threads;
			}

			if (other_index != i)
			{
				other_queues.push_back(work_queues_[other_index]);
			}
		}

		std::thread thread;
		try
		{
			thread = std::thread
			{
				pool_executor,
				i,
				pool_alive_,
				worker_ready_mutexes_[i],
				worker_ready_condvars_[i],
				my_queue,
				other_queues
			};
		}
		catch (const std::system_error& error)
		{
			// Safe shutdown and rethrow
			pool_alive_->store(false);
			for (auto& t : threads_)
			{
				t.join();
			}
			throw error;
		}

		threads_.push_back(std::move(thread));
	}
}

ThreadPool::ThreadPool(ThreadPool&&) = default;
ThreadPool::~ThreadPool() = default;

ThreadPool& ThreadPool::operator=(ThreadPool&&) = default;

void ThreadPool::begin_sema()
{
	sema_begun_ = true;
}

ThreadPool::Sema ThreadPool::end_sema()
{
	Sema ret = Sema(std::move(cur_sema_));
	cur_sema_ = nullptr;
	sema_begun_ = false;
	return ret;
}

void ThreadPool::notify()
{
	for (size_t i = 0; i < work_queues_.size(); i++)
	{
		auto& q = work_queues_[i];
		size_t count = q->size();
		if (count > 0)
		{
			worker_ready_condvars_[i]->notify_one();
		}
	}
}

void ThreadPool::notify_sema(const ThreadPool::Sema& sema)
{
	if (!sema.pseudosema_)
	{
		return;
	}
	notify();
}

void ThreadPool::wait_idle()
{
	if (immediate_mode_)
	{
		return;
	}

	ZoneScoped;

	for (size_t i = 0; i < work_queues_.size(); i++)
	{
		auto& q = work_queues_[i];

		std::optional<Task> work;
		while ((work = q->pop()).has_value())
		{
			do_work(*work);
		}
	}
}

void ThreadPool::wait_sema(const Sema& sema)
{
	if (!sema.pseudosema_)
	{
		return;
	}

	ZoneScoped;

	while (sema.pseudosema_->load(std::memory_order_seq_cst) > 0)
	{
		// spin to win
		for (size_t i = 0; i < work_queues_.size(); i++)
		{
			auto& q = work_queues_[i];

			std::optional<Task> work;
			if ((work = q->pop()).has_value())
			{
				do_work(*work);
				break;
			}
		}
	}

	if (sema.pseudosema_->load(std::memory_order_seq_cst) != 0)
	{
		throw std::exception();
	}
}

void ThreadPool::shutdown()
{
	if (immediate_mode_)
	{
		return;
	}

	wait_idle();

	pool_alive_->store(false);

	for (auto& condvar : worker_ready_condvars_)
	{
		condvar->notify_all();
	}
	for (auto& t : threads_)
	{
		t.join();
	}
}

std::unique_ptr<ThreadPool> srb2::g_main_threadpool;

void I_ThreadPoolInit(void)
{
	SRB2_ASSERT(g_main_threadpool == nullptr);
	size_t thread_count = std::min(static_cast<unsigned int>(9), std::thread::hardware_concurrency());
	if (thread_count > 1)
	{
		// The main thread will act as a worker when waiting for pool idle
		// Make one less worker thread to avoid unnecessary context switching
		thread_count -= 1;
	}

	if (M_CheckParm("-singlethreaded"))
	{
		g_main_threadpool = std::make_unique<ThreadPool>();
	}
	else
	{
		g_main_threadpool = std::make_unique<ThreadPool>(thread_count);
	}
}

void I_ThreadPoolShutdown(void)
{
	if (!g_main_threadpool)
	{
		return;
	}

	g_main_threadpool->shutdown();
	g_main_threadpool = nullptr;
}

void I_ThreadPoolSubmit(srb2cthunk_t thunk, void* data)
{
	SRB2_ASSERT(g_main_threadpool != nullptr);

	g_main_threadpool->schedule([=]() {
		(thunk)(data);
	});
	g_main_threadpool->notify();
}

void I_ThreadPoolWaitIdle(void)
{
	SRB2_ASSERT(g_main_threadpool != nullptr);

	g_main_threadpool->wait_idle();
}
