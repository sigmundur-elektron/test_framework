#define DOCTEST_CONFIG_IMPLEMENT
#include "../test/agent_a2a_test.h"
#include "../test/agent_export_test.h"
#include "../test/agent_template_test.h"
#include "../test/ai_persistence_test.h"
#include "../test/factorial_test.h"
#include "../test/mvp_gaps_test.h"
#include "../test/permissions_test.h"
#include "../test/read_file_tool_test.h"
#include "app.h"
#include <cstring>
#include <doctest/doctest.h>

// Compile-time default (handy while debugging). Overridden at runtime by the
// -t / --test command-line flag.
const bool testing = false;
static app a;

static bool wants_tests(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "-t") == 0 || std::strcmp(argv[i], "--test") == 0)
			return true;
	}
	return false;
}

int main(int _argc, char **_argv)
{
	const bool run_tests = testing || wants_tests(_argc, _argv);

	if (run_tests)
	{
		doctest::Context context;
		context.applyCommandLine(_argc, _argv);
		return context.run();
	}

	a.run();
	return 0;
}
