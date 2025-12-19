


---------------步骤 1：安装 vcpkg---------------------


克隆 vcpkg 仓库打开 PowerShell 或命令提示符，执行以下命令：
git clone https://github.com/microsoft/vcpkg
cd vcpkg

.\bootstrap-vcpkg.bat  # Windows下的脚本
执行完成后，vcpkg 目录下会生成vcpkg.exe。


（可选）添加 vcpkg 到环境变量为了方便在任意目录使用vcpkg命令，
将 vcpkg 安装目录（如C:\vcpkg）添加到系统环境变量Path。



---------------步骤 2：用 vcpkg 安装 gtest---------------------

gtest 是 Google 的 C++ 测试框架，通过 vcpkg 安装可自动处理依赖和编译。

vcpkg install gtest:x64-windows  # x64架构，Windows-MSVC版本
vcpkg install gtest:x64-windows-mingw  # x64架构，Windows-MinGW版本

集成 vcpkg 到项目

vcpkg integrate install

输出类似Applied user-wide integration for this vcpkg root.即成功。



---------------步骤 3：配置 VS Code---------------------
安装必要扩展并配置项目环境。
安装 VS Code 扩展
打开 VS Code，在扩展商店搜索并安装：
C/C++（Microsoft 官方，提供语法高亮和调试支持）
CMake Tools（用于构建 CMake 项目，可选但推荐）

---------------步骤 4：创建 gtest 项目---------------------





cl.exe /EHsc /std:c++17 /I "E:\vcpkg\installed\x64-windows\include" test.cpp /Fe:test.exe /link /LIBPATH:"E:\vcpkg\installed\x64-windows\lib" gtest.lib gtest_main.lib