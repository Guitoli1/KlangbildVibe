#include "PluginEditor.h"

using namespace vintage;

namespace { juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); } }

//==============================================================================
VibeAudioProcessorEditor::VibeAudioProcessorEditor (VibeAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    speedKnob = &addKnob ("speed", "GESCHWINDIGKEIT", true);
    depthKnob = &addKnob ("depth", "TIEFE", true);
    modeKnob  = &addKnob ("mode", "MODUS", false);
    shapeKnob = &addKnob ("shape", "FORM", false);
    divKnob   = &addKnob ("division", "NOTENWERT", false);
    outKnob   = &addKnob ("out", "AUSGANG", false);
    modeKnob->slider.getProperties().set ("stepLabels", "TREM|HARM|CHOR|VIBR");
    divKnob->slider.getProperties().set ("stepLabels", "1/2|1/4|1/4.|1/8|1/8T|1/16");
    speedKnob->slider.getProperties().set ("stepLabels", "0,3|1|3|7|12 HZ");
    outKnob->slider.getProperties().set ("stepLabels", "-12|-6|0|+6|+12");
    modeKnob->slider.setMouseDragSensitivity (120);
    divKnob->slider.setMouseDragSensitivity (140);

    setupSwitch (syncSwitch, "sync", "IM TAKT", "FREI", lampAmber);
    setupSwitch (stereoSwitch, "stereo", "STEREO", "MONO", lampRed);

    presetBox.getProperties().set ("plate", true);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    lamp.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (lamp);
    rateLabel.setJustificationType (juce::Justification::centred);
    rateLabel.setFont (lnf.labelFont (15.0f, 0.15f));
    rateLabel.setColour (juce::Label::textColourId, ink);
    rateLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (rateLabel);

    setSize (1100, 640);
    startTimerHz (60);
}

VibeAudioProcessorEditor::~VibeAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

VibeAudioProcessorEditor::Knob& VibeAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& name, bool big)
{
    auto k = std::make_unique<Knob>();
    k->name = name;
    k->big = big;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setMouseDragSensitivity (big ? 320 : 220);
    addAndMakeVisible (k->slider);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    if (auto* rp = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void VibeAudioProcessorEditor::setupSwitch (juce::ToggleButton& b, const juce::String& paramID,
                                            const juce::String& onText, const juce::String& offText, juce::Colour lampColour)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lampColour.getARGB());
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, b));
}

//==============================================================================
void VibeAudioProcessorEditor::renderTextures()
{
    tolexImage = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), false);
    {
        juce::Image::BitmapData data (tolexImage, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (11);
        for (int y = 0; y < tolexImage.getHeight(); ++y)
            for (int x = 0; x < tolexImage.getWidth(); ++x)
            {
                const int weave = ((x + y) % 4 == 0 || (x - y + 4096) % 4 == 0) ? -5 : 0;
                const int n = rnd.nextInt (13) - 6 + weave;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, 34 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 28 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 24 + n)));
            }
    }
    if (faceplate.isEmpty())
        return;
    faceplateImage = juce::Image (juce::Image::ARGB, faceplate.getWidth(), faceplate.getHeight(), true);
    juce::Graphics g (faceplateImage);
    const auto r = faceplateImage.getBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (cream.brighter (0.06f), 0, 0, creamDark, 0, r.getHeight(), false));
    g.fillRoundedRectangle (r, 6.0f);
    juce::Random rnd (5);
    for (int y = 0; y < faceplate.getHeight(); ++y)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xff6b5534)).withAlpha (0.015f + rnd.nextFloat() * 0.035f));
        g.drawHorizontalLine (y, 0.0f, r.getWidth());
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                             juce::Colour (0x30694a1c), 0, 0, true));
    g.fillRoundedRectangle (r, 6.0f);
}

void VibeAudioProcessorEditor::drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                                             juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawFittedText (text, r.translated (0.0f, 1.0f).toNearestInt(), j, 2, 1.0f);
    g.setColour (ink);
    g.drawFittedText (text, r.toNearestInt(), j, 2, 1.0f);
}

