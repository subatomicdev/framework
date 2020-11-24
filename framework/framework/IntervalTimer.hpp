#pragma once

#include <chrono>
#include <functional>
#include <future>


namespace framework
{
	class IntervalTimer
	{
	public:
		IntervalTimer(const std::chrono::milliseconds period);
		IntervalTimer(const std::chrono::seconds period);

		~IntervalTimer();

		void start(std::function<void()> callback);
		void stop();

	private:
		std::chrono::milliseconds m_period;
		std::future<void> m_future;
		std::atomic_bool m_running;
		std::function<void()> m_callback;
	};
}