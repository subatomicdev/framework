#pragma once

#include <chrono>
#include <functional>
#include <future>


namespace framework
{
	class IntervalTimer
	{
	public:
		IntervalTimer();
		IntervalTimer(const std::chrono::milliseconds period);

		~IntervalTimer();

		void start(std::function<void()> callback);
		void start(std::function<void()> callback, const std::chrono::milliseconds period);

		void stop();

	private:
		std::chrono::milliseconds m_period;
		std::future<void> m_future;
		std::atomic_bool m_running;
		std::function<void()> m_callback;
		std::condition_variable m_cv;
		std::mutex m_mux;
	};
}