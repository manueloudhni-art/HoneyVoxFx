/*
  ==============================================================================
    HoneyVox Ad-Lib FX
    Created by Nolo's Addiction
    
    SIGNAL FLOW (optimized for vocal ad-libs):
    1. INPUT
    2. SATURATION (Honey) - adds harmonics and warmth FIRST
    3. PHONE FILTER - shapes the saturated signal  
    4. UNDERWATER - modulated filtering for depth
    5. DELAY (Echo) - processes the shaped signal LAST
    6. OUTPUT GAIN
    
    This order ensures:
    - Saturation adds warmth to clean signal (no artifacts from filtered signal)
    - Phone filter works on harmonically rich material
    - Underwater creates space without muddying delays
    - Delay captures the fully processed "character" of the voice
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

HoneyVoxAudioProcessor::HoneyVoxAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Register parameter listeners for smooth bypass transitions
    apvts.addParameterListener ("phoneBypass", this);
    apvts.addParameterListener ("delayBypass", this);
    apvts.addParameterListener ("saturationBypass", this);
    apvts.addParameterListener ("underwaterBypass", this);
}

HoneyVoxAudioProcessor::~HoneyVoxAudioProcessor()
{
    apvts.removeParameterListener ("phoneBypass", this);
    apvts.removeParameterListener ("delayBypass", this);
    apvts.removeParameterListener ("saturationBypass", this);
    apvts.removeParameterListener ("underwaterBypass", this);
}

void HoneyVoxAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // Smooth bypass transitions
    if (parameterID == "phoneBypass")
        phoneMixSmoothed.setTargetValue (newValue < 0.5f ? 1.0f : 0.0f);
    else if (parameterID == "delayBypass")
        delayBypassMix.setTargetValue (newValue < 0.5f ? 1.0f : 0.0f);
    else if (parameterID == "saturationBypass")
        satMixSmoothed.setTargetValue (newValue < 0.5f ? 1.0f : 0.0f);
    else if (parameterID == "underwaterBypass")
        uwMixSmoothed.setTargetValue (newValue < 0.5f ? 1.0f : 0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout HoneyVoxAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Phone effect
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("phone", 1), "Phone", 0.0f, 100.0f, 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("phoneBypass", 1), "Phone Bypass", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("phoneMode", 1), "Phone Mode", 
        juce::StringArray { "Rotary", "Touch-Tone", "Mobile" }, 0));
    
    // Delay effect - TIME can be ms or synced
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("delayTime", 1), "Delay Time", 
        juce::NormalisableRange<float>(50.0f, 2000.0f, 1.0f, 0.4f), 250.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("delayFeedback", 1), "Delay Feedback", 0.0f, 100.0f, 35.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("delayMix", 1), "Delay Mix", 0.0f, 100.0f, 40.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("delayBypass", 1), "Delay Bypass", true));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("delayPingPong", 1), "Ping-Pong", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("delaySync", 1), "Delay Sync", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("delayDivision", 1), "Delay Division",
        juce::StringArray { "1/1", "1/2", "1/2 D", "1/2 T", "1/4", "1/4 D", "1/4 T", 
                           "1/8", "1/8 D", "1/8 T", "1/16", "1/16 D", "1/16 T" }, 4));
    
    // Saturation (Honey)
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("saturation", 1), "Honey", 0.0f, 100.0f, 25.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("saturationBypass", 1), "Saturation Bypass", true));
    
    // Underwater effect
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("underwater", 1), "Underwater", 0.0f, 100.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("underwaterBypass", 1), "Underwater Bypass", true));
    
    // Master output
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("outputGain", 1), "Output", -12.0f, 12.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("outputBypass", 1), "Output Bypass", false));
    
    return { params.begin(), params.end() };
}

float HoneyVoxAudioProcessor::divisionToMs (int division, double bpm) const
{
    if (bpm <= 0.0) bpm = 120.0;
    double beatMs = 60000.0 / bpm;  // Quarter note in ms
    
    switch (division)
    {
        case 0:  return (float)(beatMs * 4.0);           // 1/1
        case 1:  return (float)(beatMs * 2.0);           // 1/2
        case 2:  return (float)(beatMs * 3.0);           // 1/2 dotted
        case 3:  return (float)(beatMs * 4.0 / 3.0);     // 1/2 triplet
        case 4:  return (float)(beatMs);                 // 1/4
        case 5:  return (float)(beatMs * 1.5);           // 1/4 dotted
        case 6:  return (float)(beatMs * 2.0 / 3.0);     // 1/4 triplet
        case 7:  return (float)(beatMs * 0.5);           // 1/8
        case 8:  return (float)(beatMs * 0.75);          // 1/8 dotted
        case 9:  return (float)(beatMs / 3.0);           // 1/8 triplet
        case 10: return (float)(beatMs * 0.25);          // 1/16
        case 11: return (float)(beatMs * 0.375);         // 1/16 dotted
        case 12: return (float)(beatMs / 6.0);           // 1/16 triplet
        default: return (float)beatMs;
    }
}

const juce::String HoneyVoxAudioProcessor::getName() const { return JucePlugin_Name; }
bool HoneyVoxAudioProcessor::acceptsMidi() const { return false; }
bool HoneyVoxAudioProcessor::producesMidi() const { return false; }
bool HoneyVoxAudioProcessor::isMidiEffect() const { return false; }
double HoneyVoxAudioProcessor::getTailLengthSeconds() const { return 2.0; }
int HoneyVoxAudioProcessor::getNumPrograms() { return 1; }
int HoneyVoxAudioProcessor::getCurrentProgram() { return 0; }
void HoneyVoxAudioProcessor::setCurrentProgram (int) {}
const juce::String HoneyVoxAudioProcessor::getProgramName (int) { return {}; }
void HoneyVoxAudioProcessor::changeProgramName (int, const juce::String&) {}

void HoneyVoxAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    
    // Reset all filters
    phonePreEmphasisL.reset(); phonePreEmphasisR.reset();
    phoneHighpassL.reset(); phoneHighpassR.reset();
    phoneLowpassL.reset(); phoneLowpassR.reset();
    phoneMidBoostL.reset(); phoneMidBoostR.reset();
    phoneWarmthL.reset(); phoneWarmthR.reset();
    phonePostFilterL.reset(); phonePostFilterR.reset();
    
    delayLineL.reset(); delayLineR.reset();
    delayLineL.prepare(spec); delayLineR.prepare(spec);
    delayFeedbackHiCutL.reset(); delayFeedbackHiCutR.reset();
    delayFeedbackLoCutL.reset(); delayFeedbackLoCutR.reset();
    delayDampingL.reset(); delayDampingR.reset();
    feedbackL = 0.0f; feedbackR = 0.0f;
    
    uwMainFilterL.reset(); uwMainFilterR.reset();
    uwResonanceL.reset(); uwResonanceR.reset();
    uwWarmthL.reset(); uwWarmthR.reset();
    uwModDelayL.reset(); uwModDelayR.reset();
    uwModDelayL.prepare(spec); uwModDelayR.prepare(spec);
    
    satDcBlockL = 0.0f; satDcBlockR = 0.0f;
    
    // Initialize smoothed values with longer ramp for bypass (50ms)
    float bypassRampTime = 0.05f;
    float paramRampTime = 0.02f;
    
    phoneAmountSmoothed.reset(sampleRate, paramRampTime);
    phoneMixSmoothed.reset(sampleRate, bypassRampTime);
    delayTimeSmoothed.reset(sampleRate, 0.1f);  // Longer for pitch stability
    delayFeedbackSmoothed.reset(sampleRate, paramRampTime);
    delayMixSmoothed.reset(sampleRate, paramRampTime);
    delayBypassMix.reset(sampleRate, bypassRampTime);
    saturationAmountSmoothed.reset(sampleRate, paramRampTime);
    satMixSmoothed.reset(sampleRate, bypassRampTime);
    underwaterAmountSmoothed.reset(sampleRate, paramRampTime);
    uwMixSmoothed.reset(sampleRate, bypassRampTime);
    outputGainSmoothed.reset(sampleRate, paramRampTime);
    
    // Initialize bypass states
    phoneMixSmoothed.setCurrentAndTargetValue(apvts.getRawParameterValue("phoneBypass")->load() < 0.5f ? 1.0f : 0.0f);
    delayBypassMix.setCurrentAndTargetValue(apvts.getRawParameterValue("delayBypass")->load() < 0.5f ? 1.0f : 0.0f);
    satMixSmoothed.setCurrentAndTargetValue(apvts.getRawParameterValue("saturationBypass")->load() < 0.5f ? 1.0f : 0.0f);
    uwMixSmoothed.setCurrentAndTargetValue(apvts.getRawParameterValue("underwaterBypass")->load() < 0.5f ? 1.0f : 0.0f);
    
    // Delay feedback filters - warm analog-style rolloff
    delayFeedbackHiCutL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 4500.0f, 0.6f);
    delayFeedbackHiCutR.coefficients = delayFeedbackHiCutL.coefficients;
    delayFeedbackLoCutL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 80.0f, 0.7f);
    delayFeedbackLoCutR.coefficients = delayFeedbackLoCutL.coefficients;
    delayDampingL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 1000.0f, 0.7f, 0.85f);
    delayDampingR.coefficients = delayDampingL.coefficients;
}

void HoneyVoxAudioProcessor::releaseResources() {}

bool HoneyVoxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void HoneyVoxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get tempo from host
    if (auto* hostPlayHead = getPlayHead())
    {
        auto posInfo = hostPlayHead->getPosition();
        if (posInfo.hasValue())
        {
            if (auto bpm = posInfo->getBpm())
                currentBPM = *bpm;
        }
    }
    
    // === GET ALL PARAMETERS ===
    float phoneVal = apvts.getRawParameterValue("phone")->load();
    int phoneMode = static_cast<int>(apvts.getRawParameterValue("phoneMode")->load());
    
    float delayTimeVal = apvts.getRawParameterValue("delayTime")->load();
    float delayFeedbackVal = apvts.getRawParameterValue("delayFeedback")->load();
    float delayMixVal = apvts.getRawParameterValue("delayMix")->load();
    bool pingPong = apvts.getRawParameterValue("delayPingPong")->load() > 0.5f;
    bool delaySync = apvts.getRawParameterValue("delaySync")->load() > 0.5f;
    int delayDivision = static_cast<int>(apvts.getRawParameterValue("delayDivision")->load());
    
    float satVal = apvts.getRawParameterValue("saturation")->load();
    float uwVal = apvts.getRawParameterValue("underwater")->load();
    
    float outputGainDb = apvts.getRawParameterValue("outputGain")->load();
    bool outputOn = apvts.getRawParameterValue("outputBypass")->load() < 0.5f;
    float outputGain = outputOn ? juce::Decibels::decibelsToGain(outputGainDb) : 1.0f;
    
    // Calculate delay time (synced or ms)
    float actualDelayMs = delaySync ? divisionToMs(delayDivision, currentBPM) : delayTimeVal;
    actualDelayMs = juce::jlimit(20.0f, 2000.0f, actualDelayMs);
    
    // Set parameter targets
    phoneAmountSmoothed.setTargetValue(phoneVal / 100.0f);
    delayTimeSmoothed.setTargetValue(actualDelayMs);
    delayFeedbackSmoothed.setTargetValue(delayFeedbackVal / 100.0f * 0.92f);  // Cap at 92% for stability
    delayMixSmoothed.setTargetValue(delayMixVal / 100.0f);
    saturationAmountSmoothed.setTargetValue(satVal / 100.0f);
    underwaterAmountSmoothed.setTargetValue(uwVal / 100.0f);
    outputGainSmoothed.setTargetValue(outputGain);
    
    // === UPDATE PHONE FILTERS based on mode ===
    float phoneIntensity = phoneVal / 100.0f;
    
    // WARM phone filter parameters - less harsh, more musical
    float hpFreq, lpFreq, midFreq, midQ, midGainDb, warmthGain;
    
    switch (phoneMode)
    {
        case 0: // ROTARY (1920s-1950s) - Warm, lo-fi, carbon mic character
            hpFreq = 350.0f + phoneIntensity * 250.0f;    // Gentle bass cut (350-600 Hz)
            lpFreq = 2800.0f - phoneIntensity * 800.0f;   // Rounded highs (2000-2800 Hz)
            midFreq = 900.0f;                              // Lower, warmer peak
            midQ = 1.5f + phoneIntensity * 2.0f;          // Moderate resonance (1.5-3.5)
            midGainDb = 3.0f + phoneIntensity * 5.0f;     // Gentle boost (+3 to +8 dB)
            warmthGain = 1.5f + phoneIntensity * 1.5f;    // Add low warmth
            break;
            
        case 1: // TOUCH-TONE (1960s-1980s) - Clear but band-limited
            hpFreq = 280.0f + phoneIntensity * 120.0f;    // Light bass cut (280-400 Hz)
            lpFreq = 3600.0f - phoneIntensity * 600.0f;   // Clearer highs (3000-3600 Hz)
            midFreq = 1400.0f;                             // Presence
            midQ = 1.2f + phoneIntensity * 1.0f;          // Gentle (1.2-2.2)
            midGainDb = 2.0f + phoneIntensity * 3.0f;     // Subtle (+2 to +5 dB)
            warmthGain = 1.2f + phoneIntensity * 0.8f;    // Slight warmth
            break;
            
        case 2: // MOBILE (1990s-2000s) - Digital but musical, not harsh
            hpFreq = 200.0f + phoneIntensity * 200.0f;    // Moderate bass (200-400 Hz)
            lpFreq = 4200.0f - phoneIntensity * 1000.0f;  // More bandwidth (3200-4200 Hz)
            midFreq = 2000.0f;                             // Higher presence
            midQ = 2.0f + phoneIntensity * 2.5f;          // Tighter (2.0-4.5)
            midGainDb = 3.0f + phoneIntensity * 4.0f;     // Moderate (+3 to +7 dB)
            warmthGain = 1.0f + phoneIntensity * 0.5f;    // Less warmth (digital)
            break;
            
        default:
            hpFreq = 300.0f; lpFreq = 3400.0f; midFreq = 1200.0f; midQ = 1.5f; 
            midGainDb = 3.0f; warmthGain = 1.2f;
    }
    
    // Set phone filter coefficients - WARM versions
    float hpQ = 0.5f + phoneIntensity * 0.3f;  // Gentler slope
    float lpQ = 0.5f + phoneIntensity * 0.3f;
    
    phoneHighpassL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, hpFreq, hpQ);
    phoneHighpassR.coefficients = phoneHighpassL.coefficients;
    phoneLowpassL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, lpFreq, lpQ);
    phoneLowpassR.coefficients = phoneLowpassL.coefficients;
    
    float midGainLinear = juce::Decibels::decibelsToGain(midGainDb);
    phoneMidBoostL.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, midFreq, midQ, midGainLinear);
    phoneMidBoostR.coefficients = phoneMidBoostL.coefficients;
    
    // Warmth: low shelf boost
    phoneWarmthL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, 300.0f, 0.7f, warmthGain);
    phoneWarmthR.coefficients = phoneWarmthL.coefficients;
    
    // Post filter: gentle smoothing to remove harshness
    float postFreq = lpFreq * 1.1f;
    phonePostFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, postFreq, 0.5f);
    phonePostFilterR.coefficients = phonePostFilterL.coefficients;
    
    // === UPDATE UNDERWATER FILTERS ===
    float uwIntensity = uwVal / 100.0f;
    float uwCutoff = 6000.0f * std::pow(0.08f, uwIntensity);  // Less extreme
    uwCutoff = std::max(uwCutoff, 300.0f);
    float uwQ = 0.6f + uwIntensity * 0.8f;  // Gentler resonance
    
    uwMainFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, uwCutoff, uwQ);
    uwMainFilterR.coefficients = uwMainFilterL.coefficients;
    
    // Resonance for "bubble" character
    float resFreq = uwCutoff * 0.7f;
    float resQ = 1.0f + uwIntensity * 1.5f;
    float resGain = juce::Decibels::decibelsToGain(2.0f * uwIntensity);
    uwResonanceL.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, resFreq, resQ, resGain);
    uwResonanceR.coefficients = uwResonanceL.coefficients;
    
    // Warmth shelf
    float uwWarmthGain = 1.0f + uwIntensity * 0.8f;
    uwWarmthL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, 400.0f, 0.6f, uwWarmthGain);
    uwWarmthR.coefficients = uwWarmthL.coefficients;
    
    // === PROCESS SAMPLES ===
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float sr = static_cast<float>(currentSampleRate);
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Get smoothed values
        float phoneAmt = phoneAmountSmoothed.getNextValue();
        float phoneMix = phoneMixSmoothed.getNextValue();
        float delayTime = delayTimeSmoothed.getNextValue();
        float delayFb = delayFeedbackSmoothed.getNextValue();
        float delayMix = delayMixSmoothed.getNextValue();
        float delayActive = delayBypassMix.getNextValue();
        float satAmt = saturationAmountSmoothed.getNextValue();
        float satMix = satMixSmoothed.getNextValue();
        float uwAmt = underwaterAmountSmoothed.getNextValue();
        float uwMix = uwMixSmoothed.getNextValue();
        float outGain = outputGainSmoothed.getNextValue();
        
        float inL = leftChannel[sample];
        float inR = rightChannel ? rightChannel[sample] : inL;
        
        // Store dry signal for mixing
        float dryL = inL;
        float dryR = inR;
        
        // ============================================================
        // 1. SATURATION (Honey) - HG-2 inspired warm saturation
        // ============================================================
        float satL = inL;
        float satR = inR;
        
        if (satMix > 0.001f && satAmt > 0.001f)
        {
            // Input gain staging
            float inputGain = 1.0f + satAmt * 1.5f;
            satL *= inputGain;
            satR *= inputGain;
            
            // Stage 1: Tube-style warmth (even harmonics)
            // Soft asymmetric curve that adds 2nd harmonic
            float tubeDrive = 0.8f + satAmt * 0.4f;
            satL = satL * tubeDrive / (1.0f + std::abs(satL * tubeDrive) * 0.3f);
            satR = satR * tubeDrive / (1.0f + std::abs(satR * tubeDrive) * 0.3f);
            
            // Add subtle 2nd harmonic (even)
            satL += std::abs(satL) * satL * 0.15f * satAmt;
            satR += std::abs(satR) * satR * 0.15f * satAmt;
            
            // Stage 2: Tape-style saturation (odd harmonics, compression)
            float tapeDrive = 1.0f + satAmt * 0.8f;
            satL = std::tanh(satL * tapeDrive) / tapeDrive;
            satR = std::tanh(satR * tapeDrive) / tapeDrive;
            
            // Stage 3: Transformer coloration (subtle)
            float xfmrAmt = satAmt * 0.3f;
            satL = satL * (1.0f - xfmrAmt) + std::tanh(satL * 1.2f) * xfmrAmt;
            satR = satR * (1.0f - xfmrAmt) + std::tanh(satR * 1.2f) * xfmrAmt;
            
            // DC blocking (simple high-pass)
            float dcCoeff = 0.995f;
            satDcBlockL = satL - satDcBlockL * dcCoeff + satDcBlockL;
            float dcFreeL = satL - satDcBlockL;
            satDcBlockR = satR - satDcBlockR * dcCoeff + satDcBlockR;
            float dcFreeR = satR - satDcBlockR;
            satL = dcFreeL;
            satR = dcFreeR;
            
            // Output gain compensation (louder input = less makeup)
            float makeupGain = 1.0f / (1.0f + satAmt * 0.4f);
            satL *= makeupGain;
            satR *= makeupGain;
            
            // Mix with dry based on satMix (bypass crossfade)
            inL = dryL * (1.0f - satMix) + satL * satMix;
            inR = dryR * (1.0f - satMix) + satR * satMix;
        }
        
        // ============================================================
        // 2. PHONE FILTER - warm vintage phone character
        // ============================================================
        float phoneL = inL;
        float phoneR = inR;
        
        if (phoneMix > 0.001f && phoneAmt > 0.001f)
        {
            // Multi-stage filtering with warmth
            phoneL = phoneHighpassL.processSample(phoneL);
            phoneL = phoneMidBoostL.processSample(phoneL);
            phoneL = phoneWarmthL.processSample(phoneL);
            phoneL = phoneLowpassL.processSample(phoneL);
            phoneL = phonePostFilterL.processSample(phoneL);
            
            phoneR = phoneHighpassR.processSample(phoneR);
            phoneR = phoneMidBoostR.processSample(phoneR);
            phoneR = phoneWarmthR.processSample(phoneR);
            phoneR = phoneLowpassR.processSample(phoneR);
            phoneR = phonePostFilterR.processSample(phoneR);
            
            // Gentle saturation for character (mode-dependent)
            if (phoneMode == 0)  // Rotary - warm tube-like
            {
                phoneL = phoneL / (1.0f + std::abs(phoneL) * 0.2f * phoneAmt);
                phoneR = phoneR / (1.0f + std::abs(phoneR) * 0.2f * phoneAmt);
            }
            else if (phoneMode == 2)  // Mobile - subtle digital compression
            {
                float comp = 1.0f + phoneAmt * 0.3f;
                phoneL = std::tanh(phoneL * comp) / comp;
                phoneR = std::tanh(phoneR * comp) / comp;
            }
            
            // Crossfade: dry->phone based on amount, then bypass crossfade
            float phoneWet = phoneAmt;
            phoneL = inL * (1.0f - phoneWet) + phoneL * phoneWet;
            phoneR = inR * (1.0f - phoneWet) + phoneR * phoneWet;
            
            // Bypass crossfade
            inL = inL * (1.0f - phoneMix) + phoneL * phoneMix;
            inR = inR * (1.0f - phoneMix) + phoneR * phoneMix;
        }
        
        // ============================================================
        // 3. UNDERWATER - spacey, wide, warm
        // ============================================================
        float uwL = inL;
        float uwR = inR;
        
        if (uwMix > 0.001f && uwAmt > 0.001f)
        {
            // Main filtering
            uwL = uwMainFilterL.processSample(uwL);
            uwL = uwResonanceL.processSample(uwL);
            uwL = uwWarmthL.processSample(uwL);
            
            uwR = uwMainFilterR.processSample(uwR);
            uwR = uwResonanceR.processSample(uwR);
            uwR = uwWarmthR.processSample(uwR);
            
            // Modulated delay for movement and stereo width
            float modRate = 0.3f + uwAmt * 0.4f;  // 0.3-0.7 Hz
            float modDepth = 1.5f + uwAmt * 2.5f;  // 1.5-4ms
            float modDepthSamples = modDepth * sr / 1000.0f;
            
            uwModPhaseL += modRate * twoPi / sr;
            uwModPhaseR += modRate * twoPi / sr;
            if (uwModPhaseL > twoPi) uwModPhaseL -= twoPi;
            if (uwModPhaseR > twoPi) uwModPhaseR -= twoPi;
            
            float modL = std::sin(uwModPhaseL) * modDepthSamples;
            float modR = std::sin(uwModPhaseR + 1.5f) * modDepthSamples;  // Phase offset for width
            
            uwModDelayL.pushSample(0, uwL);
            uwModDelayR.pushSample(0, uwR);
            
            float delayedL = uwModDelayL.popSample(0, 10.0f + modL);
            float delayedR = uwModDelayR.popSample(0, 10.0f + modR);
            
            // Blend modulated with direct
            float modMix = 0.3f + uwAmt * 0.4f;
            uwL = uwL * (1.0f - modMix) + delayedL * modMix;
            uwR = uwR * (1.0f - modMix) + delayedR * modMix;
            
            // Subtle stereo widening
            float mid = (uwL + uwR) * 0.5f;
            float side = (uwL - uwR) * 0.5f;
            side *= 1.0f + uwAmt * 0.3f;
            uwL = mid + side;
            uwR = mid - side;
            
            // Crossfade
            float uwWet = uwAmt;
            uwL = inL * (1.0f - uwWet) + uwL * uwWet;
            uwR = inR * (1.0f - uwWet) + uwR * uwWet;
            
            // Bypass crossfade
            inL = inL * (1.0f - uwMix) + uwL * uwMix;
            inR = inR * (1.0f - uwMix) + uwR * uwMix;
        }
        
        // ============================================================
        // 4. DELAY (Echo) - H-Delay style with proper ping-pong
        // ============================================================
        float delayOutL = 0.0f;
        float delayOutR = 0.0f;
        
        if (delayActive > 0.001f && delayMix > 0.001f)
        {
            float delaySamples = (delayTime / 1000.0f) * sr;
            
            // Subtle modulation for organic feel
            delayModPhase += 0.6f * twoPi / sr;
            if (delayModPhase > twoPi) delayModPhase -= twoPi;
            float mod = std::sin(delayModPhase) * 0.3f * sr / 1000.0f;
            
            if (pingPong)
            {
                // TRUE PING-PONG: L->R->L->R alternating
                // Read from delay lines
                float tapL = delayLineL.popSample(0, delaySamples + mod);
                float tapR = delayLineR.popSample(0, delaySamples - mod * 0.5f);
                
                // Filter the feedback (analog-style degradation)
                tapL = delayFeedbackHiCutL.processSample(tapL);
                tapL = delayFeedbackLoCutL.processSample(tapL);
                tapL = delayDampingL.processSample(tapL);
                
                tapR = delayFeedbackHiCutR.processSample(tapR);
                tapR = delayFeedbackLoCutR.processSample(tapR);
                tapR = delayDampingR.processSample(tapR);
                
                // Soft saturation in feedback
                tapL = std::tanh(tapL * 1.1f) / 1.1f;
                tapR = std::tanh(tapR * 1.1f) / 1.1f;
                
                // Ping-pong routing:
                // Left delay receives: mono input + feedback from RIGHT
                // Right delay receives: feedback from LEFT only
                float monoIn = (inL + inR) * 0.5f;
                
                delayLineL.pushSample(0, monoIn + tapR * delayFb);
                delayLineR.pushSample(0, tapL * delayFb);
                
                delayOutL = tapL;
                delayOutR = tapR;
            }
            else
            {
                // Standard stereo delay
                float tapL = delayLineL.popSample(0, delaySamples + mod);
                float tapR = delayLineR.popSample(0, delaySamples - mod * 0.5f);
                
                tapL = delayFeedbackHiCutL.processSample(tapL);
                tapL = delayFeedbackLoCutL.processSample(tapL);
                tapL = delayDampingL.processSample(tapL);
                
                tapR = delayFeedbackHiCutR.processSample(tapR);
                tapR = delayFeedbackLoCutR.processSample(tapR);
                tapR = delayDampingR.processSample(tapR);
                
                tapL = std::tanh(tapL * 1.1f) / 1.1f;
                tapR = std::tanh(tapR * 1.1f) / 1.1f;
                
                delayLineL.pushSample(0, inL + tapL * delayFb);
                delayLineR.pushSample(0, inR + tapR * delayFb);
                
                delayOutL = tapL;
                delayOutR = tapR;
            }
            
            // Mix delay with dry (delayMix controls wet amount)
            float wetL = delayOutL * delayMix;
            float wetR = delayOutR * delayMix;
            
            // Bypass crossfade
            inL = inL + wetL * delayActive;
            inR = inR + wetR * delayActive;
        }
        else
        {
            // Still push to delay lines to prevent artifacts when re-enabled
            delayLineL.pushSample(0, 0.0f);
            delayLineR.pushSample(0, 0.0f);
        }
        
        // ============================================================
        // 5. CABLE HUM (subtle vintage warmth from easter egg screw)
        // ============================================================
        float humAmount = cableHumAmount.load();
        if (humAmount > 0.001f)
        {
            // 60Hz fundamental + harmonics for authentic hum
            float hum60 = std::sin(cableHumPhase) * 0.4f;
            float hum120 = std::sin(cableHumPhase * 2.0f) * 0.25f;
            float hum180 = std::sin(cableHumPhase * 3.0f) * 0.1f;
            
            // Slight random flutter for vintage character
            float flutter = std::sin(cableHumPhase2) * 0.15f;
            
            float humSignal = (hum60 + hum120 + hum180) * (1.0f + flutter);
            humSignal *= humAmount * 0.008f;  // Very subtle - max 0.8% of signal
            
            inL += humSignal;
            inR += humSignal * 0.95f;  // Slight stereo difference
            
            // Advance phases
            cableHumPhase += 2.0f * juce::MathConstants<float>::pi * 60.0f / (float)currentSampleRate;
            if (cableHumPhase > juce::MathConstants<float>::twoPi)
                cableHumPhase -= juce::MathConstants<float>::twoPi;
            
            cableHumPhase2 += 2.0f * juce::MathConstants<float>::pi * 0.3f / (float)currentSampleRate;
            if (cableHumPhase2 > juce::MathConstants<float>::twoPi)
                cableHumPhase2 -= juce::MathConstants<float>::twoPi;
        }
        
        // ============================================================
        // 6. OUTPUT GAIN
        // ============================================================
        inL *= outGain;
        inR *= outGain;
        
        // Gentle final limiting
        inL = std::tanh(inL * 0.9f) / 0.9f;
        inR = std::tanh(inR * 0.9f) / 0.9f;
        
        leftChannel[sample] = inL;
        if (rightChannel) rightChannel[sample] = inR;
    }
}

bool HoneyVoxAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* HoneyVoxAudioProcessor::createEditor() { return new HoneyVoxAudioProcessorEditor(*this); }

void HoneyVoxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HoneyVoxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HoneyVoxAudioProcessor(); }
