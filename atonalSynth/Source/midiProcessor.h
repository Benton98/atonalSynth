#pragma once

#include "JuceHeader.h"

class MidiProcessor
{
public:
	void process(juce::MidiBuffer& midiMesssages)
	{
		juce::MidiBuffer::Iterator it(midiMesssages);
		juce::MidiMessage currentMesssage;
		int samplePos;

		

		while (it.getNextEvent(currentMesssage, samplePos))
		{
			DBG (currentMesssage.getDescription());
		}
	}
};
