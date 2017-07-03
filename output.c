#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "hash.h"
#include "read.h"
#include "output.h"
#include "subst.h"

typedef struct contextgen_struct {
  char *outdir;
  pfile_t *plan;
  char *elstr;
} contextgen_t;

typedef struct contextfil_struct {
  pfile_t *plan;
  hash_t *filelist;
  hash_t *subdirlist;
} contextfil_t;

typedef struct contextfilf_struct {
  FILE *outfl;
  char *elstr;
  contextfil_t *con;
} contextfilf_t;

typedef struct contextdir_struct {
  char *outdir;
  pfile_t *plan;
  hash_t *dirlist;
} contextdir_t;

typedef struct contextdire_struct {
  FILE *outfl;
  char *elstr;
  contextdir_t *con;
} contextdire_t;

typedef struct contextxml_struct {
  char *outdir;
  pfile_t *plan;
  hash_t *dirlist;
  char *dirbody;
  char *filebody;
} contextxml_t;

typedef struct contextxmle_struct {
  FILE *outfl;
  char *elstr;
  contextxml_t *con;
} contextxmle_t;

typedef struct contextxmled_struct {
  hash_t *filelist;
  contextxmle_t *con;
} contextxmled_t;

typedef struct contextxmledel_struct {
  FILE *outfl;
  char *elstr;
  contextxmled_t *con;
} contextxmledel_t;

typedef struct contextdat_struct {
  hash_t *filelist;
  pfile_t *plan;
  int count;
  nodepair_t *arr;
  int arrlen;
  time_t interval;
  time_t curtime;
} contextdat_t;

typedef struct contextdate_struct {
  FILE *outfl;
  char *elstr;
  contextdat_t *con;
} contextdate_t;

static int ensure_dir(char *path);
static int check_thunk(char *id, void *dirptr, void *rock);
static int checkel_thunk(char *id, void *dirptr, void *rock);
static int generate_thunk(char *id, void *dirptr, void *rock);
static void dirlist_thunk(FILE *outfl, void *rock);
static void filelist_thunk(FILE *outfl, void *rock);
static void subdirlist_thunk(FILE *outfl, void *rock);
static int dirlistel_thunk(char *id, void *memptr, void *rock);
static int filelistel_thunk(char *id, void *memptr, void *rock);
static int subdirlistel_thunk(char *id, void *memptr, void *rock);
static int datelist_thunk(char *id, void *dirptr, void *rock);
static int datelistel_thunk(char *id, void *dirptr, void *rock);
static void datelistfiles_thunk(FILE *outfl, void *rock);
static int datelistfilesel_thunk(char *id, void *filptr, void *rock);
static void xmllist_thunk(FILE *outfl, void *rock);
static int xmllistel_thunk(char *id, void *memptr, void *rock);
static void xmllistdir_thunk(FILE *outfl, void *rock);
static int xmllistdirel_thunk(char *id, void *fileptr, void *rock);

static int sort_dirs(nodepair_t *v1, nodepair_t *v2);
static int sort_files(nodepair_t *v1, nodepair_t *v2);
static int sort_filedates(nodepair_t *v1, nodepair_t *v2);

int check_indexes(hash_t *dirlist)
{
  hash_iterate(dirlist, &check_thunk, NULL);

  return TRUE;
}

static int check_thunk(char *id, void *dirptr, void *rock)
{
  hash_t *dir = dirptr;
  hash_t *filelist = hash_get(dir, "@files");

  if (filelist) {
    hash_iterate(filelist, checkel_thunk, NULL);
  }

  return FALSE;
}

static int checkel_thunk(char *id, void *fileptr, void *rock)
{
  hash_t *file = fileptr;

  if (hash_get(file, "date") == 0 && hash_get(file, "xlinkdir") == 0 && hash_get(file, "islink") == 0) {
    fprintf(stderr,"Index entry without file: %s/%s\n", (char *)hash_get(file, "dir"),
      (char *)hash_get(file, "name"));
  }

  return FALSE;
}

int generate_output(char *outdir, pfile_t *plan, hash_t *dirlist)
{
  contextgen_t con;
  con.outdir = outdir;
  con.plan = plan;
  con.elstr = pfile_getbody(plan);

  hash_iterate(dirlist, &generate_thunk, &con);

  free(con.elstr);

  return TRUE;
}

