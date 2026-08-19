#pragma once
// -----------------------------------------------------------------------------
// profiler.h - single-header, zero-dependency scoped profiler.
//
// Produces two outputs:
//   1. A console summary of how long each named scope took (the "timer").
//   2. A Chrome Trace Event JSON file that can be opened directly as a
//      flamegraph in chrome://tracing, https://ui.perfetto.dev or
//      https://www.speedscope.app.
//
// Usage:
//   PROFILE_BEGIN_SESSION("startup", "startup_profile.json");
//   {
//       PROFILE_SCOPE("expensive_thing");
//       expensive_thing();
//   }
//   PROFILE_END_SESSION();   // writes JSON + prints summary
//
// Compile-time toggle: define PROFILING_ENABLED=0 to strip all overhead.
// -----------------------------------------------------------------------------

#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif

#if PROFILING_ENABLED

#include <chrono>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <print>
#include <string>
#include <thread>
#include <vector>

namespace prof
{
struct result
{
	std::string name;
	std::int64_t start_us; // microseconds since session start
	std::int64_t dur_us;   // duration in microseconds
	std::uint32_t depth;   // nesting depth (for the summary indentation)
	std::size_t tid;       // thread id hash
};

class profiler
{
  public:
	static profiler &get()
	{
		static profiler instance;
		return instance;
	}

	void begin_session(std::string name, std::string filepath)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_session_name = std::move(name);
		m_filepath = std::move(filepath);
		m_results.clear();
		m_active = true;
		m_depth = 0;
		m_epoch = std::chrono::steady_clock::now();
	}

	void end_session()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_active)
			return;
		m_active = false;
		write_json();
		print_summary();
	}

	bool active() const { return m_active; }

	std::uint32_t enter() { return m_depth++; }
	void leave()
	{
		if (m_depth > 0)
			--m_depth;
	}

	std::chrono::steady_clock::time_point epoch() const { return m_epoch; }

	void record(result r)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_active)
			m_results.push_back(std::move(r));
	}

  private:
	void write_json()
	{
		if (m_filepath.empty())
			return;
		std::ofstream out(m_filepath, std::ios::trunc);
		if (!out)
			return;

		out << "{\"otherData\":{\"session\":\"" << m_session_name
			<< "\"},\"traceEvents\":[";
		bool first = true;
		for (const auto &r : m_results)
		{
			if (!first)
				out << ',';
			first = false;

			std::string name = r.name;
			for (auto &c : name) // escape quotes/backslashes minimally
				if (c == '"' || c == '\\')
					c = '\'';

			out << "{\"cat\":\"function\",\"dur\":" << r.dur_us
				<< ",\"name\":\"" << name << "\",\"ph\":\"X\",\"pid\":0,\"tid\":"
				<< r.tid << ",\"ts\":" << r.start_us << '}';
		}
		out << "]}";
	}

	void print_summary()
	{
		std::println("");
		std::println("==== profile summary: {} ====", m_session_name);
		std::int64_t total = 0;
		for (const auto &r : m_results)
			if (r.depth == 0)
				total += r.dur_us;

		for (const auto &r : m_results)
		{
			std::string indent(static_cast<std::size_t>(r.depth) * 2, ' ');
			double ms = static_cast<double>(r.dur_us) / 1000.0;
			double pct = total > 0 ? (100.0 * static_cast<double>(r.dur_us) /
									  static_cast<double>(total))
								   : 0.0;
			std::println("  {}{:<32} {:>9.3f} ms  ({:>5.1f}%)", indent, r.name,
						 ms, pct);
		}
		std::println("  {:<34} {:>9.3f} ms", "TOTAL (top-level)",
					 static_cast<double>(total) / 1000.0);
		if (!m_filepath.empty())
			std::println("  flamegraph -> {} (open in chrome://tracing / "
						 "ui.perfetto.dev / speedscope.app)",
						 m_filepath);
		std::println("=============================================");
	}

	std::mutex m_mutex;
	std::vector<result> m_results;
	std::string m_session_name;
	std::string m_filepath;
	bool m_active = false;
	std::uint32_t m_depth = 0;
	std::chrono::steady_clock::time_point m_epoch{};
};

class scoped_timer
{
  public:
	explicit scoped_timer(std::string name) : m_name(std::move(name))
	{
		auto &p = profiler::get();
		m_depth = p.enter();
		m_start = std::chrono::steady_clock::now();
	}

	~scoped_timer()
	{
		auto end = std::chrono::steady_clock::now();
		auto &p = profiler::get();
		p.leave();

		auto start_us = std::chrono::duration_cast<std::chrono::microseconds>(
							m_start - p.epoch())
							.count();
		auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(
						  end - m_start)
						  .count();

		p.record(result{std::move(m_name), start_us, dur_us, m_depth,
						 std::hash<std::thread::id>{}(std::this_thread::get_id())});
	}

  private:
	std::string m_name;
	std::chrono::steady_clock::time_point m_start;
	std::uint32_t m_depth = 0;
};
} // namespace prof

#define PROFILE_CONCAT_INNER(a, b) a##b
#define PROFILE_CONCAT(a, b) PROFILE_CONCAT_INNER(a, b)

#define PROFILE_BEGIN_SESSION(name, filepath)                                  \
	::prof::profiler::get().begin_session((name), (filepath))
#define PROFILE_END_SESSION() ::prof::profiler::get().end_session()
#define PROFILE_SCOPE(name)                                                    \
	::prof::scoped_timer PROFILE_CONCAT(_prof_timer_, __LINE__)((name))
#define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)

#else // PROFILING_ENABLED == 0

#define PROFILE_BEGIN_SESSION(name, filepath) ((void)0)
#define PROFILE_END_SESSION() ((void)0)
#define PROFILE_SCOPE(name) ((void)0)
#define PROFILE_FUNCTION() ((void)0)

#endif // PROFILING_ENABLED
