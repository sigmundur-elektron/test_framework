#define DOCTEST_CONFIG_IMPLEMENT
#include "../test/factorial_test.h"
#include "../test/read_file_tool_test.h"
#include "../test/agent_a2a_test.h"
#include "../test/ai_persistence_test.h"
#include <doctest/doctest.h>
#include "app.h"

const bool testing = false;
static app a;

int main(int _argc, char **_argv)
{
	doctest::Context context;
	if (testing)
		int res = context.run();
	else
		a.run();

	return 0;
}
