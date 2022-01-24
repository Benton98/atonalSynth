/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <windows.h>
#include <iostream>

float peakFreq = 0;


//==============================================================================
AtonalSynthAudioProcessor::AtonalSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

AtonalSynthAudioProcessor::~AtonalSynthAudioProcessor()
{
}

//==============================================================================
const juce::String AtonalSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AtonalSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AtonalSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AtonalSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AtonalSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AtonalSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AtonalSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AtonalSynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AtonalSynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void AtonalSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AtonalSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec spec;


    spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = 1;

    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);


    updateFilters();

}

void AtonalSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AtonalSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
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

void AtonalSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();



    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        
        if (message.isNoteOn())
        {
            //apvts.getRawParameterValue()
            
            peakFreq = message.getMidiNoteInHertz(message.getNoteNumber());
            updateFilters();
        }

    }

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//==============================================================================
bool AtonalSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AtonalSynthAudioProcessor::createEditor()
{
    return new AtonalSynthAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void AtonalSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);

}

void AtonalSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())\
    {
        apvts.replaceState(tree);
        updateFilters();
    }
    
}


ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = peakFreq;
    settings.peakGainInDecibles = apvts.getRawParameterValue("Peak Gain")->load();
    settings.lowCutSlope = static_cast<Slope>  (peakFreq/2);
    settings.highCutSlope = static_cast<Slope> (peakFreq*12);

    return settings;
}

void AtonalSynthAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                            chainSettings.peakFreq,
                            chainSettings.peakQuality,
                            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));


        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<0>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<0>().coefficients, peakCoefficients);


        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq*1.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0-chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<1>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<1>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 2,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<2>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<2>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 2.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0-chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<3>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<3>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 3,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<4>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<4>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 3.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0 - chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<5>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<5>().coefficients, peakCoefficients);


        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 4,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<6>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<6>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 4.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0 - chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<7>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<7>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<8>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<8>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 5.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0 - chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<9>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<9>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 6,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<10>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<10>().coefficients, peakCoefficients);


        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 6.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0 - chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<11>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<11>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 7,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<12>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<12>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 7.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0 - chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<13>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<13>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 8,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<14>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<14>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 9.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0 - chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<15>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<15>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 10,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<16>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<16>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 10.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0 - chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<17>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<17>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 11,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<18>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<18>().coefficients, peakCoefficients);

        peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
            chainSettings.peakFreq * 11.5,
            chainSettings.peakQuality,
            juce::Decibels::decibelsToGain(0 - chainSettings.peakGainInDecibles));

        updateCoefficients(leftChain.get<ChainPositions::Peak>().get<19>().coefficients, peakCoefficients);
        updateCoefficients(rightChain.get<ChainPositions::Peak>().get<19>().coefficients, peakCoefficients);



}

void AtonalSynthAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}


void AtonalSynthAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
        getSampleRate(),
        2 * (chainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();

    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);


    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);

}

void AtonalSynthAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{

    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
        getSampleRate(),
        2 * (chainSettings.highCutSlope + 1));

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);

}

void AtonalSynthAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);

    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout 
    AtonalSynthAudioProcessor::createParameterLayout()

{

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 
                                                           20.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                            20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                           "Peak Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           750.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.f));

    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i) {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    layout.add(std::make_unique < juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique < juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AtonalSynthAudioProcessor();
}
