#include "PluginProcessor.h"
#include "PluginEditor.h"

CoroAudioProcessor::CoroAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CoroAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "choir", 1 }, "Coro",
           StringArray { CORO_TXT_NINOS, "Mujeres", "Hombres", "Mixto" }, 3));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "vowel", 1 }, "Vocal",
           StringArray { "A", "E", "I", "O", "U" }, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "voices", 1 }, "Voces", 2, 8, 6));

    auto pct = [] (const char* id, const juce::String& name, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
               NormalisableRange<float> (0.f, 100.f, 0.1f), def, AudioParameterFloatAttributes().withLabel ("%"));
    };
    l.add (pct ("detune",  CORO_TXT_DETUNE, 35.f));
    l.add (pct ("vibrato", "Vibrato",      40.f));
    l.add (pct ("formant", "Timbre vocal", 55.f));
    l.add (pct ("width",   "Amplitud",     80.f));
    l.add (pct ("reverb",  "Reverb",       35.f));
    l.add (pct ("mix",     "Mezcla",       70.f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Salida",
           NormalisableRange<float> (-24.f, 12.f, 0.1f), 0.f, AudioParameterFloatAttributes().withLabel ("dB")));
    return l;
}

bool CoroAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo()) return false;
    const auto in = l.getMainInputChannelSet();
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void CoroAudioProcessor::prepareToPlay (double sr, int blockSize)
{
    currentSampleRate = sr;
    for (int i = 0; i < numSingers; ++i) singers[(size_t) i].prepare (sr, i);
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 2 };
    reverb.prepare (spec);
    reverb.reset();
    lastChoir = lastVowel = -1;
}

void CoroAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels();
    for (int c = inCh; c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, numSamples);
    if (buffer.getNumChannels() < 2) return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const int   choirIdx = (int) get ("choir");
    const int   vowelIdx = (int) get ("vowel");
    const int   voices   = juce::jlimit (2, numSingers, (int) get ("voices"));
    const float detune   = get ("detune")  * 0.01f * 30.f;     // hasta 30 cents
    const float vibrato  = get ("vibrato") * 0.01f;
    const float formant  = get ("formant") * 0.01f;
    const float width    = get ("width")   * 0.01f;
    const float revAmt   = get ("reverb")  * 0.01f;
    const float mix      = get ("mix")     * 0.01f;
    const float outGain  = juce::Decibels::decibelsToGain (get ("output"));

    const auto& profiles = coro::profileFor ((coro::ChoirType) choirIdx);
    if (choirIdx != lastChoir || vowelIdx != lastVowel)
    {
        const auto& fm = coro::vowelFormants()[(size_t) juce::jlimit (0, 4, vowelIdx)];
        for (int i = 0; i < numSingers; ++i)
            singers[(size_t) i].setVowel (fm, profiles[(size_t) i].formantUp, 6.f);
        lastChoir = choirIdx; lastVowel = vowelIdx;
    }

    juce::AudioBuffer<float> wet (2, numSamples);
    wet.clear();
    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getReadPointer (inCh > 1 ? 1 : 0);
    float* wl = wet.getWritePointer (0);
    float* wr = wet.getWritePointer (1);
    const float voiceGain = 1.f / std::sqrt ((float) voices);

    for (int n = 0; n < numSamples; ++n)
    {
        const float mono = 0.5f * (inL[n] + inR[n]);
        float l = 0.f, r = 0.f;
        for (int v = 0; v < voices; ++v)
        {
            const auto& p = profiles[(size_t) v];
            const float s = singers[(size_t) v].process (mono, p, detune, vibrato, formant) * voiceGain;
            const float pan = juce::jlimit (-1.f, 1.f, p.pan * width);
            const float a = (pan + 1.f) * juce::MathConstants<float>::pi * 0.25f;
            l += s * std::cos (a);
            r += s * std::sin (a);
        }
        wl[n] = l * 1.4f;
        wr[n] = r * 1.4f;
    }

    // Reverb tipo iglesia, solo en la señal procesada.
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.55f + 0.4f * revAmt;
    rp.damping  = 0.35f;
    rp.wetLevel = revAmt * 0.45f;
    rp.dryLevel = 1.f - revAmt * 0.25f;
    rp.width    = 1.f;
    reverb.setParameters (rp);
    juce::dsp::AudioBlock<float> block (wet);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    reverb.process (ctx);

    float peak = 0.f;
    for (int n = 0; n < numSamples; ++n)
    {
        const float dl = inL[n], dr = inR[n];
        const float ol = (dl * (1.f - mix) + wl[n] * mix) * outGain;
        const float or_ = (dr * (1.f - mix) + wr[n] * mix) * outGain;
        buffer.setSample (0, n, ol);
        buffer.setSample (1, n, or_);
        peak = juce::jmax (peak, std::abs (ol), std::abs (or_));
    }
    smoothedLevel = juce::jmax (peak, smoothedLevel * 0.9f);
    outputLevel.store (smoothedLevel);
}

juce::AudioProcessorEditor* CoroAudioProcessor::createEditor() { return new CoroAudioProcessorEditor (*this); }

void CoroAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void CoroAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CoroAudioProcessor(); }
