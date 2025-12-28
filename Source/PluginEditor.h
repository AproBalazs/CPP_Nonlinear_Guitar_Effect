/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class NonlinearDistortionAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    NonlinearDistortionAudioProcessorEditor (NonlinearDistortionAudioProcessor&);
    ~NonlinearDistortionAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    NonlinearDistortionAudioProcessor& audioProcessor;
	juce::Slider gainSlider;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NonlinearDistortionAudioProcessorEditor)
};
