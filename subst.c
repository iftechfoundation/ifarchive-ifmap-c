#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "subst.h"

#define MAXDEPTH (16)

void substitute(char *body, hash_t *tab, void *subrock, FILE *outfl)
{
    char *cx, *bx;
    char ch;
    int activelist[MAXDEPTH];
    int depth = 0;
    
    activelist[depth] = TRUE;

    for (cx = body; *cx; cx++) {
        ch = *cx;
        if (ch != '{') {
            if (activelist[depth])
                putc(ch, outfl);
        }
        else {
            static char tagbuf[64];
            int len;
            
            bx = cx;
            cx++;
            len = 0;
            while (*cx && *cx != '}' && *cx != '\n' && len < 63) {
                tagbuf[len] = *cx;
                cx++;
                len++;
            }
            tagbuf[len] = '\0';
            if (*cx != '}') {
                show_warning2("brace-tag unclosed", tagbuf);
            }
            
            if (len == 0) {
                /* nothing */
            }
            else if (tagbuf[0] == '{') {
                if (activelist[depth])
                    putc('{', outfl);
            }
            else if (tagbuf[0] == '?') {
	        if (depth >= MAXDEPTH-1) {
		    show_warning2("brace-tags nested too deep", tagbuf);
		    break;
		}
                if (activelist[depth]) {
                    depth++;
                    activelist[depth] = (tab && hash_get(tab, tagbuf+1));
                }
                else {
                    depth++;
                    activelist[depth] = FALSE;
                }
            }
            else if (tagbuf[0] == ':') {
                if (depth==0 || activelist[depth-1]) {
                    activelist[depth] = !activelist[depth];
                }
            }
            else if (tagbuf[0] == '/') {
                depth--;
            }
            else if (activelist[depth]) {
                void *val;
                if (tab)
                    val = hash_get(tab, tagbuf);
                else
                    val = NULL;
                
                if (!val) {
                    fputs("[UNKNOWN]", outfl);
                    show_warning2("undefined brace-tag", tagbuf);
                }
                else if (tagbuf[0] == '_') {
                    substitution_fptr fptr = val;
                    (*fptr)(outfl, subrock);
                }
                else if (tagbuf[0] == '@') {
		    fputs("[NOT-PRINTABLE]", outfl);
                }
                else {
                    fputs((char *)val, outfl);
                }
            }
            else {
                /* normal tag, but not active. */
            }
        }
    }
}
