/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include <memory>
#include "MainComponent.h"
#include "Settings/AppSettings.h"
#include "LookAndFeel/MixerLookAndFeel.h"

using minixer::MainComponent;

//==============================================================================
class MinixerApplication  : public juce::JUCEApplication
{
public:
    MinixerApplication();
    ~MinixerApplication() override;

    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;

    void initialise (const juce::String& commandLine) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted (const juce::String& commandLine) override;

    class MainWindow;

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
class MinixerApplication::MainWindow  : public juce::DocumentWindow
{
public:
    class TrayIconComponent;

    MainWindow (juce::String name);
    ~MainWindow() override;

    void showAndBringToFront();
    void hideToTray();
    void toggleVisibility();

    void closeButtonPressed() override;
    void minimiseButtonPressed() override;

    bool keyPressed (const juce::KeyPress& key) override;
    bool keyStateChanged (bool isKeyDown) override;

private:
    std::unique_ptr<TrayIconComponent> trayIcon;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

//==============================================================================
class MinixerApplication::MainWindow::TrayIconComponent : public juce::SystemTrayIconComponent
{
public:
    explicit TrayIconComponent (MainWindow& ownerWindow)
        : owner (ownerWindow)
    {
        setIconImage (createTrayIconImage(), {});
        setIconTooltip (TRANS ("Minixer"));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            showTrayMenu();
        }
        else if (e.mods.isLeftButtonDown())
        {
            owner.toggleVisibility();
        }
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        owner.showAndBringToFront();
    }

private:
    MainWindow& owner;

    void showTrayMenu()
    {
        juce::PopupMenu menu;
        auto isShowing = owner.isVisible();

        menu.addItem (1, isShowing ? TRANS ("Hide Minixer") : TRANS ("Show Minixer"), true, false);
        menu.addSeparator();
        menu.addItem (2, TRANS ("Quit"), true, false);

        auto safeThis = juce::Component::SafePointer<TrayIconComponent> (this);

        menu.showMenuAsync (juce::PopupMenu::Options(),
                            [safeThis] (int result)
        {
            if (safeThis == nullptr)
                return;

            if (result == 1)
                safeThis->owner.toggleVisibility();
            else if (result == 2)
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
        });
    }

    static juce::Image createTrayIconImage()
    {
        auto loadProjectIcon = [] (const juce::File& file) -> juce::Image
        {
            if (file.existsAsFile())
            {
                auto img = juce::ImageFileFormat::loadFrom (file);

                if (img.isValid())
                    return img.rescaled (32, 32, juce::Graphics::highResamplingQuality);
            }

            return {};
        };

        auto exeDir = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                            .getParentDirectory();

        // 从可执行文件所在目录逐级向上查找项目 assets 目录
        for (auto dir = exeDir; dir.exists() && dir.getFullPathName().length() > 3; dir = dir.getParentDirectory())
        {
            auto img = loadProjectIcon (dir.getChildFile ("assets/minixer_06_warm_orange.png"));

            if (img.isValid())
                return img;
        }

        // 回退到当前工作目录
        if (auto img = loadProjectIcon (juce::File::getCurrentWorkingDirectory()
                                            .getChildFile ("assets/minixer_06_warm_orange.png"));
            img.isValid())
        {
            return img;
        }

        // 兜底：使用程序生成的圆形图标
        const int size = 32;
        juce::Image image (juce::Image::ARGB, size, size, true);
        juce::Graphics g (image);

        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size));
        g.setColour (minixer::MixerLookAndFeel::getAccentColour());
        g.fillEllipse (bounds.reduced (2.0f));

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (18.0f)).boldened());
        g.drawText ("M", bounds, juce::Justification::centred, true);

        return image;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrayIconComponent)
};

