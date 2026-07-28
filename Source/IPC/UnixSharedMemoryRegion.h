/*
  ==============================================================================

    UnixSharedMemoryRegion.h
    Linux POSIX shm_open/mmap 实现的共享内存。

  ==============================================================================
*/

#pragma once

#include "SharedMemoryRegion.h"

#if JUCE_LINUX

namespace minixer
{

//==============================================================================
/** Linux POSIX 共享内存实现，语义逐点对齐 WindowsSharedMemoryRegion。

    选用 shm_open 而非 memfd_create：子进程需要按名字打开主进程创建的区域，
    对齐 Windows OpenFileMappingW 的名称语义；memfd 匿名无法满足。
*/
class UnixSharedMemoryRegion  : public SharedMemoryRegion
{
public:
    UnixSharedMemoryRegion();
    ~UnixSharedMemoryRegion() override;

    bool create (const juce::String& key, size_t size) override;
    bool open (const juce::String& key, size_t size) override;
    void* getAddress() const override;
    size_t getSize() const override;
    void close() override;

private:
    bool openInternal (const juce::String& key, size_t size, bool createNew);

    int shmFd = -1;
    void* address = nullptr;
    size_t mappedSize = 0;
    bool creator = false;        // 仅创建者在 close() 时 shm_unlink
    juce::String shmName;        // shm_open 名称（仅创建者需要）
};

} // namespace minixer

#endif
