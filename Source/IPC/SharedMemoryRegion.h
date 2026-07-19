/*
  ==============================================================================

    SharedMemoryRegion.h
    跨平台共享内存区域抽象。

  ==============================================================================
*/

#pragma once

#include "IpcProtocol.h"

namespace minixer
{

//==============================================================================
/** 跨进程可读写的共享内存区域。

    主进程调用 create() 创建并映射；子进程调用 open() 打开同一 key 并映射。
    析构时自动解除映射并关闭句柄。
*/
class SharedMemoryRegion
{
public:
    virtual ~SharedMemoryRegion() = default;

    /** 主进程创建指定大小的共享内存对象。 */
    virtual bool create (const juce::String& key, size_t size) = 0;

    /** 子进程打开已存在的共享内存对象。 */
    virtual bool open (const juce::String& key, size_t size) = 0;

    /** 返回映射后的基地址。 */
    virtual void* getAddress() const = 0;

    /** 返回映射大小。 */
    virtual size_t getSize() const = 0;

    /** 显式关闭映射。 */
    virtual void close() = 0;
};

//==============================================================================
/** 创建平台默认的 SharedMemoryRegion 实现。 */
std::unique_ptr<SharedMemoryRegion> createDefaultSharedMemoryRegion();

} // namespace minixer
