# Linux i686（32 位）交叉编译工具链，用于在 x86_64 主机上构建 PluginHost32。
# 依赖：gcc-multilib g++-multilib 及各依赖库的 :i386 变体（见 CI workflow）。
# 若 pkg-config 找不到 i386 模块，回退：
#   PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig cmake ...

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_FLAGS_INIT "-m32")
set(CMAKE_CXX_FLAGS_INIT "-m32")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-m32")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-m32")

# 优先使用 i686 前缀的 pkg-config（部分发行版提供）；否则靠 PKG_CONFIG_PATH 回退
find_program(CMAKE_PKG_CONFIG NAMES i686-linux-gnu-pkg-config pkg-config)
set(PKG_CONFIG_EXECUTABLE "${CMAKE_PKG_CONFIG}" CACHE FILEPATH "")
