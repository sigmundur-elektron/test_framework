#include "profiler.h"
#include <chrono>
#include <iomanip> // for setprecision(2)

void profiler::record_timing(const std::string &name)
{
	std::lock_guard<std::mutex> lock(mutex_);
	timings.emplace_back(name);
}

void profiler::report()
{
	for (int i = 0; i < timings.size(); i++)
	{
		// DrawText(timings[i].c_str(), 20, global_state::screen_height() - 200 - 25 * i, 12, WHITE);
	}
	timings.clear();
}

timer::timer(const std::string &name) : m_name(name), m_start(std::chrono::high_resolution_clock::now()) {}

timer::~timer()
{
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
	double duration_milli = duration.count() / 1000.0;
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2);
	ss << "Timing for " << m_name << ": " << duration_milli << " ms\n";
	profiler::record_timing(ss.str());
}

std::vector<std::string> profiler::timings;
std::mutex profiler::mutex_;