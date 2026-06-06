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

#include <cassert>
#include <cmath>
#include "Utils.h"
#include "RandomBuffer.h"

namespace Cloudseed
{
	class MultitapDelay
	{
	public:
		static constexpr int MaxTaps = 256;
		static constexpr int DelayBufferSize = 192000; // 1000ms at 192kHz

	private:
		float delayBuffer[DelayBufferSize];

		float tapGains[MaxTaps];
		int tapOffset[MaxTaps];

		float seedValues[MaxTaps * 3];

		int writeIdx;
		int seed;
		float crossSeed;
		int count;
		float lengthSamples;
		float decay;

		int bufferCleared;
	public:
		MultitapDelay():
		delayBuffer{},
		tapGains{},
		tapOffset{},
		bufferCleared{DelayBufferSize}
		{
			writeIdx = 0;
			seed = 0;
			crossSeed = 0.0;
			count = 1;
			lengthSamples = 1000;
			decay = 1.0;
			
			UpdateSeeds();
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

		void SetTapCount(int tapCount)
		{
			if (tapCount < 1) tapCount = 1;
			count = tapCount;
			Update();
		}

		void SetTapLength(int tapLengthSamples)
		{
			if (tapLengthSamples < 10) tapLengthSamples = 10;
			lengthSamples = tapLengthSamples;
			Update();
		}

		void SetTapDecay(float tapDecay)
		{
			decay = tapDecay;
			Update();
		}

		void Process(float* input, float* output, int bufSize)
		{
			assert(DelayBufferSize % bufSize == 0);

			memcpy(delayBuffer + writeIdx, input, bufSize * sizeof *delayBuffer);
			memset(output, 0, bufSize * sizeof *output);

			for (int i = 0; i < count; i++)
			{
				int readIdx = writeIdx - tapOffset[i];
				float gain = tapGains[i];

				if (readIdx < 0) readIdx += DelayBufferSize;
				if (readIdx + bufSize <= DelayBufferSize)
				{
					for (int j = 0; j < bufSize; ++j)
						output[j] += delayBuffer[readIdx + j] * gain;
				}
				else
				{
					int readSize1 = DelayBufferSize - readIdx;
					int readSize2 = bufSize - readSize1;

					for (int j = 0; j < readSize1; ++j)
						output[j] += delayBuffer[readIdx + j] * gain;

					for (int j = 0; j < readSize2; ++j)
						output[readSize1 + j] += delayBuffer[j] * gain;
				}
			}

			writeIdx += bufSize;
			if (writeIdx >= DelayBufferSize) writeIdx -= DelayBufferSize; 

//			for (int i = 0; i < bufSize; i++)
//			{
//				delayBuffer[writeIdx] = input[i];
//				output[i] = 0;
//
//				for (int j = 0; j < count; j++)
//				{
//					int readIdx = writeIdx - tapOffset[j];
//					if (readIdx < 0) readIdx += DelayBufferSize;
//
//					output[i] += delayBuffer[readIdx] * tapGains[j];
//				}
//
//				if (++writeIdx >= DelayBufferSize) writeIdx -= DelayBufferSize;
//			}
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
			float lengthScaler = lengthSamples / (float)count;
			float totalGain = 3.0 / std::sqrtf(1 + count) * (1 + decay * 2);

			for (int i = 0, s = 0; i < MaxTaps; i++)
			{
				float phase = seedValues[s++] < 0.5 ? 1 : -1;
				tapGains[i] = Utils::dB2Gain(-20 + seedValues[s++] * 20) * phase;
				float offset = (i + seedValues[s++]) * lengthScaler;
				tapOffset[i] = (int)offset;
				float decayEffective = std::expf(-offset / lengthSamples * 3.3) * decay + (1-decay);
				tapGains[i] *= decayEffective * totalGain;
			}
		}

		void UpdateSeeds()
		{
			RandomBuffer::Generate(seed, seedValues, MaxTaps * 3, crossSeed);
			Update();
		}
	};
}