static int generate_thunk(char *id, void *dirptr, void *rock)
{
  contextgen_t *con = rock;
  contextfil_t conm;
  hash_t *dir = dirptr;
  char pathbuf[MAXPATHLEN];
  FILE *fl;

  sprintf(pathbuf, "%s/%s.html", con->outdir, 
    (char *)hash_get(dir, "xdir"));
  fl = fopen(pathbuf, "w");
  if (!fl) {
    perror(pathbuf);
    printf("Cannot create index file. Goodbye.\n");
    return FALSE;
  }

  conm.filelist = hash_get(dir, "@files");
  conm.subdirlist = hash_get(dir, "@subdirs");
  conm.plan = con->plan;

  hash_put(dir, "_files", &filelist_thunk);
  hash_put(dir, "_subdirs", &subdirlist_thunk);
  substitute(con->elstr, dir, &conm, fl);

  fclose(fl);

  return FALSE;
}

static void filelist_thunk(FILE *outfl, void *rock)
{
  contextfil_t *con = rock;
  contextfilf_t conm;

  conm.con = con;
  conm.outfl = outfl;
  conm.elstr = hash_get(con->plan->tab, "File-List-Entry");
  if (!conm.elstr)
    conm.elstr = "<li>{name}\n{desc}";

  hash_iterate_sort(con->filelist, &filelistel_thunk, sort_files, TRUE, &conm);
}

static int filelistel_thunk(char *id, void *fileptr, void *rock)
{
  contextfilf_t *conm = rock;
  hash_t *file = fileptr;
  
  substitute(conm->elstr, file, NULL, conm->outfl);
  fprintf(conm->outfl, "\n");
  
  return FALSE;
}

static void subdirlist_thunk(FILE *outfl, void *rock)
{
  contextfil_t *con = rock;
  contextfilf_t conm;

  conm.con = con;
  conm.outfl = outfl;
  conm.elstr = hash_get(con->plan->tab, "Subdir-List-Entry");
  if (!conm.elstr)
    conm.elstr = "<li>{dir}";

  hash_iterate_sort(con->subdirlist, &subdirlistel_thunk, sort_dirs, TRUE, &conm);
}

static int subdirlistel_thunk(char *id, void *dirptr, void *rock)
{
  contextfilf_t *conm = rock;
  hash_t *dir = dirptr;
  
  substitute(conm->elstr, dir, NULL, conm->outfl);
  fprintf(conm->outfl, "\n");
  
  return FALSE;
}

#define NUMINTERVALS (5)

int generate_datelist(char *outdir, pfile_t *plan, hash_t *dirlist,
  char *body)
{
  contextdat_t con;
  char pathbuf[MAXPATHLEN];
  int interval;
  static time_t intlist[NUMINTERVALS] = {
    0,
    7*24*60*60, 
    31*24*60*60,
    93*24*60*60,
    366*24*60*60
  };
  static char *intnamelist[NUMINTERVALS] = {
    NULL, "week", "month", "three months", "year"
  };
  FILE *fl;

  con.filelist = new_hash();
  con.count = 0;
  con.plan = plan;
  con.curtime = time(NULL);

  hash_iterate(dirlist, datelist_thunk, &con);

  if (!ensure_dir(outdir)) {
    printf("Goodbye.\n");
    return FALSE;
  }

  con.arr = hash_sort(con.filelist, sort_filedates, &con.arrlen);
  if (con.arr) {

    for (interval=0; interval<NUMINTERVALS; interval++) {
      if (interval)
        sprintf(pathbuf, "%s/date_%d.html", outdir, interval);
      else
        sprintf(pathbuf, "%s/date.html", outdir);

      fl = fopen(pathbuf, "w");
      if (!fl) {
        perror(pathbuf);
        printf("Cannot create datelist %d. Goodbye.\n", interval);
        return FALSE;
      }

      con.interval = intlist[interval];
      hash_put(plan->tab, "_files", &datelistfiles_thunk);
      if (intnamelist[interval])
        hash_put(plan->tab, "interval", intnamelist[interval]);
      substitute(body, plan->tab, &con, fl);

      fclose(fl);
    }

    free(con.arr);
  }

  return TRUE;
}

