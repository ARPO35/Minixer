/*
  ==============================================================================

    WindowsSharedMemoryRegion.h
    Windows 文件映射对象实现的共享内存。

  ==============================================================================
*/

#pragma once

#include "SharedMemoryRegion.h"

#if JUCE_WINDOWS
 #include <windows.h>

namespace minixer
{

//==============================================================================
class WindowsSharedMemoryRegion  : public SharedMemoryRegion
{
public:
    WindowsSharedMemoryRegion();
    ~WindowsSharedMemoryRegion() override;

    bool create (const juce::String& key, size_t size) override;
    bool open (const juce::String& key, size_t size) override;
    void* getAddress() const override;
    size_t getSize() const override;
    void close() override;

private:
    bool openInternal (const juce::String& key, size_t size, DWORD access, bool createNew);

    HANDLE mapping = nullptr;
    void* address = nullptr;
    size_t mappedSize = 0;
};

} // namespace minixer

#endif
