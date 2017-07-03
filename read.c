#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "read.h"

static long buf_size = 0;
static char *buffer = NULL;

static int ensure_buffer(long size);

static int ensure_buffer(long size)
{
    if (size > buf_size) {
        if (!buffer) {
            buf_size = size + 1024;
            buffer = (char *)malloc(sizeof(char) * buf_size);
            if (!buffer) {
                show_warning("out of memory");
                return FALSE;
            }
        }
        else {
            while (size > buf_size)
                buf_size *= 2;
            buffer = (char *)realloc(buffer, sizeof(char) * buf_size);
            if (!buffer) {
                show_warning("out of memory");
                return FALSE;
            }
        }
    }
    
    return TRUE;
}

static char *trim_whitespace(char *buf)
{
    long pos;
    
    while (*buf == ' ' || *buf == '\t')
        buf++;

    pos = strlen(buf);
    while (pos && (buf[pos-1] == ' ' || buf[pos-1] == '\t'))
        pos--;
    buf[pos] = '\0';
    
    return buf;
}

pfile_t *read_pfile(char *filename)
{
    pfile_t *pf;
    hash_t *tab;
    FILE *fl;
    long pos;
    int stopflag;
    
    pf = (pfile_t *)malloc(sizeof(pfile_t));
    tab = new_hash();
    if (!pf || !tab) {
        show_warning("out of memory");
        return NULL;
    }
        
    pf->filename = new_string(filename);
    pf->tab = tab;
    pf->bodypos = -1;
    
    fl = fopen(filename, "r");
    if (!fl) {
        perror(filename);
        show_warning("unable to open");
        return NULL;
    }
    
    stopflag = 0; /* 1 for blank line, 2 for EOF */
    
    while (!stopflag) {
        int ch;
        char *cx;
        
        pos = 0;
        
        while (ch=getc(fl), ch != EOF && ch != '\n') {
            if (!ensure_buffer(pos+2)) { /* allow space for closing null */
                return NULL;
            }
            buffer[pos] = ch;
            pos++;
        }
        buffer[pos] = '\0';
        
        if (ch == EOF) {
            stopflag = 2;
        }
        
        cx = trim_whitespace(buffer);
            
        if (*cx == '\0') {
            /* blank line */
            if (!stopflag)
                stopflag = 1;
        }
        else {
            /* header line */
            char *cx2 = strchr(cx, ':');
            if (!cx2) {
                show_warning2("no colon in header line", cx);
            }
            else {
                *cx2 = '\0';
                cx2++;
                cx = trim_whitespace(cx);
                cx2 = trim_whitespace(cx2);
                hash_put(tab, new_string(cx), new_string(cx2));
            }
        }
    }
    
    if (stopflag == 1) {
        pf->bodypos = ftell(fl);
    }
    
    fclose(fl);
    
    /*
    printf("%s: bodypos = %d, header list:\n", pf->filename, pf->bodypos);
    hash_dump(tab, TRUE);
    */
    
    return pf;
}

char *file_getbody(char *filename)
{
    FILE *fl;
    long pos;
    int ch;
    char *cx;
    
    fl = fopen(filename, "r");
    if (!fl)
        return NULL;
    
    pos = 0;
    while (ch=getc(fl), ch != EOF) {
        if (!ensure_buffer(pos+2)) { /* allow space for closing null */
            return NULL;
        }
        buffer[pos] = ch;
        pos++;
    }
    buffer[pos] = '\0';

    fclose(fl);

    return new_string(buffer);
}

char *pfile_getbody(pfile_t *pf)
{
    FILE *fl;
    long pos;
    int ch;
    char *cx;
    
    if (pf->bodypos < 0)
        return NULL;
    
    fl = fopen(pf->filename, "r");
    if (!fl)
        return NULL;
    
    fseek(fl, pf->bodypos, 0);
    
    pos = 0;
    while (ch=getc(fl), ch != EOF) {
        if (!ensure_buffer(pos+2)) { /* allow space for closing null */
            return NULL;
        }
        buffer[pos] = ch;
        pos++;
    }
    buffer[pos] = '\0';

    fclose(fl);

    return new_string(buffer);
}

hash_t *parse_list(char *str)
{
    char *cx, *bx;
    char numbuf[16];
    int counter = 0;
    hash_t *tab = new_hash();
    if (!tab)
        return NULL;

    str = new_string(str); /* for diddling */
    
    cx = str;

    while (*cx) {
        for (bx=cx; *cx && *cx != ','; cx++) { }
        if (*cx) {
            *cx = '\0';
            cx++;
        }

        bx = trim_whitespace(bx);
        sprintf(numbuf, "%d", counter);
        counter++;
        hash_put(tab, new_string(bx), new_string(numbuf));
    }

    free(str);

    return tab;
}

