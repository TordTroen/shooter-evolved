#pragma once

#include <chrono>
#include <string>

typedef std::chrono::steady_clock::time_point clock_type;

class CodeTimer
{
public:
    CodeTimer(std::string_view name);
	~CodeTimer();
private:
	clock_type m_start;
	std::string m_name;
};
