#pragma once

#include <JuceHeader.h>

namespace minixer
{

//==============================================================================
/** 自定义 HID 按钮/控制事件描述。 */
struct HidShortcutEvent
{
    uint16_t vendorId  = 0;     /**< USB Vendor ID。 */
    uint16_t productId = 0;     /**< USB Product ID。 */
    uint16_t usagePage = 0;     /**< HID Usage Page。 */
    uint16_t usage     = 0;     /**< HID Usage。 */
    uint32_t controlId = 0;     /**< 控制/按钮标识，由 Raw Input 原始数据推导。 */
    bool     isPressed = false; /**< 当前是否按下/激活。 */

    bool operator== (const HidShortcutEvent& other) const noexcept
    {
        return vendorId  == other.vendorId
            && productId == other.productId
            && usagePage == other.usagePage
            && usage     == other.usage
            && controlId == other.controlId;
    }
};

//==============================================================================
/** 监听自定义 HID 设备（通过 Windows Raw Input）并将事件转发给快捷键系统。

    当前仅在 Windows 平台启用；其他平台提供空实现以保持接口一致。
*/
class HidShortcutInputManager
{
public:
    //==============================================================================
    HidShortcutInputManager();
    ~HidShortcutInputManager();

    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** 收到 HID 控制事件（按钮按下/释放）。 */
        virtual void hidShortcutEventReceived (const HidShortcutEvent& event) = 0;
    };

    void addListener (Listener* listener)    { listeners.add (listener); }
    void removeListener (Listener* listener) { listeners.remove (listener); }

    //==============================================================================
    /** 是否成功注册 Raw Input。 */
    bool isActive() const noexcept;

    /** 启动 Raw Input 监听。 */
    bool start();

    /** 停止 Raw Input 监听。 */
    void stop();

    //==============================================================================
    /** 供窗口过程调用的内部处理函数；外部通常不需要直接调用。 */
    void handleRawInput (void* rawInputHandle);

private:
    //==============================================================================
    class Impl;
    std::unique_ptr<Impl> impl;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HidShortcutInputManager)
};

} // namespace minixer
