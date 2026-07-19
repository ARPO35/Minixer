/*
  ==============================================================================

    IpcTransport.h
    控制通道传输抽象接口。

  ==============================================================================
*/

#pragma once

#include "IpcProtocol.h"

namespace minixer
{

//==============================================================================
/** 控制通道传输抽象。

    主进程与子进程分别调用 connect / accept，之后即可通过 sendMessage /
    readMessage 收发完整的二进制帧（包含 ControlHeader + payload）。
*/
class IpcTransport
{
public:
    virtual ~IpcTransport() = default;

    /** 作为客户端连接到指定 key 的服务端。主进程使用。 */
    virtual bool connect (const juce::String& key) = 0;

    /** 作为服务端监听指定 key，等待一个客户端连接。子进程使用。 */
    virtual bool accept (const juce::String& key) = 0;

    /** 发送一帧数据。返回是否成功。 */
    virtual bool sendMessage (const juce::MemoryBlock& data) = 0;

    /** 读取一帧数据。timeoutMs <= 0 表示阻塞等待；> 0 表示最多等待指定毫秒。
        返回是否成功读取到完整一帧。
    */
    virtual bool readMessage (juce::MemoryBlock& data, int timeoutMs) = 0;

    /** 关闭连接。 */
    virtual void close() = 0;

    /** 返回当前是否处于已连接状态。 */
    virtual bool isConnected() const = 0;
};

//==============================================================================
/** 创建平台默认的 IpcTransport 实现。 */
std::unique_ptr<IpcTransport> createDefaultIpcTransport();

} // namespace minixer
