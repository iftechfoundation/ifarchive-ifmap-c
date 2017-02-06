#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

#define HASHSIZE (31)

typedef struct hashnode_struct {
  char *id;
  void *val;
  struct hashnode_struct *next;
} hashnode_t;

struct hash_struct {
  hashnode_t *bucket[HASHSIZE];
};

hash_t *new_hash()
{
  int ix;
  hash_t *tab = (hash_t *)malloc(sizeof(hash_t));
  if (!tab)
    return NULL;
  for (ix=0; ix<HASHSIZE; ix++)
    tab->bucket[ix] = NULL;
  return tab;
}

void delete_hash(hash_t *tab)
{
  int buck;
  hashnode_t *nod, *nod2;

  for (buck = 0; buck < HASHSIZE; buck++) {
    nod = tab->bucket[buck];
    while (nod) {
      nod2 = nod->next;
      free(nod);
      nod = nod2;
    }
    tab->bucket[buck] = NULL;
  }
    
  free(tab);
}

static int string_to_key(char *id)
{
  int res = 0;
  int ix = 0;
  unsigned char ch;
    
  while (*id) {
    ch = (*id);
    res += (ch << (ix & 7));
    id++;
    ix++;
  }
  return (res % HASHSIZE);
}

void *hash_get(hash_t *tab, char *id)
{
  int buck = string_to_key(id);
  hashnode_t *nod;
  for (nod = tab->bucket[buck]; nod; nod = nod->next) {
    if (!strcmp(id, nod->id))
      return nod->val;
  }
  return NULL;
}

void hash_put(hash_t *tab, char *id, void *val)
{
  int buck = string_to_key(id);
  hashnode_t *nod;

  for (nod = tab->bucket[buck]; nod; nod = nod->next) {
    if (!strcmp(id, nod->id)) {
      nod->val = val;
      return;
    }
  }
    
  nod = (hashnode_t *)malloc(sizeof(hashnode_t));
  if (!nod)
    return;
  nod->id = id;
  nod->val = val;
  nod->next = tab->bucket[buck];
  tab->bucket[buck] = nod;
}

void hash_put_escurl(hash_t *tab, char *idesc, char *idurl, char *val)
{
  char *urlval = url_escape(val);
  if (urlval == val) {
    char *esc = new_string_esc(val);
    hash_put(tab, idesc, esc);
    hash_put(tab, idurl, esc);
  }
  else {
    char *esc = new_string_esc(val);
    hash_put(tab, idesc, esc);
    esc = new_string_esc(urlval);
    hash_put(tab, idurl, esc);
  }
}

void hash_put_escxml(hash_t *tab, char *id, char *val)
{
  int len = 0;
  int newlen = 0;
  char *cx, *cx2, *str;
  for (cx=val; *cx; cx++) {
    switch (*cx) {
    case '&':
      newlen += 5;
      break;
    case '<':
    case '>':
      newlen += 4;
      break;
    default:
      newlen += 1;
      break;
    }
    len += 1;
  }
  if (len == newlen) {
    hash_put(tab, id, val);
    return;
  }
  str = (char *)malloc((1 + newlen) * sizeof(char));
  cx2 = str;
  for (cx=val; *cx; cx++) {
    switch (*cx) {
    case '&':
      strcpy(cx2, "&amp;");
      cx2 += 5;
      break;
    case '<':
      strcpy(cx2, "&lt;");
      cx2 += 4;
      break;
    case '>':
      strcpy(cx2, "&gt;");
      cx2 += 4;
      break;
    default:
      *cx2 = *cx;
      cx2++;
      break;
    }
  }
  *cx2 = '\0';
  hash_put(tab, id, str);
}

int hash_length(hash_t *tab)
{
  int ix;
  int buck;
  hashnode_t *nod;

  ix = 0;
  for (buck = 0; buck < HASHSIZE; buck++) {
    for (nod = tab->bucket[buck]; nod; nod = nod->next) {
      ix++;
    }
  }

  return ix;
}

