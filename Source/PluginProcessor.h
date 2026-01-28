/*
  ==============================================================================
    HoneyVox Ad-Lib FX
    Created by Nolo's Addiction
    
    Optimized for vocal ad-libs with warm, musical processing
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class HoneyVoxAudioProcessor : public juce::AudioProcessor,
                                public juce::AudioProcessorValueTreeState::Listener
{
public:
    HoneyVoxAudioProcessor();
    ~HoneyVoxAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    
    juce::AudioProcessorValueTreeState apvts;
    
private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // === PHONE FILTERS - warm multi-stage ===
    juce::dsp::IIR::Filter<float> phonePreEmphasisL, phonePreEmphasisR;
    juce::dsp::IIR::Filter<float> phoneHighpassL, phoneHighpassR;
    juce::dsp::IIR::Filter<float> phoneLowpassL, phoneLowpassR;
    juce::dsp::IIR::Filter<float> phoneMidBoostL, phoneMidBoostR;
    juce::dsp::IIR::Filter<float> phoneWarmthL, phoneWarmthR;
    juce::dsp::IIR::Filter<float> phonePostFilterL, phonePostFilterR;
    
    // === DELAY - H-Delay style with proper ping-pong ===
    static constexpr int maxDelaySamples = 192000;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineL{maxDelaySamples};
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineR{maxDelaySamples};
    
    juce::dsp::IIR::Filter<float> delayFeedbackHiCutL, delayFeedbackHiCutR;
    juce::dsp::IIR::Filter<float> delayFeedbackLoCutL, delayFeedbackLoCutR;
    juce::dsp::IIR::Filter<float> delayDampingL, delayDampingR;
    
    float feedbackL = 0.0f, feedbackR = 0.0f;
    
    // === SATURATION - HG-2 inspired ===
    float satDcBlockL = 0.0f, satDcBlockR = 0.0f;
    
    // === UNDERWATER - spacey, wide, warm ===
    juce::dsp::IIR::Filter<float> uwMainFilterL, uwMainFilterR;
    juce::dsp::IIR::Filter<float> uwResonanceL, uwResonanceR;
    juce::dsp::IIR::Filter<float> uwWarmthL, uwWarmthR;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> uwModDelayL{4800};
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> uwModDelayR{4800};
    float uwModPhaseL = 0.0f, uwModPhaseR = 0.33f;
    
    // === SMOOTHED PARAMETERS ===
    juce::SmoothedValue<float> phoneAmountSmoothed;
    juce::SmoothedValue<float> phoneMixSmoothed;
    juce::SmoothedValue<float> delayTimeSmoothed;
    juce::SmoothedValue<float> delayFeedbackSmoothed;
    juce::SmoothedValue<float> delayMixSmoothed;
    juce::SmoothedValue<float> delayBypassMix;
    juce::SmoothedValue<float> saturationAmountSmoothed;
    juce::SmoothedValue<float> satMixSmoothed;
    juce::SmoothedValue<float> underwaterAmountSmoothed;
    juce::SmoothedValue<float> uwMixSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> cableHumSmoothed;
    
    float delayModPhase = 0.0f;
    double currentBPM = 120.0;
    double currentSampleRate = 44100.0;
    
    // Cable hum oscillator
    float cableHumPhase = 0.0f;
    float cableHumPhase2 = 0.0f;
    
    float divisionToMs (int division, double bpm) const;
    
public:
    // Cable hum amount (set from editor)
    std::atomic<float> cableHumAmount { 0.0f };
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HoneyVoxAudioProcessor)
};
