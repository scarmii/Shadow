#define SHADOW_ENTRY
#include "Shadow.hpp"

#include "imgui/imgui.h"
#include "InstancedRendering.hpp"
#include "Sandbox2D.hpp"
#include "OffscreenRendering.hpp"
#include "GameLayer.hpp"
#include "ParticleSystem.hpp"

class SandboxGame : public Shadow::ShEngine
{
public:
	SandboxGame()
	{
		//pushLayer(new GameLayer());
		//pushLayer(new OffscreenRendering());
       	//pushLayer(new Sandbox2D());
		pushLayer(new InstancedRendering());
		//pushLayer(new ParticleSystem());
	}
};

Shadow::ShEngine* Shadow::createApp()
{
	return new SandboxGame();
}