void hash_iterate(hash_t *tab, iteration_fptr fn, void *rock)
{
  int buck;
  hashnode_t *nod;
  int res;

  for (buck = 0; buck < HASHSIZE; buck++) {
    for (nod = tab->bucket[buck]; nod; nod = nod->next) {
      res = fn(nod->id, nod->val, rock);
      if (res)
        return;
    }
  }
}

nodepair_t *hash_sort(hash_t *tab, hashsort_fptr sortfn, int *len)
{
  int buck;
  hashnode_t *nod;
  nodepair_t *arr;
  int ix, count, res;

  *len = 0;

  ix = 0;
  for (buck = 0; buck < HASHSIZE; buck++) {
    for (nod = tab->bucket[buck]; nod; nod = nod->next) {
      ix++;
    }
  }
  count = ix;

  if (count <= 0)
    return NULL;

  arr = (nodepair_t *)malloc(count * sizeof(nodepair_t));
  if (!arr) {
    return NULL;
  }

  ix = 0;
  for (buck = 0; buck < HASHSIZE; buck++) {
    for (nod = tab->bucket[buck]; nod; nod = nod->next) {
      arr[ix].id = nod->id;
      arr[ix].val = nod->val;
      ix++;
    }
  }

  qsort(arr, count, sizeof(nodepair_t), (void *)sortfn);

  *len = count;
  return arr;
}

void hash_iterate_array(nodepair_t *arr, int len, iteration_fptr fn, 
  int parity, void *rock)
{
  int ix, res;

  for (ix=0; ix<len; ix++) {
    if (parity) {
      hash_t *unit = arr[ix].val;
      hash_put(unit, "parity", (ix & 1) ? "Odd" : "Even");
    }
    res = fn(arr[ix].id, arr[ix].val, rock);
    if (res)
      return;
  }
}

void hash_iterate_sort(hash_t *tab, iteration_fptr fn, 
  hashsort_fptr sortfn, int parity, void *rock)
{
  int len;
  nodepair_t *arr;

  arr = hash_sort(tab, sortfn, &len);
  if (arr) {
    hash_iterate_array(arr, len, fn, parity, rock);
    free(arr);
  }
}

void hash_dump(hash_t *tab, int isstring)
{
  int buck;
  hashnode_t *nod;
  char *formstr;
    
  if (isstring)
    formstr = "%s: %s\n";
  else
    formstr = "%s\n";
    
  for (buck = 0; buck < HASHSIZE; buck++) {
    for (nod = tab->bucket[buck]; nod; nod = nod->next) {
      printf(formstr, nod->id, nod->val);
    }
  }
}

int is_string_nonwhite(char *str)
{
  char *cx;

  if (!str)
    return FALSE;

  for (cx=str; *cx; cx++) {
    if (!(*cx == ' ' || *cx == '\n' || *cx == '\t'))
      return TRUE;
  }

  return FALSE;
}

void trim_extra_newlines(char *str)
{
  int len;

  len = strlen(str);
  while (len >= 2 && str[len-1] == '\n' && str[len-2] == '\n') {
    str[len-1] = '\0';
    len--;
  }
}

char *new_string(char *str)
{
  char *res;
  if (!str)
    return NULL;
  res = (char *)malloc((1 + strlen(str)) * sizeof(char));
  if (!res)
    return NULL;
  strcpy(res, str);
  return res;
}

char *append_spaces(char *str, int numspaces)
{
  int ix, pos;
  if (!str)
    str = new_string("");
  if (numspaces < 0)
    numspaces = 0;
  pos = strlen(str);
  str = (char *)realloc(str, (1 + pos + numspaces) * sizeof(char));
  for (ix=0; ix<numspaces; ix++) {
    str[pos+ix] = ' ';
  }
  str[pos+numspaces] = '\0';
  return str;
}

char *append_string(char *str, char *str2)
{
  /* str2 is effectively const */
  if (!str)
    return new_string(str2);
  if (!str2)
    return str;
  str = (char *)realloc(str, (1 + strlen(str) + strlen(str2)) * sizeof(char));
  strcat(str, str2);
  return str;
}

