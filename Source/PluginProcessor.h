#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

class NonlinearDistortionAudioProcessor : public juce::AudioProcessor
{
public:
    NonlinearDistortionAudioProcessor();
    ~NonlinearDistortionAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    static float applyDistortion(float input, float gain);

private:
    void fillupsampleBuffer(const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& upsampled,
        int L);
    void filldownsampleBuffer(const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& downsampled,
        int L);
    void copyToOutput(const juce::AudioBuffer<float>& src,
        juce::AudioBuffer<float>& dst);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NonlinearDistortionAudioProcessor)
    juce::AudioBuffer<float> upsampledBuffer;
    juce::AudioBuffer<float> downsampledBuffer;

        std::shared_ptr<juce::dsp::FIR::Coefficients<float>> firCoeffsPtr;
    juce::dsp::ProcessorDuplicator<
        juce::dsp::FIR::Filter<float>,
        juce::dsp::FIR::Coefficients<float>
    > upsampleFIR, downsampleFIR;
};
