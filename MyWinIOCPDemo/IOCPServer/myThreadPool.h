#ifndef _MYTHREADOOL_H_
#define _MYTHREADOOL_H_

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

class MyThreadPool {

public:
	MyThreadPool(int nThreads);
	~MyThreadPool();

	template<class F, class ... Args>
	auto EnQueue(F&& f, Args&& ... args) ->std::future<decltype(f(args...))>;

private:

private:
	std::vector< std::thread > m_threadWorkers;
	std::queue< std::function<void()> > m_queTasks;
	std::mutex m_mtxQueue;
	std::condition_variable m_cdvQueue;
	bool m_bStop;
};



MyThreadPool::MyThreadPool(int nThreads)
	: m_bStop(false)
{
	for (int i = 0; i < nThreads; ++i)
	{
		m_threadWorkers.emplace_back(
			[this]
			{
				for (;;)
				{
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(this->m_mtxQueue);
						this->m_cdvQueue.wait(lock,
									[this] { return this->m_bStop || !this->m_queTasks.empty(); });
						if (this->m_bStop && this->m_queTasks.empty())
							return;
						task = std::move(this->m_queTasks.front());
						this->m_queTasks.pop();
					}
					task();
				}
			}
		);
	}
}

MyThreadPool::~MyThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(m_mtxQueue);
		m_bStop = true;
	}
	m_cdvQueue.notify_all();
	for (auto& thread : m_threadWorkers)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

template<class F, class... Args>
auto MyThreadPool::EnQueue(F&& f, Args&& ... args)
	->std::future<decltype(f(args...))>
{
	using ReturnType = decltype(f(args...));
	auto task = std::make_shared< std::packaged_task<ReturnType()> >(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
	std::future<ReturnType> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(m_mtxQueue);
		if (m_bStop)
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}
		m_queTasks.emplace([task]() { (*task)(); });
	}
	m_cdvQueue.notify_one();
	return res;
}
#endif // !_MYTHREADOOL_H_