//==============================================================================
MinixerApplication::MainWindow::MainWindow (juce::String name)
    : DocumentWindow (name,
                      juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                  .findColour (juce::ResizableWindow::backgroundColourId),
                      DocumentWindow::closeButton),
      trayIcon (std::make_unique<TrayIconComponent> (*this))
{
    setUsingNativeTitleBar (true);
    setContentOwned (new MainComponent(), true);

   #if JUCE_IOS || JUCE_ANDROID
    setFullScreen (true);
   #else
    setResizable (true, true);
    setResizeLimits (360, 640, 540, 640);

    {
        auto savedBounds = minixer::AppSettings::getInstance().loadMainWindowBounds();

        if (savedBounds.isEmpty())
            centreWithSize (getWidth(), getHeight());
        else
            setBounds (savedBounds);
    }
   #endif

    auto& settings = minixer::AppSettings::getInstance();
    const bool startHidden = settings.getStartMinimized() && settings.getMinimizeToTray();

    if (! startHidden)
        setVisible (true);

    if (settings.getStartMinimized() && ! settings.getMinimizeToTray())
        setMinimised (true);
}

MinixerApplication::MainWindow::~MainWindow()
{
    minixer::AppSettings::getInstance().saveMainWindowBounds (getBounds());
}

void MinixerApplication::MainWindow::showAndBringToFront()
{
    setVisible (true);
    setMinimised (false);
    toFront (true);
}

void MinixerApplication::MainWindow::hideToTray()
{
    setVisible (false);
}

void MinixerApplication::MainWindow::toggleVisibility()
{
    if (isVisible())
        hideToTray();
    else
        showAndBringToFront();
}

void MinixerApplication::MainWindow::closeButtonPressed()
{
    if (minixer::AppSettings::getInstance().getMinimizeToTray())
        hideToTray();
    else
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MinixerApplication::MainWindow::minimiseButtonPressed()
{
    if (minixer::AppSettings::getInstance().getMinimizeToTray())
        hideToTray();
    else
        juce::DocumentWindow::minimiseButtonPressed();
}

bool MinixerApplication::MainWindow::keyPressed (const juce::KeyPress& key)
{
    // JUCE 窗口获得焦点时会调用 component.grabKeyboardFocus()，使顶层窗口自身成为当前焦点。
    // 此时按键事件只派发到顶层窗口，无法到达 MainComponent，需要显式转发。
    auto* focused = juce::Component::getCurrentlyFocusedComponent();

    if (focused == nullptr || focused == this)
    {
        if (auto* mainComponent = dynamic_cast<MainComponent*> (getContentComponent()))
            if (mainComponent->keyPressed (key, this))
                return true;
    }

    return juce::DocumentWindow::keyPressed (key);
}

bool MinixerApplication::MainWindow::keyStateChanged (bool isKeyDown)
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();

    if (focused == nullptr || focused == this)
    {
        if (auto* mainComponent = dynamic_cast<MainComponent*> (getContentComponent()))
            if (mainComponent->keyStateChanged (isKeyDown, this))
                return true;
    }

    return juce::DocumentWindow::keyStateChanged (isKeyDown);
}

//==============================================================================
MinixerApplication::MinixerApplication() = default;
MinixerApplication::~MinixerApplication() = default;

const juce::String MinixerApplication::getApplicationName()       { return ProjectInfo::projectName; }
const juce::String MinixerApplication::getApplicationVersion()    { return ProjectInfo::versionString; }
bool MinixerApplication::moreThanOneInstanceAllowed()             { return false; }

void MinixerApplication::initialise (const juce::String& commandLine)
{
    juce::ignoreUnused (commandLine);
    mainWindow.reset (new MainWindow (getApplicationName()));
}

void MinixerApplication::shutdown()
{
    mainWindow = nullptr;
}

void MinixerApplication::systemRequestedQuit()
{
    quit();
}

void MinixerApplication::anotherInstanceStarted (const juce::String& commandLine)
{
    if (mainWindow != nullptr)
        mainWindow->showAndBringToFront();

    // 当前版本无命令行文件/参数处理逻辑，故忽略。
    // 若未来支持 preset 文件双击打开或 --preset=Name，可在此解析并转发。
    juce::ignoreUnused (commandLine);
}

//==============================================================================
START_JUCE_APPLICATION (MinixerApplication)
