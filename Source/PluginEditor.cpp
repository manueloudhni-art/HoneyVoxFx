/*
  ==============================================================================

    HoneyVox Ad-Lib FX - Editor
    Created by Nolo's Addiction
    
    Vintage Studio Hardware Aesthetic

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

// Static member definitions for shared knob images
juce::Image HoneyVoxLookAndFeel::sharedKnobImage;
int HoneyVoxLookAndFeel::sharedKnobFrames = 0;
int HoneyVoxLookAndFeel::sharedKnobFrameHeight = 0;

juce::Image HoneyVoxLookAndFeel::sharedHoneyKnobImage;
int HoneyVoxLookAndFeel::sharedHoneyKnobFrames = 0;
int HoneyVoxLookAndFeel::sharedHoneyKnobFrameHeight = 0;

//==============================================================================
HoneyVoxLookAndFeel::HoneyVoxLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, honeyGold);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF1a1a1a));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF1A1A1A));
    setColour (juce::ComboBox::textColourId, honeyGold);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF3A3A3A));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xFF1A1A1A));
    setColour (juce::PopupMenu::textColourId, honeyGold);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFF3A3020));
}

void HoneyVoxLookAndFeel::setKnobImage (const juce::Image& img, int numFrames)
{
    sharedKnobImage = img;
    sharedKnobFrames = numFrames;
    if (sharedKnobImage.isValid() && sharedKnobFrames > 0)
        sharedKnobFrameHeight = sharedKnobImage.getHeight() / sharedKnobFrames;
}

void HoneyVoxLookAndFeel::setHoneyKnobImage (const juce::Image& img, int numFrames)
{
    sharedHoneyKnobImage = img;
    sharedHoneyKnobFrames = numFrames;
    if (sharedHoneyKnobImage.isValid() && sharedHoneyKnobFrames > 0)
        sharedHoneyKnobFrameHeight = sharedHoneyKnobImage.getHeight() / sharedHoneyKnobFrames;
}

void HoneyVoxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPosProportional, float rotaryStartAngle,
                                             float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused (slider, rotaryStartAngle, rotaryEndAngle);
    
    // Reduced padding for bigger knobs
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6);
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    
    // Determine which knob image to use
    bool useHoney = useHoneyKnob && sharedHoneyKnobImage.isValid() && sharedHoneyKnobFrames > 1;
    juce::Image& knobImg = useHoney ? sharedHoneyKnobImage : sharedKnobImage;
    int knobFrames = useHoney ? sharedHoneyKnobFrames : sharedKnobFrames;
    int knobFrameH = useHoney ? sharedHoneyKnobFrameHeight : sharedKnobFrameHeight;
    
    // === USE FILMSTRIP PNG IF AVAILABLE ===
    if (knobImg.isValid() && knobFrames > 1)
    {
        float frameIndexF = sliderPosProportional * (float)(knobFrames - 1);
        int frameIndex = juce::jlimit (0, knobFrames - 1, (int)std::round(frameIndexF));
        
        int frameWidth = knobImg.getWidth();
        int srcY = frameIndex * knobFrameH;
        
        // Scale to fit bounds
        float availableSize = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 4;
        float scale = availableSize / (float)juce::jmax(frameWidth, knobFrameH);
        int destW = (int)(frameWidth * scale);
        int destH = (int)(knobFrameH * scale);
        int destX = (int)(centreX - destW / 2);
        int destY = (int)(centreY - destH / 2);
        
        // Draw value ring behind the knob
        float ringRadius = availableSize / 2.0f + 2;
        float ringWidth = 3.0f;
        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
        g.setColour (juce::Colour (0xFF2A2520));
        juce::Path bgRing;
        bgRing.addCentredArc (centreX, centreY, ringRadius, ringRadius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
        g.strokePath (bgRing, juce::PathStrokeType (ringWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        if (sliderPosProportional > 0.0f)
        {
            g.setColour (juce::Colour (0xFFFF9933));
            juce::Path valueRing;
            valueRing.addCentredArc (centreX, centreY, ringRadius, ringRadius, 0.0f,
                                     rotaryStartAngle, angle, true);
            g.strokePath (valueRing, juce::PathStrokeType (ringWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        
        g.drawImage (knobImg, destX, destY, destW, destH,
                     0, srcY, frameWidth, knobFrameH);
        
        return;
    }
    
    // === FALLBACK: Draw programmatic knob ===
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f - 4;
    float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    
    // Value ring - smaller to avoid clipping
    float ringRadius = radius + 3;
    float ringWidth = 3.0f;
    
    g.setColour (juce::Colour (0xFF2A2520));
    juce::Path bgRing;
    bgRing.addCentredArc (centreX, centreY, ringRadius, ringRadius, 0.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
    g.strokePath (bgRing, juce::PathStrokeType (ringWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    if (sliderPosProportional > 0.0f)
    {
        juce::ColourGradient ringGradient (
            juce::Colour (0xFFFF9933), centreX - ringRadius, centreY,
            juce::Colour (0xFFFFAA44), centreX + ringRadius, centreY,
            false
        );
        g.setGradientFill (ringGradient);
        
        juce::Path valueRing;
        valueRing.addCentredArc (centreX, centreY, ringRadius, ringRadius, 0.0f,
                                 rotaryStartAngle, angle, true);
        g.strokePath (valueRing, juce::PathStrokeType (ringWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    
    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (centreX - radius + 2, centreY - radius + 3, radius * 2, radius * 2);
    
    // Outer chrome bezel
    juce::ColourGradient bezelGradient (
        juce::Colour (0xFF7A7A7A), centreX - radius, centreY - radius,
        juce::Colour (0xFF3A3A3A), centreX + radius, centreY + radius,
        true
    );
    g.setGradientFill (bezelGradient);
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2, radius * 2);
    
    // Main knob body
    float innerRadius = radius * 0.82f;
    juce::ColourGradient knobGradient (
        juce::Colour (0xFF3A3028), centreX - innerRadius * 0.5f, centreY - innerRadius * 0.5f,
        juce::Colour (0xFF1A1612), centreX + innerRadius * 0.5f, centreY + innerRadius * 0.5f,
        true
    );
    g.setGradientFill (knobGradient);
    g.fillEllipse (centreX - innerRadius, centreY - innerRadius, innerRadius * 2, innerRadius * 2);
    
    // Top highlight
    juce::ColourGradient topShine (
        juce::Colours::white.withAlpha (0.1f), centreX, centreY - innerRadius * 0.6f,
        juce::Colours::transparentWhite, centreX, centreY,
        false
    );
    g.setGradientFill (topShine);
    g.fillEllipse (centreX - innerRadius * 0.7f, centreY - innerRadius * 0.85f, innerRadius * 1.4f, innerRadius * 0.8f);
    
    // Knurled grip
    g.setColour (juce::Colour (0xFF2A2420).withAlpha (0.4f));
    for (int i = 0; i < 24; ++i)
    {
        float lineAngle = (float)i * juce::MathConstants<float>::twoPi / 24.0f;
        float x1 = centreX + std::cos (lineAngle) * (innerRadius * 0.5f);
        float y1 = centreY + std::sin (lineAngle) * (innerRadius * 0.5f);
        float x2 = centreX + std::cos (lineAngle) * (innerRadius * 0.88f);
        float y2 = centreY + std::sin (lineAngle) * (innerRadius * 0.88f);
        g.drawLine (x1, y1, x2, y2, 0.8f);
    }
    
    // White pointer
    juce::Path pointer;
    float pointerLength = innerRadius * 0.65f;
    float pointerWidth = 3.0f;
    pointer.addRoundedRectangle (-pointerWidth / 2, -innerRadius * 0.15f - pointerLength, 
                                  pointerWidth, pointerLength, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centreX, centreY));
    
    // Center cap
    float capRadius = innerRadius * 0.2f;
    g.setColour (juce::Colour (0xFF2A2420));
    g.fillEllipse (centreX - capRadius, centreY - capRadius, capRadius * 2, capRadius * 2);
}

void HoneyVoxLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    auto bounds = button.getLocalBounds().toFloat();
    
    bool isOn = !button.getToggleState();  // Inverted for bypass logic
    
    // Layout: [OFF text] [switch] [hexagon light] - all centered
    float totalWidth = 65.0f;  // Total width of all elements
    float startX = bounds.getCentreX() - totalWidth / 2;
    
    float switchWidth = 20.0f;
    float switchHeight = 10.0f;
    float switchX = startX + 22;  // After OFF text
    float switchY = bounds.getCentreY() - switchHeight / 2;
    
    // === "OFF" TEXT (to the LEFT of switch) ===
    g.setFont (juce::Font (juce::FontOptions (8.0f, juce::Font::bold)));
    if (isOn)
        g.setColour (juce::Colour (0xFF555555));  // Muted gray when ON
    else
        g.setColour (juce::Colour (0xFFDD4444));  // Red when OFF
    g.drawText ("OFF", (int)startX, (int)(switchY - 2), 20, (int)(switchHeight + 4), juce::Justification::centred);
    
    // === INTERESTING SWITCH - beveled 3D look ===
    g.setColour (juce::Colour (0xFF050505));
    g.fillRoundedRectangle (switchX - 1, switchY - 1, switchWidth + 2, switchHeight + 2, 5.0f);
    
    juce::ColourGradient trackGrad (
        juce::Colour (0xFF0A0A0A), switchX, switchY,
        juce::Colour (0xFF252525), switchX, switchY + switchHeight,
        false
    );
    g.setGradientFill (trackGrad);
    g.fillRoundedRectangle (switchX, switchY, switchWidth, switchHeight, 4.0f);
    
    g.setColour (juce::Colour (0xFF1A1A1A));
    g.fillRoundedRectangle (switchX + 1, switchY + 1, switchWidth - 2, switchHeight - 2, 3.0f);
    
    // Switch thumb
    float thumbWidth = 9.0f;
    float thumbX = isOn ? (switchX + switchWidth - thumbWidth - 1) : (switchX + 1);
    float thumbY = switchY + 1;
    float thumbH = switchHeight - 2;
    
    g.setColour (juce::Colour (0x40000000));
    g.fillRoundedRectangle (thumbX + 0.5f, thumbY + 0.5f, thumbWidth, thumbH, 2.5f);
    
    juce::ColourGradient thumbGrad (
        juce::Colour (0xFFAAAAAA), thumbX, thumbY,
        juce::Colour (0xFF666666), thumbX, thumbY + thumbH,
        false
    );
    g.setGradientFill (thumbGrad);
    g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbH, 2.5f);
    
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawHorizontalLine ((int)(thumbY + 1), thumbX + 2, thumbX + thumbWidth - 2);
    
    g.setColour (juce::Colour (0x30000000));
    for (float line = thumbX + 2; line < thumbX + thumbWidth - 2; line += 2)
        g.drawVerticalLine ((int)line, thumbY + 2, thumbY + thumbH - 2);
    
    g.setColour (juce::Colour (0xFF0A0A0A));
    g.drawRoundedRectangle (switchX, switchY, switchWidth, switchHeight, 4.0f, 0.5f);
    
    // === SMALLER HEXAGON INDICATOR LIGHT ===
    float lightSize = 8.0f;  // Smaller
    float lightX = switchX + switchWidth + 5;
    float lightY = switchY + (switchHeight - lightSize) / 2;
    float lightCenterX = lightX + lightSize / 2;
    float lightCenterY = lightY + lightSize / 2;
    float hexRadius = lightSize / 2;
    
    // Create hexagon path
    juce::Path hexPath;
    for (int i = 0; i < 6; ++i)
    {
        float angle = juce::MathConstants<float>::pi / 6.0f + i * juce::MathConstants<float>::pi / 3.0f;
        float hx = lightCenterX + hexRadius * std::cos(angle);
        float hy = lightCenterY + hexRadius * std::sin(angle);
        if (i == 0)
            hexPath.startNewSubPath(hx, hy);
        else
            hexPath.lineTo(hx, hy);
    }
    hexPath.closeSubPath();
    
    // Hex bezel
    g.setColour (juce::Colour (0xFF080808));
    g.strokePath (hexPath, juce::PathStrokeType (2.5f));
    
    if (isOn)
    {
        // Green glow
        juce::ColourGradient glowGrad (
            juce::Colour (0xA000FF66), lightCenterX, lightCenterY,
            juce::Colours::transparentBlack, lightCenterX - lightSize, lightCenterY - lightSize,
            true
        );
        g.setGradientFill (glowGrad);
        g.fillEllipse (lightX - 5, lightY - 5, lightSize + 10, lightSize + 10);
        
        // Bright green hex
        juce::ColourGradient lightGrad (
            juce::Colour (0xFF99FF99), lightCenterX - 1, lightCenterY - 1,
            juce::Colour (0xFF00CC44), lightCenterX + 1, lightCenterY + 1,
            true
        );
        g.setGradientFill (lightGrad);
        g.fillPath (hexPath);
    }
    else
    {
        // Dark hex
        juce::ColourGradient darkGrad (
            juce::Colour (0xFF3A3530), lightCenterX - 1, lightCenterY - 1,
            juce::Colour (0xFF252220), lightCenterX + 1, lightCenterY + 1,
            true
        );
        g.setGradientFill (darkGrad);
        g.fillPath (hexPath);
    }
    
    // Glass highlight
    g.setColour (juce::Colours::white.withAlpha (isOn ? 0.3f : 0.1f));
    g.fillEllipse (lightCenterX - 2, lightCenterY - 2.5f, 4.0f, 3.0f);
}

//==============================================================================
VintageSwitch::VintageSwitch()
{
    button.setLookAndFeel (&lf);
    button.setClickingTogglesState (true);
    addAndMakeVisible (button);
}

void VintageSwitch::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void VintageSwitch::mouseDown (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    button.setToggleState (!button.getToggleState(), juce::sendNotification);
}

void VintageSwitch::resized()
{
    button.setBounds (getLocalBounds());
}

//==============================================================================
VintageScreen::VintageScreen (const juce::String& text) : displayText (text)
{
}

void VintageScreen::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Screen bezel - dark metal frame
    g.setColour (juce::Colour (0xFF0A0808));
    g.fillRoundedRectangle (bounds, 3.0f);
    
    // Inner bezel highlight
    g.setColour (juce::Colour (0xFF2A2520));
    g.drawRoundedRectangle (bounds.reduced (1), 2.5f, 1.0f);
    
    // Screen background - dark
    auto screenBounds = bounds.reduced (3);
    g.setColour (juce::Colour (0xFF080805));
    g.fillRoundedRectangle (screenBounds, 2.0f);
    
    // Glowing hexagonal dot matrix - slow wavy animation
    float hexSize = 3.0f;
    float rowHeight = hexSize * 1.6f;
    float colWidth = hexSize * 1.9f;
    int row = 0;
    
    // Large padding to completely avoid screws
    float hPad = 14.0f;
    float vPad = 4.0f;
    
    for (float y = screenBounds.getY() + vPad; y < screenBounds.getBottom() - vPad; y += rowHeight)
    {
        float xOffset = (row % 2) * (colWidth / 2);
        int col = 0;
        for (float x = screenBounds.getX() + hPad + xOffset; x < screenBounds.getRight() - hPad; x += colWidth)
        {
            // Wavy animation - wave moves across the screen
            float waveX = (x - screenBounds.getX()) * 0.06f;
            float waveY = (y - screenBounds.getY()) * 0.1f;
            float wave = std::sin(animPhase + waveX) * std::cos(animPhase * 0.7f + waveY);
            
            // Brighter intensity
            float intensity = 0.55f + 0.35f * wave;
            
            // Brighter warm honey color
            juce::Colour hexColor = juce::Colour(0xFFFFAA33).withAlpha(0.18f + intensity * 0.25f);
            
            // Draw glow behind brighter hexes
            if (intensity > 0.6f)
            {
                g.setColour(juce::Colour(0xFFFFBB55).withAlpha((intensity - 0.5f) * 0.35f));
                g.fillEllipse(x - hexSize * 0.5f, y - hexSize * 0.5f, hexSize * 1.5f, hexSize * 1.5f);
            }
            
            // Draw small hexagon
            juce::Path hex;
            for (int i = 0; i < 6; ++i)
            {
                float angle = (float)i * juce::MathConstants<float>::pi / 3.0f;
                float hx = x + (hexSize/2) * std::cos(angle);
                float hy = y + (hexSize/2) * std::sin(angle);
                if (i == 0) hex.startNewSubPath(hx, hy);
                else hex.lineTo(hx, hy);
            }
            hex.closeSubPath();
            
            g.setColour(hexColor);
            g.fillPath(hex);
            
            col++;
        }
        row++;
    }
    
    // Text size based on content - BIGGER for ECHO, DELAY; smaller for UNDERWATER
    float fontSize = 14.0f;  // Default big for short text
    if (displayText.equalsIgnoreCase("UNDERWATER"))
        fontSize = 8.0f;  // Smaller for UNDERWATER
    else if (displayText.length() > 10)
        fontSize = 9.0f;
    else if (displayText.length() > 7)
        fontSize = 11.0f;
    else if (displayText.equalsIgnoreCase("ECHO") || displayText.equalsIgnoreCase("DELAY"))
        fontSize = 15.0f;  // Extra big for ECHO/DELAY
    
    // Honey gold glow effect (like enabled light)
    g.setColour (juce::Colour (0x60D4A030));
    g.setFont (juce::Font (juce::FontOptions (fontSize, juce::Font::bold)));
    g.drawText (displayText, screenBounds.translated (0, 1), juce::Justification::centred);
    g.drawText (displayText, screenBounds.translated (1, 0), juce::Justification::centred);
    g.drawText (displayText, screenBounds.translated (-1, 0), juce::Justification::centred);
    g.drawText (displayText, screenBounds.translated (0, -1), juce::Justification::centred);
    
    // Main text - bright honey gold
    g.setColour (juce::Colour (0xFFFFBB33));
    g.drawText (displayText, screenBounds, juce::Justification::centred);
    
    // Subtle screen reflection/glare at top
    juce::ColourGradient glare (
        juce::Colours::white.withAlpha (0.06f), screenBounds.getCentreX(), screenBounds.getY(),
        juce::Colours::transparentWhite, screenBounds.getCentreX(), screenBounds.getY() + screenBounds.getHeight() * 0.4f,
        false
    );
    g.setGradientFill (glare);
    g.fillRoundedRectangle (screenBounds.getX(), screenBounds.getY(), screenBounds.getWidth(), screenBounds.getHeight() * 0.5f, 2.0f);
}

//==============================================================================
ParameterKnob::ParameterKnob (const juce::String& labelText, const juce::String& suffixText)
    : suffix (suffixText)
{
    setLookAndFeel (&lookAndFeel);
    
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (knob);
    
    // Label - neutral gray/tan (NOT honey gold)
    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xFFBBBBAA));  // Neutral tan/gray
    label.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    addAndMakeVisible (label);
    
    // Value label - honey gold for values
    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFD4A030));
    valueLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xFF0D0D08));
    valueLabel.setColour (juce::Label::outlineColourId, juce::Colour (0xFF2A2520));
    valueLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    valueLabel.setEditable (true);
    addAndMakeVisible (valueLabel);
    
    knob.onValueChange = [this]() {
        juce::String text = juce::String (knob.getValue(), suffix == "ms" ? 0 : 1);
        if (suffix.isNotEmpty()) text += suffix;
        valueLabel.setText (text, juce::dontSendNotification);
    };
    
    valueLabel.onTextChange = [this]() {
        double val = valueLabel.getText().getDoubleValue();
        knob.setValue (val, juce::sendNotification);
    };
}

ParameterKnob::~ParameterKnob()
{
    setLookAndFeel (nullptr);
}

void ParameterKnob::resized()
{
    auto bounds = getLocalBounds();
    
    // Fixed knob size for consistency
    int knobSize = juce::jmin(bounds.getWidth() - 4, bounds.getHeight() - 28);
    knobSize = juce::jmax(knobSize, 65);
    
    label.setBounds (0, 2, bounds.getWidth(), 12);
    knob.setBounds ((bounds.getWidth() - knobSize) / 2, 14, knobSize, knobSize);
    valueLabel.setBounds ((bounds.getWidth() - 50) / 2, 14 + knobSize + 2, 50, 14);
}

void ParameterKnob::paint (juce::Graphics& g)
{
    // Subtle glow behind value label only
    auto valueBounds = valueLabel.getBounds().toFloat();
    g.setColour (juce::Colour (0x25D4A030));
    g.fillRoundedRectangle (valueBounds.expanded (1), 2.0f);
}

//==============================================================================
EffectSection::EffectSection (const juce::String& name) : sectionName (name), screen (name)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (bypassSwitch);
    addAndMakeVisible (screen);
}

EffectSection::~EffectSection()
{
    setLookAndFeel (nullptr);
}

void EffectSection::addKnob (ParameterKnob* knob)
{
    knobRefs.push_back (knob);
    addAndMakeVisible (knob);
}

void EffectSection::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Brushed metal background
    juce::ColourGradient metalGradient (
        juce::Colour (0xFF4A4540), bounds.getX(), bounds.getY(),
        juce::Colour (0xFF2A2825), bounds.getX(), bounds.getBottom(),
        false
    );
    g.setGradientFill (metalGradient);
    g.fillRoundedRectangle (bounds, 6.0f);
    
    // Brushed metal texture - horizontal lines
    for (float i = bounds.getY(); i < bounds.getBottom(); i += 2.0f)
    {
        float brightness = 0.95f + 0.1f * std::sin(i * 0.8f);
        g.setColour (juce::Colours::white.withAlpha (0.03f * brightness));
        g.drawHorizontalLine (static_cast<int>(i), bounds.getX() + 2, bounds.getRight() - 2);
    }
    
    // Inner shadow at top
    juce::ColourGradient innerShadow (
        juce::Colours::black.withAlpha (0.3f), bounds.getX(), bounds.getY(),
        juce::Colours::transparentBlack, bounds.getX(), bounds.getY() + 15,
        false
    );
    g.setGradientFill (innerShadow);
    g.fillRoundedRectangle (bounds.getX(), bounds.getY(), bounds.getWidth(), 15, 6.0f);
    
    // Highlight at bottom edge
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (static_cast<int>(bounds.getBottom() - 2), bounds.getX() + 6, bounds.getRight() - 6);
    
    // Border - dark edge
    g.setColour (juce::Colour (0xFF1A1815));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.5f);
    
    // Subtle inner highlight
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 5.0f, 1.0f);
    
    // === SCREWS - 6 screws around the frame ===
    float screwSize = 10.0f;
    float inset = 8.0f;
    
    auto drawScrew = [&](float sx, float sy) {
        // Recessed hole
        g.setColour (juce::Colour (0xFF0A0808));
        g.fillEllipse (sx - 1, sy - 1, screwSize + 2, screwSize + 2);
        
        // Screw head
        juce::ColourGradient screwGrad (
            juce::Colour (0xFF908070), sx + screwSize * 0.25f, sy + screwSize * 0.25f,
            juce::Colour (0xFF504540), sx + screwSize * 0.75f, sy + screwSize * 0.75f,
            true
        );
        g.setGradientFill (screwGrad);
        g.fillEllipse (sx, sy, screwSize, screwSize);
        
        // Phillips cross
        g.setColour (juce::Colour (0xFF1A1510));
        g.fillRect (sx + screwSize * 0.5f - 0.5f, sy + screwSize * 0.2f, 1.0f, screwSize * 0.6f);
        g.fillRect (sx + screwSize * 0.2f, sy + screwSize * 0.5f - 0.5f, screwSize * 0.6f, 1.0f);
        
        // Highlight
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        g.drawEllipse (sx + 0.5f, sy + 0.5f, screwSize - 1, screwSize - 1, 0.5f);
    };
    
    // Top row - 2 screws
    drawScrew (bounds.getX() + inset, bounds.getY() + inset);
    drawScrew (bounds.getRight() - inset - screwSize, bounds.getY() + inset);
    
    // Middle row - 2 screws (for wider sections)
    if (bounds.getWidth() > 150)
    {
        drawScrew (bounds.getX() + inset, bounds.getCentreY() - screwSize / 2);
        drawScrew (bounds.getRight() - inset - screwSize, bounds.getCentreY() - screwSize / 2);
    }
    
    // Bottom row - 2 screws
    drawScrew (bounds.getX() + inset, bounds.getBottom() - inset - screwSize);
    drawScrew (bounds.getRight() - inset - screwSize, bounds.getBottom() - inset - screwSize);
}

void EffectSection::resized()
{
    auto bounds = getLocalBounds();
    
    // Larger horizontal padding so screen box doesn't touch screws
    int hPadding = 22;
    int vPadding = 6;
    
    auto contentBounds = bounds.reduced(hPadding, vPadding);
    
    // Screen at top - bigger text display
    screen.setBounds (contentBounds.removeFromTop (24));
    contentBounds.removeFromTop (2);
    
    // Switch at bottom
    auto bottomArea = contentBounds.removeFromBottom (28);
    bypassSwitch.setBounds (bottomArea.withSizeKeepingCentre (70, 22));
    
    // Knobs in middle - ALL SAME SIZE, as big as possible
    if (!knobRefs.empty())
    {
        int numKnobs = (int)knobRefs.size();
        int knobWidth = contentBounds.getWidth() / numKnobs;
        // Use fixed large size for all knobs
        int knobSize = 95;  // Fixed size for consistency
        
        for (auto* knob : knobRefs)
        {
            auto knobArea = contentBounds.removeFromLeft (knobWidth);
            knob->setBounds (knobArea.withSizeKeepingCentre(knobSize, knobSize));
        }
    }
}

//==============================================================================
HoneyVoxAudioProcessorEditor::HoneyVoxAudioProcessorEditor (HoneyVoxAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);
    
    // Load logo PNG
    logoImage = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);
    logoLoaded = logoImage.isValid();
    
    // Helper lambda to load filmstrip and detect frames
    auto loadFilmstrip = [](const char* data, int size) -> std::pair<juce::Image, int> {
        juce::Image img = juce::ImageCache::getFromMemory(data, size);
        int numFrames = 1;
        if (img.isValid())
        {
            int imgW = img.getWidth();
            int imgH = img.getHeight();
            int frameCounts[] = { 128, 127, 101, 100, 65, 64, 63, 61, 60, 51, 50, 49, 48, 32, 31, 25, 24, 16 };
            for (int fc : frameCounts)
            {
                if (imgH % fc == 0)
                {
                    int frameH = imgH / fc;
                    if (frameH >= imgW * 0.5 && frameH <= imgW * 2.0)
                    {
                        numFrames = fc;
                        break;
                    }
                }
            }
            if (numFrames == 1 && imgH > imgW)
                numFrames = imgH / imgW;
        }
        return {img, numFrames};
    };
    
    // Load knob filmstrips from BinaryData
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        juce::String resName = BinaryData::namedResourceList[i];
        int size = 0;
        const char* data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], size);
        
        if (data != nullptr && size > 0)
        {
            // Check for honeyknob first (for saturation)
            if (resName.containsIgnoreCase("honeyknob"))
            {
                auto [img, frames] = loadFilmstrip(data, size);
                if (frames > 1)
                {
                    HoneyVoxLookAndFeel::setHoneyKnobImage(img, frames);
                    DBG("Loaded honeyknob: " + juce::String(img.getWidth()) + "x" + juce::String(img.getHeight()) + " with " + juce::String(frames) + " frames");
                }
            }
            // Then check for regular knob
            else if (resName.containsIgnoreCase("knob") && !resName.containsIgnoreCase("honey"))
            {
                auto [img, frames] = loadFilmstrip(data, size);
                if (frames > 1)
                {
                    HoneyVoxLookAndFeel::setKnobImage(img, frames);
                    DBG("Loaded knob: " + juce::String(img.getWidth()) + "x" + juce::String(img.getHeight()) + " with " + juce::String(frames) + " frames");
                }
            }
        }
    }
    
    // Phone section
    phoneSection.addKnob (&phoneKnob);
    addAndMakeVisible (phoneSection);
    
    phoneModeBox.addItem ("Rotary", 1);
    phoneModeBox.addItem ("Touch-Tone", 2);
    phoneModeBox.addItem ("Mobile", 3);
    phoneModeBox.setSelectedId (1);
    phoneModeBox.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (phoneModeBox);
    
    // ERAs label - hidden, text drawn on plate in paint()
    phoneModeLabel.setText ("", juce::dontSendNotification);
    phoneModeLabel.setVisible (false);
    
    // Delay section
    delaySection.addKnob (&delayTimeKnob);
    delaySection.addKnob (&delayFeedbackKnob);
    delaySection.addKnob (&delayMixKnob);
    addAndMakeVisible (delaySection);
    
    addAndMakeVisible (pingPongSwitch);
    pingPongLabel.setText ("P-PONG", juce::dontSendNotification);
    pingPongLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFCCCCCC));
    pingPongLabel.setFont (juce::Font (juce::FontOptions (7.0f, juce::Font::bold)));
    pingPongLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (pingPongLabel);
    
    // Delay sync controls - visible under TIME knob
    addAndMakeVisible (delaySyncSwitch);
    delaySyncLabel.setText ("SYNC", juce::dontSendNotification);
    delaySyncLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFD4A030));  // Honey gold
    delaySyncLabel.setFont (juce::Font (juce::FontOptions (8.0f, juce::Font::bold)));
    delaySyncLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (delaySyncLabel);
    
    delayDivisionBox.addItem ("1/1", 1);
    delayDivisionBox.addItem ("1/2", 2);
    delayDivisionBox.addItem ("1/2 D", 3);
    delayDivisionBox.addItem ("1/2 T", 4);
    delayDivisionBox.addItem ("1/4", 5);
    delayDivisionBox.addItem ("1/4 D", 6);
    delayDivisionBox.addItem ("1/4 T", 7);
    delayDivisionBox.addItem ("1/8", 8);
    delayDivisionBox.addItem ("1/8 D", 9);
    delayDivisionBox.addItem ("1/8 T", 10);
    delayDivisionBox.addItem ("1/16", 11);
    delayDivisionBox.addItem ("1/16 D", 12);
    delayDivisionBox.addItem ("1/16 T", 13);
    delayDivisionBox.setSelectedId (5);  // Default 1/4
    delayDivisionBox.setLookAndFeel (&lookAndFeel);
    
    // When division changes, update the TIME knob to show corresponding ms value
    delayDivisionBox.onChange = [this]() {
        // Check if sync is enabled
        bool syncOn = audioProcessor.apvts.getRawParameterValue("delaySync")->load() > 0.5f;
        if (syncOn)
        {
            // Get BPM (default 120 if not available)
            double bpm = 120.0;
            if (auto* playHead = audioProcessor.getPlayHead())
            {
                auto posInfo = playHead->getPosition();
                if (posInfo.hasValue())
                {
                    if (auto hostBpm = posInfo->getBpm())
                        bpm = *hostBpm;
                }
            }
            
            // Calculate ms for selected division
            int division = delayDivisionBox.getSelectedId() - 1;  // 0-indexed
            double beatMs = 60000.0 / bpm;
            double ms = beatMs;  // Default 1/4
            
            switch (division)
            {
                case 0:  ms = beatMs * 4.0; break;       // 1/1
                case 1:  ms = beatMs * 2.0; break;       // 1/2
                case 2:  ms = beatMs * 3.0; break;       // 1/2 D
                case 3:  ms = beatMs * 4.0 / 3.0; break; // 1/2 T
                case 4:  ms = beatMs; break;             // 1/4
                case 5:  ms = beatMs * 1.5; break;       // 1/4 D
                case 6:  ms = beatMs * 2.0 / 3.0; break; // 1/4 T
                case 7:  ms = beatMs * 0.5; break;       // 1/8
                case 8:  ms = beatMs * 0.75; break;      // 1/8 D
                case 9:  ms = beatMs / 3.0; break;       // 1/8 T
                case 10: ms = beatMs * 0.25; break;      // 1/16
                case 11: ms = beatMs * 0.375; break;     // 1/16 D
                case 12: ms = beatMs / 6.0; break;       // 1/16 T
            }
            
            // Update the TIME knob
            delayTimeKnob.getSlider().setValue (juce::jlimit(50.0, 2000.0, ms), juce::sendNotification);
        }
    };
    addAndMakeVisible (delayDivisionBox);
    
    // Other sections
    saturationSection.addKnob (&saturationKnob);
    addAndMakeVisible (saturationSection);
    
    underwaterSection.addKnob (&underwaterKnob);
    addAndMakeVisible (underwaterSection);
    
    // Output
    addAndMakeVisible (outputScreen);
    addAndMakeVisible (outputSwitch);
    addAndMakeVisible (outputKnob);
    
    // Attachments
    phoneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "phone", phoneKnob.getSlider());
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "delayTime", delayTimeKnob.getSlider());
    delayFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "delayFeedback", delayFeedbackKnob.getSlider());
    delayMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "delayMix", delayMixKnob.getSlider());
    saturationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "saturation", saturationKnob.getSlider());
    underwaterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "underwater", underwaterKnob.getSlider());
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "outputGain", outputKnob.getSlider());
    
    phoneBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "phoneBypass", phoneSection.getBypassButton());
    delayBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "delayBypass", delaySection.getBypassButton());
    saturationBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "saturationBypass", saturationSection.getBypassButton());
    underwaterBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "underwaterBypass", underwaterSection.getBypassButton());
    outputBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "outputBypass", outputSwitch.getButton());
    pingPongAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "delayPingPong", pingPongSwitch.getButton());
    delaySyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "delaySync", delaySyncSwitch.getButton());
    phoneModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "phoneMode", phoneModeBox);
    delayDivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "delayDivision", delayDivisionBox);
    
    // Preset controls
    presetBox.setLookAndFeel (&lookAndFeel);
    presetBox.setTextWhenNothingSelected ("Presets...");
    presetBox.onChange = [this]() { loadPreset(); };
    addAndMakeVisible (presetBox);
    
    savePresetButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF3A3530));
    savePresetButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFD4A030));
    savePresetButton.onClick = [this]() { savePreset(); };
    addAndMakeVisible (savePresetButton);
    
    updatePresetList();
    
    // Start timer for hex glow animation
    startTimer(50);
    
    setSize (980, 720);
}

HoneyVoxAudioProcessorEditor::~HoneyVoxAudioProcessorEditor()
{
    presetBox.setLookAndFeel (nullptr);
    delayDivisionBox.setLookAndFeel (nullptr);
    phoneModeBox.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void HoneyVoxAudioProcessorEditor::savePreset()
{
    auto presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("HoneyVoxFX Presets");
    if (!presetDir.exists())
        presetDir.createDirectory();
    
    fileChooser = std::make_unique<juce::FileChooser>("Save Preset", presetDir, "*.hvpreset");
    
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File{})
            {
                auto finalFile = file.hasFileExtension(".hvpreset") ? file : file.withFileExtension(".hvpreset");
                auto state = audioProcessor.apvts.copyState();
                std::unique_ptr<juce::XmlElement> xml(state.createXml());
                if (xml != nullptr)
                    xml->writeTo(finalFile);
                updatePresetList();
            }
        });
}

void HoneyVoxAudioProcessorEditor::loadPreset()
{
    int selectedId = presetBox.getSelectedId();
    if (selectedId <= 0) return;
    
    juce::String presetName = presetBox.getItemText(presetBox.getSelectedItemIndex());
    auto presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("HoneyVoxFX Presets");
    auto file = presetDir.getChildFile(presetName + ".hvpreset");
    
    if (file.existsAsFile())
    {
        std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
        if (xml != nullptr)
        {
            auto state = juce::ValueTree::fromXml(*xml);
            if (state.isValid())
                audioProcessor.apvts.replaceState(state);
        }
    }
}

void HoneyVoxAudioProcessorEditor::updatePresetList()
{
    presetBox.clear();
    
    auto presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("HoneyVoxFX Presets");
    if (!presetDir.exists())
        presetDir.createDirectory();
    
    auto files = presetDir.findChildFiles(juce::File::findFiles, false, "*.hvpreset");
    files.sort();
    
    for (int i = 0; i < files.size(); ++i)
        presetBox.addItem(files[i].getFileNameWithoutExtension(), i + 1);
}

void HoneyVoxAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    // Color screw toggle
    if (colorScrewBounds.contains (event.position))
    {
        purpleMode = !purpleMode;
        repaint();
    }
    
    // Hex bomb easter egg
    if (hexBombScrewBounds.contains (event.position))
    {
        showHexBomb = true;
        hexBombTimer = 0;
        startTimer (50);
        repaint();
    }
    
    // Left bee - previous preset
    if (leftBeeBounds.contains (event.position))
    {
        leftBeePressed = true;
        // Navigate to previous preset
        int currentId = presetBox.getSelectedId();
        if (currentId > 1)
            presetBox.setSelectedId(currentId - 1);
        else if (presetBox.getNumItems() > 0)
            presetBox.setSelectedId(presetBox.getNumItems());  // Wrap to last
        repaint();
    }
    
    // Right bee - next preset
    if (rightBeeBounds.contains (event.position))
    {
        rightBeePressed = true;
        // Navigate to next preset
        int currentId = presetBox.getSelectedId();
        if (currentId < presetBox.getNumItems())
            presetBox.setSelectedId(currentId + 1);
        else if (presetBox.getNumItems() > 0)
            presetBox.setSelectedId(1);  // Wrap to first
        repaint();
    }
    
    // Cable screw drag start
    if (cableScrewBounds.contains (event.position))
    {
        cableScrewDragging = true;
        lastCableScrewY = event.position.y;
    }
}

void HoneyVoxAudioProcessorEditor::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    
    // Release bee presses
    if (leftBeePressed || rightBeePressed)
    {
        leftBeePressed = false;
        rightBeePressed = false;
        repaint();
    }
    
    // Release cable screw drag
    cableScrewDragging = false;
}

void HoneyVoxAudioProcessorEditor::mouseDrag (const juce::MouseEvent& event)
{
    // Cable screw rotation
    if (cableScrewDragging)
    {
        float deltaY = lastCableScrewY - event.position.y;
        cableScrewAngle += deltaY * 0.01f;
        cableScrewAngle = juce::jlimit(0.0f, 1.0f, cableScrewAngle);
        lastCableScrewY = event.position.y;
        
        // Update cable hum amount in processor
        audioProcessor.cableHumAmount.store(cableScrewAngle);
        
        repaint();
    }
}

void HoneyVoxAudioProcessorEditor::timerCallback()
{
    // Hex bomb animation
    if (showHexBomb)
    {
        hexBombTimer++;
        if (hexBombTimer > 40)
        {
            showHexBomb = false;
        }
    }
    
    // Hex glow animation - faster wavy
    hexGlowPhase += 0.07f;
    if (hexGlowPhase > juce::MathConstants<float>::twoPi * 10.0f)
        hexGlowPhase -= juce::MathConstants<float>::twoPi * 10.0f;
    
    // Update all screens with animation phase
    phoneSection.getScreen().setAnimPhase(hexGlowPhase);
    delaySection.getScreen().setAnimPhase(hexGlowPhase);
    saturationSection.getScreen().setAnimPhase(hexGlowPhase);
    underwaterSection.getScreen().setAnimPhase(hexGlowPhase);
    outputScreen.setAnimPhase(hexGlowPhase);
    
    repaint();
}

void HoneyVoxAudioProcessorEditor::drawCornerScrew (juce::Graphics& g, float x, float y, bool isColorScrew)
{
    float size = 16.0f;
    
    g.setColour (juce::Colour (0xFF1A1510));
    g.fillEllipse (x - 1, y - 1, size + 2, size + 2);
    
    juce::Colour screwColor = isColorScrew ? 
        (purpleMode ? juce::Colour (0xFF9060C0) : juce::Colour (0xFFD4A030)) :
        juce::Colour (0xFFB89860);
    
    juce::ColourGradient screwGradient (
        screwColor.brighter (0.3f), x + size * 0.25f, y + size * 0.25f,
        screwColor.darker (0.3f), x + size * 0.75f, y + size * 0.75f,
        true
    );
    g.setGradientFill (screwGradient);
    g.fillEllipse (x, y, size, size);
    
    g.setColour (juce::Colour (0xFF2A2018));
    g.fillRect (x + size * 0.5f - 0.75f, y + size * 0.2f, 1.5f, size * 0.6f);
    g.fillRect (x + size * 0.2f, y + size * 0.5f - 0.75f, size * 0.6f, 1.5f);
    
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawEllipse (x + 1, y + 1, size - 2, size - 2, 1.0f);
    
    if (isColorScrew)
    {
        g.setColour (screwColor.withAlpha (0.4f));
        g.drawEllipse (x - 2, y - 2, size + 4, size + 4, 2.0f);
    }
}

void HoneyVoxAudioProcessorEditor::drawBee (juce::Graphics& g, float centerX, float centerY, float size, bool pressed)
{
    float scale = size / 100.0f;
    
    // If pressed, offset and darken slightly for embossed look
    float pressOffset = pressed ? 2.0f : 0.0f;
    float brightnessMult = pressed ? 0.85f : 1.0f;
    centerX += pressOffset;
    centerY += pressOffset;
    
    // Hexagonal bee helper
    auto drawHexagon = [&g, brightnessMult](float cx, float cy, float radius, juce::Colour fillColor, juce::Colour strokeColor, float strokeWidth)
    {
        juce::Path hex;
        for (int i = 0; i < 6; ++i)
        {
            float angle = (float)i * juce::MathConstants<float>::twoPi / 6.0f - juce::MathConstants<float>::halfPi;
            float px = cx + std::cos(angle) * radius;
            float py = cy + std::sin(angle) * radius;
            if (i == 0) hex.startNewSubPath(px, py);
            else hex.lineTo(px, py);
        }
        hex.closeSubPath();
        g.setColour(fillColor.withMultipliedBrightness(brightnessMult));
        g.fillPath(hex);
        if (strokeWidth > 0) { g.setColour(strokeColor); g.strokePath(hex, juce::PathStrokeType(strokeWidth)); }
    };
    
    // Draw hexagonal pressed shadow if pressed
    if (pressed)
    {
        juce::Path hexShadow;
        float shadowRadius = size * 0.6f;
        float shadowCX = centerX - 2;
        float shadowCY = centerY - 2;
        for (int i = 0; i < 6; ++i)
        {
            float angle = (float)i * juce::MathConstants<float>::twoPi / 6.0f - juce::MathConstants<float>::halfPi;
            float px = shadowCX + std::cos(angle) * shadowRadius;
            float py = shadowCY + std::sin(angle) * shadowRadius;
            if (i == 0) hexShadow.startNewSubPath(px, py);
            else hexShadow.lineTo(px, py);
        }
        hexShadow.closeSubPath();
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillPath(hexShadow);
    }
    
    // Wings (with honey glow when not pressed)
    if (!pressed)
    {
        // Subtle honey glow around wings
        g.setColour(juce::Colour(0x30FF9900));
        g.fillEllipse(centerX - 48 * scale, centerY - 25 * scale, 40 * scale, 40 * scale);
        g.fillEllipse(centerX + 8 * scale, centerY - 25 * scale, 40 * scale, 40 * scale);
    }
    drawHexagon(centerX - 28 * scale, centerY - 5 * scale, 18 * scale, juce::Colour(0x50FFE0A0), juce::Colour(0x80FFCC66), 1.5f);
    drawHexagon(centerX + 28 * scale, centerY - 5 * scale, 18 * scale, juce::Colour(0x50FFE0A0), juce::Colour(0x80FFCC66), 1.5f);
    
    // Antenna
    g.setColour (juce::Colour (0xFF2A2018));
    g.drawLine (centerX - 8 * scale, centerY - 25 * scale, centerX - 18 * scale, centerY - 42 * scale, 2.5f);
    g.drawLine (centerX + 8 * scale, centerY - 25 * scale, centerX + 18 * scale, centerY - 42 * scale, 2.5f);
    drawHexagon(centerX - 18 * scale, centerY - 45 * scale, 5 * scale, juce::Colour(0xFFFFAA33), juce::Colour(0xFF2A2018), 1.0f);
    drawHexagon(centerX + 18 * scale, centerY - 45 * scale, 5 * scale, juce::Colour(0xFFFFAA33), juce::Colour(0xFF2A2018), 1.0f);
    
    // Head
    drawHexagon(centerX, centerY - 18 * scale, 15 * scale, juce::Colour(0xFFFFCC44), juce::Colour(0xFF2A2018), 2.0f);
    g.setColour (juce::Colour (0xFF1A1510));
    g.fillEllipse (centerX - 8 * scale, centerY - 22 * scale, 6 * scale, 8 * scale);
    g.fillEllipse (centerX + 2 * scale, centerY - 22 * scale, 6 * scale, 8 * scale);
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.fillEllipse (centerX - 6 * scale, centerY - 20 * scale, 2 * scale, 2 * scale);
    g.fillEllipse (centerX + 4 * scale, centerY - 20 * scale, 2 * scale, 2 * scale);
    
    // Body
    drawHexagon(centerX, centerY + 5 * scale, 18 * scale, juce::Colour(0xFFFFDD55), juce::Colour(0xFF2A2018), 2.0f);
    drawHexagon(centerX, centerY + 5 * scale, 11 * scale, juce::Colour(0xFF1A1510), juce::Colours::transparentBlack, 0);
    drawHexagon(centerX, centerY + 28 * scale, 14 * scale, juce::Colour(0xFFFFDD55), juce::Colour(0xFF2A2018), 2.0f);
    drawHexagon(centerX, centerY + 28 * scale, 9 * scale, juce::Colour(0xFF1A1510), juce::Colours::transparentBlack, 0);
    
    // Stinger
    g.setColour (juce::Colour (0xFF2A2018));
    juce::Path stinger;
    stinger.startNewSubPath (centerX - 4 * scale, centerY + 40 * scale);
    stinger.lineTo (centerX, centerY + 52 * scale);
    stinger.lineTo (centerX + 4 * scale, centerY + 40 * scale);
    stinger.closeSubPath();
    g.fillPath (stinger);
}

void HoneyVoxAudioProcessorEditor::drawBacklitLED (juce::Graphics& g, float x, float y, float size, bool isOn)
{
    if (isOn)
    {
        juce::ColourGradient glow (juce::Colour (0x60FF8800), x + size/2, y + size/2,
                                   juce::Colours::transparentBlack, x + size/2, y - size, true);
        g.setGradientFill (glow);
        g.fillEllipse (x - size * 0.5f, y - size * 0.5f, size * 2, size * 2);
    }
    
    g.setColour (juce::Colour (0xFF1A1A1A));
    g.fillEllipse (x, y, size, size);
    
    juce::Colour ledColor = isOn ? juce::Colour (0xFFFF9933) : juce::Colour (0xFF4A3A2A);
    juce::ColourGradient ledGradient (ledColor.brighter (0.3f), x + size * 0.3f, y + size * 0.3f,
                                      ledColor.darker (0.2f), x + size * 0.7f, y + size * 0.7f, true);
    g.setGradientFill (ledGradient);
    g.fillEllipse (x + 2, y + 2, size - 4, size - 4);
    
    g.setColour (juce::Colours::white.withAlpha (isOn ? 0.4f : 0.15f));
    g.fillEllipse (x + 3, y + 2, size * 0.4f, size * 0.3f);
}

void HoneyVoxAudioProcessorEditor::drawCableScrew (juce::Graphics& g, float x, float y)
{
    float size = 16.0f;
    
    // Screw base
    g.setColour (juce::Colour (0xFF2A2520));
    g.fillEllipse (x - 1, y - 1, size + 2, size + 2);
    
    juce::ColourGradient screwGrad (
        juce::Colour (0xFFC0B090), x + size * 0.3f, y + size * 0.3f,
        juce::Colour (0xFF706050), x + size * 0.7f, y + size * 0.7f, true
    );
    g.setGradientFill (screwGrad);
    g.fillEllipse (x, y, size, size);
    
    // Phillips head slot (rotated based on angle)
    g.setColour (juce::Colour (0xFF1A1510));
    float slotAngle = cableScrewAngle * juce::MathConstants<float>::twoPi * 2.0f;
    float cx = x + size / 2;
    float cy = y + size / 2;
    float slotLen = size * 0.3f;
    
    g.drawLine(cx + std::cos(slotAngle) * slotLen, cy + std::sin(slotAngle) * slotLen,
               cx - std::cos(slotAngle) * slotLen, cy - std::sin(slotAngle) * slotLen, 1.5f);
    g.drawLine(cx + std::cos(slotAngle + 1.57f) * slotLen, cy + std::sin(slotAngle + 1.57f) * slotLen,
               cx - std::cos(slotAngle + 1.57f) * slotLen, cy - std::sin(slotAngle + 1.57f) * slotLen, 1.5f);
    
    // Highlight
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawEllipse (x + 1, y + 1, size - 2, size - 2, 0.5f);
    
    // Draw winding intertwined cables based on angle
    if (cableScrewAngle > 0.05f)
    {
        juce::Colour cableColors[] = {
            juce::Colour(0xFFDD4444),  // Red
            juce::Colour(0xFF4488DD),  // Blue
            juce::Colour(0xFFAA44DD),  // Purple
            juce::Colour(0xFFFF9933),  // Orange
            juce::Colour(0xFF44DD66)   // Green
        };
        
        float maxLength = cableScrewAngle * 50.0f;  // Max 50px
        
        for (int i = 0; i < 5; ++i)
        {
            float cableStart = (float)i * 0.18f;
            if (cableScrewAngle > cableStart)
            {
                float cableProgress = (cableScrewAngle - cableStart) / (1.0f - cableStart);
                cableProgress = juce::jmin(cableProgress, 1.0f);
                
                // Create winding path using bezier curves
                juce::Path cablePath;
                float startX = cx;
                float startY = cy;
                
                // Each cable winds in a helix pattern, intertwined with others
                float baseAngle = (float)i * 1.2f + 0.3f;
                float windFreq = 3.0f + (float)i * 0.5f;  // Different winding frequency per cable
                float windAmp = 8.0f + (float)i * 2.0f;   // Different amplitude
                
                cablePath.startNewSubPath(startX, startY);
                
                int segments = 12;
                for (int s = 1; s <= segments; ++s)
                {
                    float t = (float)s / (float)segments * cableProgress;
                    float dist = t * maxLength;
                    
                    // Helix winding
                    float windOffset = std::sin(t * windFreq * juce::MathConstants<float>::pi) * windAmp * t;
                    
                    // Main direction goes down-right
                    float px = startX + dist * 0.7f + windOffset * std::cos(baseAngle + t * 2.0f);
                    float py = startY + dist * 0.5f + windOffset * std::sin(baseAngle + t * 2.0f);
                    
                    cablePath.lineTo(px, py);
                }
                
                // Cable shadow
                g.setColour(juce::Colours::black.withAlpha(0.25f));
                g.strokePath(cablePath, juce::PathStrokeType(2.5f), 
                    juce::AffineTransform::translation(1.0f, 1.0f));
                
                // Cable
                g.setColour(cableColors[i]);
                g.strokePath(cablePath, juce::PathStrokeType(2.0f));
                
                // Cable end - small connector
                auto endPt = cablePath.getCurrentPosition();
                g.setColour(cableColors[i].darker(0.3f));
                g.fillEllipse(endPt.x - 2.5f, endPt.y - 2.5f, 5.0f, 5.0f);
                g.setColour(cableColors[i].brighter(0.2f));
                g.fillEllipse(endPt.x - 1.5f, endPt.y - 1.5f, 3.0f, 3.0f);
            }
        }
    }
}

void HoneyVoxAudioProcessorEditor::drawGlowingHexMatrix (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Pulsing glow intensity based on phase
    float pulse1 = 0.5f + 0.3f * std::sin(hexGlowPhase);
    float pulse2 = 0.5f + 0.3f * std::sin(hexGlowPhase * 0.7f + 1.0f);
    float pulse3 = 0.5f + 0.2f * std::sin(hexGlowPhase * 1.3f + 2.0f);
    
    float hexSize = 4.0f;
    float rowHeight = hexSize * 1.8f;
    float colWidth = hexSize * 2.0f;
    
    int row = 0;
    for (float y = bounds.getY() + 3; y < bounds.getBottom() - 3; y += rowHeight)
    {
        float xOffset = (row % 2) * (colWidth / 2);
        int col = 0;
        for (float x = bounds.getX() + 3 + xOffset; x < bounds.getRight() - 3; x += colWidth)
        {
            // Vary intensity per hex for organic look
            float localPulse = (col % 3 == 0) ? pulse1 : ((col % 3 == 1) ? pulse2 : pulse3);
            localPulse *= (row % 2 == 0) ? 1.0f : 0.85f;
            
            // Honey orange glow color
            juce::Colour glowColor = juce::Colour(0xFFFF9922).withAlpha(0.15f + localPulse * 0.2f);
            
            // Draw hexagon
            juce::Path hex;
            for (int i = 0; i < 6; ++i)
            {
                float angle = (float)i * juce::MathConstants<float>::pi / 3.0f;
                float hx = x + (hexSize/2) * std::cos(angle);
                float hy = y + (hexSize/2) * std::sin(angle);
                if (i == 0) hex.startNewSubPath(hx, hy);
                else hex.lineTo(hx, hy);
            }
            hex.closeSubPath();
            
            // Glow behind
            if (localPulse > 0.5f)
            {
                g.setColour(juce::Colour(0xFFFFAA44).withAlpha((localPulse - 0.5f) * 0.3f));
                g.fillEllipse(x - hexSize, y - hexSize, hexSize * 2, hexSize * 2);
            }
            
            g.setColour(glowColor);
            g.fillPath(hex);
            
            // Subtle white highlight on some hexes
            if ((row + col) % 5 == 0 && localPulse > 0.6f)
            {
                g.setColour(juce::Colours::white.withAlpha(0.1f * localPulse));
                g.fillPath(hex);
            }
            
            col++;
        }
        row++;
    }
}

void HoneyVoxAudioProcessorEditor::drawHexBomb (juce::Graphics& g)
{
    float progress = (float)hexBombTimer / 40.0f;
    float alpha = 1.0f - progress * 0.7f;
    
    for (int row = 0; row < 5; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            float startX = 60.0f + col * 120.0f;
            float startY = 80.0f + row * 90.0f;
            float angle = ((row + col) * 0.7f) + progress * 3.0f;
            float distance = progress * 80.0f;
            float bx = startX + std::cos(angle) * distance;
            float by = startY + std::sin(angle) * distance;
            float bombSize = 25.0f * (1.0f - progress * 0.3f);
            
            juce::Path hex;
            for (int j = 0; j < 6; ++j)
            {
                float a = (float)j * juce::MathConstants<float>::twoPi / 6.0f - juce::MathConstants<float>::halfPi;
                if (j == 0) hex.startNewSubPath(bx + std::cos(a) * bombSize, by + std::sin(a) * bombSize);
                else hex.lineTo(bx + std::cos(a) * bombSize, by + std::sin(a) * bombSize);
            }
            hex.closeSubPath();
            
            g.setColour(juce::Colour (0xFF1A1A1A).withAlpha(alpha));
            g.fillPath(hex);
            g.setColour(juce::Colour (0xFF000000).withAlpha(alpha));
            g.strokePath(hex, juce::PathStrokeType(2.5f));
            
            if (progress < 0.5f)
            {
                float sparkAlpha = (0.5f - progress) * 2.0f * alpha;
                g.setColour(juce::Colour (0xFFFF6600).withAlpha(sparkAlpha));
                g.fillEllipse(bx - 4, by - bombSize - 8, 8, 8);
            }
        }
    }
    
    if (progress > 0.8f)
    {
        g.setColour(juce::Colours::white.withAlpha((progress - 0.8f) * 1.5f));
        g.fillRect(getLocalBounds());
    }
}

void HoneyVoxAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    juce::Colour baseColor = purpleMode ? juce::Colour (0xFF5A3A7A) : juce::Colour (0xFFFFC000);
    g.fillAll (baseColor);
    
    for (int i = 0; i < getHeight(); ++i)
    {
        float brightness = 0.98f + 0.02f * std::sin(i * 0.3f);
        g.setColour (baseColor.withMultipliedBrightness (brightness).withAlpha (0.15f));
        g.drawHorizontalLine (i, 0, (float)getWidth());
    }
    
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawHorizontalLine (0, 0, (float)getWidth());
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawHorizontalLine (getHeight() - 1, 0, (float)getWidth());
    
    // Screws - cable screw top-left, others normal
    cableScrewBounds = juce::Rectangle<float>(12, 12, 16.0f, 16.0f);
    drawCableScrew(g, 12, 12);  // Top-left is cable screw
    drawCornerScrew (g, getWidth() - 28.0f, 12, false);
    hexBombScrewBounds = juce::Rectangle<float> (12, getHeight() - 28.0f, 16.0f, 16.0f);
    drawCornerScrew (g, hexBombScrewBounds.getX(), hexBombScrewBounds.getY(), false);
    colorScrewBounds = juce::Rectangle<float> (getWidth() - 28.0f, getHeight() - 28.0f, 16.0f, 16.0f);
    drawCornerScrew (g, colorScrewBounds.getX(), colorScrewBounds.getY(), true);
    
    // === BACKPLATE (the physical mounting surface) ===
    // This is the main panel that holds hardware elements
    if (logoLoaded)
    {
        int logoWidth = 650;
        int logoHeight = (int)(logoImage.getHeight() * ((float)logoWidth / logoImage.getWidth()));
        int logoX = (getWidth() - logoWidth) / 2;
        
        // BACKPLATE dimensions and position
        int backplateWidth = logoWidth + 80;
        int backplateHeight = 110;
        int backplateX = (getWidth() - backplateWidth) / 2;
        int backplateY = 50;  // Backplate position
        
        // PNG positioned ABOVE backplate - moved UP by 1.5 inches total (144px)
        int logoY = backplateY - 144;  // 1.5 inches = 144px
        
        // Backplate shadow
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle ((float)backplateX + 4, (float)backplateY + 4, (float)backplateWidth, (float)backplateHeight, 6.0f);
        
        // Backplate body - brushed metal
        juce::ColourGradient backplateGrad (
            juce::Colour (0xFF5A5550), (float)backplateX, (float)backplateY,
            juce::Colour (0xFF3A3835), (float)backplateX, (float)backplateY + backplateHeight,
            false
        );
        g.setGradientFill (backplateGrad);
        g.fillRoundedRectangle ((float)backplateX, (float)backplateY, (float)backplateWidth, (float)backplateHeight, 6.0f);
        
        // Brushed texture on backplate
        for (int i = backplateY; i < backplateY + backplateHeight; i += 2)
        {
            float texBrightness = 0.95f + 0.1f * std::sin((float)i * 0.5f);
            g.setColour (juce::Colours::white.withAlpha (0.04f * texBrightness));
            g.drawHorizontalLine (i, (float)backplateX + 3, (float)backplateX + backplateWidth - 3);
        }
        
        // Backplate border
        g.setColour (juce::Colour (0xFF2A2825));
        g.drawRoundedRectangle ((float)backplateX, (float)backplateY, (float)backplateWidth, (float)backplateHeight, 6.0f, 2.0f);
        
        // Inner highlight
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawRoundedRectangle ((float)backplateX + 2, (float)backplateY + 2, (float)backplateWidth - 4, (float)backplateHeight - 4, 5.0f, 1.0f);
        
        // === BEES - FLUSH ON BACKPLATE (preset navigation) ===
        float beeSize = 55.0f;
        float beeY = (float)backplateY + backplateHeight / 2;
        
        // Left bee - previous preset
        float leftBeeX = (float)backplateX + 50;
        leftBeeBounds = juce::Rectangle<float>(leftBeeX - beeSize/2, beeY - beeSize/2, beeSize, beeSize * 1.2f);
        drawBee (g, leftBeeX, beeY, beeSize, leftBeePressed);
        
        // Right bee - next preset
        float rightBeeX = (float)backplateX + backplateWidth - 50;
        rightBeeBounds = juce::Rectangle<float>(rightBeeX - beeSize/2, beeY - beeSize/2, beeSize, beeSize * 1.2f);
        drawBee (g, rightBeeX, beeY, beeSize, rightBeePressed);
        
        // Backplate screws (4 corners) - flush on backplate
        float screwSize = 12.0f;
        float screwInset = 12.0f;
        
        auto drawBackplateScrew = [&](float sx, float sy) {
            g.setColour (juce::Colour (0xFF1A1510));
            g.fillEllipse (sx - 1, sy - 1, screwSize + 2, screwSize + 2);
            
            juce::ColourGradient screwGrad (
                juce::Colour (0xFFC0B090), sx + screwSize * 0.25f, sy + screwSize * 0.25f,
                juce::Colour (0xFF807060), sx + screwSize * 0.75f, sy + screwSize * 0.75f,
                true
            );
            g.setGradientFill (screwGrad);
            g.fillEllipse (sx, sy, screwSize, screwSize);
            
            g.setColour (juce::Colour (0xFF2A2018));
            g.fillRect (sx + screwSize * 0.5f - 0.75f, sy + screwSize * 0.2f, 1.5f, screwSize * 0.6f);
            g.fillRect (sx + screwSize * 0.2f, sy + screwSize * 0.5f - 0.75f, screwSize * 0.6f, 1.5f);
            
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawEllipse (sx + 1, sy + 1, screwSize - 2, screwSize - 2, 0.5f);
        };
        
        drawBackplateScrew ((float)backplateX + screwInset, (float)backplateY + screwInset);
        drawBackplateScrew ((float)backplateX + backplateWidth - screwInset - screwSize, (float)backplateY + screwInset);
        drawBackplateScrew ((float)backplateX + screwInset, (float)backplateY + backplateHeight - screwInset - screwSize);
        drawBackplateScrew ((float)backplateX + backplateWidth - screwInset - screwSize, (float)backplateY + backplateHeight - screwInset - screwSize);
        
        // PNG Logo - 100% opacity, no transparency, overlays on top
        g.setOpacity (1.0f);
        g.drawImage (logoImage, logoX, logoY, logoWidth, logoHeight, 0, 0, logoImage.getWidth(), logoImage.getHeight());
    }
    else
    {
        g.setColour (purpleMode ? juce::Colour (0xFFE0D0F0) : juce::Colour (0xFF2A2018));
        g.setFont (juce::Font (juce::FontOptions (48.0f, juce::Font::bold)));
        g.drawText ("HONEYVOX", 0, 63, getWidth(), 50, juce::Justification::centred);
    }
    
    // === HEXAGONAL AD-LIB FX TEXT ===
    {
        float hexY = 175;  // Below the plaque
        float hexSize = 12.0f;
        juce::String adlibText = "AD-LIB FX";
        float totalTextWidth = adlibText.length() * hexSize * 1.8f;
        float startTextX = (getWidth() - totalTextWidth) / 2;
        
        juce::Colour hexColor = purpleMode ? juce::Colour (0xFFE0D0F0) : juce::Colour (0xFF2A2018);
        
        for (int c = 0; c < adlibText.length(); ++c)
        {
            if (adlibText[c] == ' ') continue;
            
            float cx = startTextX + c * hexSize * 1.8f + hexSize;
            float cy = hexY + hexSize;
            
            // Draw hexagon for each letter
            juce::Path hex;
            for (int i = 0; i < 6; ++i)
            {
                float angle = (float)i * juce::MathConstants<float>::twoPi / 6.0f - juce::MathConstants<float>::halfPi;
                float px = cx + std::cos(angle) * hexSize;
                float py = cy + std::sin(angle) * hexSize;
                if (i == 0) hex.startNewSubPath(px, py);
                else hex.lineTo(px, py);
            }
            hex.closeSubPath();
            
            // Fill hexagon
            g.setColour (hexColor.withAlpha (0.15f));
            g.fillPath (hex);
            g.setColour (hexColor);
            g.strokePath (hex, juce::PathStrokeType (1.5f));
            
            // Draw letter
            g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
            g.drawText (juce::String::charToString (adlibText[c]), 
                       (int)(cx - hexSize), (int)(cy - hexSize * 0.6f), 
                       (int)(hexSize * 2), (int)(hexSize * 1.2f), 
                       juce::Justification::centred);
        }
    }
    
    // Created by text - same font as MANOLO SOUND
    juce::Colour textColor = purpleMode ? juce::Colour (0xFFE0D0F0) : juce::Colour (0xFF2A2018);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.setColour (textColor.withAlpha (0.7f));
    g.drawText ("Created by Nolo's Addiction", 0, 205, getWidth(), 16, juce::Justification::centred);
    
    // === LAYOUT CALCULATIONS ===
    int sectionHeight = 185;
    int sectionY = 280;
    int singleWidth = 110;
    int delayWidth = 290;
    int spacing = 10;
    int totalLayoutWidth = singleWidth * 3 + delayWidth + spacing * 3;
    int startX = (getWidth() - totalLayoutWidth - 130) / 2;
    
    // === ERAS PLATE - taller for dropdown ===
    int erasPlateW = 100;
    int erasPlateH = 58;  // Taller
    int erasPlateX = startX + (singleWidth - erasPlateW) / 2;
    int erasPlateY = sectionY + sectionHeight + 5;
    float esScrewSize = 6.0f;
    
    // Plate shadow
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle ((float)erasPlateX + 2, (float)erasPlateY + 2, (float)erasPlateW, (float)erasPlateH, 4.0f);
    
    // Plate background - brushed metal
    juce::ColourGradient erasBg (
        juce::Colour (0xFF4A4540), (float)erasPlateX, (float)erasPlateY,
        juce::Colour (0xFF2A2825), (float)erasPlateX, (float)erasPlateY + erasPlateH,
        false
    );
    g.setGradientFill (erasBg);
    g.fillRoundedRectangle ((float)erasPlateX, (float)erasPlateY, (float)erasPlateW, (float)erasPlateH, 4.0f);
    
    // Brushed texture
    for (int py = erasPlateY; py < erasPlateY + erasPlateH; py += 2)
    {
        g.setColour (juce::Colours::white.withAlpha (0.03f));
        g.drawHorizontalLine (py, (float)erasPlateX + 2, (float)erasPlateX + erasPlateW - 2);
    }
    
    // Plate border
    g.setColour (juce::Colour (0xFF1A1815));
    g.drawRoundedRectangle ((float)erasPlateX, (float)erasPlateY, (float)erasPlateW, (float)erasPlateH, 4.0f, 1.5f);
    
    // Corner screws
    auto drawSmallScrew = [&](float sx, float sy, float sz) {
        g.setColour (juce::Colour (0xFF080606));
        g.fillEllipse (sx - 0.5f, sy - 0.5f, sz + 1, sz + 1);
        juce::ColourGradient screwGrad (
            juce::Colour (0xFF908070), sx + sz * 0.25f, sy + sz * 0.25f,
            juce::Colour (0xFF504540), sx + sz * 0.75f, sy + sz * 0.75f,
            true
        );
        g.setGradientFill (screwGrad);
        g.fillEllipse (sx, sy, sz, sz);
        g.setColour (juce::Colour (0xFF1A1510));
        g.fillRect (sx + sz * 0.5f - 0.4f, sy + sz * 0.2f, 0.8f, sz * 0.6f);
        g.fillRect (sx + sz * 0.2f, sy + sz * 0.5f - 0.4f, sz * 0.6f, 0.8f);
    };
    
    drawSmallScrew ((float)erasPlateX + 4, (float)erasPlateY + 4, esScrewSize);
    drawSmallScrew ((float)erasPlateX + erasPlateW - esScrewSize - 4, (float)erasPlateY + 4, esScrewSize);
    drawSmallScrew ((float)erasPlateX + 4, (float)erasPlateY + erasPlateH - esScrewSize - 4, esScrewSize);
    drawSmallScrew ((float)erasPlateX + erasPlateW - esScrewSize - 4, (float)erasPlateY + erasPlateH - esScrewSize - 4, esScrewSize);
    
    // ERAS text - centered at top with glow
    g.setColour (juce::Colour (0x40D4A030));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText ("ERAs", erasPlateX, erasPlateY + 7, erasPlateW, 14, juce::Justification::centred);
    g.setColour (juce::Colour (0xFFD4A030));
    g.drawText ("ERAs", erasPlateX, erasPlateY + 8, erasPlateW, 14, juce::Justification::centred);
    
    // === HONEYPONG PLATE - same width as delay section, includes SYNC + dropdown ===
    int delayPlateW = delayWidth;  // Same width as ECHO DELAY backplate
    int delayPlateH = erasPlateH;  // Same height as ERAs plate
    int delayPlateX = startX + singleWidth + spacing;  // Aligned with delay section
    int delayPlateY = sectionY + sectionHeight + 5;
    float dpScrewSize = 6.0f;
    
    // Plate shadow
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle ((float)delayPlateX + 2, (float)delayPlateY + 2, (float)delayPlateW, (float)delayPlateH, 4.0f);
    
    // Plate background - brushed metal
    juce::ColourGradient delayPlateBg (
        juce::Colour (0xFF4A4540), (float)delayPlateX, (float)delayPlateY,
        juce::Colour (0xFF2A2825), (float)delayPlateX, (float)delayPlateY + delayPlateH,
        false
    );
    g.setGradientFill (delayPlateBg);
    g.fillRoundedRectangle ((float)delayPlateX, (float)delayPlateY, (float)delayPlateW, (float)delayPlateH, 4.0f);
    
    // Brushed texture
    for (int py = delayPlateY; py < delayPlateY + delayPlateH; py += 2)
    {
        g.setColour (juce::Colours::white.withAlpha (0.03f));
        g.drawHorizontalLine (py, (float)delayPlateX + 2, (float)delayPlateX + delayPlateW - 2);
    }
    
    // Plate border
    g.setColour (juce::Colour (0xFF1A1815));
    g.drawRoundedRectangle ((float)delayPlateX, (float)delayPlateY, (float)delayPlateW, (float)delayPlateH, 4.0f, 1.5f);
    
    // Inner highlight
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle ((float)delayPlateX + 2, (float)delayPlateY + 2, (float)delayPlateW - 4, (float)delayPlateH - 4, 3.0f, 1.0f);
    
    // Corner screws on plate
    auto drawDelayScrew = [&](float sx, float sy) {
        g.setColour (juce::Colour (0xFF080606));
        g.fillEllipse (sx - 0.5f, sy - 0.5f, dpScrewSize + 1, dpScrewSize + 1);
        juce::ColourGradient screwGrad (
            juce::Colour (0xFF908070), sx + dpScrewSize * 0.25f, sy + dpScrewSize * 0.25f,
            juce::Colour (0xFF504540), sx + dpScrewSize * 0.75f, sy + dpScrewSize * 0.75f,
            true
        );
        g.setGradientFill (screwGrad);
        g.fillEllipse (sx, sy, dpScrewSize, dpScrewSize);
        g.setColour (juce::Colour (0xFF1A1510));
        g.fillRect (sx + dpScrewSize * 0.5f - 0.4f, sy + dpScrewSize * 0.2f, 0.8f, dpScrewSize * 0.6f);
        g.fillRect (sx + dpScrewSize * 0.2f, sy + dpScrewSize * 0.5f - 0.4f, dpScrewSize * 0.6f, 0.8f);
    };
    
    // Four corner screws
    drawDelayScrew ((float)delayPlateX + 5, (float)delayPlateY + 5);
    drawDelayScrew ((float)delayPlateX + delayPlateW - dpScrewSize - 5, (float)delayPlateY + 5);
    drawDelayScrew ((float)delayPlateX + 5, (float)delayPlateY + delayPlateH - dpScrewSize - 5);
    drawDelayScrew ((float)delayPlateX + delayPlateW - dpScrewSize - 5, (float)delayPlateY + delayPlateH - dpScrewSize - 5);
    
    // Draw "PING PONG" label at top with honey glow
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.setColour (juce::Colour (0x50D4A030));
    g.drawText ("PING PONG", delayPlateX, delayPlateY + 6, delayPlateW, 14, juce::Justification::centred);
    g.setColour (juce::Colour (0xFFFFCC44));
    g.drawText ("PING PONG", delayPlateX, delayPlateY + 7, delayPlateW, 12, juce::Justification::centred);
    
    // Labels for controls: OFF | SYNC | DIV - equally spaced
    int thirdW = delayPlateW / 3;
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.setColour (juce::Colour (0xFFD4A030));
    g.drawText ("SYNC", delayPlateX + thirdW, delayPlateY + 24, thirdW, 12, juce::Justification::centred);
    
    // Output panel - ALIGNED with other sections
    int outputX = startX + singleWidth * 3 + delayWidth + spacing * 4;
    
    auto outputBounds = juce::Rectangle<float> ((float)outputX, (float)sectionY, 120.0f, 185.0f);
    
    // Metal background for output
    juce::ColourGradient outputBg (
        juce::Colour (0xFF4A4540), outputBounds.getX(), outputBounds.getY(),
        juce::Colour (0xFF2A2825), outputBounds.getX(), outputBounds.getBottom(),
        false
    );
    g.setGradientFill (outputBg);
    g.fillRoundedRectangle (outputBounds, 6.0f);
    
    // Brushed texture on output
    for (float i = outputBounds.getY(); i < outputBounds.getBottom(); i += 2.0f)
    {
        float texBrightness = 0.95f + 0.1f * std::sin(i * 0.8f);
        g.setColour (juce::Colours::white.withAlpha (0.03f * texBrightness));
        g.drawHorizontalLine (static_cast<int>(i), outputBounds.getX() + 2, outputBounds.getRight() - 2);
    }
    
    g.setColour (juce::Colour (0xFF1A1815));
    g.drawRoundedRectangle (outputBounds, 6.0f, 1.5f);
    
    // Output screws
    float outScrewSize = 10.0f;
    float outInset = 8.0f;
    
    auto drawOutScrew = [&](float sx, float sy) {
        g.setColour (juce::Colour (0xFF0A0808));
        g.fillEllipse (sx - 1, sy - 1, outScrewSize + 2, outScrewSize + 2);
        juce::ColourGradient screwGrad (
            juce::Colour (0xFF908070), sx + outScrewSize * 0.25f, sy + outScrewSize * 0.25f,
            juce::Colour (0xFF504540), sx + outScrewSize * 0.75f, sy + outScrewSize * 0.75f,
            true
        );
        g.setGradientFill (screwGrad);
        g.fillEllipse (sx, sy, outScrewSize, outScrewSize);
        g.setColour (juce::Colour (0xFF1A1510));
        g.fillRect (sx + outScrewSize * 0.5f - 0.5f, sy + outScrewSize * 0.2f, 1.0f, outScrewSize * 0.6f);
        g.fillRect (sx + outScrewSize * 0.2f, sy + outScrewSize * 0.5f - 0.5f, outScrewSize * 0.6f, 1.0f);
    };
    
    drawOutScrew (outputBounds.getX() + outInset, outputBounds.getY() + outInset);
    drawOutScrew (outputBounds.getRight() - outInset - outScrewSize, outputBounds.getY() + outInset);
    drawOutScrew (outputBounds.getX() + outInset, outputBounds.getBottom() - outInset - outScrewSize);
    drawOutScrew (outputBounds.getRight() - outInset - outScrewSize, outputBounds.getBottom() - outInset - outScrewSize);
    
    // Bottom plate
    auto plateBounds = juce::Rectangle<float> (getWidth() / 2 - 160, getHeight() - 45.0f, 320, 28);
    juce::ColourGradient plateBg (purpleMode ? juce::Colour (0xFF5A4A6A) : juce::Colour (0xFF8A7850),
                                  plateBounds.getX(), plateBounds.getY(),
                                  purpleMode ? juce::Colour (0xFF3A2A4A) : juce::Colour (0xFF6A5840),
                                  plateBounds.getX(), plateBounds.getBottom(), false);
    g.setGradientFill (plateBg);
    g.fillRoundedRectangle (plateBounds, 4.0f);
    g.setColour (purpleMode ? juce::Colour (0xFFE0D0F0) : juce::Colour (0xFF2A2018));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("MANOLO SOUND  -  MODEL HV-1 BEEKEEPER EDITION", plateBounds, juce::Justification::centred);
    
    // Hex bomb
    if (showHexBomb) drawHexBomb (g);
}

void HoneyVoxAudioProcessorEditor::resized()
{
    int sectionHeight = 185;
    int sectionY = 280;
    int singleWidth = 110;
    int delayWidth = 290;
    int spacing = 10;
    
    int totalWidth = singleWidth * 3 + delayWidth + spacing * 3;
    int startX = (getWidth() - totalWidth - 130) / 2;
    
    phoneSection.setBounds (startX, sectionY, singleWidth, sectionHeight);
    
    // ERAs plate - taller, dropdown below text
    int erasPlateW = 100;
    int erasPlateH = 58;
    int erasPlateX = startX + (singleWidth - erasPlateW) / 2;
    int erasPlateY = sectionY + sectionHeight + 5;
    phoneModeLabel.setBounds (0, 0, 0, 0);  // Hidden
    phoneModeBox.setBounds (erasPlateX + 10, erasPlateY + 28, 80, 22);
    
    delaySection.setBounds (startX + singleWidth + spacing, sectionY, delayWidth, sectionHeight);
    
    // PING PONG plate - same width as delay section, same height as ERAs
    int delayPlateW = delayWidth;
    int delayPlateH = erasPlateH;
    int delayPlateX = startX + singleWidth + spacing;
    int delayPlateY = sectionY + sectionHeight + 5;
    
    // Controls equally spaced: [OFF switch] [SYNC switch] [dropdown]
    int thirdW = delayPlateW / 3;
    int controlY = delayPlateY + 32;
    
    pingPongLabel.setBounds (0, 0, 0, 0);  // Hidden
    pingPongSwitch.setBounds (delayPlateX + 15, controlY, 75, 20);  // OFF switch left
    
    delaySyncLabel.setBounds (0, 0, 0, 0);  // Hidden - drawn in paint
    delaySyncSwitch.setBounds (delayPlateX + thirdW + (thirdW - 60) / 2, controlY, 60, 20);  // SYNC middle
    
    delayDivisionBox.setBounds (delayPlateX + thirdW * 2 + 10, controlY - 2, thirdW - 25, 22);  // Dropdown right
    
    saturationSection.setBounds (startX + singleWidth + delayWidth + spacing * 2, sectionY, singleWidth, sectionHeight);
    underwaterSection.setBounds (startX + singleWidth * 2 + delayWidth + spacing * 3, sectionY, singleWidth, sectionHeight);
    
    // Output - narrower screen to avoid screws (screws at ~8px and ~112px in 120px panel)
    int outputX = startX + singleWidth * 3 + delayWidth + spacing * 4;
    int outputContentX = outputX + 22;  // More padding from left screw
    int outputContentW = 76;  // Narrower
    outputScreen.setBounds (outputContentX, sectionY + 10, outputContentW, 22);
    outputKnob.setBounds (outputX + 14, sectionY + 34, 92, 92);
    outputSwitch.setBounds (outputX + 25, sectionY + 142, 70, 22);
    
    // Preset controls - moved left to avoid covering top-right screw
    presetBox.setBounds (getWidth() - 220, 15, 130, 24);
    savePresetButton.setBounds (getWidth() - 85, 15, 45, 24);
}
