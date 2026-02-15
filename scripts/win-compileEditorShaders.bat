@echo off
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/shadow/shadow-editor/assets/shaders/lighting.vert -o C:/dev/shadow/shadow-editor/assets/shaders/lighting.vert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/shadow/shadow-editor/assets/shaders/lighting.frag -o C:/dev/shadow/shadow-editor/assets/shaders/lighting.frag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/shadow/shadow-editor/assets/shaders/offscreen.vert -o C:/dev/shadow/shadow-editor/assets/shaders/offscreen.vert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/shadow/shadow-editor/assets/shaders/offscreen.frag -o C:/dev/shadow/shadow-editor/assets/shaders/offscreen.frag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/shadow/shadow-editor/assets/shaders/reversedColor.frag -o C:/dev/shadow/shadow-editor/assets/shaders/reversedColor.frag.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/shadow/shadow-editor/assets/shaders/blackWhite.frag -o C:/dev/shadow/shadow-editor/assets/shaders/blackWhite.frag.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/shadow/shadow-editor/assets/shaders/particle.comp -o C:/dev/shadow/shadow-editor/assets/shaders/particle.comp.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/shadow/shadow-editor/assets/shaders/brushTool.comp -o C:/dev/shadow/shadow-editor/assets/shaders/brushTool.comp.spv
pause

