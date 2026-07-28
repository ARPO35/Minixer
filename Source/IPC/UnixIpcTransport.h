/*
  ==============================================================================

    UnixIpcTransport.h
    Linux AF_UNIX 套接字实现的控制通道。

  ==============================================================================
*/

#pragma once

#include "IpcTransport.h"

#if JUCE_LINUX

namespace minixer
{

//==============================================================================
/** Linux AF_UNIX SOCK_STREAM 控制通道实现，语义逐点对齐 WindowsIpcTransport。 */
class UnixIpcTransport  : public IpcTransport
{
public:
    UnixIpcTransport();
    ~UnixIpcTransport() override;

    bool connect (const juce::String& key) override;
    bool accept (const juce::String& key) override;
    bool sendMessage (const juce::MemoryBlock& data) override;
    bool readMessage (juce::MemoryBlock& data, int timeoutMs) override;
    void close() override;
    bool isConnected() const override;

private:
    bool readExact (void* buffer, size_t bytesToRead, int timeoutMs);
    bool sendExact (const void* buffer, size_t bytesToSend);

    int socketFd = -1;           // 已建立连接的套接字
    bool connected = false;
    bool serverSide = false;     // accept() 的服务端在 close() 时额外 unlink 路径
    juce::String socketPath;     // 服务端套接字文件路径（仅 serverSide 有效）
};

} // namespace minixer

#endif
