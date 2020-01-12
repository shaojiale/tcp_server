#pragma once
#include <thread>
#include <mutex>
#include <list>
#include <functional>
//任务基类
class CellTask
{
public:
	CellTask()
	{

	}
	virtual ~CellTask()
	{

	}
	virtual void doTask()
	{

	}
private:
};
typedef std::shared_ptr<CellTask> CellTaskPtr;
class CellTaskServer
{
public:
	void addTask(CellTaskPtr& task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_taskBuf.push_back(task);
	}
	void Start()
	{
		std::thread thread(std::mem_fn(&CellTaskServer::OnRun), this);
		thread.detach();
	}
protected:
	void OnRun()
	{
		while (true)
		{
			if (!_taskBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _taskBuf)
				{
					_tasks.push_back(pTask);
				}
				_taskBuf.clear();
			}
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			for (auto pTask : _tasks)
			{
				pTask->doTask();
			}
			_tasks.clear();
		}
	}
private:
	//任务数据
	std::list<CellTaskPtr> _tasks;
	//任务缓冲区
	std::list<CellTaskPtr> _taskBuf;
	//
	std::mutex _mutex;
};



