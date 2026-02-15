@echo off
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/attachmentWrite.vert -o C:/dev/Shadow/Shadow/assets/shaders/attachmentWriteVert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/attachmentWrite.frag -o C:/dev/Shadow/Shadow/assets/shaders/attachmentWriteFrag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/attachmentRead.vert -o C:/dev/Shadow/Shadow/assets/shaders/attachmentReadVert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/attachmentRead.frag -o C:/dev/Shadow/Shadow/assets/shaders/attachmentReadFrag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/texture.vert -o C:/dev/Shadow/Shadow/assets/shaders/texture.vert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/texture.frag -o C:/dev/Shadow/Shadow/assets/shaders/texture.frag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/lighting.vert -o C:/dev/Shadow/Shadow/assets/shaders/lighting.vert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/lighting.frag -o C:/dev/Shadow/Shadow/assets/shaders/lighting.frag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/subpassInput.vert -o C:/dev/Shadow/Shadow/assets/shaders/subpassInput.vert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/subpassInput.frag -o C:/dev/Shadow/Shadow/assets/shaders/subpassInput.frag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/offscreen.vert -o C:/dev/Shadow/Shadow/assets/shaders/offscreen.vert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/offscreen.frag -o C:/dev/Shadow/Shadow/assets/shaders/offscreen.frag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/reversedColor.frag -o C:/dev/Shadow/Shadow/assets/shaders/reversedColor.frag.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/blackWhite.frag -o C:/dev/Shadow/Shadow/assets/shaders/blackWhite.frag.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/instanced2d.vert -o C:/dev/Shadow/Shadow/assets/shaders/instanced2d.vert.spv

C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/particle.vert -o C:/dev/Shadow/Shadow/assets/shaders/particle.vert.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/particle.frag -o C:/dev/Shadow/Shadow/assets/shaders/particle.frag.spv
C:/VulkanSDK/1.3.290.0/Bin/glslc.exe C:/dev/Shadow/Shadow/assets/shaders/particle.comp -o C:/dev/Shadow/Shadow/assets/shaders/particle.comp.spv
pause