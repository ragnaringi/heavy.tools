//
//  main.c
//  hv.tools
//
//  Created by Ragnar Hrafnkelsson on 29/03/2016.
//  Copyright (c) 2016 Reactify. All rights reserved.
//

/*
 * This example is adapted from https://github.com/enzienaudio/examples/blob/master/portaudio/src/main.c
 */

#include <stdio.h>
#include "portaudio.h"
#include "Heavy_Test.h"

// UserData struct to be passed to the audio callback
typedef struct {
  HeavyContextInterface *hvContext;
} UserData;


// Helper methods
int check_pa_error(int tag, PaError error);

void printHook(HeavyContextInterface *c, const char *printName, const char *str, const HvMessage *m) {
  double timestampMs = 1000.0 * ((double) hv_msg_getTimestamp(m)) / hv_getSampleRate(c);
  printf("[@ %.3fms] %s: %s\n", timestampMs, printName, str);
}

// Main Audio Processing Callback
static int paCallback(const void *input, void *output,
                      unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags, void *userData)
{
  UserData *data = (UserData *) userData;
  
  float *in  = (float *)input;
  float *out = (float *)output;
  
  // process buffers through heavy patch
  // expected data format here is interleaved float 32 bit buffers
  // i.e: L R L R L R ...
  hv_processInlineInterleaved(data->hvContext, in, out, (int)frameCount);
  
  return paContinue;
}


int main(int argc, const char * argv[]) {
  
  UserData data = { NULL };
  
  const double sampleRate = 44100.0;
  const unsigned long blockSize = 256;
  
  // Setup PortAudio
  if (check_pa_error(1, Pa_Initialize())) return 1;
  
  // Setup Heavy context
  data.hvContext = hv_Test_new(sampleRate);
  hv_setPrintHook(data.hvContext, &printHook);
  
  printf("Instantiated heavy context:\n - numInputChannels: %d\n - numOutputChannels: %d\n\n",
         hv_getNumInputChannels(data.hvContext), hv_getNumOutputChannels(data.hvContext));
  
  const int inChannels = hv_getNumInputChannels(data.hvContext) ? : 1;
  const int outChannels = hv_getNumOutputChannels(data.hvContext) ? : 1;
  
  // Opening stream
  PaStream *stream = NULL;
  if (check_pa_error(2,
                     Pa_OpenDefaultStream(&stream, inChannels, outChannels,
                                          paFloat32, sampleRate, blockSize, paCallback, &data))) {
                       return 1;
                     }
  
  // Start Processing
  if (check_pa_error(3, Pa_StartStream(stream))) return 1;
  
  while (Pa_IsStreamActive(stream)) {
    Pa_Sleep(1000);
  }
  
  // Stop Processing
  if (check_pa_error(5, Pa_CloseStream(stream))) return 1;
  
  // Teardown
  hv_delete(data.hvContext);
  
  if (check_pa_error(7, Pa_Terminate())) return 1;
  
  printf("Success\n");
  return 0;
}


int check_pa_error(int tag, PaError error) {
  if (error != paNoError) {
    printf("(#%d portaudio error) %s\n", tag, Pa_GetErrorText(error));
    return 1;
  }
  return 0;
}
