#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "ChoirEngine.h"

// Textos con acentos como UTF-8 explícito (válido en cualquier compilador).
#define CORO_TXT_NINOS    juce::String::fromUTF8 ("Ni\xC3\xB1os")
#define CORO_TXT_DETUNE   juce::String::fromUTF8 ("Desafinaci\xC3\xB3n")

class CoroAudioProcessor : public juce::AudioProcessor
{
public:
    CoroAudioProcessor();
    ~CoroAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Coro"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Para animar el medidor en la interfaz.
    std::atomic<float> outputLevel { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr int numSingers = 8;
    std::array<coro::Singer, numSingers> singers;
    juce::dsp::Reverb reverb;
    double currentSampleRate = 44100.0;
    int lastChoir = -1, lastVowel = -1;
    float smoothedLevel = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CoroAudioProcessor)
};
