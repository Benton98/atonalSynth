/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    {

    }
};


//==============================================================================
/**
*/
class AtonalSynthAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AtonalSynthAudioProcessorEditor (AtonalSynthAudioProcessor&);
    ~AtonalSynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AtonalSynthAudioProcessor& audioProcessor;

    CustomRotarySlider peakFreqSlider,
        peakGainSlider,
        peakQualitySlider,
        dipGainSlider,
        dipQualitySlider,
        dipFreqSlider,
        lowCutFreqSlider,
        highCutFreqSlider,
        lowCutSlopeSlider,
        highCutSlopeSlider;

    juce::Label peakFreqLabel,
        peakGainLabel,
        peakQualityLabel,
        dipGainLabel,
        dipQualityLabel,
        dipFreqLabel,
        lowCutFreqLabel,
        highCutFreqLabel,
        lowCutSlopeLabel,
        highCutSlopeLabel;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment,
        peakGainSliderAttachment,
        peakQualitySliderAttachment,
        dipFreqSliderAttachment,
        dipGainSliderAttachment,
        dipQualitySliderAttachment,
        lowCutFreqSliderAttachment,
        highCutFreqSliderAttachment,
        lowCutSlopeSliderAttachment,
        highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtonalSynthAudioProcessorEditor)
};
