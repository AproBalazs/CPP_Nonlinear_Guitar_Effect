/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NonlinearDistortionAudioProcessorEditor::NonlinearDistortionAudioProcessorEditor (NonlinearDistortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    gainSlider.setSliderStyle(juce::Slider::Rotary);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    audioProcessor.apvts, "GAIN", gainSlider);
   
    addAndMakeVisible(gainSlider);

    setSize (300, 300);
}

NonlinearDistortionAudioProcessorEditor::~NonlinearDistortionAudioProcessorEditor()
{
}

//==============================================================================
void NonlinearDistortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Non-linear Guitar Effect", getLocalBounds(), juce::Justification::centredTop, 1);

}

void NonlinearDistortionAudioProcessorEditor::resized()
{
    gainSlider.setBounds(50, 60, 100, 100);
}
