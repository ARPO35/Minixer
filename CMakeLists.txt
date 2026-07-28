cmake_minimum_required(VERSION 3.22)

project(Minixer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 仅构建 32 位 PluginHost32（Linux i686 交叉编译用）
option(MINIXER_BUILD_32BIT "仅构建 32 位 PluginHost32" OFF)

#===============================================================================
# JUCE 引入：优先使用调用方提供的本机 JUCE 路径，否则使用 deps/JUCE 子模块
if(DEFINED MINIXER_JUCE_PATH)
    get_filename_component(MINIXER_JUCE_PATH_ABS "${MINIXER_JUCE_PATH}" ABSOLUTE
                           BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    add_subdirectory("${MINIXER_JUCE_PATH_ABS}" juce-build EXCLUDE_FROM_ALL)
else()
    # EXCLUDE_FROM_ALL：不带入 JUCE 自身的 install 规则（其头文件不应进包）
    add_subdirectory(deps/JUCE EXCLUDE_FROM_ALL)
endif()

# VST2 宿主：社区净室头文件（deps/vst2-headers，须含 pluginterfaces/vst2.x/{aeffect.h,aeffectx.h}）
juce_set_vst2_sdk_path("${CMAKE_CURRENT_SOURCE_DIR}/deps/vst2-headers")

#===============================================================================
# 共用设置

set(MINIXER_JUCE_MODULES
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_cryptography
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra)

set(MINIXER_HOST_COMPILE_DEFS
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=1
    JUCE_WEB_BROWSER=0
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_USE_CURL=0 # 全应用无网络调用；同时规避 i386 curl dev 的 multiarch 冲突
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

# PluginHost 子进程源文件（主程序与 PluginHost 共用 IPC 与架构检测代码）
set(MINIXER_PLUGINHOST_SOURCES
    Source/PluginHost/Main.cpp
    Source/PluginHost/PluginHostServer.cpp
    Source/PluginHost/PluginWrapper.cpp
    Source/IPC/UnixIpcTransport.cpp
    Source/IPC/UnixSharedMemoryRegion.cpp
    Source/IPC/WindowsIpcTransport.cpp
    Source/IPC/WindowsSharedMemoryRegion.cpp
    Source/Plugin/PluginArchitecture.cpp)

# Windows 下 32 位 configure（-A Win32）时子进程目标名为 PluginHost32.exe
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(MINIXER_HOST_TARGET_NAME PluginHost32)
elseif(MINIXER_BUILD_32BIT)
    set(MINIXER_HOST_TARGET_NAME PluginHost32)
else()
    set(MINIXER_HOST_TARGET_NAME PluginHost64)
endif()

#===============================================================================
if(NOT MINIXER_BUILD_32BIT)

# 主程序
    juce_add_gui_app(Minixer
    PRODUCT_NAME "Minixer"
    COMPANY_NAME ""
    VERSION 1.0.0
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/minixer_06_warm_orange.png")

target_sources(Minixer PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/Components/ChannelStripComponent.cpp
    Source/Components/FaderComponent.cpp
    Source/Components/KeyCaptureButton.cpp
    Source/Components/LevelMeterComponent.cpp
    Source/Components/LevelMeterProcessor.cpp
    Source/Components/PluginSlotComponent.cpp
    Source/Components/PresetBarComponent.cpp
    Source/Components/RotaryKnobComponent.cpp
    Source/Components/SettingsComponent.cpp
    Source/Components/ShortcutsSettingsComponent.cpp
    Source/IPC/UnixIpcTransport.cpp
    Source/IPC/UnixSharedMemoryRegion.cpp
    Source/IPC/WindowsIpcTransport.cpp
    Source/IPC/WindowsSharedMemoryRegion.cpp
    Source/LookAndFeel/MixerLookAndFeel.cpp
    Source/Plugin/PluginArchitecture.cpp
    Source/Plugin/PluginBlacklist.cpp
    Source/Plugin/PluginBridgeNode.cpp
    Source/Plugin/PluginEditorWindow.cpp
    Source/Plugin/PluginHostClient.cpp
    Source/Plugin/PluginHostLauncher.cpp
    Source/Plugin/PluginManagerComponent.cpp
    Source/Plugin/PluginRegistry.cpp
    Source/Settings/AppSettings.cpp
    Source/Settings/HidShortcutInputManager.cpp
    Source/Settings/KeyboardShortcut.cpp
    Source/Settings/MidiShortcutInputManager.cpp
    Source/Settings/ShortcutInputSource.cpp)

target_include_directories(Minixer PRIVATE JuceLibraryCode)
target_compile_definitions(Minixer PRIVATE ${MINIXER_HOST_COMPILE_DEFS})
target_link_libraries(Minixer PRIVATE
    ${MINIXER_JUCE_MODULES}
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

endif() # NOT MINIXER_BUILD_32BIT

#===============================================================================
# PluginHost 沙盒子进程（64 位或 32 位，取决于 configure）

juce_add_gui_app(${MINIXER_HOST_TARGET_NAME}
    PRODUCT_NAME "${MINIXER_HOST_TARGET_NAME}"
    COMPANY_NAME ""
    VERSION 1.0.0
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE)

target_sources(${MINIXER_HOST_TARGET_NAME} PRIVATE ${MINIXER_PLUGINHOST_SOURCES})
target_include_directories(${MINIXER_HOST_TARGET_NAME} PRIVATE JuceLibraryCode)
target_compile_definitions(${MINIXER_HOST_TARGET_NAME} PRIVATE ${MINIXER_HOST_COMPILE_DEFS})
target_link_libraries(${MINIXER_HOST_TARGET_NAME} PRIVATE
    ${MINIXER_JUCE_MODULES}
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

#===============================================================================
# 安装与打包（产物布局：lib/minixer/{Minixer,PluginHost64,PluginHost32,assets/}）

if(NOT MINIXER_BUILD_32BIT)
    install(TARGETS Minixer ${MINIXER_HOST_TARGET_NAME} RUNTIME DESTINATION lib/minixer)
    install(DIRECTORY assets/ DESTINATION lib/minixer/assets)
    install(FILES packaging/Minixer.desktop DESTINATION share/applications)
    install(FILES assets/minixer_06_warm_orange.png
            DESTINATION share/pixmaps RENAME minixer.png)
    install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E make_directory \
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin\")")
    install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink \
        ../lib/minixer/Minixer \
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/minixer\")")
else()
    install(TARGETS ${MINIXER_HOST_TARGET_NAME} RUNTIME DESTINATION lib/minixer)
endif()

set(CPACK_PACKAGE_NAME minixer)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Minixer")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Standalone VST mixer for microphone processing")
set(CPACK_GENERATOR "TGZ;RPM")
set(CPACK_RPM_PACKAGE_LICENSE "AGPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_REQUIRES "alsa-lib, freetype, fontconfig, libX11, libXext, libXinerama, libXrandr, libXcursor, libXcomposite, libXi, libXrender")
set(CPACK_PACKAGE_CONTACT "")
include(CPack)
