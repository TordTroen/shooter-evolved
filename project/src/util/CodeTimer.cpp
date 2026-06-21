#include "CodeTimer.h"

#include <chrono>
#include <iostream>

namespace Private
{
static std::string time_to_string(clock_type start, clock_type end)
{
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	return std::to_string(ms) + "ms";
	
}
}

CodeTimer::CodeTimer(std::string_view name)
{
	m_start = std::chrono::steady_clock::now();
	m_name = name;
}

CodeTimer::~CodeTimer()
{
    const auto end = std::chrono::steady_clock::now();
	std::cout << "[time:" << m_name << "] " << Private::time_to_string(m_start, end) << std::endl;
}
