#include "KeyboardShortcut.h"

namespace minixer
{

namespace
{
    static constexpr int defaultRawFlags = 0;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> SlotShortcut::toXml (const juce::String& tagName) const
{
    auto xml = std::make_unique<juce::XmlElement> (tagName);

    inputSource.writeToXml (*xml);
    xml->setAttribute ("defaultBypassed", defaultBypassed);
    xml->setAttribute ("mode",            ShortcutSettings::slotShortcutModeToString (mode));

    return xml;
}

//==============================================================================
bool SlotShortcut::fromXml (const juce::XmlElement& xml)
{
    defaultBypassed = xml.getBoolAttribute ("defaultBypassed", false);
    mode            = ShortcutSettings::stringToSlotShortcutMode (xml.getStringAttribute ("mode", "cycle"));

    // 优先读取新版 inputType 字段
    if (xml.hasAttribute ("inputType"))
        return inputSource.readFromXml (xml);

    // 兼容旧版：仅有 keyCode / modifiers / textCharacter
    if (xml.hasAttribute ("keyCode"))
        return inputSource.readLegacyKeyboardFromXml (xml);

    return false;
}

//==============================================================================
ShortcutSettings::ShortcutSettings()
{
    resetToDefaults();
}

//==============================================================================
void ShortcutSettings::resetToDefaults()
{
    globalShortcuts[static_cast<size_t> (GlobalShortcutAction::bringWindowToFront)]    = juce::KeyPress ('m', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0);
    globalShortcuts[static_cast<size_t> (GlobalShortcutAction::toggleAllPluginsBypass)] = juce::KeyPress ('b', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0);
    globalShortcuts[static_cast<size_t> (GlobalShortcutAction::toggleSettingsPanel)]   = juce::KeyPress ('d', juce::ModifierKeys::ctrlModifier, 0);
    globalShortcuts[static_cast<size_t> (GlobalShortcutAction::deleteFocusedSlot)]     = juce::KeyPress (juce::KeyPress::deleteKey);

    for (auto& slot : slotShortcuts)
        slot.clear();
}

//==============================================================================
juce::KeyPress ShortcutSettings::getGlobalShortcut (GlobalShortcutAction action) const
{
    auto index = static_cast<size_t> (action);

    if (index < globalShortcuts.size())
        return globalShortcuts[index];

    return {};
}

//==============================================================================
void ShortcutSettings::setGlobalShortcut (GlobalShortcutAction action, const juce::KeyPress& key)
{
    auto index = static_cast<size_t> (action);

    if (index < globalShortcuts.size())
        globalShortcuts[index] = key;
}

//==============================================================================
const SlotShortcut& ShortcutSettings::getSlotShortcut (int slotIndex) const
{
    jassert (juce::isPositiveAndBelow (slotIndex, static_cast<int> (slotShortcuts.size())));
    return slotShortcuts[static_cast<size_t> (slotIndex)];
}

//==============================================================================
void ShortcutSettings::setSlotShortcut (int slotIndex, const SlotShortcut& shortcut)
{
    jassert (juce::isPositiveAndBelow (slotIndex, static_cast<int> (slotShortcuts.size())));
    slotShortcuts[static_cast<size_t> (slotIndex)] = shortcut;
}

//==============================================================================
juce::Array<int> ShortcutSettings::findSlotIndicesForInputSource (const ShortcutInputSource& source) const
{
    juce::Array<int> result;

    if (! source.isValid())
        return result;

    for (int i = 0; i < static_cast<int> (slotShortcuts.size()); ++i)
    {
        if (slotShortcuts[static_cast<size_t> (i)].inputSource.matches (source))
            result.add (i);
    }

    return result;
}

//==============================================================================
juce::Array<int> ShortcutSettings::findSlotIndicesForKey (const juce::KeyPress& key) const
{
    if (! key.isValid())
        return {};

    return findSlotIndicesForInputSource (ShortcutInputSource (key));
}

//==============================================================================
std::unique_ptr<juce::XmlElement> ShortcutSettings::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("ShortcutSettings");
    xml->setAttribute ("version", 2);

    auto globalsXml = std::make_unique<juce::XmlElement> ("GlobalShortcuts");

