/*
 * This file is part of the CitizenFX project - http://citizen.re/
 *
 * See LICENSE and MENTIONS in the root of the source tree for information
 * regarding licensing.
 */

#include "StdInc.h"
#include <signal.h>

static void HandleAbort(int signal)
{
	RaiseException(0xC000DEAD, 0, 0, nullptr);
}

static InitFunction initFunction([] ()
{
	signal(SIGABRT, HandleAbort);

	_set_abort_behavior(0, _WRITE_ABORT_MSG);
});