/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output audio to nowhere... */

#include "SDL_rwops.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_n3dsaudio.h"

#include <3ds.h>
#include <assert.h>

//extern volatile bool app_pause;
extern volatile bool app_exiting;

static Handle wbufDoneEvent;
static aptHookCookie aptCookie;

static const u64 EventTimeout = 1000LL * 1000LL * 100LL;	// 100 ms

/* The tag name used by N3DS audio */
#define N3DSAUD_DRIVER_NAME         "n3ds"

/* Audio driver functions */
static int N3DSAUD_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void N3DSAUD_WaitAudio(_THIS);
static void N3DSAUD_PlayAudio(_THIS);
static Uint8 *N3DSAUD_GetAudioBuf(_THIS);
static void N3DSAUD_CloseAudio(_THIS);

/* Audio driver bootstrap functions */
static int N3DSAUD_Available(void)
{
	return(1);
}

static inline void contextLock(_THIS)
{
	LightLock_Lock(&this->hidden->lock);
}

static inline void contextUnlock(_THIS)
{
	LightLock_Unlock(&this->hidden->lock);
}

static void N3DSAUD_LockAudio(_THIS)
{
	contextLock(this);
}

static void N3DSAUD_UnlockAudio(_THIS)
{
	contextUnlock(this);
}

static void N3DSAUD_DeleteDevice(SDL_AudioDevice *device)
{
	if ( device->hidden->mixbuf != NULL ) {
		SDL_FreeAudioMem(device->hidden->mixbuf);
		device->hidden->mixbuf = NULL;
	}
	if 	( device->hidden->waveBuf[0].data_vaddr!= NULL ) {
		linearFree((void *)device->hidden->waveBuf[0].data_vaddr);
		device->hidden->waveBuf[0].data_vaddr = NULL;
	}

	ndspExit();

	SDL_free(device->hidden);

	SDL_free(device);
}

static SDL_AudioDevice *N3DSAUD_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		SDL_memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				SDL_malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			SDL_free(this);
		}
		return(0);
	}
	SDL_memset(this->hidden, 0, (sizeof *this->hidden));

	/* Set the function pointers */
	this->OpenAudio = N3DSAUD_OpenAudio;
	this->WaitAudio = N3DSAUD_WaitAudio;
	this->PlayAudio = N3DSAUD_PlayAudio;
	this->GetAudioBuf = N3DSAUD_GetAudioBuf;
	this->CloseAudio = N3DSAUD_CloseAudio;
	this->LockAudio = N3DSAUD_LockAudio;
	this->UnlockAudio = N3DSAUD_UnlockAudio;
	this->free = N3DSAUD_DeleteDevice;

	return this;
}

AudioBootStrap N3DSAUD_bootstrap = {
	N3DSAUD_DRIVER_NAME, "SDL N3DS audio driver",
	N3DSAUD_Available, N3DSAUD_CreateDevice
};


static void aptCallback(APT_HookType hook, void* param)
{
	switch (hook)
	{
		case APTHOOK_ONEXIT:
			app_exiting = true;
			svcSignalEvent(wbufDoneEvent);
			break;
		default:
			break;
	}
}

/* called by ndsp thread on each audio frame */
static void audioFrameFinished(void *context)
{
	SDL_AudioDevice *this = (SDL_AudioDevice *) context;

	contextLock(this);
	bool done = this->hidden->waveBuf[this->hidden->nextbuf].status == NDSP_WBUF_DONE;
	contextUnlock(this);

	if (done)
	{
		svcSignalEvent(wbufDoneEvent);
	}
}

/* This function blocks until it is possible to write a full sound buffer */
static void N3DSAUD_WaitAudio(_THIS)
{
	bool not_done;
	size_t wait_index;

	contextLock(this);
	wait_index = this->hidden->nextbuf;

retry:
			
	not_done = this->hidden->waveBuf[wait_index].status != NDSP_WBUF_DONE;
	contextUnlock(this);

	if (not_done)
	{
		svcWaitSynchronization(wbufDoneEvent, EventTimeout);
		
		if (app_exiting)
		{
			return;
		}

		contextLock(this);
		goto retry;
	}
}

static void N3DSAUD_PlayAudio(_THIS)
{
	if (app_exiting) return;

	contextLock(this);

	size_t nextbuf = this->hidden->nextbuf;
	size_t sampleLen = this->hidden->mixlen;

	assert(this->hidden->waveBuf[nextbuf].status == NDSP_WBUF_DONE);

	this->hidden->nextbuf = (nextbuf + 1) % NUM_BUFFERS;
	contextUnlock(this);

	if (this->hidden->format==NDSP_FORMAT_STEREO_PCM8 || this->hidden->format==NDSP_FORMAT_MONO_PCM8) {
		memcpy(this->hidden->waveBuf[nextbuf].data_pcm8,this->hidden->mixbuf,sampleLen);
		DSP_FlushDataCache(this->hidden->waveBuf[nextbuf].data_pcm8,sampleLen);
	} else {
		memcpy(this->hidden->waveBuf[nextbuf].data_pcm16,this->hidden->mixbuf,sampleLen);
		DSP_FlushDataCache(this->hidden->waveBuf[nextbuf].data_pcm16,sampleLen);
	}

	this->hidden->waveBuf[nextbuf].offset=0;
	this->hidden->waveBuf[nextbuf].status=NDSP_WBUF_QUEUED;
	ndspChnWaveBufAdd(0, &this->hidden->waveBuf[nextbuf]);
}