    for (size_t i = 0; i < globalShortcuts.size(); ++i)
    {
        auto action = static_cast<GlobalShortcutAction> (i);
        auto keyXml = std::make_unique<juce::XmlElement> ("Shortcut");
        keyXml->setAttribute ("action", static_cast<int> (action));
        keyXml->setAttribute ("keyCode", globalShortcuts[i].getKeyCode());
        keyXml->setAttribute ("modifiers", static_cast<int> (globalShortcuts[i].getModifiers().getRawFlags()));
        keyXml->setAttribute ("textCharacter", juce::String::charToString (globalShortcuts[i].getTextCharacter()));
        globalsXml->addChildElement (keyXml.release());
    }

    xml->addChildElement (globalsXml.release());

    auto slotsXml = std::make_unique<juce::XmlElement> ("SlotShortcuts");

    for (int i = 0; i < static_cast<int> (slotShortcuts.size()); ++i)
    {
        auto slotXml = slotShortcuts[static_cast<size_t> (i)].toXml ("Slot");
        slotXml->setAttribute ("index", i);
        slotsXml->addChildElement (slotXml.release());
    }

    xml->addChildElement (slotsXml.release());
    return xml;
}

//==============================================================================
bool ShortcutSettings::fromXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("ShortcutSettings"))
        return false;

    if (auto* globalsXml = xml.getChildByName ("GlobalShortcuts"))
    {
        for (auto* child = globalsXml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
        {
            if (child->hasTagName ("Shortcut"))
            {
                auto actionIndex = child->getIntAttribute ("action", -1);

                if (juce::isPositiveAndBelow (actionIndex, static_cast<int> (GlobalShortcutAction::numActions)))
                {
                    auto keyCode   = child->getIntAttribute ("keyCode");
                    auto modifiers = juce::ModifierKeys (child->getIntAttribute ("modifiers", 0));
                    auto textChar  = child->getStringAttribute ("textCharacter")[0];

                    globalShortcuts[static_cast<size_t> (actionIndex)] = juce::KeyPress (keyCode, modifiers, textChar);
                }
            }
        }
    }

    if (auto* slotsXml = xml.getChildByName ("SlotShortcuts"))
    {
        for (auto* child = slotsXml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
        {
            if (child->hasTagName ("Slot"))
            {
                auto index = child->getIntAttribute ("index", -1);

                if (juce::isPositiveAndBelow (index, static_cast<int> (slotShortcuts.size())))
                {
                    SlotShortcut slot;

                    if (slot.fromXml (*child))
                        slotShortcuts[static_cast<size_t> (index)] = slot;
                }
            }
        }
    }

    return true;
}

//==============================================================================
juce::String ShortcutSettings::toXmlString() const
{
    auto xml = toXml();
    return xml->toString();
}

//==============================================================================
bool ShortcutSettings::fromXmlString (const juce::String& text)
{
    if (text.isEmpty())
        return false;

    auto xml = juce::XmlDocument::parse (text);

    if (xml == nullptr)
        return false;

    return fromXml (*xml);
}

//==============================================================================
juce::String ShortcutSettings::getGlobalShortcutActionName (GlobalShortcutAction action)
{
    switch (action)
    {
        case GlobalShortcutAction::bringWindowToFront:     return TRANS ("Bring window to front");
        case GlobalShortcutAction::toggleAllPluginsBypass: return TRANS ("Toggle all plugins bypass");
        case GlobalShortcutAction::toggleSettingsPanel:    return TRANS ("Toggle settings panel");
        case GlobalShortcutAction::deleteFocusedSlot:      return TRANS ("Delete focused slot");
        default:                                           return TRANS ("Unknown");
    }
}

//==============================================================================
juce::String ShortcutSettings::getGlobalShortcutActionDescription (GlobalShortcutAction action)
{
    switch (action)
    {
        case GlobalShortcutAction::bringWindowToFront:     return TRANS ("Show and focus the main Minixer window.");
        case GlobalShortcutAction::toggleAllPluginsBypass: return TRANS ("Bypass or unbypass all loaded plugins at once.");
        case GlobalShortcutAction::toggleSettingsPanel:    return TRANS ("Open or close the settings panel.");
        case GlobalShortcutAction::deleteFocusedSlot:      return TRANS ("Remove the plugin in the currently focused slot.");
        default:                                           return {};
    }
}

//==============================================================================
juce::String ShortcutSettings::slotShortcutModeToString (SlotShortcutMode mode)
{
    return mode == SlotShortcutMode::holdToggle ? "hold" : "cycle";
}

//==============================================================================
SlotShortcutMode ShortcutSettings::stringToSlotShortcutMode (const juce::String& text)
{
    return text.equalsIgnoreCase ("hold") ? SlotShortcutMode::holdToggle : SlotShortcutMode::cycleToggle;
}

} // namespace minixer