int url_length(char *str)
{
  const char* http = "http://";
  const char* https = "https://";
  if (strncmp(str,http,strlen(http)) == 0
      || strncmp(str,https,strlen(https)) == 0) {
    char *cx = strchr(str,'>');
    if (cx != NULL) {
      return cx-str;
    }
  }
  return 0;
}

#define IS_PRINTABLE(ch)  \
   (((ch) >= ' ' && (ch) <= '~') || (ch) == '\n')

char *append_string_esc(char *str, char *str2, int xml)
{
  /* str2 is effectively const */
  int oldlen, newlen, urllen, esccount;
  char *cx, *cx2;
  if (!str2)
    return str;

  esccount = 0;
  newlen = 0;
  for (cx=str2; *cx; cx++) {
    switch (*cx) {
    case '<':
      urllen = url_length(cx+1);
      if (urllen > 0) {
        cx += urllen+1;          /* <url> */
        newlen += (2*urllen)+15; /* <a href="url">url</a> */
      } else {
        newlen += 4;
      }
      esccount++;
      break;
    case '>':
      newlen += 4;
      esccount++;
      break;
    case '&':
      if (xml) {
        newlen += 5;
        esccount++;
        break;
      }
    default:
      if (IS_PRINTABLE(*cx)) {
        newlen++;
      }
      else {
	printf("Warning: non-printable character %d\n",(int)(unsigned char)*cx);
        newlen += 6;
        esccount++;
      }
      break;
    }
  }

  if (!esccount)
    return append_string(str, str2);

  if (!str) {
    str = (char *)malloc((1 + newlen) * sizeof(char));
    cx2 = str;
  }
  else {
    oldlen = strlen(str);
    str = (char *)realloc(str, (1 + oldlen + newlen) * sizeof(char));
    cx2 = str + oldlen;
  }
    
  for (cx=str2; *cx; cx++) {
    switch (*cx) {
    case '<':
      urllen = url_length(cx+1);
      if (urllen > 0) {
        sprintf(cx2,"<a href=\"%.*s\">%.*s</a>",urllen,cx+1,urllen,cx+1);
        cx2 += (2*urllen)+15;
        cx += urllen+1;
      } else {
        strcpy(cx2, "&lt;");
        cx2 += 4;
      }
      break;
    case '>':
      strcpy(cx2, "&gt;");
      cx2 += 4;
      break;
    case '&':
      if (xml) {
        strcpy(cx2, "&amp;");
        cx2 += 5;
        break;
      }
    default:
      if (IS_PRINTABLE(*cx)) {
        *cx2 = *cx;
        cx2++;
      }
      else {
        sprintf(cx2, "&#x%02X;", (*cx) & 0xFF);
        cx2 += 6;
      }
      break;
    }
  }
  *cx2 = '\0';

  return str;
}

#define IS_URLABLE(ch)   \
    (((ch) > '*' && (ch) <= ';') || ((ch) >= '@' && (ch) <= 'z'))

char *url_escape(char *str)
{
  int esccount, newlen, oldlen;
  char *cx, *cx2, *newstr;

  newlen = 0;
  esccount = 0;
  for (cx=str; *cx; cx++) {
    if (IS_URLABLE(*cx)) {
      newlen += 1;
    }
    else {
      esccount += 1;
      newlen += 3;
    }
  }

  if (!esccount)
    return str;

  newstr = (char *)malloc((1 + newlen) * sizeof(char));
  for (cx=str, cx2=newstr; *cx; cx++) {
    if (IS_URLABLE(*cx)) {
      *cx2 = *cx;
      cx2++;
    }
    else {
      *cx2 = '%';
      cx2++;
      sprintf(cx2, "%02X", (*cx) & 0xFF);
      cx2 += 2;
    }
  }
  *cx2 = '\0';
  cx2++;

  if (newstr+newlen+1 != cx2)
    printf("Warning: url_escape generated wrong length.\n");

  return newstr;
}
