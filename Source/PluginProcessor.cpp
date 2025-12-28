/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>
#include <vector>
//==============================================================================
float firCoeffs[] = {
    -0.0001f, 0.0004f, 0.0014f, -0.0034f, -0.0073f,
    0.0139f, 0.0247f, -0.0423f, -0.0729f, 0.1392f,
    0.4464f, 0.4464f, 0.1392f, -0.0729f, -0.0423f,
    0.0247f, 0.0139f, -0.0073f, -0.0034f, 0.0014f,
    0.0004f, -0.0001f
};

NonlinearDistortionAudioProcessor::NonlinearDistortionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
}

NonlinearDistortionAudioProcessor::~NonlinearDistortionAudioProcessor()
{
}

//==============================================================================
const juce::String NonlinearDistortionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NonlinearDistortionAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool NonlinearDistortionAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool NonlinearDistortionAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double NonlinearDistortionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NonlinearDistortionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int NonlinearDistortionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NonlinearDistortionAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String NonlinearDistortionAudioProcessor::getProgramName(int index)
{
    return {};
}

void NonlinearDistortionAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void NonlinearDistortionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    int L = 4;

    // --- 1. KOEFFICIENSEK BETÖLTÉSE ---
    // Helyi vektorba másoljuk az adatokat
    std::vector<float> coeffsUpsample(std::begin(firCoeffs), std::end(firCoeffs));
    std::vector<float> coeffsDownsample(std::begin(firCoeffs), std::end(firCoeffs));

    // Upsample szűrő: Gain kompenzáció (szorzás L-lel)
    // Mivel a "zero stuffing" leosztja az energiát, a szűrőnek kell visszahoznia.
    for (auto& c : coeffsUpsample)
        c *= (float)L;

    // Downsample szűrő: Hagyjuk Unity Gain-en (összeg ~1.0), nem kell módosítani.
    // (A te kódodban osztva volt, ami halkulást okozott volna)

    // JUCE specifikus pointer létrehozása
    auto upState = new juce::dsp::FIR::Coefficients<float>(coeffsUpsample.data(), coeffsUpsample.size());
    auto downState = new juce::dsp::FIR::Coefficients<float>(coeffsDownsample.data(), coeffsDownsample.size());

    upsampleFIR.state = upState;
    downsampleFIR.state = downState;

    // --- 2. SPEC & RESET ---
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate * L;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock * L);
    spec.numChannels = getTotalNumOutputChannels();

    upsampleFIR.prepare(spec);
    downsampleFIR.prepare(spec);

    upsampleFIR.reset();
    downsampleFIR.reset();

    // --- 3. BUFFER ALLOKÁCIÓ ---
    // Itt foglalunk memóriát, nem a processBlock-ban!
    upsampledBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock * L);
    downsampledBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock); // Opcionális, ha csak helyben használnád
}



void NonlinearDistortionAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NonlinearDistortionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void NonlinearDistortionAudioProcessor::fillupsampleBuffer(
    const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& upsampled,
    int L)
{
    int numInputSamples = input.getNumSamples();
    int numChannels = input.getNumChannels();

    upsampled.clear();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* in = input.getReadPointer(ch);
        auto* out = upsampled.getWritePointer(ch);

        for (int n = 0; n < numInputSamples; ++n)
            out[n * L] = in[n];
    }
}

void NonlinearDistortionAudioProcessor::filldownsampleBuffer(
    const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& downsampled,
    int L)
{
    int numOutputSamples = downsampled.getNumSamples();
    int numChannels = input.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* in = input.getReadPointer(ch);
        auto* out = downsampled.getWritePointer(ch);

        for (int n = 0; n < numOutputSamples; ++n)
            out[n] = in[n * L];
    }
}

void NonlinearDistortionAudioProcessor::copyToOutput(
    const juce::AudioBuffer<float>& src,
    juce::AudioBuffer<float>& dst)
{
    int numChannels = dst.getNumChannels();
    int numSamples = dst.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        std::memcpy(dst.getWritePointer(ch),
            src.getReadPointer(ch),
            numSamples * sizeof(float));
    }
}


juce::AudioProcessorValueTreeState::ParameterLayout NonlinearDistortionAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GAIN",   // azonos
        "Gain",   // felirat a GUI-n
        1.0f,     // minimum
        50.f,    // maximum
        10.0f));   // default ertek

    return { params.begin(), params.end() };
}
float NonlinearDistortionAudioProcessor::applyDistortion(float input, float gain)
{
    return std::clamp(gain * input, -0.6f, 0.6f);
}

void NonlinearDistortionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Paraméterek
    float gain = *apvts.getRawParameterValue("GAIN");
    int L = 4;
    int numInputSamples = buffer.getNumSamples();

    // Biztonsági ellenőrzés (ha a buffer méret változna menet közben ritka esetekben)
    if (upsampledBuffer.getNumSamples() < numInputSamples * L)
        upsampledBuffer.setSize(buffer.getNumChannels(), numInputSamples * L);

    upsampledBuffer.clear();
    // downsampledBuffer-t nem feltétlen kell clear-elni, mert felülírjuk, de biztonságosabb:
    // downsampledBuffer.setSize(buffer.getNumChannels(), numInputSamples); // Ha kellene

    // 1. Interpoláció (Zero stuffing)
    fillupsampleBuffer(buffer, upsampledBuffer, L);

    // 2. Anti-Imaging szűrő (Upsampling után kötelező, különben tükörfrekvenciák lesznek)
    {
        juce::dsp::AudioBlock<float> block(upsampledBuffer);
        // Csak a releváns mintákat dolgozzuk fel
        juce::dsp::AudioBlock<float> subBlock = block.getSubBlock(0, numInputSamples * L);
        juce::dsp::ProcessContextReplacing<float> ctx(subBlock);
        upsampleFIR.process(ctx);
    }

    // 3. Torzítás
    for (int ch = 0; ch < upsampledBuffer.getNumChannels(); ++ch)
    {
        auto* data = upsampledBuffer.getWritePointer(ch);
        // Fontos: csak addig iteráljunk, amennyi minta ténylegesen van
        for (int n = 0; n < numInputSamples * L; ++n)
            data[n] = applyDistortion(data[n], gain);
    }

    // 4. Anti-Aliasing szűrő (Decimálás előtt)
    {
        juce::dsp::AudioBlock<float> block(upsampledBuffer);
        juce::dsp::AudioBlock<float> subBlock = block.getSubBlock(0, numInputSamples * L);
        juce::dsp::ProcessContextReplacing<float> ctx(subBlock);
        downsampleFIR.process(ctx);
    }

    // 5. Decimálás
    filldownsampleBuffer(upsampledBuffer, downsampledBuffer, L);

    //Eredeti bufferbe való viisszaírás
	copyToOutput(downsampledBuffer, buffer);
}


//==============================================================================
bool NonlinearDistortionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NonlinearDistortionAudioProcessor::createEditor()
{
    return new NonlinearDistortionAudioProcessorEditor(*this);
}

//==============================================================================
void NonlinearDistortionAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void NonlinearDistortionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NonlinearDistortionAudioProcessor();
}