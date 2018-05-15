/** @file paex_saw.c
@ingroup examples_src
@brief Play a simple (aliasing) sawtooth wave.
@author Phil Burk  http://www.softsynth.com
*/
/*
* $Id$
*
* This program uses the PortAudio Portable Audio Library.
* For more information see: http://www.portaudio.com
* Copyright (c) 1999-2000 Ross Bencina and Phil Burk
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files
* (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
* ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
* The text above constitutes the entire PortAudio license; however,
* the PortAudio community also makes the following non-binding requests:
*
* Any person wishing to distribute modifications to the Software is
* requested to send the modifications to the original developer so that
* they can be incorporated into the canonical version. It is also
* requested that these non-binding requests be included along with the
* license above.
*/

#include <stdio.h>
#include <math.h>
#include "portaudio.h"
#include "Heavy_test_osc.hpp"
#include "sndfile.h"

#define NUM_SECONDS   (2)
#define SAMPLE_RATE   (44100)

// UserData struct to be passed to the audio callback
typedef struct {
	Heavy_test_osc *hvContext;
	float *fileBuffer;
	unsigned long fileNumFrames;
	unsigned long fileIndex;
} UserData;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	/* Cast data passed through stream to our structure. */
	UserData *data = (UserData*)userData;
	float *out = (float*)outputBuffer;
	unsigned int i;
	(void)inputBuffer; /* Prevent unused variable warning. */

	 // calculate num frames left to read
	long numFramesLeft = data->fileNumFrames - data->fileIndex;
	long numFramesToRead = (numFramesLeft < framesPerBuffer) ? numFramesLeft : framesPerBuffer;

	// process buffers through heavy patch
	numFramesToRead = data->hvContext->processInline(NULL, out, (int)numFramesToRead);

	printf("numFramesToRead %i \n", (int)numFramesToRead);

	// increment read index
	data->fileIndex += numFramesToRead;

	return (data->fileIndex >= data->fileNumFrames) ? paComplete : paContinue;
}

/*******************************************************************/
static UserData data;
int main();
int main()
{
	PaStream *stream;
	PaError err;
	PaStreamParameters outputParameters;
	PaStreamParameters inputParameters;

	UserData data = { NULL, NULL, 0, 0 };

	// printf("PortAudio Test: output sawtooth wave.\n");
	/* Initialize our data for use by callback. */
	// data.left_phase = data.right_phase = 0.0;
	/* Initialize library before making any other calls. */
	err = Pa_Initialize();
	if (err != paNoError) goto error;

	int numDevices;
	numDevices = Pa_GetDeviceCount();
	if (numDevices < 0)
	{
		printf("ERROR: Pa_CountDevices returned 0x%x\n", numDevices);
		err = numDevices;
		goto error;
	}
	const   PaDeviceInfo *deviceInfo;
	// for (int d = 0; d<numDevices; d++)
	// {
	// 	deviceInfo = Pa_GetDeviceInfo(d);
	// 	printf("DEVICE INFO %x", deviceInfo);
	// }

	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	if (outputParameters.device == paNoDevice) {
		fprintf(stderr, "Error: No default output device.\n");
		goto error;
	}
	outputParameters.channelCount = 1;       /* mono output */
	outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
	if (inputParameters.device == paNoDevice) {
		fprintf(stderr, "Error: No default input device.\n");
		goto error;
	}
	inputParameters.channelCount = 2;                    /* stereo input */
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	// Setup Heavy context
	data.hvContext = new Heavy_test_osc(SAMPLE_RATE);
	data.fileNumFrames = 48000;

	printf("Instantiated heavy context:\n - numInputChannels: %d\n - numOutputChannels: %d\n\n",
		hv_getNumInputChannels(data.hvContext), hv_getNumOutputChannels(data.hvContext));

	/* Open an audio I/O stream. */
	err = Pa_OpenStream(
		&stream,
		&inputParameters,
		&outputParameters,
		SAMPLE_RATE,
		256,
		paClipOff,
		patestCallback,
		&data);
	
	if (err != paNoError) goto error;

	err = Pa_StartStream(stream);
	if (err != paNoError) goto error;
	
	/* Sleep for several seconds. */
	Pa_Sleep(NUM_SECONDS * 1000);
	
	err = Pa_StopStream(stream);
	if (err != paNoError) goto error;
	err = Pa_CloseStream(stream);
	if (err != paNoError) goto error;
	Pa_Terminate();
	printf("Test finished.\n");
	return err; 
error:
	Pa_Terminate();
	fprintf(stderr, "An error occured while using the portaudio stream\n");
	fprintf(stderr, "Error number: %d\n", err);
	fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
	return err;
}
