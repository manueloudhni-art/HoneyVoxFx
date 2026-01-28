/*
  ==============================================================================

    HoneyVox Ad-Lib FX - Editor
    Created by Nolo's Addiction

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class HoneyVoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HoneyVoxLookAndFeel();
    
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;
    
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    // Load filmstrip knob images (shared across all instances)
    static void setKnobImage (const juce::Image& img, int numFrames);
    static void setHoneyKnobImage (const juce::Image& img, int numFrames);
    static bool hasKnobImage() { return sharedKnobImage.isValid(); }
    static bool hasHoneyKnobImage() { return sharedHoneyKnobImage.isValid(); }
    
    // Colors
    juce::Colour honeyGold { 0xFFD4A030 };
    juce::Colour warmAmber { 0xFFCC8800 };
    
    // Flag for using honey knob
    bool useHoneyKnob = false;
    
private:
    // Static shared knob images for all instances
    static juce::Image sharedKnobImage;
    static int sharedKnobFrames;
    static int sharedKnobFrameHeight;
    
    static juce::Image sharedHoneyKnobImage;
    static int sharedHoneyKnobFrames;
    static int sharedHoneyKnobFrameHeight;
};

//==============================================================================
class VintageSwitch : public juce::Component
{
public:
    VintageSwitch();
    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    
    juce::ToggleButton& getButton() { return button; }
    
private:
    juce::ToggleButton button;
    HoneyVoxLookAndFeel lf;
};

//==============================================================================
class VintageScreen : public juce::Component
{
public:
    VintageScreen (const juce::String& text);
    void paint (juce::Graphics& g) override;
    void setText (const juce::String& text) { displayText = text; repaint(); }
    void setAnimPhase (float phase) { animPhase = phase; }
    
private:
    juce::String displayText;
    float animPhase = 0.0f;
};

//==============================================================================
class ParameterKnob : public juce::Component
{
public:
    ParameterKnob (const juce::String& labelText, const juce::String& suffix = "");
    ~ParameterKnob() override;
    
    void resized() override;
    void paint (juce::Graphics& g) override;
    
    juce::Slider& getSlider() { return knob; }
    
private:
    juce::Slider knob;
    juce::Label label;
    juce::Label valueLabel;
    juce::String suffix;
    
    HoneyVoxLookAndFeel lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterKnob)
};

//==============================================================================
class EffectSection : public juce::Component
{
public:
    EffectSection (const juce::String& name);
    ~EffectSection() override;
    
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    void addKnob (ParameterKnob* knob);
    juce::ToggleButton& getBypassButton() { return bypassSwitch.getButton(); }
    VintageScreen& getScreen() { return screen; }
    
private:
    juce::String sectionName;
    VintageSwitch bypassSwitch;
    VintageScreen screen;
    std::vector<ParameterKnob*> knobRefs;
    HoneyVoxLookAndFeel lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectSection)
};

//==============================================================================
class HoneyVoxAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    HoneyVoxAudioProcessorEditor (HoneyVoxAudioProcessor&);
    ~HoneyVoxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HoneyVoxAudioProcessor& audioProcessor;
    HoneyVoxLookAndFeel lookAndFeel;
    
    // Effect sections
    EffectSection phoneSection { "PHONE" };
    EffectSection delaySection { "ECHO (DELAY)" };
    EffectSection saturationSection { "HONEY" };
    EffectSection underwaterSection { "UNDERWATER" };
    
    // Knobs
    ParameterKnob phoneKnob { "AMOUNT" };
    ParameterKnob delayTimeKnob { "TIME", "ms" };
    ParameterKnob delayFeedbackKnob { "FDBK", "%" };
    ParameterKnob delayMixKnob { "MIX", "%" };
    ParameterKnob saturationKnob { "SAT DRIVE" };
    ParameterKnob underwaterKnob { "DEPTH" };
    ParameterKnob outputKnob { "GAIN", "dB" };
    
    // Phone mode selector
    juce::ComboBox phoneModeBox;
    juce::Label phoneModeLabel;
    
    // Ping-pong toggle
    VintageSwitch pingPongSwitch;
    juce::Label pingPongLabel;
    
    // Delay sync controls
    VintageSwitch delaySyncSwitch;
    juce::Label delaySyncLabel;
    juce::ComboBox delayDivisionBox;
    
    // Output section
    VintageScreen outputScreen { "OUTPUT" };
    VintageSwitch outputSwitch;
    
    // Preset controls
    juce::ComboBox presetBox;
    juce::TextButton savePresetButton { "Save" };
    juce::TextButton loadPresetButton { "Load" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    void savePreset();
    void loadPreset();
    void updatePresetList();
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> phoneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> saturationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> underwaterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phoneBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> saturationBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> underwaterBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> outputBypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pingPongAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delaySyncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phoneModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayDivisionAttachment;
    
    juce::Image logoImage;
    bool logoLoaded = false;
    
    // Color scheme toggle
    bool purpleMode = false;
    juce::Rectangle<float> colorScrewBounds;
    
    // Hex bomb easter egg
    bool showHexBomb = false;
    int hexBombTimer = 0;
    juce::Rectangle<float> hexBombScrewBounds;
    
    // Bee preset navigation
    juce::Rectangle<float> leftBeeBounds;
    juce::Rectangle<float> rightBeeBounds;
    bool leftBeePressed = false;
    bool rightBeePressed = false;
    
    // Cable screw easter egg (top-left)
    juce::Rectangle<float> cableScrewBounds;
    float cableScrewAngle = 0.0f;  // 0.0 to 1.0
    float lastCableScrewY = 0.0f;
    bool cableScrewDragging = false;
    
    // Hex matrix glow animation
    float hexGlowPhase = 0.0f;
    
    void drawCornerScrew (juce::Graphics& g, float x, float y, bool isColorScrew = false);
    void drawCableScrew (juce::Graphics& g, float x, float y);
    void drawBee (juce::Graphics& g, float centerX, float centerY, float size, bool pressed = false);
    void drawBacklitLED (juce::Graphics& g, float x, float y, float size, bool isOn);
    void drawHexBomb (juce::Graphics& g);
    void drawGlowingHexMatrix (juce::Graphics& g, juce::Rectangle<float> bounds);
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void timerCallback () override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HoneyVoxAudioProcessorEditor)
};
