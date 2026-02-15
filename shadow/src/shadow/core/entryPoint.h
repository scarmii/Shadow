#pragma once

#include "shadow/core/core.h"
#include "shadow/debug/instrumentor.h"

#ifdef SH_DEBUG
	#include <crtdbg.h>
#endif

extern Shadow::ShApp* Shadow::createApp();

int main(int argc, char** argv)
{
	{
		SH_PROFILE_BEGIN_SESSION("startup", "shadowProfile-startup.json")
		auto game = Shadow::createApp();
		SH_PROFILE_END_SESSION();

		SH_PROFILE_BEGIN_SESSION("runtime", "shadowProfile-runtime.json");
		game->run();
		SH_PROFILE_END_SESSION();

		SH_PROFILE_BEGIN_SESSION("shutdown", "shadowProfile-shutdown.json")
		delete game;
		SH_PROFILE_END_SESSION();

		delete& Shadow::Instrumentor::get();
	}

	_CrtDumpMemoryLeaks();
	return 0;
}