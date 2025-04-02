#pragma once

#include <android/log.h>

 

/*
static int my_fprintf(FILE *stream, const char *format, ...){
    va_list ap;
    va_start(ap, format);
    __android_log_vprint(ANDROID_LOG_DEBUG, "XXX", format, ap);
    va_end(ap);
    return 0;
}


void
dump_buffer(FILE *fp, char *tag, char *buffer, int bytes)
{
    int i, j, ch;

    my_fprintf(fp, "%s %d(0x%x) bytes\n", (tag ? tag : ""), bytes, bytes);
    for (i = 0; i < bytes; i += 16)
    {
        my_fprintf(fp, "%s   ", (tag ? tag : ""));

        for (j = 0; j < 16 && (i + j) < bytes; j ++)
            my_fprintf(fp, " %02X", buffer[i + j] & 255);

        while (j < 16)
        {
            my_fprintf(fp, "   ");
            j++;
        }

        my_fprintf(fp, "    ");
        for (j = 0; j < 16 && (i + j) < bytes; j ++)
        {
            ch = buffer[i + j] & 255;
            if (ch < ' ' || ch == 127)
                //ch = '.';
                //putc(ch, fp);
                my_fprintf(fp, ".");
        }
        //putc('\n', fp);
        my_fprintf(fp, "\n");
    }
    fflush(fp);
}*/
