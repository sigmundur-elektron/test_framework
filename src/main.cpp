#define DOCTEST_CONFIG_IMPLEMENT
#include "../test/factorial_test.h"
#include <doctest/doctest.h>
#include "app.h"

const bool testing = true;
static app a;

int main(int _argc, char **_argv)
{
	doctest::Context context;
	if (testing)
		int res = context.run(); // run tests
	else
		a.run();

	return 0;
}
