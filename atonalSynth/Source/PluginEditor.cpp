/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================


AtonalSynthAudioProcessorEditor::AtonalSynthAudioProcessorEditor (AtonalSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    dipQualitySliderAttachment(audioProcessor.apvts, "Dip Quality", dipQualitySlider),
    dipFreqSliderAttachment(audioProcessor.apvts, "Dip Freq", dipFreqSlider),
    dipGainSliderAttachment(audioProcessor.apvts, "Dip Gain", dipGainSlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
    

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    // Display

    //peakFreqSlider.setRange(20.0f, 20480.0f);
    peakFreqLabel.setText("Peak Freq", juce::NotificationType::dontSendNotification);
    peakFreqLabel.setJustificationType(juce::Justification::centredTop);
    peakFreqLabel.attachToComponent(&peakFreqSlider,false);

    /*
    peakGainLabel.setText("Peak Gain", juce::NotificationType::dontSendNotification);
    peakGainLabel.setJustificationType(juce::Justification::bottom);
    peakGainLabel.attachToComponent(&peakGainSlider, false);
    */
    /*
    peakQualityLabel.setText("Peak Quality", juce::NotificationType::dontSendNotification);
    peakQualityLabel.setJustificationType(juce::Justification::centredTop);
    peakQualityLabel.attachToComponent(&peakQualitySlider, false);

    dipGainLabel.setText("Dip Gain", juce::NotificationType::dontSendNotification);
    dipGainLabel.setJustificationType(juce::Justification::centredTop);
    dipGainLabel.attachToComponent(&dipGainSlider, false);
    
    dipQualityLabel.setText("Dip Freq", juce::NotificationType::dontSendNotification);
    dipQualityLabel.setJustificationType(juce::Justification::centredTop);
    dipQualityLabel.attachToComponent(&dipQualitySlider, false);
    */
    dipFreqLabel.setText("Dip Freq", juce::NotificationType::dontSendNotification);
    dipFreqLabel.setJustificationType(juce::Justification::centredTop);
    dipFreqLabel.attachToComponent(&dipFreqSlider, false);
    /*
    lowCutFreqLabel.setText("HPF", juce::NotificationType::dontSendNotification);
    lowCutFreqLabel.setJustificationType(juce::Justification::centredTop);
    lowCutFreqLabel.attachToComponent(&lowCutFreqLabel, false);

    highCutFreqLabel.setText("LPF", juce::NotificationType::dontSendNotification);
    highCutFreqLabel.setJustificationType(juce::Justification::centredTop);
    highCutFreqLabel.attachToComponent(&highCutFreqLabel, false);

    lowCutSlopeLabel.setText("HPF Slope", juce::NotificationType::dontSendNotification);
    lowCutSlopeLabel.setJustificationType(juce::Justification::centredTop);
    lowCutSlopeLabel.attachToComponent(&lowCutSlopeLabel, false);

    highCutSlopeLabel.setText("LPF Slope", juce::NotificationType::dontSendNotification);
    highCutSlopeLabel.setJustificationType(juce::Justification::centredTop);
    highCutSlopeLabel.attachToComponent(&highCutSlopeLabel, false);
    */

    for (auto* comp : getComps())
    {

        addAndMakeVisible(comp);
    }

    setSize (600, 400);
}

AtonalSynthAudioProcessorEditor::~AtonalSynthAudioProcessorEditor()
{
}

//==============================================================================
void AtonalSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("", getLocalBounds(), juce::Justification::centred, 1);
    
}

void AtonalSynthAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.25);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.33);
    auto dipArea = bounds.removeFromLeft(bounds.getWidth() * .5);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(bounds.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutFreqSlider.setBounds(highCutArea.removeFromTop(bounds.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    dipFreqSlider.setBounds(dipArea.removeFromTop(dipArea.getHeight() * 0.33));
    dipGainSlider.setBounds(dipArea.removeFromTop(dipArea.getHeight() * 0.5));
    dipQualitySlider.setBounds(dipArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
    
    peakFreqSlider.setSkewFactorFromMidPoint(1024.0);;

}

std::vector<juce::Component*> AtonalSynthAudioProcessorEditor::getComps() {

    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &dipFreqSlider,
        &dipGainSlider,
        &dipQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider

    };

}