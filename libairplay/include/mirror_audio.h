#pragma once
#ifndef MIRROR_AUDIO_H
#define MIRROR_AUDIO_H
 

typedef void(*audio_pcm_process_cb)(const void *buffer, int buflen, int sample_rate, int frame_size, int num_channels, const unsigned char* remote);
typedef void(*audio_pcm_destroy_cb)(const unsigned char* remote);

int aac_dec_init(audio_pcm_process_cb cb_process, audio_pcm_destroy_cb cb_destroy); 
int aac_dec_uninit();
int aac_dec_process(char* aac_buf, int sz);
 
void mirror_audio_start();
void mirror_audio_join();

 
#endif