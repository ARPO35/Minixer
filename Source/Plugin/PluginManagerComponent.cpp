/*
  ==============================================================================

    PluginManagerComponent.cpp

  ==============================================================================
*/

#include "PluginManagerComponent.h"
#include "PluginRegistry.h"
#include "../Settings/AppSettings.h"
#include "../LookAndFeel/MixerLookAndFeel.h"

namespace minixer
{

//==============================================================================
PluginManagerComponent::PluginManagerComponent()
    : pluginListComponent (PluginRegistry::getInstance().getFormatManager(),
                           PluginRegistry::getInstance().getKnownPluginList(),
                           AppSettings::getInstance().getDeadMansPedalFile(),
                           AppSettings::getInstance().getPropertiesFile(),
                           false)
{
    pluginListComponent.setLookAndFeel (&getLookAndFeel());

    // 使用 JUCE 原生的扫描进度对话框，并配置同步扫描。
    // 说明：VST3 插件的实例化与总线信息查询（如 juce_VST3PluginFormat.cpp:301
    // 的 JUCE_ASSERT_MESSAGE_THREAD）必须在消息线程执行。将扫描线程数设为 0，
    // PluginListComponent 会改为在消息线程上通过 Timer 轮询同步扫描；进度对话
    // 框仍会正常显示，并在每个插件扫描间隙刷新进度。
    pluginListComponent.setScanDialogText (TRANS ("Scanning for VST3 plugins…"),
                                           TRANS ("Please wait while the plugins are scanned."));
    pluginListComponent.setNumberOfThreadsForScanning (0);

    rescanFailedPluginsButton.setButtonText (TRANS ("Rescan previously failed plugins"));
    rescanFailedPluginsButton.setTooltip (TRANS ("When checked, plugins that crashed or failed in the last scan will be scanned again."));
    rescanFailedPluginsButton.onClick = [this] { onRescanFailedPluginsToggled(); };
    addAndMakeVisible (rescanFailedPluginsButton);

    addAndMakeVisible (pluginListComponent);

    // 监听 KnownPluginList 变化，在扫描结束后展示扫描报告。
    PluginRegistry::getInstance().getKnownPluginList().addChangeListener (this);

    setSize (600, 500);
}

//==============================================================================
PluginManagerComponent::~PluginManagerComponent()
{
    PluginRegistry::getInstance().getKnownPluginList().removeChangeListener (this);
    stopTimer();

    // 如果用户勾选了复选框但未实际触发扫描，确保不遗留重试标志。
    PluginRegistry::getInstance().setRescanFailedPlugins (false);
}

//==============================================================================
void PluginManagerComponent::paint (juce::Graphics& g)
{
    g.fillAll (MixerLookAndFeel::getBackgroundColour());
}

//==============================================================================
void PluginManagerComponent::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto buttonHeight = 28;
    rescanFailedPluginsButton.setBounds (bounds.removeFromTop (buttonHeight));

    pluginListComponent.setBounds (bounds.withTrimmedTop (8));
}

//==============================================================================
void PluginManagerComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source != &PluginRegistry::getInstance().getKnownPluginList())
        return;

    // 每次列表变化都重新启动防抖定时器，扫描真正结束后再展示报告。
    startTimer (scanReportDelayMs);
}

//==============================================================================
void PluginManagerComponent::timerCallback()
{
    stopTimer();

    auto& registry = PluginRegistry::getInstance();

    // 让 PluginRegistry 检测扫描是否已空闲，若已空闲则结束本次报告。
    registry.checkAndFinishIdleScan();

    // 扫描仍在进行中则继续等待。
    if (registry.isScanInProgress())
    {
        startTimer (scanReportDelayMs);
        return;
    }

    // 扫描已真正结束，且存在未展示的报告时，弹出最终报告并清理一次性状态。
    if (registry.getLastScanReport().hasUnshownReport)
    {
        rescanFailedPluginsButton.setToggleState (false, juce::dontSendNotification);
        showScanReportIfNeeded();
    }
}

//==============================================================================
juce::String PluginManagerComponent::formatScanReport (const PluginScanReport& report)
{
    juce::String text;
    text << TRANS ("Scan completed") << ":\n\n";
    text << TRANS ("Total files scanned") << ": " << report.totalFiles << "\n";
    text << TRANS ("Successful") << ": " << report.successCount << "\n";
    text << TRANS ("New plugins") << ": " << report.newCount << "\n";
    text << TRANS ("Updated plugins") << ": " << report.updatedCount << "\n";
    text << TRANS ("Skipped (unchanged)") << ": " << report.skippedCount << "\n";
    text << TRANS ("Skipped (blacklisted)") << ": " << report.blacklistedCount << "\n";
    text << TRANS ("Failed") << ": " << report.failedCount << "\n";

    if (! report.failedEntries.isEmpty())
    {
        text << "\n" << TRANS ("Failed files") << ":\n";

        constexpr int maxFailedEntriesToShow = 10;
        const int numToShow = juce::jmin (report.failedEntries.size(), maxFailedEntriesToShow);

        for (int i = 0; i < numToShow; ++i)
        {
            const auto& entry = report.failedEntries.getReference (i);
            text << "  " << juce::File (entry.filePath).getFileName()
                 << " - " << entry.reason << "\n";
        }

        if (report.failedEntries.size() > maxFailedEntriesToShow)
        {
            text << "  " << TRANS ("…and") << " "
                 << (report.failedEntries.size() - maxFailedEntriesToShow)
                 << " " << TRANS ("more") << "\n";
        }
    }

    if (report.blacklistedCount > 0)
    {
        text << "\n" << TRANS ("Blacklisted plugins skipped") << ": "
             << report.blacklistedCount << "\n";
        text << TRANS ("Check \"Rescan previously failed plugins\" to retry them.") << "\n";
    }

    return text;
}

//==============================================================================
void PluginManagerComponent::showScanReportIfNeeded()
{
    auto report = PluginRegistry::getInstance().getLastScanReport();

    if (! report.hasUnshownReport)
        return;

    PluginRegistry::getInstance().markLastScanReportAsShown();

    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                            TRANS ("Plugin Scan Report"),
                                            formatScanReport (report),
                                            TRANS ("OK"));
}

//==============================================================================
void PluginManagerComponent::onRescanFailedPluginsToggled()
{
    PluginRegistry::getInstance().setRescanFailedPlugins (rescanFailedPluginsButton.getToggleState());
}

} // namespace minixer
