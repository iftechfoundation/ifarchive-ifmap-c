
extern int check_indexes(hash_t *dirlist);

extern int generate_datelist(char *outdir, pfile_t *plan, hash_t *dirlist,
  char *datelist_body);

extern int generate_dirlist(char *outdir, pfile_t *plan, hash_t *dirlist,
  char *dirlist_body);

extern int generate_xmllist(char *outdir, char *indir, pfile_t *plan, 
  hash_t *dirlist, char *xmllist_body);

extern int generate_output(char *outdir, pfile_t *plan, hash_t *dirlist);