int datelist_thunk(char *id, void *dirptr, void *rock)
{
  contextdat_t *con = rock;
  hash_t *dir = dirptr;
  hash_t *filelist = hash_get(dir, "@files");

  if (filelist)
    hash_iterate(filelist, datelistel_thunk, con);

  return FALSE;
}

int datelistel_thunk(char *id, void *filptr, void *rock)
{
  contextdat_t *con = rock;
  hash_t *fil = filptr;

  if (hash_get(fil, "date")) {
    char countbuf[32];
    sprintf(countbuf, "%d", con->count);
    hash_put(con->filelist, new_string(countbuf), fil);
    con->count++;
  }

  return FALSE;
}

static void datelistfiles_thunk(FILE *outfl, void *rock)
{
  contextdat_t *con = rock;
  contextdate_t cone;

  cone.con = con;
  cone.outfl = outfl;
  cone.elstr = hash_get(con->plan->tab, "Date-List-Entry");
  if (!cone.elstr)
    cone.elstr = "<li>{name}";

  hash_iterate_array(con->arr, con->arrlen, &datelistfilesel_thunk, TRUE, &cone);
}

static int datelistfilesel_thunk(char *id, void *filptr, void *rock)
{
  contextdate_t *cone = rock;
  hash_t *fil = filptr;
  time_t filedate;
  
  if (cone->con->interval) {
    filedate = atol(hash_get(fil, "date"));
    if (filedate + cone->con->interval < cone->con->curtime)
      return TRUE;
  }
  
  substitute(cone->elstr, fil, NULL, cone->outfl);
  fprintf(cone->outfl, "\n");
  
  return FALSE;
}

int generate_dirlist(char *outdir, pfile_t *plan, hash_t *dirlist,
  char *body)
{
  int ix;
  char pathbuf[MAXPATHLEN];
  FILE *fl;
  contextdir_t con;

  if (!ensure_dir(outdir)) {
    printf("Goodbye.\n");
    return FALSE;
  }

  sprintf(pathbuf, "%s/dirlist.html", outdir);
  fl = fopen(pathbuf, "w");
  if (!fl) {
    perror(pathbuf);
    printf("Cannot create dirlist. Goodbye.\n");
    return FALSE;
  }

  con.outdir = outdir;
  con.dirlist = dirlist;
  con.plan = plan;

  hash_put(plan->tab, "_dirs", &dirlist_thunk);
  substitute(body, plan->tab, &con, fl);

  fclose(fl);

  return TRUE;
}

static void dirlist_thunk(FILE *outfl, void *rock)
{
  contextdir_t *con = rock;
  contextdire_t conm;

  conm.con = con;
  conm.outfl = outfl;
  conm.elstr = hash_get(con->plan->tab, "Dir-List-Entry");
  if (!conm.elstr)
    conm.elstr = "<li>{dir}";

  hash_iterate_sort(con->dirlist, &dirlistel_thunk, sort_dirs, TRUE, &conm);
}

static int dirlistel_thunk(char *id, void *dirptr, void *rock)
{
  contextdire_t *conm = rock;
  hash_t *dir = dirptr;
  
  substitute(conm->elstr, dir, NULL, conm->outfl);
  fprintf(conm->outfl, "\n");
  
  return FALSE;
}

static int sort_dirs(nodepair_t *v1, nodepair_t *v2)
{
  hash_t *pf1 = v1->val;
  hash_t *pf2 = v2->val;
  char *c1, *c2;
  
  c1 = hash_get(pf1, "dir");
  c2 = hash_get(pf2, "dir");
  
  return strcasecmp(c1, c2);
}

static int sort_files(nodepair_t *v1, nodepair_t *v2)
{
  hash_t *pf1 = v1->val;
  hash_t *pf2 = v2->val;
  char *c1, *c2;
  
  c1 = hash_get(pf1, "name");
  c2 = hash_get(pf2, "name");
  
  return strcasecmp(c1, c2);
}

