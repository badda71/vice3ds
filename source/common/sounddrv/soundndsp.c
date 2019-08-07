/*
 * soundndsp.c - Implementation of the Nintenod 3DS sound device
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include "log.h"
#include "sound.h"
#include <string.h>
#include <3ds.h>

#define CHANNEL 0x08
#define WAVBUFNR 10

bool		ndsp_isinit = false;
int			ndsp_channels = 0;
int			ndsp_bufsize = 0;
int			ndsp_activeBuf = 0;
ndspWaveBuf	ndsp_waveBuf[WAVBUFNR];

/* init-routine to be called at device initialization. Should use
   suggested values if possible or return new values if they cannot be
   used */
static int ndsp_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
//log_3ds("enter %s: speed = %d, fragsize = %d, fragnr = %d, channels = %d", __func__, *speed, *fragsize, *fragnr, *channels);

	// Init ndsp and init variables
	if(ndspInit() < 0) return -1;
	ndsp_isinit = true;
	if (*channels > 2 || *channels < 1) return -1;
	ndsp_channels = *channels;
	ndsp_bufsize = (*fragsize) * (*fragnr);
	
	ndsp_activeBuf = 0;
	memset(ndsp_waveBuf, 0, sizeof(ndsp_waveBuf));

	for (int i=0;i < WAVBUFNR ;i++ )
	{
		ndsp_waveBuf[i].status = NDSP_WBUF_DONE;
		ndsp_waveBuf[i].data_vaddr = linearAlloc(ndsp_bufsize * sizeof(int16_t));
	}

	// set up channel
	ndspChnReset(CHANNEL);
	ndspChnWaveBufClear(CHANNEL);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(CHANNEL, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(CHANNEL, *speed);
	ndspChnSetFormat(CHANNEL,
			*channels == 2 ? NDSP_FORMAT_STEREO_PCM16 :
			NDSP_FORMAT_MONO_PCM16);
    return 0;
}

/* send number of bytes to the soundcard. it is assumed to block if kernel buffer is full */
static int ndsp_write(int16_t *pbuf, size_t nr)
{
//log_3ds("enter %s: %d samples", __func__, nr);

	// block until the active buffer is done playing
	while (ndsp_waveBuf[ndsp_activeBuf].status != NDSP_WBUF_DONE)
	{
		svcSleepThread(1000 * 1000); // 1/1000 second
	}

	// set and queue a new buffer
	memcpy((void *)ndsp_waveBuf[ndsp_activeBuf].data_vaddr, pbuf, nr * sizeof(int16_t));
	ndsp_waveBuf[ndsp_activeBuf].nsamples = nr / ndsp_channels;
	ndspChnWaveBufAdd(CHANNEL, &ndsp_waveBuf[ndsp_activeBuf]);
//	DSP_FlushDataCache(ndsp_waveBuf[ndsp_activeBuf].data_vaddr, nr * sizeof(int16_t));
	ndsp_activeBuf = (ndsp_activeBuf + 1) % WAVBUFNR;

	return 0;
}

static int ndsp_suspend()
{
//log_3ds("enter %s",__func__);
	ndspChnSetPaused(CHANNEL, true);
	return 0;
}

static int ndsp_resume()
{
//log_3ds("enter %s",__func__);
	ndspChnSetPaused(CHANNEL, false);
	return 0;
}

static void ndsp_close(void)
{
//log_3ds("enter %s",__func__);
	if(ndsp_isinit == true)
	{
		ndspChnWaveBufClear(CHANNEL);
		ndspExit();
		for (int i = 0 ; i < WAVBUFNR ; i++ )
		{
			linearFree((void *)ndsp_waveBuf[i].data_vaddr);
		}
		ndsp_isinit = false;
	}
}

static sound_device_t ndsp_device =
{
    // name of the device 
    "ndsp",
    // init -routine to be called at device initialization. Should use
    // suggested values if possible or return new values if they cannot be
    // used 
    // int (*init)(const char *param, int *speed, int *fragsize, int *fragnr, int *channels);
	ndsp_init,
    // send number of bytes to the soundcard. it is assumed to block if kernel buffer is full 
    // int (*write)(int16_t *pbuf, size_t nr);
    ndsp_write,
    // dump-routine to be called for every write to SID 
    //int (*dump)(uint16_t addr, uint8_t byte, CLOCK clks);
    NULL,
    // flush-routine to be called every frame 
    // int (*flush)(char *state);
    NULL,
    // return number of samples currently available in the kernel buffer 
    // int (*bufferspace)(void);
    NULL,
    // close and cleanup device 
    // void (*close)(void);
    ndsp_close,
    // suspend device 
    // int (*suspend)(void);
    ndsp_suspend,
    // resume device 
    // int (*resume)(void);
    ndsp_resume,
    // is attenuation needed on suspend or not 
    // int need_attenuation;
    0,
    // maximum amount of channels 
    // int max_channels;
	2
};

int sound_init_ndsp_device(void)
{
    return sound_register_device(&ndsp_device);
}
