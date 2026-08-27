#include "permissions.h"

bool permissions::allowed(scope /*s*/) const
{
	// TODO: replace with real policy. Default: read allowed, write gated.
	return true;
}