#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>

#define LOGIFLE "log.txt"

void strtime(char * s_time){
    time_t t = time(NULL);
    strcpy(s_time, ctime(&t));
    s_time[strlen(ctime(&t)) - 1] = 0;
}

void LOG(const char * mess, bool onScreen, bool toFile){
    if (onScreen){
        printf("%s\n", mess);
    }

    if (toFile){
        char s_time[30];
        strtime(s_time);

        FILE * f = fopen(LOGIFLE, "at");
        fprintf(f, "%s  --  %s\n", s_time, mess);
        fclose(f);
    }
}

void LOG_Begin(char * version){
    char s_time[30];
    strtime(s_time);

    FILE * f = fopen(LOGIFLE, "at");
    fprintf(f, "\n\n");
    fprintf(f, "*****************************************************\n");
    fprintf(f, "*              SUPER NUCLEI SEGMENTER               *\n");
    fprintf(f, "*****************************************************\n");
    fprintf(f, "ver %s\n\n", version);
    fprintf(f, "%s  --  START\n", s_time);
    fclose(f);
}

void LOG_End(){
    char s_time[30];
    strtime(s_time);

    FILE * f = fopen(LOGIFLE, "at");
    fprintf(f, "%s  --  SUCCESSFULLY FINISHED\n", s_time);
    fprintf(f, "\n\n");
    fclose(f);
}
