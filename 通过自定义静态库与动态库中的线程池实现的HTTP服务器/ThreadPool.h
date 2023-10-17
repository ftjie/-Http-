#pragma once
#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>



//线程池类
class ThreadPool
{
private:
	ThreadPool() {}; // 单例模式下将空构造函数设置为私有
public:
	ThreadPool(const ThreadPool& temp) = delete; // 单例模式下删除拷贝构造函数
	ThreadPool& operator = (const ThreadPool& temp) = delete; // 单例模式下删除拷贝复制运算符
	ThreadPool(int numThreads);

	static ThreadPool& GetInstance(int numThreads)
	{
		static ThreadPool pool(numThreads);
		return pool;
	}

	~ThreadPool();

	template<class F, class ...Args>
	void enqueue(F&& f, Args &&... args)
	{
		std::function<void()> task =
			std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		{
			std::unique_lock<std::mutex> lock(mtx);
			tasks.emplace(task);
		}
		condition.notify_one();
	}

private:
	std::mutex mtx; // 互斥锁
	std::condition_variable condition; // 条件变量
	std::vector<std::thread> threads; // 线程数组
	std::queue<std::function<void()>> tasks; // 任务队列

	bool stop; // 判断线程池是否终止
};

ThreadPool* getInstance();