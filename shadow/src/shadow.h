#pragma once

#include "shadow/core/log.h"
#include "shadow/core/core.h"
#include "shadow/core/shApp.h"
#include "shadow/core/timestep.h"
#include "shadow/core/types.h"
#include "shadow/core/inputCodes.h"
#include "shadow/core/input.h"
#include "shadow/renderer/camera.h"
#include "shadow/renderer/cameraController.h"

#include "shadow/imGui/imGuiLayer.h"

#include "shadow/scene/scene.h"
#include "shadow/scene/entity.h"
#include "shadow/scene/components.h"
		 
// -- renderer --------------------------
#include "shadow/renderer/renderer.h"
#include "shadow/renderer/renderer2D.h"
#include "shadow/renderer/mesh.h"
#include "shadow/renderer/rendergraph.h"
#include "shadow/vulkan/context.h"
#include "shadow/vulkan/device.h"
#include "shadow/vulkan/commandBuffer.h"
#include "shadow/vulkan/buffer.h"
#include "shadow/vulkan/texture.h"
#include "shadow/vulkan/pipeline.h"
#include "shadow/vulkan/renderPass.h"
#include "shadow/vulkan/shader.h"
// --------------------------------------

// -- utils -----------------------------
#include "shadow/utils/random.h"
#include "shadow/utils/rendergraphWriter.h"
// --------------------------------------

#ifdef SHADOW_ENTRY
	// -- Entry Point -----------------------
	#include "shadow/core/entryPoint.h"
	// --------------------------------------
#endif