void VibeAudioProcessorEditor::timerCallback()
{
    const float l = proc.lampLevel.load();
    if (std::abs (l - lamp.light) > 0.003f) { lamp.light = l; lamp.repaint(); }

    const bool sync = proc.apvts.getRawParameterValue ("sync")->load() > 0.5f;
    const int mode = (int) proc.apvts.getRawParameterValue ("mode")->load();
    // Regler, die gerade nichts bewirken, treten zurück
    speedKnob->slider.setAlpha (sync ? 0.45f : 1.0f);
    divKnob->slider.setAlpha (sync ? 1.0f : 0.45f);
    shapeKnob->slider.setAlpha (mode >= 2 ? 0.45f : 1.0f);

    const auto txt = juce::String (proc.currentRateHz.load(), 2).replaceCharacter ('.', ',') + " Hz";
    if (rateLabel.getText() != txt)
        rateLabel.setText (txt, juce::dontSendNotification);

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void VibeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (tolexImage, 0, 0);

    const auto scriptF = lnf.scriptFont (64.0f);
    const auto logoArea = juce::Rectangle<float> (40.0f, 8.0f, 400.0f, 84.0f);
    g.setFont (scriptF);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("Klangbild", logoArea.translated (2.0f, 3.0f), juce::Justification::centredLeft);
    g.setColour (logo);
    g.drawText ("Klangbild", logoArea, juce::Justification::centredLeft);
    const float sw = (float) juce::GlyphArrangement::getStringWidthInt (scriptF, "Klangbild");
    g.setColour (gold);
    g.setFont (lnf.labelFont (26.0f, 0.35f));
    g.drawText ("VIBE", juce::Rectangle<float> (logoArea.getX() + sw + 14.0f, logoArea.getY() + 30.0f, 140.0f, 40.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.55f));
    g.setFont (lnf.labelFont (12.0f, 0.3f));
    g.drawText (u8 ("TREMOLO \xc2\xb7 HARMONISCHES TREMOLO \xc2\xb7 VIBE CHORUS & VIBRATO"),
                juce::Rectangle<float> (44.0f, 90.0f, 600.0f, 16.0f), juce::Justification::centredLeft);
    const auto pa = presetArea.toFloat();
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText ("PROGRAMM", pa.withY (pa.getY() - 15.0f).withHeight (13.0f), juce::Justification::centredLeft);

    // Frontplatte
    const auto fp = faceplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (fp.expanded (4.0f).translated (0, 2.0f), 9.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.3f), 0, fp.getY(), gold.darker (0.6f), 0, fp.getBottom(), false));
    g.fillRoundedRectangle (fp.expanded (3.0f), 8.0f);
    g.drawImageAt (faceplateImage, faceplate.getX(), faceplate.getY());
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (fp, 6.0f, 1.0f);
    for (auto [x, y, a] : { std::tuple { fp.getX() + 12.0f, fp.getY() + 12.0f, 0.4f }, std::tuple { fp.getRight() - 12.0f, fp.getY() + 12.0f, 1.3f },
                            std::tuple { fp.getX() + 12.0f, fp.getBottom() - 12.0f, 2.2f }, std::tuple { fp.getRight() - 12.0f, fp.getBottom() - 12.0f, 0.9f } })
        VintageLookAndFeel::drawScrew (g, { x, y }, 5.0f, a);

    // Rahmen um das Lampenfenster
    const auto lb = lampBezel.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (lb.translated (2.0f, 3.0f), 12.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.25f), 0, lb.getY(), gold.darker (0.55f), 0, lb.getBottom(), false));
    g.fillRoundedRectangle (lb, 12.0f);
    for (auto pt : { lb.getTopLeft().translated (11, 11), lb.getTopRight().translated (-11, 11),
                     lb.getBottomLeft().translated (11, -11), lb.getBottomRight().translated (-11, -11) })
        VintageLookAndFeel::drawScrew (g, pt, 4.0f, pt.x * 0.013f);

    // Trennlinie
    const auto dv = divider.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawHorizontalLine ((int) dv.getY() + 1, dv.getX(), dv.getRight());
    g.setColour (ink.withAlpha (0.45f));
    g.drawHorizontalLine ((int) dv.getY(), dv.getX(), dv.getRight());

    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        drawEngraved (g, k->name, { sb.getX() - 20.0f, sb.getBottom() - 4.0f, sb.getWidth() + 40.0f, k->big ? 24.0f : 18.0f },
                      lnf.labelFont (k->big ? 18.0f : 13.0f, k->big ? 0.2f : 0.14f), juce::Justification::centred);
    }
    drawEngraved (g, "TEMPO", switchLabels[0].toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);
    drawEngraved (g, "STEREO-BILD", switchLabels[1].toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);

    const auto pl = pilotArea.toFloat();
    VintageLookAndFeel::drawJewel (g, juce::Rectangle<float> (28.0f, 28.0f).withCentre ({ pl.getX() + 16.0f, pl.getCentreY() }), lampRed, true);
    drawEngraved (g, "BETRIEB", { pl.getX() + 36.0f, pl.getCentreY() - 8.0f, 80.0f, 16.0f }, lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);
    const auto tag = juce::Rectangle<float> (120.0f, 24.0f).withCentre ({ pl.getRight() - 60.0f, pl.getCentreY() });
    g.setColour (ink.withAlpha (0.85f));
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (gold);
    g.setFont (lnf.labelFont (11.0f, 0.2f));
    g.drawText ("MODELL KB-69", tag, juce::Justification::centred);
}

