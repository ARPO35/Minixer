#pragma once

#include <JuceHeader.h>

namespace minixer
{

//==============================================================================
/** 快捷键可绑定的输入来源类型。 */
enum class ShortcutInputType
{
    none,       /**< 未绑定。 */
    keyPress,   /**< 标准键盘按键（JUCE KeyPress）。 */
    midiNote,   /**< MIDI Note On 消息。 */
    midiCc,     /**< MIDI Control Change 消息。 */
    hidButton   /**< 自定义 HID 按钮/控制（通过 Raw Input 等原生接口）。 */
};

//==============================================================================
/** 单个快捷键输入源。

    统一封装键盘、MIDI、HID 三类来源，使 SlotShortcut 不依赖具体输入类型。
*/
struct ShortcutInputSource
{
    //==============================================================================
    ShortcutInputType type = ShortcutInputType::none;

    //==============================================================================
    /** 键盘来源数据。仅当 type == keyPress 时有效。 */
    juce::KeyPress keyPress;

    //==============================================================================
    /** MIDI 来源数据。仅当 type 为 midiNote 或 midiCc 时有效。

        midiChannel:  1-16，0 表示“任意通道”。
        midiNumber:   Note 编号（0-127）或 CC 编号（0-127）。
        midiValue:    触发阈值；0 表示“任意值/力度”，非 0 表示要求 >= 该值。
    */
    int midiChannel = 0;
    int midiNumber  = 0;
    int midiValue   = 0;

    //==============================================================================
    /** HID 来源数据。仅当 type == hidButton 时有效。

        hidVendorId / hidProductId: USB VID/PID，0 表示“任意设备”。
        hidUsagePage / hidUsage:    HID 使用页与使用 ID，0 表示“任意”。
        hidButtonId:                设备上的控制编号/按钮索引。
    */
    uint16_t hidVendorId  = 0;
    uint16_t hidProductId = 0;
    uint16_t hidUsagePage = 0;
    uint16_t hidUsage     = 0;
    uint32_t hidButtonId  = 0;

    //==============================================================================
    ShortcutInputSource() = default;
    explicit ShortcutInputSource (const juce::KeyPress& key);

    static ShortcutInputSource midiNote (int channel, int noteNumber, int velocityThreshold = 0);
    static ShortcutInputSource midiCc   (int channel, int controllerNumber, int valueThreshold = 0);
    static ShortcutInputSource hidButton (uint16_t vendorId, uint16_t productId,
                                          uint16_t usagePage, uint16_t usage,
                                          uint32_t buttonId);

    //==============================================================================
    bool isValid() const noexcept;
    bool isKeyboard() const noexcept { return type == ShortcutInputType::keyPress; }
    bool isMidi() const noexcept     { return type == ShortcutInputType::midiNote || type == ShortcutInputType::midiCc; }
    bool isHid() const noexcept      { return type == ShortcutInputType::hidButton; }

    /** 返回键盘按键；若不是键盘来源则返回无效 KeyPress。 */
    juce::KeyPress getKeyPress() const noexcept { return isKeyboard() ? keyPress : juce::KeyPress(); }

    /** 判断两个输入源是否表示同一触发条件。 */
    bool matches (const ShortcutInputSource& other) const noexcept;

    /** 判断当前源是否能被指定 MIDI 消息触发。 */
    bool matchesMidiMessage (const juce::MidiMessage& message) const noexcept;

    /** 返回用于界面显示的文本描述。 */
    juce::String getDisplayString() const;

    //==============================================================================
    /** 序列化为 XML 属性。 */
    void writeToXml (juce::XmlElement& xml) const;

    /** 从 XML 属性反序列化；失败时保持当前值。 */
    bool readFromXml (const juce::XmlElement& xml);

    /** 读取旧版“keyCode / modifiers / textCharacter”字段作为键盘来源。 */
    bool readLegacyKeyboardFromXml (const juce::XmlElement& xml);

    //==============================================================================
    bool operator== (const ShortcutInputSource& other) const noexcept { return matches (other); }
    bool operator!= (const ShortcutInputSource& other) const noexcept { return ! matches (other); }

private:
    //==============================================================================
    static juce::String typeToString (ShortcutInputType t);
    static ShortcutInputType stringToType (const juce::String& text);
};

} // namespace minixer
