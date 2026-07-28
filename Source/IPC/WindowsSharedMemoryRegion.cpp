/*
  ==============================================================================

    WindowsSharedMemoryRegion.cpp

  ==============================================================================
*/

#include "WindowsSharedMemoryRegion.h"

#if JUCE_WINDOWS

namespace minixer
{

namespace
{

juce::String makeMappingName (const juce::String& key)
{
    return "Local\\" + key + "_AudioShm";
}

} // anonymous namespace

//==============================================================================
WindowsSharedMemoryRegion::WindowsSharedMemoryRegion() = default;

WindowsSharedMemoryRegion::~WindowsSharedMemoryRegion()
{
    close();
}

//==============================================================================
bool WindowsSharedMemoryRegion::create (const juce::String& key, size_t size)
{
    return openInternal (key, size, FILE_MAP_ALL_ACCESS, true);
}

//==============================================================================
bool WindowsSharedMemoryRegion::open (const juce::String& key, size_t size)
{
    return openInternal (key, size, FILE_MAP_ALL_ACCESS, false);
}

//==============================================================================
void* WindowsSharedMemoryRegion::getAddress() const
{
    return address;
}

//==============================================================================
size_t WindowsSharedMemoryRegion::getSize() const
{
    return mappedSize;
}

//==============================================================================
void WindowsSharedMemoryRegion::close()
{
    if (address != nullptr)
    {
        UnmapViewOfFile (address);
        address = nullptr;
    }

    if (mapping != nullptr)
    {
        CloseHandle (mapping);
        mapping = nullptr;
    }

    mappedSize = 0;
}

//==============================================================================
bool WindowsSharedMemoryRegion::openInternal (const juce::String& key,
                                              size_t size,
                                              DWORD access,
                                              bool createNew)
{
    close();

    if (size == 0)
        return false;

    const auto nameStr = makeMappingName (key);
    const auto* name = nameStr.toWideCharPointer();
    const SIZE_T sz = static_cast<SIZE_T> (size);

    if (createNew)
    {
        mapping = CreateFileMappingW (INVALID_HANDLE_VALUE,
                                      nullptr,
                                      PAGE_READWRITE,
                                      0,
                                      static_cast<DWORD> (sz),
                                      name);
    }
    else
    {
        mapping = OpenFileMappingW (access, FALSE, name);
    }

    if (mapping == nullptr)
        return false;

    address = MapViewOfFile (mapping, access, 0, 0, sz);

    if (address == nullptr)
    {
        close();
        return false;
    }

    mappedSize = size;

    if (createNew)
        ZeroMemory (address, static_cast<SIZE_T> (size));

    return true;
}

//==============================================================================
std::unique_ptr<SharedMemoryRegion> createDefaultSharedMemoryRegion()
{
    return std::make_unique<WindowsSharedMemoryRegion>();
}

} // namespace minixer

#elif JUCE_LINUX

 #include "UnixSharedMemoryRegion.h"

namespace minixer
{
std::unique_ptr<SharedMemoryRegion> createDefaultSharedMemoryRegion()
{
    return std::make_unique<UnixSharedMemoryRegion>();
}
} // namespace minixer

#else

namespace minixer
{
std::unique_ptr<SharedMemoryRegion> createDefaultSharedMemoryRegion() { return {}; }
} // namespace minixer

#endif
