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
u32 micbuf_size = 0x30000;
u8* micbuf = NULL;
u32 micbuf_datasize = 0;

static void n3dsaudio_start_sampling(int channels)
{
    if (stream_started) {
        log_error(n3dsaudio_log, "Attempted to start n3dsaudio twice");
    } else {
        // init
		micbuf = memalign(0x1000, micbuf_size);
		if (!micbuf) {
            log_error(n3dsaudio_log, "Could not allocate microphone buffer");
			return;
		}

		if (R_FAILED(micInit(micbuf, micbuf_size)))
		{
            log_error(n3dsaudio_log, "Could not init n3dsaudio");
			free(micbuf);
			micbuf=NULL;
			return;
		}
		micbuf_datasize = micGetSampleDataSize();

		// start the stream
		if (R_FAILED(MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, MICU_SAMPLE_RATE_16360, 0, micbuf_datasize, true)))
		{
			log_error(n3dsaudio_log, "Could not start sampling with n3dsaudio");
			free(micbuf);
			micbuf=NULL;
			micExit();
			return;
		}
		stream_started=1;
    }
}

static void n3dsaudio_stop_sampling(void)
{
	if (stream_started) {
		MICU_StopSampling();
		free (micbuf);
		micbuf=NULL;
		micExit();
		stream_started = 0;
	}
}

static uint8_t n3dsaudio_get_sample(int channel)
{
    if (!stream_started) return 0x80;

	// just return the latest sample, converted to u8
	int i = ((signed char*)micbuf)[micGetLastSampleOffset()-1] + 0x80;

	return i;
}

static void n3dsaudio_shutdown(void)
{
    n3dsaudio_stop_sampling();
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
