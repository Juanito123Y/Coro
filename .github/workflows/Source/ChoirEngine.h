#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace coro
{
enum class ChoirType { Children = 0, Women, Men, Mixed };

struct VoiceProfile
{
    float semitones;   // transposición respecto a la voz de entrada
    float formantUp;   // factor de desplazamiento de formantes (1 = igual)
    float delayMs;     // retardo base del cantante
    float pan;         // -1..1
};

// 8 cantantes por coro.
inline const std::array<VoiceProfile, 8>& profileFor (ChoirType t)
{
    static const std::array<VoiceProfile, 8> children {{
        { 12.f, 1.30f, 14.f, -0.80f }, { 12.f, 1.35f, 22.f,  0.80f },
        { 12.f, 1.25f, 30.f, -0.40f }, { 19.f, 1.32f, 18.f,  0.40f },
        { 12.f, 1.28f, 36.f, -0.95f }, { 19.f, 1.36f, 26.f,  0.95f },
        {  7.f, 1.22f, 12.f, -0.15f }, { 12.f, 1.30f, 40.f,  0.15f } }};
    static const std::array<VoiceProfile, 8> women {{
        { 12.f, 1.15f, 14.f, -0.80f }, {  0.f, 1.12f, 22.f,  0.80f },
        {  7.f, 1.10f, 30.f, -0.40f }, { 12.f, 1.14f, 18.f,  0.40f },
        {  0.f, 1.08f, 36.f, -0.95f }, {  4.f, 1.12f, 26.f,  0.95f },
        { 12.f, 1.10f, 12.f, -0.15f }, {  0.f, 1.14f, 40.f,  0.15f } }};
    static const std::array<VoiceProfile, 8> men {{
        {  0.f, 0.90f, 14.f, -0.80f }, { -12.f, 0.85f, 22.f,  0.80f },
        { -5.f, 0.88f, 30.f, -0.40f }, {  0.f, 0.92f, 18.f,  0.40f },
        { -12.f, 0.84f, 36.f, -0.95f }, { -7.f, 0.88f, 26.f,  0.95f },
        {  0.f, 0.90f, 12.f, -0.15f }, { -12.f, 0.86f, 40.f,  0.15f } }};
    static const std::array<VoiceProfile, 8> mixed {{
        { 12.f, 1.15f, 14.f, -0.80f }, {  0.f, 1.00f, 22.f,  0.80f },
        { -12.f, 0.88f, 30.f, -0.40f }, {  7.f, 1.10f, 18.f,  0.40f },
        { -5.f, 0.92f, 36.f, -0.95f }, { 12.f, 1.18f, 26.f,  0.95f },
        {  0.f, 0.95f, 12.f, -0.15f }, { -12.f, 0.86f, 40.f,  0.15f } }};
    switch (t) { case ChoirType::Children: return children;
                 case ChoirType::Women:    return women;
                 case ChoirType::Men:      return men;
                 default:                  return mixed; }
}

// Formantes (Hz) de cada vocal: F1, F2, F3.
inline const std::array<std::array<float, 3>, 5>& vowelFormants()
{
    static const std::array<std::array<float, 3>, 5> v {{
        {  800.f, 1150.f, 2900.f },  // A
        {  400.f, 2000.f, 2800.f },  // E
        {  330.f, 2500.f, 3200.f },  // I
        {  450.f,  800.f, 2830.f },  // O
        {  325.f,  700.f, 2700.f }   // U
    }};
    return v;
}

// Cambio de tono por dos líneas de retardo con crossfade (ventana de grano).
class PitchShifter
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        buffer.assign (static_cast<size_t> (sr * 0.5) + 8, 0.f);
        writePos = 0; phase = 0.f;
    }
    void reset() { std::fill (buffer.begin(), buffer.end(), 0.f); phase = 0.f; }

    float process (float in, float ratio, float extraDelaySamples)
    {
        const int n = static_cast<int> (buffer.size());
        buffer[(size_t) writePos] = in;

        const float windowSamples = static_cast<float> (sampleRate) * 0.050f;
        phase += (1.f - ratio) / windowSamples;
        phase -= std::floor (phase);

        auto tap = [&] (float p)
        {
            const float d = p * windowSamples + extraDelaySamples + 2.f;
            float rp = static_cast<float> (writePos) - d;
            while (rp < 0.f) rp += (float) n;
            const int i0 = (int) rp; const int i1 = (i0 + 1) % n;
            const float fr = rp - (float) i0;
            return buffer[(size_t) i0] * (1.f - fr) + buffer[(size_t) i1] * fr;
        };
        const float p2 = phase + 0.5f - std::floor (phase + 0.5f);
        const float g1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase);
        const float g2 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * p2);
        const float out = tap (phase) * g1 + tap (p2) * g2;
        writePos = (writePos + 1) % n;
        return out;
    }
private:
    std::vector<float> buffer;
    int writePos = 0; float phase = 0.f; double sampleRate = 44100.0;
};

// Un cantante: pitch + vibrato + formantes de vocal + retardo.
class Singer
{
public:
    void prepare (double sr, int index)
    {
        sampleRate = sr;
        shifter.prepare (sr);
        lfoPhase = (float) index * 0.137f;
        lfoRate  = 4.6f + 0.35f * (float) index;   // cada cantante vibra distinto
        driftPhase = (float) index * 0.71f;
        for (auto& f : bands) f.reset();
    }
    void reset() { shifter.reset(); for (auto& f : bands) f.reset(); }

    void setVowel (const std::array<float, 3>& fm, float formantScale, float q)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            const float freq = juce::jlimit (200.f, 0.45f * (float) sampleRate, fm[i] * formantScale);
            bands[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, freq, q);
        }
    }

    float process (float in, const VoiceProfile& p, float detuneCents, float vibratoDepth, float vowelAmount)
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        lfoPhase   += twoPi * lfoRate / (float) sampleRate;  if (lfoPhase > twoPi)   lfoPhase -= twoPi;
        driftPhase += twoPi * 0.3f / (float) sampleRate;     if (driftPhase > twoPi) driftPhase -= twoPi;

        const float vib = std::sin (lfoPhase) * vibratoDepth * 25.f;      // cents
        const float drift = std::sin (driftPhase) * detuneCents * 0.5f;
        const float cents = detuneCents * (p.pan > 0 ? 1.f : -1.f) * 0.6f + vib + drift;
        const float ratio = std::pow (2.f, (p.semitones + cents * 0.01f) / 12.f);

        const float delaySamples = p.delayMs * 0.001f * (float) sampleRate;
        const float shifted = shifter.process (in, ratio, delaySamples);

        float formant = 0.f;
        for (auto& f : bands) formant += f.processSample (shifted);
        return shifted * (1.f - vowelAmount) + formant * 1.6f * vowelAmount;
    }
private:
    PitchShifter shifter;
    std::array<juce::dsp::IIR::Filter<float>, 3> bands;
    double sampleRate = 44100.0;
    float lfoPhase = 0.f, lfoRate = 5.f, driftPhase = 0.f;
};
} // namespace coro
