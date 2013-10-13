
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include "sha256.h"
#include "base64.h"

unsigned char TARGET[100][32];

int count = 0;

typedef struct seg seg;
struct seg {
    int threadNum;
    double start, end;
};

// memcmp
int compare(const void *s1, const void *s2) {
    size_t      n = 32;
    unsigned char u1, u2;

    for ( ; n-- ; s1++, s2++) {
        u1 = * (unsigned char *) s1;
        u2 = * (unsigned char *) s2;
        if ( u1 != u2) {
            return (u1-u2);
        }
    }
    return 0;
}

// gets the nth possible string
char* getNthString(double num) {
    // calculate the string
    char* str = malloc(9);
    int i = 1;
    str[0] = 'g';
    str[8] = '\0';
    do {
        double rem = fmod(num, 26);
        num = (num-rem)/26;
        str[i++] = 'a' + (int) rem;
        //num = (num-rem)/26;
    } while (num > 1);

    return str;
}

void incrementString(char* curr_string) {
  int i = 1;
  while (++curr_string[i]=='{') { // ak som na pismene Z
    curr_string[i++] = 'a';
    if (i==8) {
      break;
    }
  }
}

// each thread tries a range of strings
void* crack(seg* range) {
    FILE * fw;
    char name[10];
    memset(name, '\0', 10);
    sprintf(name,"solved%d",range->threadNum);
    sha256_context ctx;
    double i = range->start;
    unsigned char sha256sum[32];
    char* guess = getNthString(i);
    while (i++<range->end) {
        // magic here
        sha256_starts( &ctx );
        sha256_update( &ctx, (unsigned char*)guess, 8 );
        sha256_finish( &ctx, sha256sum );
        char * pItem;
        pItem = (char*) bsearch (sha256sum, TARGET, 100, 32, (int(*)(const void*,const void*)) compare);
        if (pItem!=NULL)  {
            count++;
            char* encodedHash = malloc(Base64encode_len(32));
            Base64encode(encodedHash, pItem, 32);
            fw = fopen (name,"a+");
            fprintf(fw, "%s %s\n", guess, encodedHash);
            fprintf(stdout, "%d %s %s\n", count, guess, encodedHash);
            fclose(fw);

            if(count > 100) {
                return 0;
            }
        }

        incrementString(guess);

    }
    return 0;
}

int main(int argc, char* argv[]) {

    int i = 0;
    FILE * pFile;
    pFile = fopen ("hashes.txt","r");
    if (pFile == NULL) {
        perror("error:File hashes.txt not found!\n");
        exit(EXIT_FAILURE);
    }

    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, pFile)) != -1) {
        //printf("Retrieved line of length %zu :\n", read);
        //printf("%s", line);
        Base64decode(TARGET[i], line);
        i++;
        if(i == 100) {
            break;
        }
    }

    fclose(pFile);

    qsort (TARGET, 100, 32, (int(*)(const void*,const void*))compare);

    double possibilities = 8031810176;

    fprintf(stdout, "possibilities: %f \n", possibilities);

    int       threads   = 4;
    double    perThread = floor(possibilities/threads);
    seg       range[threads];

    i = 0;
    pthread_t thread_id[threads];
    while (i<threads) {
        fprintf(stdout, "start %d %f \n", i, perThread * i + possibilities);
        range[i].threadNum = i;
        range[i].start = perThread * i + possibilities;

        // posledne vlakno do konca
        if (i==threads-1) {
            fprintf(stdout, "end   %d %f \n", i, 2*possibilities);
            range[i].end = 2*possibilities;
        } else {
            range[i].end = range[i].start + perThread;
            fprintf(stdout, "end   %d %f \n", i, (perThread * i) + possibilities + perThread);
        }
        if (pthread_create(&thread_id[i], NULL, (void *)crack, &range[i])) {
            perror("Creating thread");
            exit(1);
        }
        ++i;
    }

    // join threads
    i = 0;
    do {
        pthread_join(thread_id[i++], NULL);
    } while (i<threads);

    return 0;
}