void VibeAudioProcessorEditor::resized()
{
    presetArea = { 44, 128, 300, 32 };
    presetBox.setBounds (presetArea);
    faceplate = { 24, 178, getWidth() - 48, getHeight() - 202 };
    const auto in = faceplate.reduced (34, 22);

    // obere Reihe: Geschwindigkeit | Lampenfenster | Tiefe
    speedKnob->slider.setBounds (in.getX() + 20, in.getY() + 10, 230, 210);
    depthKnob->slider.setBounds (in.getRight() - 250, in.getY() + 10, 230, 210);
    lampBezel = juce::Rectangle<int> (330, 196).withCentre ({ in.getCentreX(), in.getY() + 108 });
    lamp.setBounds (lampBezel.reduced (16).withTrimmedBottom (26));
    rateLabel.setBounds (lampBezel.getX() + 16, lampBezel.getBottom() - 38, lampBezel.getWidth() - 32, 22);

    divider = { in.getX() + 10, in.getY() + 246, in.getWidth() - 20, 2 };

    // untere Reihe: Modus, Form, Notenwert, Ausgang | Schalter
    const int ry = in.getY() + 268;
    constexpr int kw = 118, kh = 114;
    Knob* row[] = { modeKnob, shapeKnob, divKnob, outKnob };
    const int rowW = 640;
    for (int i = 0; i < 4; ++i)
        row[i]->slider.setBounds (in.getX() + i * (rowW / 4) + (rowW / 4 - kw) / 2, ry, kw, kh);

    const int sx = in.getX() + rowW + 30;
    switchLabels[0] = { sx, ry + 2, 180, 16 };
    syncSwitch.setBounds (sx, ry + 22, 170, 40);
    switchLabels[1] = { sx + 190, ry + 2, 180, 16 };
    stereoSwitch.setBounds (sx + 190, ry + 22, 160, 40);
    pilotArea = { sx, ry + 84, in.getRight() - sx, 34 };
    renderTextures();
}

