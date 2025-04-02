// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_JNI_AUDIO_OUTPUT_H
#define CHIAKI_JNI_AUDIO_OUTPUT_H


#include <stdint.h>
#include <inttypes.h>
#include <memory>


void *oboe_audio_output_new(bool samplerate_conversion);
void oboe_audio_output_free(void *audio_output);
void oboe_audio_output_settings(uint32_t channels, uint32_t rate, void *audio_output);
void oboe_audio_output_frame(int16_t *buf, size_t samples_count, void *audio_output);


/*
 * 	decoder->settings_cb(header->channels, header->rate, decoder->cb_user);
 *
				size_t codec_buf_size;
				uint8_t *codec_buf = AMediaCodec_getOutputBuffer(decoder->codec, (size_t)codec_buf_index, &codec_buf_size);
				size_t samples_count = info.size / sizeof(int16_t);
				//CHIAKI_LOGD(decoder->log, "Got %llu samples => %f ms of audio", (unsigned long long)samples_count, 1000.0f * (float)(samples_count / 2) / (float)decoder->audio_header.rate);
				decoder->frame_cb((int16_t *)codec_buf, samples_count, decoder->cb_user);

 */


#endif //CHIAKI_JNI_AUDIO_OUTPUT_H
