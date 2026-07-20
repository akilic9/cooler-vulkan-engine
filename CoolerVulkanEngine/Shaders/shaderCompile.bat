%VK_SDK_PATH%\Bin\dxc.exe -spirv -T vs_6_0 -E main .\triangle.vert -Fo .\triangle.vert.spv
%VK_SDK_PATH%\Bin\dxc.exe -spirv -T ps_6_0 -E main .\triangle.frag -Fo .\triangle.frag.spv

pause