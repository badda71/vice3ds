/*
 * n3dsaudio_drv.c - Nintendo 3DS audio input driver.
 *
 * Written by
 *  Sebastian Weber <me@badda.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
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

#include <3ds.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "vice.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "n3dsaudio_drv.h"
#include "sampler.h"

static log_t n3dsaudio_log = LOG_ERR;

static int stream_started = 0;
static unsigned int sound_frames_per_sec;
static unsigned int sound_cycles_per_frame;
static unsigned int sound_samples_per_frame;
u32 micbuf_size = 0;
u8* micbuf = NULL;
u32 micbuf_datasize =0;

static void n3dsaudio_start_sampling(int channels)
{
log_citra("enter %s",__func__);
    if (stream_started) {
        log_error(n3dsaudio_log, "Attempted to start n3dsaudio twice");
    } else {
        // init
		sound_cycles_per_frame = machine_get_cycles_per_frame();
        sound_frames_per_sec = machine_get_cycles_per_second() / sound_cycles_per_frame;
        sound_samples_per_frame = 10910 / sound_frames_per_sec;
		micbuf_size = sound_samples_per_frame * 2; // 16 bit
		micbuf = memalign(0x1000, micbuf_size);
		memset(micbuf, 0, micbuf_size);

log_citra("allc micbuf: %d, size: %d",micbuf, micbuf_size);
		Result rc = micInit(micbuf, micbuf_size);
		if (rc) {
            log_error(n3dsaudio_log, "Could not init n3dsaudio");
log_citra("error");
			return;
		}
		micbuf_datasize = micGetSampleDataSize();

		// start the stream
log_citra("start sampling: %d",micbuf_datasize);
		rc=MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, MICU_SAMPLE_RATE_10910, 0, micbuf_datasize, true);
		if (rc) {
            log_error(n3dsaudio_log, "Could not start sampling with n3dsaudio");
			return;
		}
		stream_started=1;
    }
}

static void n3dsaudio_stop_sampling(void)
{
	MICU_StopSampling();
	if (micbuf) {
		free (micbuf);
		micbuf=NULL;
	}
	stream_started = 0;
	micExit();
}

static uint8_t n3dsaudio_get_sample(int channel)
{
    if (!micbuf) {
        return 0x80;
    }

	// just return the latest sample, converted to u8
	u32 o = micGetLastSampleOffset();
	uint8_t i = (uint8_t)(micbuf[o] + 0x80);
log_citra("sample: %d at offset %d",i,o);
	return i;
}

static void n3dsaudio_shutdown(void)
{
    if (stream_started) {
        n3dsaudio_stop_sampling();
    }
}

static sampler_device_t n3dsaudio_device =
{
    "3DS built in microphone audio input",
    n3dsaudio_start_sampling,
    n3dsaudio_stop_sampling,
    n3dsaudio_get_sample,
    n3dsaudio_shutdown,
    NULL, /* no resources */
    NULL, /* no cmdline options */
    NULL  /* no reset */
};


void n3dsaudio_init(void)
{
    n3dsaudio_log = log_open("Sampler N3DSAudio");

    sampler_device_register(&n3dsaudio_device, SAMPLER_DEVICE_N3DSAUDIO);

}
