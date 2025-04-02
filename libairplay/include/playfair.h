#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

    extern unsigned char* fairplay_setup(char* message, int length);
    extern unsigned char* fairplay_decrypt(char* message3, unsigned char* cipherText);
#ifdef __cplusplus
}
#endif

