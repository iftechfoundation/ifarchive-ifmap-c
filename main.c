#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/param.h>
#include "hash.h"
#include "read.h"
#include "output.h"
#include "subst.h"
#include "parse.h"

static char *master = "Master-Index";
static char *indir = "lib";
static char *outdir = "out";
static char *treedir = NULL;
static int doxml = FALSE;
static int doexclude = FALSE;

int verbose = FALSE;

int main(int argc, char *argv[])
{
  int ix;
  char pathbuf[MAXPATHLEN];
  pfile_t *plan;
  char *toplevel_body, *dirlist_body, *datelist_body, *xmllist_body;
  hash_t *dirlist;
  char *cx;

  for (ix=1; ix<argc; ix++) {
    if (!strcmp(argv[ix], "-index") && ix+1<argc) {
      ix++;
      master = argv[ix];
    }
    else if (!strcmp(argv[ix], "-src") && ix+1<argc) {
      ix++;
      indir = argv[ix];
    }
    else if (!strcmp(argv[ix], "-dest") && ix+1<argc) {
      ix++;
      outdir = argv[ix];
    }
    else if (!strcmp(argv[ix], "-tree") && ix+1<argc) {
      ix++;
      treedir = argv[ix];
    }
    else if (!strcmp(argv[ix], "-v")) {
      verbose = TRUE;
    }
    else if (!strcmp(argv[ix], "-xml")) {
      doxml = TRUE;
    }
    else if (!strcmp(argv[ix], "-exclude")) {
      doexclude = TRUE;
    }
    else {
      printf(
	"Usage: %s [ -v ] [ -index Master-Index ] [ -src srcdir ] [ -dest destdir ]\n"
	"       [ -tree treedir ] [ -xml ] [ -exclude ]\n",
	argv[0]);
      return -1;
    }
  }
    
  /* printf("Welcome to ifmap.\n"); */

  sprintf(pathbuf, "%s/index", indir);
  plan = read_pfile(pathbuf);
  if (!plan) {
    printf("Cannot find index file. Goodbye.\n");
    return -1;
  }

  toplevel_body = NULL;
  cx = hash_get(plan->tab, "Top-Level-Template");
  if (cx) {
    sprintf(pathbuf, "%s/%s", indir, cx);
    toplevel_body = file_getbody(pathbuf);
    if (!toplevel_body) {
      printf("Cannot find top-level template file. Using default.\n");
    }
  }
  if (!toplevel_body) {
    toplevel_body = "Welcome to the archive.\n";
  }

  dirlist_body = NULL;
  cx = hash_get(plan->tab, "Dir-List-Template");
  if (cx) {
    sprintf(pathbuf, "%s/%s", indir, cx);
    dirlist_body = file_getbody(pathbuf);
    if (!dirlist_body) {
      printf("Cannot find dirlist template file. Using default.\n");
    }
  }
  if (!dirlist_body) {
    dirlist_body = "<html><body>\n{_dirs}\n"
      "</body></html>\n";
  }

  xmllist_body = NULL;
  cx = hash_get(plan->tab, "XML-Template");
  if (cx) {
    sprintf(pathbuf, "%s/%s", indir, cx);
    xmllist_body = file_getbody(pathbuf);
    if (!xmllist_body) {
      printf("Cannot find xmllist template file. Using default.\n");
    }
  }
  if (!xmllist_body) {
    xmllist_body = "<xml>\n{_dirs}\n"
      "</xml>\n";
  }

  datelist_body = NULL;
  cx = hash_get(plan->tab, "Date-List-Template");
  if (cx) {
    sprintf(pathbuf, "%s/%s", indir, cx);
    datelist_body = file_getbody(pathbuf);
    if (!datelist_body) {
      printf("Cannot find datelist template file. Using default.\n");
    }
  }
  if (!datelist_body) {
    datelist_body = "<html><body>\n{_files}\n"
      "</body></html>\n";
  }

  dirlist = new_hash();
  hash_put(plan->tab, "@dirlist", dirlist);

  if (!parse_master(master, dirlist, toplevel_body, treedir, indir, doexclude, doxml))
    return -1;

  if (!check_indexes(dirlist))
    return -1;

  if (!generate_dirlist(outdir, plan, dirlist, dirlist_body))
    return -1;

  if (!generate_output(outdir, plan, dirlist))
    return -1;

  if (!generate_datelist(outdir, plan, dirlist, datelist_body))
    return -1;

  if (doxml) {
    if (!generate_xmllist(outdir, indir, plan, dirlist, xmllist_body))
      return -1;
  }

  /* printf("No problem.\n"); */

  return 0;
}

void show_warning2(char *msg, char *msg2)
{
  if (msg2)
    printf("Problem: %s: %s\n", msg, msg2);
  else
    printf("Problem: %s\n", msg);
}
