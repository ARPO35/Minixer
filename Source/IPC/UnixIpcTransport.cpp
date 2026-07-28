/*
  ==============================================================================

    UnixIpcTransport.cpp

  ==============================================================================
*/

#include "UnixIpcTransport.h"

#if JUCE_LINUX

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace minixer
{

namespace
{

juce::String makeSocketPath (const juce::String& key)
{
    // 按 uid 隔离路径，避免多用户环境下 /tmp 内文件名冲突。
    return "/tmp/minixer-" + juce::String ((unsigned int) ::getuid()) + "-" + key + ".sock";
}

/** 填充 AF_UNIX 地址；路径超过 sun_path 容量时返回 false。 */
bool fillSocketAddress (const juce::String& path, sockaddr_un& address)
{
    const auto pathUtf8 = path.toRawUTF8();

    if (std::strlen (pathUtf8) >= sizeof (address.sun_path))
        return false;

    address = {};
    address.sun_family = AF_UNIX;
    std::strcpy (address.sun_path, pathUtf8);
    return true;
}

} // anonymous namespace

//==============================================================================
UnixIpcTransport::UnixIpcTransport() = default;

UnixIpcTransport::~UnixIpcTransport()
{
    close();
}

//==============================================================================
bool UnixIpcTransport::connect (const juce::String& key)
{
    close();

    const auto path = makeSocketPath (key);

    sockaddr_un address;
    if (! fillSocketAddress (path, address))
    {
        DBG ("UnixIpcTransport: socket path too long: " << path);
        return false;
    }

    // 服务端可能稍后才创建套接字并开始监听，重试一段时间
    //（对齐 Windows 版 50 次尝试的重试结构）。
    for (int attempt = 0; attempt < 50; ++attempt)
    {
        socketFd = ::socket (AF_UNIX, SOCK_STREAM, 0);

        if (socketFd < 0)
        {
            const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
            DBG ("UnixIpcTransport: socket() failed: " << ::strerror (error));
            return false;
        }

        if (::connect (socketFd, reinterpret_cast<sockaddr*> (&address), sizeof (address)) == 0)
        {
            connected = true;
            return true;
        }

        const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
        ::close (socketFd);
        socketFd = -1;

        // 套接字文件尚未出现 / 服务端尚未 listen：对齐 Windows 版
        // “服务端未就绪”路径的等待语义，间隔后重试。
        if (error == ECONNREFUSED || error == ENOENT)
        {
            juce::Thread::sleep (50);
            continue;
        }

        DBG ("UnixIpcTransport: connect() failed: " << ::strerror (error));
        return false;
    }

    return false;
}

//==============================================================================
bool UnixIpcTransport::accept (const juce::String& key)
{
    close();

    const auto path = makeSocketPath (key);

    sockaddr_un address;
    if (! fillSocketAddress (path, address))
    {
        DBG ("UnixIpcTransport: socket path too long: " << path);
        return false;
    }

    const auto listenFd = ::socket (AF_UNIX, SOCK_STREAM, 0);

    if (listenFd < 0)
    {
        const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
        DBG ("UnixIpcTransport: socket() failed: " << ::strerror (error));
        return false;
    }

    // 上次异常退出可能残留同名套接字文件，bind 前先删除。
    ::unlink (address.sun_path);

    if (::bind (listenFd, reinterpret_cast<sockaddr*> (&address), sizeof (address)) != 0)
    {
        const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
        DBG ("UnixIpcTransport: bind() failed: " << ::strerror (error));
        ::close (listenFd);
        return false;
    }

    if (::listen (listenFd, 1) != 0)
    {
        const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
        DBG ("UnixIpcTransport: listen() failed: " << ::strerror (error));
        ::unlink (address.sun_path);
        ::close (listenFd);
        return false;
    }

    // 阻塞等待唯一客户端，对齐 Windows ConnectNamedPipe 的阻塞语义。
    int connectionFd;
    do
    {
        connectionFd = ::accept (listenFd, nullptr, nullptr);
    }
    while (connectionFd < 0 && errno == EINTR);

    if (connectionFd < 0)
    {
        const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
        DBG ("UnixIpcTransport: accept() failed: " << ::strerror (error));
        ::unlink (address.sun_path);
        ::close (listenFd);
        return false;
    }

    ::close (listenFd);

    socketFd = connectionFd;
    socketPath = path;
    serverSide = true;
    connected = true;
    return true;
}

//==============================================================================
bool UnixIpcTransport::sendMessage (const juce::MemoryBlock& data)
{
    if (! isConnected())
        return false;

    // 与 Windows 版一致的线格式：4 字节长度前缀 + 负载
    //（两端均为小端 x86，直接按内存表示写出）。
    const uint32_t totalSize = static_cast<uint32_t> (juce::jmin<size_t> (data.getSize(), (std::numeric_limits<uint32_t>::max)()));

    if (! sendExact (&totalSize, sizeof (totalSize)))
        return false;

    if (totalSize > 0)
        return sendExact (data.getData(), totalSize);

    return true;
}

//==============================================================================
bool UnixIpcTransport::readMessage (juce::MemoryBlock& data, int timeoutMs)
{
    if (! isConnected())
        return false;

    uint32_t totalSize = 0;
    if (! readExact (&totalSize, sizeof (totalSize), timeoutMs))
        return false;

    if (totalSize == 0)
    {
        data.reset();
        return true;
    }

    if (totalSize > 64 * 1024 * 1024) // 64MB 上限，防止畸形消息
        return false;

    data.setSize (totalSize, false);
    return readExact (data.getData(), totalSize, timeoutMs);
}

//==============================================================================
void UnixIpcTransport::close()
{
    if (socketFd >= 0)
    {
        ::close (socketFd);
        socketFd = -1;
    }

    if (serverSide && socketPath.isNotEmpty())
        ::unlink (socketPath.toRawUTF8());

    socketPath = {};
    serverSide = false;
    connected = false;
}

//==============================================================================
bool UnixIpcTransport::isConnected() const
{
    return connected && socketFd >= 0;
}

//==============================================================================
bool UnixIpcTransport::readExact (void* buffer, size_t bytesToRead, int timeoutMs)
{
    auto* dest = static_cast<uint8_t*> (buffer);
    size_t totalRead = 0;
    const auto deadline = juce::Time::getCurrentTime()
                        + juce::RelativeTime::milliseconds (juce::jmax (0, timeoutMs));

    while (totalRead < bytesToRead)
    {
        if (timeoutMs > 0)
        {
            // 对齐 Windows 版 PeekNamedPipe 的“数据到齐前等待、超时返回”语义：
            // 用 poll 等待可读，整体截止时间为调用时刻 + timeoutMs。
            pollfd pfd { socketFd, POLLIN, 0 };

            int pollResult;
            do
            {
                // EINTR 重试时重算剩余时间，保证整体不越过 deadline。
                const auto remainingMs = static_cast<int> (juce::jmax (juce::int64 (0),
                                             (deadline - juce::Time::getCurrentTime()).inMilliseconds()));
                pfd.revents = 0;
                pollResult = ::poll (&pfd, 1, remainingMs);
            }
            while (pollResult < 0 && errno == EINTR);

            if (pollResult < 0)
            {
                const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
                DBG ("UnixIpcTransport: poll() failed: " << ::strerror (error));
                return false;
            }

            if (pollResult == 0 || (pfd.revents & POLLNVAL) != 0)
                return false; // 超时或套接字已失效
        }

        const auto received = ::recv (socketFd, dest + totalRead, bytesToRead - totalRead, 0);

        if (received < 0)
        {
            if (errno == EINTR)
                continue;

            const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
            DBG ("UnixIpcTransport: recv() failed: " << ::strerror (error));
            return false;
        }

        if (received == 0)
        {
            // 对端有序关闭连接。
            connected = false;
            return false;
        }

        totalRead += static_cast<size_t> (received);
    }

    return true;
}

//==============================================================================
bool UnixIpcTransport::sendExact (const void* buffer, size_t bytesToSend)
{
    const auto* src = static_cast<const uint8_t*> (buffer);
    size_t totalSent = 0;

    while (totalSent < bytesToSend)
    {
        // MSG_NOSIGNAL：对端关闭时产生 EPIPE 而不是 SIGPIPE，避免进程被杀。
        const auto sent = ::send (socketFd, src + totalSent, bytesToSend - totalSent, MSG_NOSIGNAL);

        if (sent < 0)
        {
            if (errno == EINTR)
                continue;

            const auto error [[maybe_unused]] = errno; // Release 下 DBG 为空，避免 unused 警告
            DBG ("UnixIpcTransport: send() failed: " << ::strerror (error));
            return false;
        }

        if (sent == 0)
            return false;

        totalSent += static_cast<size_t> (sent);
    }

    return true;
}

} // namespace minixer

#endif
