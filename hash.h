#ifndef HASH_H
#define HASH_H

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

extern void show_warning2(char *msg, char *msg2);
#define show_warning(msg)  (show_warning2((msg), NULL))

typedef struct hash_struct hash_t;
typedef struct nodepair_struct {
  char *id;
  void *val;
} nodepair_t;
typedef int (*iteration_fptr)(char *, void *, void *);
typedef int (*hashsort_fptr)(nodepair_t *, nodepair_t *);

extern hash_t *new_hash(void);
extern void delete_hash(hash_t *tab);
extern void *hash_get(hash_t *tab, char *id);
extern void hash_put(hash_t *tab, char *id, void *val);
extern void hash_put_escurl(hash_t *tab, char *idesc, char *idurl, char *val);
extern void hash_put_escxml(hash_t *tab, char *id, char *val);
extern int hash_length(hash_t *tab);
extern void hash_iterate_sort(hash_t *tab, iteration_fptr fn, 
  hashsort_fptr sortfn, int parity, void *rock);
extern nodepair_t *hash_sort(hash_t *tab, hashsort_fptr sortfn, int *len);
extern void hash_iterate_array(nodepair_t *arr, int len, iteration_fptr fn, 
  int parity, void *rock);
extern void hash_iterate(hash_t *tab, iteration_fptr fn, void *rock);
extern void hash_dump(hash_t *tab, int isstring);
extern char *new_string(char *str);
extern char *append_string(char *str, char *str2);
extern char *append_string_esc(char *str, char *str2, int xml);
#define new_string_esc(str) (append_string_esc(NULL, (str), 0))
#define new_string_esc_xml(str) (append_string_esc(NULL, (str), 1))
extern char *append_spaces(char *str, int numspaces);
extern char *url_escape(char *str);
extern int is_string_nonwhite(char *str);
extern void trim_extra_newlines(char *str);

#endif /* HASH_H */
