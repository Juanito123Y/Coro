#pragma once
#include "PluginProcessor.h"

namespace theme
{
    const juce::Colour black   { 0xff000000 };
    const juce::Colour panel   { 0xff111113 };
    const juce::Colour card    { 0xff1c1c1e };
    const juce::Colour stroke  { 0xff2c2c2e };
    const juce::Colour red     { 0xffff3b30 };
    const juce::Colour redDim  { 0xff8a1f19 };
    const juce::Colour text    { 0xfff5f5f7 };
    const juce::Colour textDim { 0xff8e8e93 };
}

class AppleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AppleLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    static juce::Font sans (float size, bool bold = false);
};

class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& title);
    void resized() override;
private:
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label title;
    juce::AudioProcessorValueTreeState::SliderAttachment attach;
};

// Control segmentado estilo iOS para elegir el coro.
class Segmented : public juce::Component
{
public:
    Segmented (juce::AudioProcessorValueTreeState&, const juce::String& id, juce::StringArray items);
    ~Segmented() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::RangedAudioParameter* param;
    juce::StringArray labels;
    int selected = 0;
    struct Listener : juce::AudioProcessorParameter::Listener, juce::AsyncUpdater
    {
        Segmented& o;
        explicit Listener (Segmented& s) : o (s) {}
        void parameterValueChanged (int, float) override { triggerAsyncUpdate(); }
        void parameterGestureChanged (int, bool) override {}
        void handleAsyncUpdate() override { o.refresh(); }
    } listener { *this };
    void refresh();
};

class CoroAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CoroAudioProcessorEditor (CoroAudioProcessor&);
    ~CoroAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    CoroAudioProcessor& proc;
    AppleLookAndFeel laf;
    Segmented choir;
    juce::ComboBox vowelBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> vowelAttach;
    juce::Label vowelLabel, voicesLabel;
    juce::Slider voicesSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::AudioProcessorValueTreeState::SliderAttachment voicesAttach;
    Knob detune, vibrato, formant, width, reverb, mix, output;
    float level = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CoroAudioProcessorEditor)
};
