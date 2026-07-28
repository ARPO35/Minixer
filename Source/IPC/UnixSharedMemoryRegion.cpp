/*
  ==============================================================================

    UnixSharedMemoryRegion.cpp

  ==============================================================================
*/

#include "UnixSharedMemoryRegion.h"

#if JUCE_LINUX

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace minixer
{

namespace
{

juce::String makeShmName (const juce::String& key)
{
    // POSIX shm 名必须以 '/' 开头且不含后续 '/'；按 uid 隔离避免多用户冲突。
    return "/minixer-" + juce::String ((unsigned int) ::getuid()) + "-" + key;
}

} // anonymous namespace

//==============================================================================
UnixSharedMemoryRegion::UnixSharedMemoryRegion() = default;

UnixSharedMemoryRegion::~UnixSharedMemoryRegion()
{
    close();
}

//==============================================================================
bool UnixSharedMemoryRegion::create (const juce::String& key, size_t size)
{
    return openInternal (key, size, true);
}

//==============================================================================
bool UnixSharedMemoryRegion::open (const juce::String& key, size_t size)
{
    return openInternal (key, size, false);
}

//==============================================================================
void* UnixSharedMemoryRegion::getAddress() const
{
    return address;
}

//==============================================================================
size_t UnixSharedMemoryRegion::getSize() const
{
    return mappedSize;
}

//==============================================================================
void UnixSharedMemoryRegion::close()
{
    if (address != nullptr)
    {
        ::munmap (address, mappedSize);
        address = nullptr;
    }

    if (shmFd >= 0)
    {
        ::close (shmFd);
        shmFd = -1;
    }

    // POSIX shm 对象在内核中持续存在，仅创建者负责删除；
    // 子进程的映射不受 unlink 影响（对象存续到最后一个映射解除），
    // 对齐 Windows 文件映射随最后句柄关闭而销毁的语义。
    if (creator && shmName.isNotEmpty())
        ::shm_unlink (shmName.toRawUTF8());

    shmName = {};
    creator = false;
    mappedSize = 0;
}

//==============================================================================
bool UnixSharedMemoryRegion::openInternal (const juce::String& key,
                                           size_t size,
                                           bool createNew)
{
    close();

    if (size == 0)
        return false;

    const auto name = makeShmName (key);
    const auto nameUtf8 = name.toRawUTF8();

    int fd = -1;

    if (createNew)
    {
        fd = ::shm_open (nameUtf8, O_RDWR | O_CREAT | O_EXCL, 0600);

        // 上次异常退出可能残留同名对象：先删除再以创建者身份重建，
        // 对齐 Windows CreateFileMappingW 的“创建者”语义。
        if (fd < 0 && errno == EEXIST)
        {
            ::shm_unlink (nameUtf8);
            fd = ::shm_open (nameUtf8, O_RDWR | O_CREAT | O_EXCL, 0600);
        }

        if (fd < 0)
        {
            const auto error = errno;
            DBG ("UnixSharedMemoryRegion: shm_open() failed: " << ::strerror (error));
            return false;
        }

        if (::ftruncate (fd, static_cast<off_t> (size)) != 0)
        {
            const auto error = errno;
            DBG ("UnixSharedMemoryRegion: ftruncate() failed: " << ::strerror (error));
            ::close (fd);
            ::shm_unlink (nameUtf8);
            return false;
        }
    }
    else
    {
        fd = ::shm_open (nameUtf8, O_RDWR, 0600);

        if (fd < 0)
        {
            const auto error = errno;
            DBG ("UnixSharedMemoryRegion: shm_open() failed: " << ::strerror (error));
            return false;
        }
    }

    auto* mapped = ::mmap (nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (mapped == MAP_FAILED)
    {
        const auto error = errno;
        DBG ("UnixSharedMemoryRegion: mmap() failed: " << ::strerror (error));
        ::close (fd);
        if (createNew)
            ::shm_unlink (nameUtf8);
        return false;
    }

    // 新建 / ftruncate 扩展的 POSIX shm 对象由内核保证零填充，
    // 等价于 Windows 版创建路径的 ZeroMemory，无需显式清零。
    shmFd = fd;
    address = mapped;
    mappedSize = size;
    creator = createNew;
    shmName = name;
    return true;
}

} // namespace minixer

#endif
