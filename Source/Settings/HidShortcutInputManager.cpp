#include "HidShortcutInputManager.h"

namespace minixer
{

#if JUCE_WINDOWS

#include <windows.h>

namespace
{
    // 使用一个简单的窗口子类化来拦截 WM_INPUT。
    static constexpr char* hiddenClassName = "MinixerHidShortcutInputWndClass";

    LRESULT CALLBACK hidInputWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
        {
            auto* manager = reinterpret_cast<HidShortcutInputManager*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));

            if (manager != nullptr)
                manager->handleRawInput (reinterpret_cast<void*> (lParam));

            return DefWindowProc (hWnd, message, wParam, lParam);
        }

        if (message == WM_CREATE)
        {
            auto* createStruct = reinterpret_cast<CREATESTRUCT*> (lParam);
            SetWindowLongPtr (hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (createStruct->lpCreateParams));
            return 0;
        }

        return DefWindowProc (hWnd, message, wParam, lParam);
    }
}

//==============================================================================
class HidShortcutInputManager::Impl
{
public:
    Impl() = default;
    ~Impl() { stop(); }

    bool start (HidShortcutInputManager* owner)
    {
        stop();

        WNDCLASSEX wcex{};
        wcex.cbSize        = sizeof (wcex);
        wcex.lpfnWndProc   = hidInputWndProc;
        wcex.hInstance     = GetModuleHandle (nullptr);
        wcex.lpszClassName = hiddenClassName;

        if (RegisterClassEx (&wcex) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        hwnd = CreateWindowEx (0, hiddenClassName, "Minixer HID Shortcut Input",
                               0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                               GetModuleHandle (nullptr), owner);

        if (hwnd == nullptr)
            return false;

        // 同时注册 Generic Desktop / Game Pad / Joystick / Keypad，以覆盖更多外设
        RAWINPUTDEVICE devices[] =
        {
            { 0x01, 0x04, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, hwnd }, // Joystick
            { 0x01, 0x05, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, hwnd }, // Game pad
            { 0x01, 0x07, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, hwnd }, // Keypad
        };

        if (! RegisterRawInputDevices (devices, static_cast<UINT> (std::size (devices)), sizeof (RAWINPUTDEVICE)))
        {
            stop();
            return false;
        }

        active = true;
        return true;
    }

    void stop()
    {
        active = false;

        if (hwnd != nullptr)
        {
            DestroyWindow (hwnd);
            hwnd = nullptr;
        }

        UnregisterClass (hiddenClassName, GetModuleHandle (nullptr));
    }

    HWND getHwnd() const noexcept { return hwnd; }
    bool isActive() const noexcept { return active; }

private:
    HWND hwnd = nullptr;
    bool active = false;
};

//==============================================================================
HidShortcutInputManager::HidShortcutInputManager()
    : impl (std::make_unique<Impl>())
{
}

HidShortcutInputManager::~HidShortcutInputManager() = default;

bool HidShortcutInputManager::isActive() const noexcept { return impl->isActive(); }

bool HidShortcutInputManager::start() { return impl->start (this); }

void HidShortcutInputManager::stop() { impl->stop(); }

void HidShortcutInputManager::handleRawInput (void* rawInputHandle)
{
    HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT> (rawInputHandle);
    UINT size = 0;

    if (GetRawInputData (hRawInput, RID_INPUT, nullptr, &size, sizeof (RAWINPUTHEADER)) != 0)
        return;

    std::vector<BYTE> buffer (size);

    if (GetRawInputData (hRawInput, RID_INPUT, buffer.data(), &size, sizeof (RAWINPUTHEADER)) == static_cast<UINT> (-1))
        return;

    auto* raw = reinterpret_cast<RAWINPUT*> (buffer.data());

    if (raw->header.dwType != RIM_TYPEHID && raw->header.dwType != RIM_TYPEKEYBOARD)
        return;

    // 获取设备信息
    RID_DEVICE_INFO devInfo{};
    UINT devInfoSize = sizeof (devInfo);
    GetRawInputDeviceInfo (raw->header.hDevice, RIDI_DEVICEINFO, &devInfo, &devInfoSize);

    uint16_t vendorId  = 0;
    uint16_t productId = 0;
    uint16_t usagePage = 0;
    uint16_t usage     = 0;

    if (raw->header.dwType != RIM_TYPEHID)
        return;

    vendorId  = static_cast<uint16_t> (devInfo.hid.dwVendorId);
    productId = static_cast<uint16_t> (devInfo.hid.dwProductId);
    usagePage = devInfo.hid.usUsagePage;
    usage     = devInfo.hid.usUsage;

    // 对 HID 原始数据做简单变化检测：将首次非零字节的位置+值作为 controlId。
    // 这不是完美的 HID 解析，但对大多数宏键盘/自定义按钮足够使用。
    if (raw->header.dwType == RIM_TYPEHID && raw->data.hid.dwSizeHid > 0 && raw->data.hid.dwCount > 0)
    {
        const auto* data = raw->data.hid.bRawData;
        const auto  dataSize = raw->data.hid.dwSizeHid;

        for (DWORD i = 0; i < raw->data.hid.dwCount; ++i)
        {
            const auto* report = data + (i * dataSize);

            for (DWORD byte = 0; byte < dataSize; ++byte)
            {
                auto value = report[byte];

                if (value == 0)
                    continue;

                HidShortcutEvent event;
                event.vendorId  = vendorId;
                event.productId = productId;
                event.usagePage = usagePage;
                event.usage     = usage;
                event.controlId = static_cast<uint32_t> ((byte << 16) | value);
                event.isPressed = true;

                juce::MessageManager::callAsync ([this, event]()
                {
                    listeners.call ([&event] (Listener& l)
                    {
                        l.hidShortcutEventReceived (event);
                    });
                });
            }
        }
    }
}

#else // 非 Windows 平台提供空实现

class HidShortcutInputManager::Impl
{
public:
    bool isActive() const noexcept { return false; }
    bool start (HidShortcutInputManager*) { return false; }
    void stop() {}
};

HidShortcutInputManager::HidShortcutInputManager()  : impl (std::make_unique<Impl>()) {}
HidShortcutInputManager::~HidShortcutInputManager() = default;
bool HidShortcutInputManager::isActive() const noexcept { return false; }
bool HidShortcutInputManager::start() { return false; }
void HidShortcutInputManager::stop() {}
void HidShortcutInputManager::handleRawInput (void*) {}

#endif

} // namespace minixer
