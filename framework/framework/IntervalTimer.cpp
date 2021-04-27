#include "IntervalTimer.hpp"


namespace framework
{
	IntervalTimer::IntervalTimer() : m_running(false), m_period(std::chrono::microseconds::rep{ 0 })
	{

	}

	IntervalTimer::IntervalTimer(const std::chrono::milliseconds period) : m_period(period), m_running(false)
	{

	}

	IntervalTimer::~IntervalTimer()
	{
		stop();
	}


	void IntervalTimer::start(std::function<void()> callback, const std::chrono::milliseconds period)
	{
		m_period = period;
		start(callback);
	}


	void IntervalTimer::start(std::function<void()> callback)
	{
		m_callback = callback;
		m_running.store(true);

		m_future = std::async(std::launch::async, [this]
		{
			while (m_running.load())
			{
				std::unique_lock lock(m_mux);
				m_cv.wait_for(lock, m_period, [this] { return m_running.load() == false; });
				m_callback();
			}
		});
	}


	void IntervalTimer::stop()
	{
		m_running.store(false);
		m_cv.notify_one();

		if (m_future.valid())
			m_future.wait();
	}
}