static Uint8 *N3DSAUD_GetAudioBuf(_THIS)
{
	return(this->hidden->mixbuf);
}

static void N3DSAUD_CloseAudio(_THIS)
{
	contextLock(this);

	if ( this->hidden->mixbuf != NULL ) {
		SDL_FreeAudioMem(this->hidden->mixbuf);
		this->hidden->mixbuf = NULL;
	}
	if ( this->hidden->waveBuf[0].data_vaddr!= NULL ) {
		linearFree((void *)this->hidden->waveBuf[0].data_vaddr);
		this->hidden->waveBuf[0].data_vaddr = NULL;
	}

	contextUnlock(this);
}

static int N3DSAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
   //start 3ds DSP init
	Result rc = ndspInit();
	if (R_FAILED(rc)) {
		if ((R_SUMMARY(rc) == RS_NOTFOUND) && (R_MODULE(rc) == RM_DSP))
			SDL_SetError("DSP init failed: dspfirm.cdc missing!");
		else
			SDL_SetError("DSP init failed. Error code: 0x%X", rc);
		return -1;
	}

	if(spec->channels > 2)
		spec->channels = 2;

    Uint16 test_format = SDL_FirstAudioFormat(spec->format);
    int valid_datatype = 0;
    while ((!valid_datatype) && (test_format)) {
        spec->format = test_format;
        switch (test_format) {

			case AUDIO_S8:
				/* Signed 8-bit audio supported */
				this->hidden->format=(spec->channels==2)?NDSP_FORMAT_STEREO_PCM8:NDSP_FORMAT_MONO_PCM8;
				this->hidden->isSigned=1;
				this->hidden->bytePerSample = (spec->channels);
				   valid_datatype = 1;
				break;
			case AUDIO_S16:
				/* Signed 16-bit audio supported */
				this->hidden->format=(spec->channels==2)?NDSP_FORMAT_STEREO_PCM16:NDSP_FORMAT_MONO_PCM16;
				this->hidden->isSigned=1;
				this->hidden->bytePerSample = (spec->channels) * 2;
				   valid_datatype = 1;
				break;
			default:
				test_format = SDL_NextAudioFormat();
				break;
		}
	}

    if (!valid_datatype) {  /* shouldn't happen, but just in case... */
        SDL_SetError("Unsupported audio format");
		ndspExit();
        return (-1);
    }

	/* Update the fragment size as size in bytes */
	SDL_CalculateAudioSpec(spec);

	/* Allocate mixing buffer */
	if (spec->size >= UINT32_MAX/2)
	{
		ndspExit();
		return(-1);
	}
	this->hidden->mixlen = spec->size;
	this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(spec->size); 
	if ( this->hidden->mixbuf == NULL ) {
		ndspExit();
		return(-1);
	}
	SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);

	Uint8 * temp = (Uint8 *) linearAlloc(this->hidden->mixlen*NUM_BUFFERS);
	if (temp == NULL ) {
		SDL_free(this->hidden->mixbuf);
		ndspExit();
		return(-1);
	}
	memset(temp,0,this->hidden->mixlen*NUM_BUFFERS);
	DSP_FlushDataCache(temp,this->hidden->mixlen*NUM_BUFFERS);

	this->hidden->nextbuf = 0;
	this->hidden->channels = spec->channels;
	this->hidden->samplerate = spec->freq;

	LightLock_Init(&this->hidden->lock);

	ndspChnWaveBufClear(0);
 	ndspChnReset(0);

	ndspSetOutputMode((this->hidden->channels==2)?NDSP_OUTPUT_STEREO:NDSP_OUTPUT_MONO);

	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, spec->freq);
	ndspChnSetFormat(0, this->hidden->format);

	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = 1.0;
	mix[1] = 1.0;
	ndspChnSetMix(0, mix);

	memset(this->hidden->waveBuf,0,sizeof(ndspWaveBuf)*NUM_BUFFERS);

	this->hidden->waveBuf[0].data_vaddr = temp;
	this->hidden->waveBuf[0].nsamples = this->hidden->mixlen / this->hidden->bytePerSample;
	this->hidden->waveBuf[0].status = NDSP_WBUF_DONE;

	this->hidden->waveBuf[1].data_vaddr = temp + this->hidden->mixlen;
	this->hidden->waveBuf[1].nsamples = this->hidden->mixlen / this->hidden->bytePerSample;
	this->hidden->waveBuf[1].status = NDSP_WBUF_DONE;

	/* Setup callback */
	svcCreateEvent(&wbufDoneEvent, RESET_ONESHOT);
	ndspSetCallback(audioFrameFinished, this);

	aptHook(&aptCookie, aptCallback, NULL);

    // end 3ds DSP init

	/* We're ready to rock and roll. :-) */
	return(0);
}
