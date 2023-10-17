#pragma once
#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>



//�̳߳���
class ThreadPool
{
private:
	ThreadPool() {}; // ����ģʽ�½��չ��캯������Ϊ˽��
public:
	ThreadPool(const ThreadPool& temp) = delete; // ����ģʽ��ɾ���������캯��
	ThreadPool& operator = (const ThreadPool& temp) = delete; // ����ģʽ��ɾ���������������
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
	std::mutex mtx; // ������
	std::condition_variable condition; // ��������
	std::vector<std::thread> threads; // �߳�����
	std::queue<std::function<void()>> tasks; // �������

	bool stop; // �ж��̳߳��Ƿ���ֹ
};

ThreadPool* getInstance();