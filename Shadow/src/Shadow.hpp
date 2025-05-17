#pragma once

#include "Shadow/Core/Log.hpp"
#include "Shadow/Core/Core.hpp"
#include "Shadow/Core/ShEngine.hpp"
#include "Shadow/WindowLayer/Input.hpp"
#include "Shadow/Core/InputDefines.hpp"
#include "Shadow/Renderer/Camera.hpp"
#include "Shadow/Renderer/CameraController.hpp"
		 
#include "Shadow/Core/Timestep.hpp"

// -- Renderer --------------------------
#include "Shadow/Renderer/Renderer.hpp"
#include "Shadow/Renderer/Renderer2D.hpp"
#include "Shadow/Renderer/Pipeline.hpp"
#include "Shadow/Renderer/Renderpass.hpp"
#include "Shadow/Renderer/Shader.hpp"
#include "Shadow/Renderer/Buffer.hpp"
#include "Shadow/Renderer/UniformBuffer.hpp"
#include "Shadow/Renderer/Texture.hpp"
#include "Shadow/Renderer/Mesh.hpp"
// --------------------------------------

#ifdef SHADOW_ENTRY
	// -- Entry Point -----------------------
	#include "Shadow/Core/EntryPoint.hpp"
	// --------------------------------------
#endif