static int sort_filedates(nodepair_t *v1, nodepair_t *v2)
{
  hash_t *pf1 = v1->val;
  hash_t *pf2 = v2->val;
  char *c1, *c2;
  long dat1, dat2;

  c1 = hash_get(pf1, "date");
  c2 = hash_get(pf2, "date");

  if (c1)
    dat1 = atol(c1);
  else
    dat1 = 0;
  if (c2)
    dat2 = atol(c2);
  else
    dat2 = 0;

  if (dat1-dat2 < 0)
    return 1;
  else if (dat1-dat2 > 0)
    return -1;

  c1 = hash_get(pf1, "name");
  c2 = hash_get(pf2, "name");
  
  return strcasecmp(c1, c2);
}

int generate_xmllist(char *outdir, char *indir, pfile_t *plan, hash_t *dirlist,
  char *body)
{
  int ix;
  char pathbuf[MAXPATHLEN];
  char *cx;
  FILE *fl;
  char *dirbody, *filebody;
  contextxml_t con;

  if (!ensure_dir(outdir)) {
    printf("Goodbye.\n");
    return FALSE;
  }

  dirbody = NULL;
  cx = hash_get(plan->tab, "XML-Dir-Template");
  if (cx) {
    sprintf(pathbuf, "%s/%s", indir, cx);
    dirbody = file_getbody(pathbuf);
  }
  if (!dirbody) {
    dirbody = "<directory>\n{dir}\n</directory>\n";
  }

  filebody = NULL;
  cx = hash_get(plan->tab, "XML-File-Template");
  if (cx) {
    sprintf(pathbuf, "%s/%s", indir, cx);
    filebody = file_getbody(pathbuf);
  }
  if (!filebody) {
    filebody = "<file>\n{name}\n</file>\n";
  }

  sprintf(pathbuf, "%s/Master-Index.xml", outdir);
  fl = fopen(pathbuf, "w");
  if (!fl) {
    perror(pathbuf);
    printf("Cannot create xml. Goodbye.\n");
    return FALSE;
  }

  con.outdir = outdir;
  con.dirlist = dirlist;
  con.plan = plan;
  con.dirbody = dirbody;
  con.filebody = filebody;

  hash_put(plan->tab, "_dirs", &xmllist_thunk);
  substitute(body, plan->tab, &con, fl);

  fclose(fl);

  return TRUE;
}

static void xmllist_thunk(FILE *outfl, void *rock)
{
  contextxml_t *con = rock;
  contextxmle_t conm;

  conm.con = con;
  conm.outfl = outfl;
  conm.elstr = con->dirbody;

  hash_iterate_sort(con->dirlist, &xmllistel_thunk, sort_dirs, TRUE, &conm);
}

static int xmllistel_thunk(char *id, void *dirptr, void *rock)
{
  contextxmle_t *conm = rock;
  contextxmled_t conz;
  hash_t *dir = dirptr;

  conz.con = conm;
  conz.filelist = hash_get(dir, "@files");

  hash_put(dir, "_files", &xmllistdir_thunk);
  
  substitute(conm->elstr, dir, &conz, conm->outfl);
  fprintf(conm->outfl, "\n");
  
  return FALSE;
}

static void xmllistdir_thunk(FILE *outfl, void *rock)
{
  contextxmled_t *conz = rock;
  contextxmledel_t cony;

  cony.con = conz;
  cony.outfl = outfl;
  cony.elstr = conz->con->con->filebody;

  hash_iterate_sort(conz->filelist, &xmllistdirel_thunk, sort_files, TRUE, &cony);
}

static int xmllistdirel_thunk(char *id, void *fileptr, void *rock)
{
  contextxmledel_t *cony = rock;
  hash_t *file = fileptr;
  
  substitute(cony->elstr, file, NULL, cony->outfl);
  fprintf(cony->outfl, "\n");
  
  return FALSE;
}

static int ensure_dir(char *path)
{
  int ix;
  struct stat sta;

  ix = stat(path, &sta);
  if (ix && errno != ENOENT) {
    perror(path);
    show_warning("Cannot find destination directory.");
    return FALSE;
  }
  if (!ix && !S_ISDIR(sta.st_mode)) {
    show_warning("Destination exists, but is not a directory.");
    return FALSE;
  }
  if (ix && errno == ENOENT) {
    ix = mkdir(path, 0755);
    if (ix) {
      perror(path);
      show_warning("Cannot create destination directory.");
      return FALSE;
    }
  }
  return TRUE;
}
