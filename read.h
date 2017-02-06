
typedef struct pfile_struct {
    char *filename;
    hash_t *tab;
    long bodypos;
} pfile_t;

extern pfile_t *read_pfile(char *flnm);
extern char *pfile_getbody(pfile_t *pf);
extern char *file_getbody(char *flnm);
extern hash_t *parse_list(char *str);
