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
#include "FastSin.h"
#include "Utils.h"
#include <cmath>

namespace Cloudseed
{
	class ModulatedAllpass
	{
	public:
		static constexpr int DelayBufferSize = 19200; // 100ms at 192Khz
		static constexpr int ModulationUpdateRate = 8;

	private:
		float delayBuffer[DelayBufferSize];
		int index;
		uint64_t samplesProcessed;

		float modPhase;
		int delayA;
		int delayB;
		float gainA;
		float gainB;

		int bufferCleared;
	public:

		int SampleDelay;
		float Feedback;
		float ModAmount;
		float ModRate;

		bool InterpolationEnabled;
		bool ModulationEnabled;

		ModulatedAllpass():
		delayBuffer{},
		bufferCleared{}
		{
			index = DelayBufferSize - 1;
			samplesProcessed = 0;

			modPhase = 0.01 + 0.98 * std::rand() / (float)RAND_MAX;
			delayA = 0;
			delayB = 0;
			gainA = 0;
			gainB = 0;

			SampleDelay = 100;
			Feedback = 0.5;
			ModAmount = 0.0;
			ModRate = 0.0;

			InterpolationEnabled = true;
			ModulationEnabled = true;
			Update();
		}

		void ClearBuffers()
		{
			memset(delayBuffer, 0, sizeof delayBuffer);
		}

		void StartSlowClear()
		{
			bufferCleared = 0;
		}

		int SlowClearDone(int& bytes)
		{
			if (bufferCleared >= DelayBufferSize) return bytes;
			int clearSize = std::min(bytes, DelayBufferSize - bufferCleared);
			memset(delayBuffer + bufferCleared, 0, clearSize * sizeof *delayBuffer);
			bufferCleared += clearSize;
			return bytes -= clearSize;
		}

		void Process(float* input, float* output, int sampleCount)
		{
			if (ModulationEnabled)
				if (InterpolationEnabled)
					ProcessWithModInterpolation(input, output, sampleCount);
				else
					ProcessWithModNoInterpolation(input, output, sampleCount);
			else
				ProcessNoMod(input, output, sampleCount);
		}

	private:
		void ProcessNoMod(float* input, float* output, int sampleCount)
		{
			int delayedIndex = index - SampleDelay;
			if (delayedIndex < 0) delayedIndex += DelayBufferSize;

			for (int i = 0; i < sampleCount; i++)
			{
				float bufOut = delayBuffer[delayedIndex];
				float inVal = input[i] + bufOut * Feedback;

				delayBuffer[index] = inVal;
				output[i] = bufOut - inVal * Feedback;

				index++;
				delayedIndex++;
				if (index >= DelayBufferSize) index -= DelayBufferSize;
				if (delayedIndex >= DelayBufferSize) delayedIndex -= DelayBufferSize;
				samplesProcessed++;
			}
		}

		void ProcessWithModInterpolation(float* input, float* output, int sampleCount)
		{
			for (int i = 0; i < sampleCount; i++)
			{
				if (samplesProcessed >= ModulationUpdateRate)
				{
					Update();
					samplesProcessed = 0;
				}

				int idxA = index - delayA;
				int idxB = index - delayB;
				idxA += DelayBufferSize * (idxA < 0); // modulo
				idxB += DelayBufferSize * (idxB < 0); // modulo

				float bufOut = delayBuffer[idxA] * gainA + delayBuffer[idxB] * gainB;

				float inVal = input[i] + bufOut * Feedback;
				delayBuffer[index] = inVal;
				output[i] = bufOut - inVal * Feedback;

				index++;
				if (index >= DelayBufferSize) index -= DelayBufferSize;
				samplesProcessed++;
			}
		}

		void ProcessWithModNoInterpolation(float* input, float* output, int sampleCount)
		{
			for (int i = 0; i < sampleCount; i++)
			{
				if (samplesProcessed >= ModulationUpdateRate)
				{
					Update();
					samplesProcessed = 0;
				}

				int idxA = index - delayA;
				idxA += DelayBufferSize * (idxA < 0); // modulo
				float bufOut = delayBuffer[idxA];

				float inVal = input[i] + bufOut * Feedback;
				delayBuffer[index] = inVal;
				output[i] = bufOut - inVal * Feedback;

				index++;
				if (index >= DelayBufferSize) index -= DelayBufferSize;
				samplesProcessed++;
			}
		}

		inline float Get(int delay)
		{
			int idx = index - delay;
			if (idx < 0)
				idx += DelayBufferSize;

			return delayBuffer[idx];
		}

		void Update()
		{
			modPhase += ModRate * ModulationUpdateRate;
			if (modPhase > 1.0f)
				modPhase -= (int)modPhase;

			float mod = FastSin::Get(modPhase);

			if (ModAmount >= SampleDelay) // don't modulate to negative value
				ModAmount = SampleDelay - 1;

			float totalDelay = SampleDelay + ModAmount * mod;

			delayA = (int)totalDelay;
			delayB = (int)totalDelay + 1;

			float partial = totalDelay - delayA;

			gainA = 1 - partial;
			gainB = partial;
		}
	};
}
