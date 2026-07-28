/*
  ==============================================================================

    Main.cpp (PluginHost)
    PluginHost 子进程入口。

    命令行参数：
      --mode=scan|runtime
      --plugin-id=<uuid>
      --plugin-path=<absolute-path-to-vst3>
      --ipc-key=<unique-shared-memory-key>
      [--log-path=<path>]

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginHostServer.h"

#if JUCE_WINDOWS
 #include <windows.h>
 #include <dbghelp.h>
 #pragma comment(lib, "dbghelp.lib")
#elif JUCE_LINUX
 #include <cstdio>
 #include <execinfo.h>
 #include <fcntl.h>
 #include <signal.h>
 #include <unistd.h>
#endif

// 崩溃记录基础设施同时服务于 Windows（MiniDump）与 Linux（backtrace）两条路径，
// 因此匿名命名空间不再包裹在平台警卫内；平台专属部分在命名空间内部再分支。
namespace
{

juce::String g_crashDumpIpcKey;
juce::String g_crashDumpLogPath;

//==============================================================================
juce::File getCrashDumpDirectory()
{
    if (g_crashDumpLogPath.isNotEmpty())
    {
        auto dir = juce::File (g_crashDumpLogPath).getChildFile ("CrashDumps");
        dir.createDirectory();
        return dir;
    }

    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Minixer")
                   .getChildFile ("CrashDumps");

    dir.createDirectory();
    return dir;
}

//==============================================================================
#if JUCE_WINDOWS
LONG WINAPI writeMiniDumpOnUnhandledException (EXCEPTION_POINTERS* exceptionInfo)
{
    if (exceptionInfo == nullptr)
        return EXCEPTION_EXECUTE_HANDLER;

    auto dumpFile = getCrashDumpDirectory()
                        .getChildFile ("PluginHost_" + g_crashDumpIpcKey
                                       + "_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S")
                                       + ".dmp");

    auto dumpPathW = dumpFile.getFullPathName().toWideCharPointer();
    auto* file = CreateFileW (dumpPathW, GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION miniInfo;
        miniInfo.ThreadId          = GetCurrentThreadId();
        miniInfo.ExceptionPointers = exceptionInfo;
        miniInfo.ClientPointers    = FALSE;

        MiniDumpWriteDump (GetCurrentProcess(),
                           GetCurrentProcessId(),
                           file,
                           MiniDumpNormal,
                           &miniInfo,
                           nullptr,
                           nullptr);

        CloseHandle (file);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#elif JUCE_LINUX
/** Linux 崩溃信号处理器：把 backtrace 追加写入崩溃日志后立刻退出。

    与 Windows 的 MiniDump 对应，日志写到同一个 getCrashDumpDirectory() 下，
    文件名为 PluginHost_<ipcKey>_<pid>.crash.log。处理器内只用 async-signal-safe
    或近似安全的调用（open/write/backtrace/backtrace_symbols_fd/_exit），属于崩溃
    路径上的 best-effort 记录。
*/
void writeBacktraceOnCrashSignal (int signalNumber)
{
    const auto dumpFile = getCrashDumpDirectory()
                              .getChildFile ("PluginHost_" + g_crashDumpIpcKey
                                             + "_" + juce::String (static_cast<int> (::getpid()))
                                             + ".crash.log");

    const int fd = ::open (dumpFile.getFullPathName().toRawUTF8(),
                           O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd >= 0)
    {
        char header[160];
        const auto headerLength = ::snprintf (header, sizeof (header),
                                              "Minixer PluginHost crashed: signal %d, pid %d\n",
                                              signalNumber, static_cast<int> (::getpid()));

        if (headerLength > 0)
        {
            const auto bytesWritten = ::write (fd, header, static_cast<size_t> (headerLength));
            (void) bytesWritten;
        }

        void* frames[64];
        const int numFrames = ::backtrace (frames, 64);

        if (numFrames > 0)
            ::backtrace_symbols_fd (frames, numFrames, fd);

        ::close (fd);
    }

    ::_exit (134);
}

