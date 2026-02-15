#define SHADOW_ENTRY
#include "shadow.h"

#include "instancedRendering.h"
#include "sandbox2D.h"
#include "offscreen.h"
#include "particleSystem.h"
#include "renderGraphDemo.h"
#include "imgui/imgui.h"

Shadow::ShApp* Shadow::createApp()
{
	//return new OffscreenDemo();
	//return new InstancedRendering();
	//return new Sandbox2D();
	//return new RenderGraphDemo();
	return nullptr;
}