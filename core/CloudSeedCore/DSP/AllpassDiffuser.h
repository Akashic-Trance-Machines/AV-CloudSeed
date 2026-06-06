/*
Copyright (c) 2024 Ghost Note Engineering Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
THE SOFTWARE.
*/

#pragma once

#include "ModulatedAllpass.h"
#include "RandomBuffer.h"

namespace Cloudseed
{
	class AllpassDiffuser
	{
	public:
		static constexpr int MaxStageCount = 12;

	private:
		float samplerate;

		ModulatedAllpass filters[MaxStageCount];
		int delay;
		float modAmount;
		float modRate;
		float seedValues[MaxStageCount * 3];
		int seed;
		float crossSeed;

	public:
		int Stages;

		AllpassDiffuser(float samplerate):
		samplerate{samplerate},
		delay{},
		modAmount{},
		modRate{},
		seed{1},
		crossSeed{0.0f},
		Stages{1}
		{
			UpdateSeeds();
		}

		int GetSamplerate()
		{
			return samplerate;
		}

		void SetSamplerate(int samplerate)
		{
			this->samplerate = samplerate;
			SetModRate(modRate);
		}

		void SetSeed(int seed)
		{
			this->seed = seed;
			UpdateSeeds();
		}

		void SetCrossSeed(float crossSeed)
		{
			this->crossSeed = crossSeed;
			UpdateSeeds();
		}

		void SetSeeds(int seed, float crossSeed)
		{
			this->seed = seed;
			this->crossSeed = crossSeed;
			UpdateSeeds();
		}

		bool GetModulationEnabled()
		{
			return filters[0].ModulationEnabled;
		}

		void SetModulationEnabled(bool value)
		{
			for (int i = 0; i < MaxStageCount; i++)
				filters[i].ModulationEnabled = value;

		}

		void SetInterpolationEnabled(bool enabled)
		{
			for (int i = 0; i < MaxStageCount; i++)
				filters[i].InterpolationEnabled = enabled;
		}

		void SetDelay(int delaySamples)
		{
			delay = delaySamples;
			UpdateDelay();
		}

		void SetFeedback(float feedback)
		{
			for (int i = 0; i < MaxStageCount; i++)
				filters[i].Feedback = feedback;
		}

		void SetModAmount(float amount)
		{
			modAmount = amount;
			UpdateModAmount();
		}

		void SetModRate(float rate)
		{
			modRate = rate;
			UpdateModRate();
		}

		void Process(float* input, float* output, int bufSize)
		{
			filters[0].Process(input, output, bufSize);

			for (int i = 1; i < Stages; i++)
				filters[i].Process(output, output, bufSize);
		}

		void ClearBuffers()
		{
			for (int i = 0; i < MaxStageCount; i++)
				filters[i].ClearBuffers();
		}

		void StartSlowClear()
		{
			for (int i = 0; i < MaxStageCount; i++)
				filters[i].StartSlowClear();
		}

		int SlowClearDone(int& bytes)
		{
			for (int i = 0; i < MaxStageCount; i++)
				if (!filters[i].SlowClearDone(bytes))
					return 0;
			return bytes;
		}

	private:
		void UpdateDelay()
		{
			for (int i = 0; i < MaxStageCount; i++)
			{
				float r = seedValues[i];
				float d = std::pow(10.0f, r) * 0.1f; // 0.1 ... 1.0
				filters[i].SampleDelay = (int)(delay * d);
			}
		}

		void UpdateModAmount()
		{
			for (int i = 0; i < MaxStageCount; i++)
				filters[i].ModAmount = modAmount * (0.85 + 0.3 * seedValues[MaxStageCount + i]);
		}

		void UpdateModRate()
		{
			for (int i = 0; i < MaxStageCount; i++)
				filters[i].ModRate = modRate * (0.85 + 0.3 * seedValues[MaxStageCount * 2 + i]) / samplerate;
		}

		void UpdateSeeds()
		{
			RandomBuffer::Generate(seed, seedValues, MaxStageCount * 3, crossSeed);
			UpdateDelay();
			UpdateModAmount();
			UpdateModRate();
		}

	};
}
