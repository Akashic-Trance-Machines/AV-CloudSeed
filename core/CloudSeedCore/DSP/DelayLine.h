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

#include "Lp1.h"
#include "ModulatedDelay.h"
#include "AllpassDiffuser.h"
#include "Biquad.h"

namespace Cloudseed
{
	class DelayLine
	{
	private:
		ModulatedDelay delay;
		AllpassDiffuser diffuser;
		Biquad lowShelf;
		Biquad highShelf;
		Lp1 lowPass;
		float feedbackBuffer[BUFFER_SIZE];
		float feedback;

		int bufferCleared;
	public:
		bool DiffuserEnabled;
		bool LowShelfEnabled;
		bool HighShelfEnabled;
		bool CutoffEnabled;
		bool TapPostDiffuser;

		DelayLine(float samplerate):
		diffuser{samplerate},
		lowShelf(Biquad::FilterType::LowShelf, samplerate, 20, -20),
		highShelf(Biquad::FilterType::HighShelf, samplerate, 19000, -20),
		lowPass{samplerate, 1000},
		feedbackBuffer{},
		feedback{},
		bufferCleared{}
		{
		}

		void SetSamplerate(int samplerate)
		{
			diffuser.SetSamplerate(samplerate);
			lowPass.SetSamplerate(samplerate);
			lowShelf.SetSamplerate(samplerate);
			highShelf.SetSamplerate(samplerate);
		}

		void SetDiffuserSeed(int seed, float crossSeed)
		{
			diffuser.SetSeeds(seed, crossSeed);
		}

		void SetDelay(int delaySamples)
		{
			delay.SampleDelay = delaySamples;
		}

		void SetFeedback(float feedb)
		{
			feedback = feedb;
		}

		void SetDiffuserDelay(int delaySamples)
		{
			diffuser.SetDelay(delaySamples);
		}

		void SetDiffuserFeedback(float feedb)
		{
			diffuser.SetFeedback(feedb);
		}

		void SetDiffuserStages(int stages)
		{
			diffuser.Stages = stages;
		}

		void SetLowShelfGain(float gainDb)
		{
			lowShelf.SetGainDb(gainDb);
			lowShelf.Update();
		}

		void SetLowShelfFrequency(float frequency)
		{
			lowShelf.Frequency = frequency;
			lowShelf.Update();
		}

		void SetHighShelfGain(float gainDb)
		{
			highShelf.SetGainDb(gainDb);
			highShelf.Update();
		}

		void SetHighShelfFrequency(float frequency)
		{
			highShelf.Frequency = frequency;
			highShelf.Update();
		}

		void SetCutoffFrequency(float frequency)
		{
			lowPass.SetCutoffHz(frequency);
		}

		void SetLineModAmount(float amount)
		{
			delay.ModAmount = amount;
		}

		void SetLineModRate(float rate)
		{
			delay.ModRate = rate;
		}

		void SetDiffuserModAmount(float amount)
		{
			diffuser.SetModulationEnabled(amount > 0.0);
			diffuser.SetModAmount(amount);
		}

		void SetDiffuserModRate(float rate)
		{
			diffuser.SetModRate(rate);
		}

		void SetInterpolationEnabled(bool value)
		{
			diffuser.SetInterpolationEnabled(value);
		}

		void Process(float* input, float* output, int bufSize)
		{
			for (int i = 0; i < bufSize; i++)
				feedbackBuffer[i] = input[i] + feedbackBuffer[i] * feedback;

			delay.Process(feedbackBuffer, feedbackBuffer, bufSize);
			
			if (!TapPostDiffuser)
				memcpy(output, feedbackBuffer, bufSize * sizeof *output);
			if (DiffuserEnabled)
				diffuser.Process(feedbackBuffer, feedbackBuffer, bufSize);
			if (LowShelfEnabled)
				lowShelf.Process(feedbackBuffer, feedbackBuffer, bufSize);
			if (HighShelfEnabled)
				highShelf.Process(feedbackBuffer, feedbackBuffer, bufSize);
			if (CutoffEnabled)
				lowPass.Process(feedbackBuffer, feedbackBuffer, bufSize);

			if (TapPostDiffuser)
				memcpy(output, feedbackBuffer, bufSize * sizeof *output);
		}

		void ClearDiffuserBuffer()
		{
			diffuser.ClearBuffers();
		}

		void ClearBuffers()
		{
			delay.ClearBuffers();
			diffuser.ClearBuffers();
			lowShelf.ClearBuffers();
			highShelf.ClearBuffers();
			lowPass.Output = 0;
			memset(feedbackBuffer, 0, sizeof feedbackBuffer);
		}

		void StartSlowClear()
		{
			delay.StartSlowClear();
			diffuser.StartSlowClear();
			bufferCleared = 0;
		}

		int SlowClearDone(int& bytes)
		{
			if (!delay.SlowClearDone(bytes)) return 0;
			if (!diffuser.SlowClearDone(bytes)) return 0;

			if (bufferCleared != BUFFER_SIZE)
			{
				lowShelf.ClearBuffers();
				highShelf.ClearBuffers();
				lowPass.Output = 0;
				memset(feedbackBuffer, 0, sizeof feedbackBuffer);
				bufferCleared = BUFFER_SIZE;
			}

			return bytes;
		}
	};
}
