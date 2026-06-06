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

#include "ModulatedDelay.h"
#include "Utils.h"
#include "FastSin.h"
#include <stdint.h>

namespace Cloudseed
{
	class ModulatedDelay
	{
	private:

		static constexpr int ModulationUpdateRate = 8;
		static constexpr int DelayBufferSize = 192000 * 2;

		float delayBuffer[DelayBufferSize];
		int writeIndex;
		int readIndexA;
		int readIndexB;
		uint64_t samplesProcessed;

		float modPhase;
		float gainA;
		float gainB;

		int bufferCleared;
	public:
		int SampleDelay;

		float ModAmount;
		float ModRate;

		ModulatedDelay():
		delayBuffer{},
		bufferCleared{}
		{
			writeIndex = 0;
			readIndexA = 0;
			readIndexB = 0;
			samplesProcessed = 0;

			modPhase = 0.01 + 0.98 * (std::rand() / (float)RAND_MAX);
			gainA = 0;
			gainB = 0;

			SampleDelay = 100;
			ModAmount = 0.0;
			ModRate = 0.0;

			Update();
		}

		void Process(float* input, float* output, int bufSize)
		{
			for (int i = 0; i < bufSize; i++)
			{
				if (samplesProcessed >= ModulationUpdateRate)
				{
					Update();
					samplesProcessed = 0;
				}

				delayBuffer[writeIndex] = input[i];
				output[i] = delayBuffer[readIndexA] * gainA + delayBuffer[readIndexB] * gainB;

				writeIndex++;
				readIndexA++;
				readIndexB++;
				if (writeIndex >= DelayBufferSize) writeIndex -= DelayBufferSize;
				if (readIndexA >= DelayBufferSize) readIndexA -= DelayBufferSize;
				if (readIndexB >= DelayBufferSize) readIndexB -= DelayBufferSize;
				samplesProcessed++;
			}
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

	private:
		void Update()
		{
			modPhase += ModRate * ModulationUpdateRate;
			if (modPhase > 1.0f)
				modPhase -= (int)modPhase;

			float mod = FastSin::Get(modPhase);
			float totalDelay = SampleDelay + ModAmount * mod;

			float delayA = (int)totalDelay;
			float delayB = (int)totalDelay + 1;

			float partial = totalDelay - delayA;

			gainA = 1 - partial;
			gainB = partial;

			readIndexA = writeIndex - delayA;
			readIndexB = writeIndex - delayB;
			if (readIndexA < 0) readIndexA += DelayBufferSize;
			if (readIndexB < 0) readIndexB += DelayBufferSize;
		}
	};
}
