#include "PluginEditor.h"

using namespace theme;

//==============================================================================
juce::Font AppleLookAndFeel::sans (float size, bool bold)
{
    juce::FontOptions o;
    o = o.withName (juce::Font::getDefaultSansSerifFontName()).withHeight (size);
    if (bold) o = o.withStyle ("Bold");
    return juce::Font (o);
}

AppleLookAndFeel::AppleLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, text);
    setColour (juce::ComboBox::backgroundColourId, card);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, stroke);
    setColour (juce::ComboBox::arrowColourId, red);
    setColour (juce::PopupMenu::backgroundColourId, card);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, red);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Slider::trackColourId, red);
    setColour (juce::Slider::backgroundColourId, stroke);
    setColour (juce::Slider::thumbColourId, juce::Colours::white);
}

juce::Label* AppleLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::textColourId, text);
    l->setFont (sans (13.f, true));
    return l;
}

void AppleLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));
    if (label.isBeingEdited()) return;
    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (label.getFont());
    auto area = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
    g.drawFittedText (label.getText(), area, label.getJustificationType(), 1, label.getMinimumHorizontalScale());
}

juce::Font AppleLookAndFeel::getLabelFont (juce::Label&)      { return sans (12.f); }
juce::Font AppleLookAndFeel::getComboBoxFont (juce::ComboBox&) { return sans (14.f); }
juce::Font AppleLookAndFeel::getPopupMenuFont()                { return sans (14.f); }

void AppleLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto c = bounds.withSizeKeepingCentre (d, d);
    const float radius = d * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // cuerpo
    g.setGradientFill (juce::ColourGradient (card.brighter (0.15f), c.getX(), c.getY(),
                                             panel, c.getX(), c.getBottom(), false));
    g.fillEllipse (c);
    g.setColour (stroke);
    g.drawEllipse (c, 1.f);

    // arco
    auto arc = c.expanded (4.f);
    juce::Path track, value;
    track.addCentredArc (arc.getCentreX(), arc.getCentreY(), arc.getWidth() * 0.5f, arc.getHeight() * 0.5f,
                         0.f, startAngle, endAngle, true);
    value.addCentredArc (arc.getCentreX(), arc.getCentreY(), arc.getWidth() * 0.5f, arc.getHeight() * 0.5f,
                         0.f, startAngle, angle, true);
    g.setColour (stroke);
    g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (red);
    g.strokePath (value, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // indicador
    juce::Path tick;
    tick.addRoundedRectangle (-1.5f, -radius + 5.f, 3.f, radius * 0.32f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (tick, juce::AffineTransform::rotation (angle).translated (c.getCentreX(), c.getCentreY()));
}

void AppleLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h);
    g.setColour (card);
    g.fillRoundedRectangle (r, 10.f);
    g.setColour (stroke);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.f, 1.f);
    juce::Path chevron;
    const float cx = (float) w - 18.f, cy = (float) h * 0.5f;
    chevron.startNewSubPath (cx - 4.f, cy - 2.f);
    chevron.lineTo (cx, cy + 2.f);
    chevron.lineTo (cx + 4.f, cy - 2.f);
    g.setColour (red);
    g.strokePath (chevron, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void AppleLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (12, 1, box.getWidth() - 36, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& t)
    : attach (s, id, slider)
{
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
    slider.setDoubleClickReturnValue (true, s.getParameter (id)->convertFrom0to1 (s.getParameter (id)->getDefaultValue()));
    addAndMakeVisible (slider);
    title.setText (t.replace (juce::String::fromUTF8 ("\xC3\xB3"), juce::String::fromUTF8 ("\xC3\x93")).toUpperCase(),
                   juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    title.setColour (juce::Label::textColourId, textDim);
    title.setFont (AppleLookAndFeel::sans (10.5f, true));
    addAndMakeVisible (title);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    title.setBounds (r.removeFromTop (18));
    slider.setBounds (r);
}

//==============================================================================
Segmented::Segmented (juce::AudioProcessorValueTreeState& s, const juce::String& id, juce::StringArray items)
    : param (s.getParameter (id)), labels (std::move (items))
{
    param->addListener (&listener);
    refresh();
}

Segmented::~Segmented()
{
    param->removeListener (&listener);
    listener.cancelPendingUpdate();
}

void Segmented::refresh()
{
    selected = param->convertFrom0to1 (param->getValue()) > 0.f
                   ? juce::roundToInt (param->convertFrom0to1 (param->getValue())) : 0;
    repaint();
}

void Segmented::mouseDown (const juce::MouseEvent& e)
{
    const int idx = juce::jlimit (0, labels.size() - 1, e.x * labels.size() / juce::jmax (1, getWidth()));
    param->beginChangeGesture();
    param->setValueNotifyingHost (param->convertTo0to1 ((float) idx));
    param->endChangeGesture();
    selected = idx;
    repaint();
}

void Segmented::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (card);
    g.fillRoundedRectangle (r, 12.f);
    const float segW = r.getWidth() / (float) labels.size();
    auto sel = juce::Rectangle<float> (r.getX() + segW * (float) selected, r.getY(), segW, r.getHeight()).reduced (3.f);
    g.setColour (red);
    g.fillRoundedRectangle (sel, 9.f);
    for (int i = 0; i < labels.size(); ++i)
    {
        g.setColour (i == selected ? juce::Colours::white : textDim);
        g.setFont (AppleLookAndFeel::sans (14.f, i == selected));
        g.drawText (labels[i], juce::Rectangle<float> (r.getX() + segW * (float) i, r.getY(), segW, r.getHeight()),
                    juce::Justification::centred);
    }
}

//==============================================================================
CoroAudioProcessorEditor::CoroAudioProcessorEditor (CoroAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      choir (p.apvts, "choir", { CORO_TXT_NINOS, "Mujeres", "Hombres", "Mixto" }),
      voicesAttach (p.apvts, "voices", voicesSlider),
      detune (p.apvts, "detune", CORO_TXT_DETUNE), vibrato (p.apvts, "vibrato", "Vibrato"),
      formant (p.apvts, "formant", "Timbre"), width (p.apvts, "width", "Amplitud"),
      reverb (p.apvts, "reverb", "Reverb"), mix (p.apvts, "mix", "Mezcla"),
      output (p.apvts, "output", "Salida")
{
    setLookAndFeel (&laf);
    setSize (760, 430);

    addAndMakeVisible (choir);
    vowelBox.addItemList ({ "A", "E", "I", "O", "U" }, 1);
    addAndMakeVisible (vowelBox);
    vowelAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "vowel", vowelBox);

    for (auto* l : { &vowelLabel, &voicesLabel })
    {
        l->setColour (juce::Label::textColourId, textDim);
        l->setFont (AppleLookAndFeel::sans (10.5f, true));
        addAndMakeVisible (*l);
    }
    vowelLabel.setText ("VOCAL", juce::dontSendNotification);
    voicesLabel.setText ("VOCES", juce::dontSendNotification);
    voicesSlider.setColour (juce::Slider::trackColourId, red);
    addAndMakeVisible (voicesSlider);

    for (auto* k : { &detune, &vibrato, &formant, &width, &reverb, &mix, &output })
        addAndMakeVisible (*k);

    startTimerHz (30);
}

CoroAudioProcessorEditor::~CoroAudioProcessorEditor() { setLookAndFeel (nullptr); }

void CoroAudioProcessorEditor::timerCallback()
{
    const float target = juce::jlimit (0.f, 1.f, proc.outputLevel.load());
    level = juce::jmax (target, level * 0.85f);
    repaint (getLocalBounds().removeFromBottom (24));
}

void CoroAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (black);
    auto r = getLocalBounds().toFloat().reduced (16.f);

    g.setColour (panel);
    g.fillRoundedRectangle (r, 22.f);
    g.setColour (stroke);
    g.drawRoundedRectangle (r.reduced (0.5f), 22.f, 1.f);

    auto header = r.reduced (24.f, 18.f).removeFromTop (44.f);
    g.setColour (text);
    g.setFont (AppleLookAndFeel::sans (28.f, true));
    g.drawText ("Coro", header.removeFromLeft (120.f), juce::Justification::centredLeft);
    g.setColour (red);
    g.fillEllipse (header.getX() - 4.f, header.getCentreY() - 4.f, 8.f, 8.f);
    g.setColour (textDim);
    g.setFont (AppleLookAndFeel::sans (13.f));
    g.drawText ("Transforma tu voz en un coro", header.withTrimmedLeft (14.f), juce::Justification::centredLeft);

    // medidor de salida
    auto meter = juce::Rectangle<float> (r.getX() + 24.f, r.getBottom() - 22.f, r.getWidth() - 48.f, 4.f);
    g.setColour (stroke);
    g.fillRoundedRectangle (meter, 2.f);
    g.setColour (red);
    g.fillRoundedRectangle (meter.withWidth (meter.getWidth() * level), 2.f);
}

void CoroAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16).reduced (24, 18);
    r.removeFromTop (56);
    choir.setBounds (r.removeFromTop (44));
    r.removeFromTop (18);

    auto row = r.removeFromTop (48);
    auto left = row.removeFromLeft (row.getWidth() / 2 - 10);
    row.removeFromLeft (20);
    vowelLabel.setBounds (left.removeFromTop (16));
    vowelBox.setBounds (left.removeFromTop (30).withWidth (140));
    voicesLabel.setBounds (row.removeFromTop (16));
    voicesSlider.setBounds (row.removeFromTop (30));

    r.removeFromTop (12);
    r.removeFromBottom (30);
    const int w = r.getWidth() / 7;
    for (auto* k : { &detune, &vibrato, &formant, &width, &reverb, &mix, &output })
        k->setBounds (r.removeFromLeft (w));
}
