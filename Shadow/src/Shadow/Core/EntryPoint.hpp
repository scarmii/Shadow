#pragma once
#include "Shadow/Core/Core.hpp"
#include "Shadow/Debug/Instrumentor.hpp"

extern Shadow::ShEngine* Shadow::createApp();

int main(int argc, char** argv)
{
	SH_PROFILE_BEGIN_SESSION("startup", "ShadowProfile-startup.json")
	auto game = Shadow::createApp();
	SH_PROFILE_END_SESSION();

	SH_PROFILE_BEGIN_SESSION("runtime", "ShadowProfile-runtime.json");
	game->run();
	SH_PROFILE_END_SESSION();

	SH_PROFILE_BEGIN_SESSION("shutdown", "ShadowProfile-shutdown.json")
	delete game;
	SH_PROFILE_END_SESSION();

	delete& Shadow::Instrumentor::get();
	return 0;
}