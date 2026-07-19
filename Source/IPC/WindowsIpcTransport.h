/*
  ==============================================================================

    WindowsIpcTransport.h
    Windows 命名管道实现的控制通道。

  ==============================================================================
*/

#pragma once

#include "IpcTransport.h"

#if JUCE_WINDOWS
 #include <windows.h>

namespace minixer
{

//==============================================================================
/** Windows Named Pipe 控制通道实现。 */
class WindowsIpcTransport  : public IpcTransport
{
public:
    WindowsIpcTransport();
    ~WindowsIpcTransport() override;

    bool connect (const juce::String& key) override;
    bool accept (const juce::String& key) override;
    bool sendMessage (const juce::MemoryBlock& data) override;
    bool readMessage (juce::MemoryBlock& data, int timeoutMs) override;
    void close() override;
    bool isConnected() const override;

private:
    bool readExact (void* buffer, DWORD bytesToRead, int timeoutMs);
    bool sendExact (const void* buffer, DWORD bytesToSend);

    HANDLE pipe = INVALID_HANDLE_VALUE;
    bool connected = false;
};

} // namespace minixer

#endif