//==============================================================================
void VibeAudioProcessorEditor::LampWindow::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float L = juce::jlimit (0.0f, 1.0f, light);
    const juce::Colour hot (0xffffa040), core (0xfffff0c0);

    // dunkle Kammer
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff15110d), 0, b.getY(), juce::Colour (0xff060504), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 4.0f);
    // warmer Schein im Raum
    const auto c = juce::Point<float> (b.getCentreX(), b.getCentreY() + 4.0f);
    g.setGradientFill (juce::ColourGradient (hot.withAlpha (0.55f * L), c.x, c.y, hot.withAlpha (0.0f), c.x + b.getWidth() * 0.55f, c.y, true));
    g.fillRect (b);

    // zwei Fotowiderstände links und rechts, vom Licht angestrahlt
    for (float side : { -1.0f, 1.0f })
    {
        const auto pc = juce::Rectangle<float> (34.0f, 34.0f).withCentre ({ c.x + side * b.getWidth() * 0.33f, c.y });
        g.setColour (juce::Colour (0xff6b6258));
        g.fillEllipse (pc.expanded (3.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9853a).interpolatedWith (juce::Colour (0xffffd48a), L), pc.getCentreX(), pc.getY(),
                                                 juce::Colour (0xff6a3f17), pc.getCentreX(), pc.getBottom(), false));
        g.fillEllipse (pc);
        g.setColour (juce::Colour (0xff2c1a0b).withAlpha (0.85f));   // Mäander der Fotoschicht
        juce::Path m;
        for (int k = 0; k < 5; ++k)
        {
            const float y = pc.getY() + 7.0f + (float) k * 5.0f;
            m.startNewSubPath (pc.getX() + 8.0f, y);
            m.lineTo (pc.getRight() - 8.0f, y);
        }
        g.strokePath (m, juce::PathStrokeType (1.4f));
        g.setColour (juce::Colour (0xff9a9188));
        g.fillRect (pc.getCentreX() - 1.0f, pc.getBottom() + 3.0f, 2.0f, b.getBottom() - pc.getBottom());
    }

    // Glühbirne
    const auto bulb = juce::Rectangle<float> (64.0f, 74.0f).withCentre ({ c.x, c.y - 6.0f });
    for (int k = 5; k >= 1; --k)
    {
        g.setColour (hot.withAlpha (0.09f * L * (float) (6 - k)));
        g.fillEllipse (bulb.expanded ((float) k * 9.0f * (0.4f + 0.6f * L)));
    }
    g.setGradientFill (juce::ColourGradient (core.withAlpha (0.25f + 0.65f * L), bulb.getCentreX(), bulb.getCentreY(),
                                             hot.withAlpha (0.08f + 0.35f * L), bulb.getRight(), bulb.getCentreY(), true));
    g.fillEllipse (bulb);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawEllipse (bulb, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.fillEllipse (bulb.getX() + 12.0f, bulb.getY() + 10.0f, 9.0f, 18.0f);   // Glanz

    // Glühwendel
    juce::Path fil;
    const float fy = bulb.getCentreY() - 2.0f;
    fil.startNewSubPath (bulb.getCentreX() - 14.0f, fy + 14.0f);
    fil.lineTo (bulb.getCentreX() - 12.0f, fy);
    for (int k = 0; k < 6; ++k)
        fil.quadraticTo (bulb.getCentreX() - 10.0f + (float) k * 4.0f, fy - 5.0f * (k % 2 ? -1.0f : 1.0f),
                         bulb.getCentreX() - 8.0f + (float) k * 4.0f, fy);
    fil.lineTo (bulb.getCentreX() + 14.0f, fy + 14.0f);
    g.setColour (hot.withAlpha (0.4f * L));
    g.strokePath (fil, juce::PathStrokeType (5.0f));
    g.setColour (juce::Colour (0xff4a3a2a).interpolatedWith (core, juce::jlimit (0.0f, 1.0f, L * 1.3f)));
    g.strokePath (fil, juce::PathStrokeType (1.6f));

    // Fassung
    const auto base = juce::Rectangle<float> (30.0f, 18.0f).withCentre ({ bulb.getCentreX(), bulb.getBottom() + 7.0f });
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffcfc6b3), base.getX(), 0, juce::Colour (0xff6d6556), base.getRight(), 0, false));
    g.fillRoundedRectangle (base, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int k = 1; k < 4; ++k)
        g.drawHorizontalLine ((int) (base.getY() + (float) k * 4.5f), base.getX(), base.getRight());

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 2.0f);
}