/** 安装 Linux 崩溃信号处理器（SIGSEGV/SIGABRT）。

    SA_RESETHAND 使处理器触发一次后即恢复默认行为：若处理器自身再次崩溃，
    进程仍按系统默认方式终止（可产生 core dump），不会死循环。
*/
void installCrashSignalHandlers()
{
    struct sigaction action = {};
    action.sa_handler = writeBacktraceOnCrashSignal;
    action.sa_flags   = SA_RESETHAND;
    ::sigemptyset (&action.sa_mask);

    ::sigaction (SIGSEGV, &action, nullptr);
    ::sigaction (SIGABRT, &action, nullptr);
}
#endif

} // anonymous namespace

//==============================================================================
static juce::String getCommandLineParameter (const juce::String& name,
                                              const juce::String& defaultValue = {})
{
    auto cmd = juce::JUCEApplicationBase::getCommandLineParameterArray();

    for (const auto& arg : cmd)
    {
        if (arg.startsWith ("--" + name + "="))
            return arg.substring (name.length() + 3);
    }

    return defaultValue;
}

//==============================================================================
class PluginHostApplication  : public juce::JUCEApplicationBase
{
public:
    PluginHostApplication() = default;

    const juce::String getApplicationName() override { return "Minixer PluginHost"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const juce::String&) override
    {
        auto mode       = getCommandLineParameter ("mode");
        auto pluginPath = getCommandLineParameter ("plugin-path");
        auto ipcKey     = getCommandLineParameter ("ipc-key");
        auto logPath    = getCommandLineParameter ("log-path");
        auto maxFramesStr = getCommandLineParameter ("max-frames", "4096");

       #if JUCE_WINDOWS
        g_crashDumpIpcKey  = ipcKey;
        g_crashDumpLogPath = logPath;
        SetUnhandledExceptionFilter (writeMiniDumpOnUnhandledException);
       #elif JUCE_LINUX
        g_crashDumpIpcKey  = ipcKey;
        g_crashDumpLogPath = logPath;
        installCrashSignalHandlers();
       #endif

        const uint32_t maxFrames = static_cast<uint32_t> (juce::jmax (1, maxFramesStr.getIntValue()));
        const uint32_t numInputs = 2;
        const uint32_t numOutputs = 2;

        if (logPath.isNotEmpty())
        {
            auto* fileLogger = juce::FileLogger::createDateStampedLogger (logPath,
                                                                           "PluginHost_" + ipcKey,
                                                                           ".log",
                                                                           "Minixer PluginHost started");
            juce::Logger::setCurrentLogger (fileLogger);
            logOwner.reset (fileLogger);
        }

        if (mode != "scan" && mode != "runtime")
        {
            juce::Logger::writeToLog ("Missing or invalid --mode");
            setApplicationReturnValue (1);
            quit();
            return;
        }

        if (pluginPath.isEmpty() || ipcKey.isEmpty())
        {
            juce::Logger::writeToLog ("Missing --plugin-path or --ipc-key");
            setApplicationReturnValue (1);
            quit();
            return;
        }

        minixer::PluginHostServer server;

        if (! server.connect (ipcKey, pluginPath, maxFrames, numInputs, numOutputs))
        {
            juce::Logger::writeToLog ("Failed to connect IPC");
            setApplicationReturnValue (1);
            quit();
            return;
        }

        int result = 0;

        if (mode == "scan")
            result = server.runScanMode();
        else
            result = server.runRuntimeMode();

        setApplicationReturnValue (result);
        quit();
    }

    void shutdown() override {}
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted (const juce::String&) override {}
    void suspended() override {}
    void resumed() override {}
    void unhandledException (const std::exception*, const juce::String&, int) override {}

private:
    std::unique_ptr<juce::Logger> logOwner;
};

//==============================================================================
START_JUCE_APPLICATION (PluginHostApplication)
