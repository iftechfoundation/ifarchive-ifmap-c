
typedef void (*substitution_fptr)(FILE *, void *);

extern void substitute(char *body, hash_t *tab, void *subrock, FILE *outfl);
