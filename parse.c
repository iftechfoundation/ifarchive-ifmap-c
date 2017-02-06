#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "hash.h"
#include "read.h"
#include "output.h"
#include "subst.h"
#include "parse.h"
#include "md5.h"

#define ROOTNAME "if-archive"

#define BUFSIZE (255)
#define EXCLUDESIZE (32)

typedef struct contextsubdir_struct {
  hash_t *dirlist;
} contextsubdir_t;

typedef struct contextsubdire_struct {
  contextsubdir_t *con;
  hash_t *dir;
  char *dirname;
  hash_t *subdirlist;
} contextsubdire_t;

typedef struct exclude_struct {
  int do_exclude;
  char allowed[EXCLUDESIZE][MAXPATHLEN];
} exclude_t;

static void scan_directory(hash_t *dirlist, char *treedir, char *dirname,
  hash_t *parentlist, char *parentdir, exclude_t *exclude, int domd5);
static int subdir_thunk(char *id, void *dirptr, void *rock);
static int subdirel_thunk(char *id, void *dirptr, void *rock);

static void tabsub(char *buf);
static int bracket_count(char* buf);

static char *monthlist[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void add_dir_links(hash_t *dir, char* path)
{
  static char buf[65535];
  int ix = 0, jx = 0, kx;

  buf[0] = '\0';
  while (1)
  {
    if (path[jx] == '\0' || path[jx] == '/')
    {
      char link[BUFSIZE+1];
      sprintf(link, "<a href=\"%.*s.html\">%.*s</a>", jx, path, jx-ix, path+ix);
      for (kx = 0; link[kx] != '>'; kx++)
      {
        if (link[kx] == '/')
          link[kx] = 'X';
      }
      kx = strlen(buf);
      if (kx > 0)
        buf[kx++] = '/';
      strcpy(buf+kx, link);
      if (path[jx] == '\0')
        break;
      ix = jx+1;
    }
    jx++;
  }
	hash_put(dir, "xdirlinks", new_string(buf));
}

int parse_master(char *master, hash_t *dirlist, char *toplevelbody,
  char *treedir, char *indir, int doexclude, int domd5)
{
  int ix, jx;
  FILE *infl;
  char buf[BUFSIZE+1];
  char *cx, *bx;
  char *headerstr = NULL;
  char *headerstrraw = NULL;
  char *filestr = NULL;
  char *filestrraw = NULL;
  int buflen, indent, firstindent;
  hash_t *dir, *filelist, *file;
  int inheader = TRUE;
  int headerpara = TRUE;
  int done = FALSE;
  int brackets = 0;
  exclude_t exclude;

  infl = fopen(master, "r");
  if (!infl) {
    printf("Cannot open %s. Goodbye.\n", master);
    return 0;
  }

  dir = NULL;
  filelist = NULL;
  file = NULL;
  filestr = NULL;
  filestrraw = NULL;

  dir = new_hash();
  hash_put(dir, "dir", ROOTNAME);
  hash_put(dir, "xdir", ROOTNAME);
  add_dir_links(dir, ROOTNAME);
  hash_put(dir, "@files", new_hash());
  if (toplevelbody) {
    hash_put(dir, "header", toplevelbody);
    hash_put(dir, "hasdesc", "1");
  }
  hash_put(dirlist, ROOTNAME, dir);
  dir = NULL;

  while (!done) {
    cx = fgets(buf, BUFSIZE, infl);
    if (!cx) {
      buflen = 0;
      buf[0] = '\0';
      done = TRUE;
    }
    else {
      buflen = strlen(buf);
      if (buflen && buf[buflen-1] == '\n') {
	buflen--;
	buf[buflen] = '\0';
      }
    }

    tabsub(buf);

    if (done || (!strncmp(buf, ROOTNAME, 10) && buf[buflen-1] == ':')) {
      if (dir) {
	int len;

	if (file) {
	  if (filestr) {
	    hash_put(file, "desc", filestr);
            if (is_string_nonwhite(filestr))
              hash_put(file, "hasdesc", "1");
	    filestr = NULL;
	  }
	  if (filestrraw) {
            char *dx = new_string_esc_xml(filestrraw);
            trim_extra_newlines(dx);
	    hash_put(file, "xmldesc", dx);
            if (is_string_nonwhite(filestrraw))
              hash_put(file, "hasxmldesc", "1");
	    filestrraw = NULL;
	  }
	  hash_put(filelist, (char *)hash_get(file, "rawname"), file);
	  file = NULL;
	}

	cx = hash_get(dir, "dir");
	if (verbose)
	  printf("Completing %s...\n", cx);
	hash_put(dirlist, cx, dir);
	if (headerstr) {
	  hash_put(dir, "header", headerstr);
          if (is_string_nonwhite(headerstr))
            hash_put(dir, "hasdesc", "1");
	  headerstr = NULL;
	}
        if (headerstrraw) {
          trim_extra_newlines(headerstrraw);
          hash_put(dir, "xmlheader", new_string_esc_xml(headerstrraw));
          if (is_string_nonwhite(headerstrraw))
            hash_put(dir, "hasxmldesc", "1");
          headerstrraw = NULL;
        }
	dir = NULL;
	filelist = NULL;
      }

      if (!done) {
	char *cx2, *cx3;

	dir = new_hash();
	filelist = new_hash();
	file = NULL;
	filestr = NULL;
        filestrraw = NULL;
	hash_put(dir, "@files", filelist);
	buflen--;
        buf[buflen] = '\0';
	if (verbose)
	  printf("Starting %s:\n", buf);

	hash_put(dir, "dir", new_string(buf));

	cx = new_string(buf);
	for (cx2 = cx; *cx2; cx2++)
	  if (*cx2 == '/')
            *cx2 = 'X';
	hash_put(dir, "xdir", cx);
  add_dir_links(dir, buf);

	cx = new_string(buf);
	for (cx2=cx, cx3=NULL; *cx2; cx2++)
	  if (*cx2 == '/')
	    cx3 = cx2;
	if (cx3) {
	  *cx3 = '\0';
	  hash_put(dir, "parentdir", new_string(cx));
	  for (cx2 = cx; *cx2; cx2++)
	    if (*cx2 == '/')
	      *cx2 = 'X';
	  hash_put(dir, "xparentdir", cx);
	}
	
	inheader = TRUE;
	headerpara = TRUE;
	headerstr = NULL;
        headerstrraw = NULL;
      }

      continue;
    }

    if (!dir) {
      continue;
    }

    jx = 0;
    for (cx = buf; *cx; cx++) {
      if (*cx == ' ')
	continue;
      if (!(*cx >= ' ' && *cx <= '-'))
	break;
      jx++;
    }
    if (*cx == '\0' && jx)
      continue;

    for (cx = buf, indent = 0; *cx == ' '; cx++, indent++);
    bx = cx;

    if (inheader) {
      if (!strncmp(cx, "Index", 5)) {
	cx += 5;
	for (; *cx == ' '; cx++);
	if (!strncasecmp(cx, "this file", 9)) {
	  inheader = FALSE;
	  continue;
	}
      }
    }

    if (inheader) {
      if (strlen(bx)) {
	headerstr = append_string_esc(headerstr, bx, 0);
	headerstr = append_string(headerstr, "\n");
	headerpara = FALSE;
        headerstrraw = append_string(headerstrraw, bx);
	headerstrraw = append_string(headerstrraw, "\n");
      }
      else {
	if (!headerpara) {
	  headerstr = append_string(headerstr, "<p>\n");
	  headerpara = TRUE;
	}
        if (headerstrraw && headerstrraw[0]) {
          headerstrraw = append_string(headerstrraw, "\n");
        }
      }
      continue;
    }

    if (indent == 0 && *bx) {
      char *cx2, *id;
      int len;

      if (file) {
	if (filestr) {
	  hash_put(file, "desc", filestr);
          if (is_string_nonwhite(filestr))
            hash_put(file, "hasdesc", "1");
	  filestr = NULL;
	}
	if (filestrraw) {
          char *dx = new_string_esc_xml(filestrraw);
          trim_extra_newlines(dx);
	  hash_put(file, "xmldesc", dx);
          if (is_string_nonwhite(filestrraw))
            hash_put(file, "hasxmldesc", "1");
	  filestrraw = NULL;
	}
	hash_put(filelist, (char *)hash_get(file, "rawname"), file);
	file = NULL;
      }

      file = new_hash();
      filestr = NULL;
      filestrraw = NULL;
      firstindent = -1;

      for (cx2 = bx; *cx2 && *cx2 != ' '; cx2++);
      len = cx2-bx;
      id = (char *)malloc(sizeof(char) * (len+1));
      strncpy(id, bx, len);
      id[len] = '\0';

      while (*cx2 == ' ')
	cx2++;
      if (*cx2) {
	firstindent = (cx2-buf);
	filestr = append_string_esc(NULL, cx2, 0);
	filestr = append_string(filestr, "\n");
	filestrraw = append_string(NULL, cx2);
	filestrraw = append_string(filestrraw, "\n");
	brackets = bracket_count(cx2);
      }
      else {
	firstindent = -1;
	filestr = new_string("");
        filestrraw = new_string("");
      }

      hash_put(file, "rawname", id);
      hash_put_escurl(file, "name", "nameurl", id);
      hash_put_escxml(file, "namexml", id);
      hash_put(file, "dir", (char *)hash_get(dir, "dir"));
    }
    else {
      if (strlen(bx)) {
	if (firstindent < 0) {
	  firstindent = indent;
	  brackets = 0;
	}
	if ((firstindent != indent) && (brackets == 0)) {
	  filestr = append_string(filestr, "<br>&nbsp;&nbsp;");
          filestrraw = append_spaces(filestrraw, indent-firstindent);
        }
	filestr = append_string_esc(filestr, bx, 0);
	filestr = append_string(filestr, "\n");
        filestrraw = append_string(filestrraw, bx);
	brackets += bracket_count(bx);
      }
      filestrraw = append_string(filestrraw, "\n");
    }
  }

  fclose(infl);

  exclude.do_exclude = doexclude;
  for (ix = 0; ix < EXCLUDESIZE; ix++)
    exclude.allowed[ix][0] = '\0';

  sprintf(buf, "%s/no-index-entry", indir);
  infl = fopen(buf, "r");
  if (!infl) {
    perror(buf);
    printf("Cannot open no-index-entry file. Goodbye.\n");
    return FALSE;
  }

  ix = 0;
  while (!feof(infl) && (ix < EXCLUDESIZE)) {
    fgets(exclude.allowed[ix], MAXPATHLEN, infl);

    jx = strlen(exclude.allowed[ix]);
    if ((jx > 0) && exclude.allowed[ix][jx-1] == '\n')
      exclude.allowed[ix][jx-1] = '\0';
    ix++;
  }
  fclose(infl);

  /* Do an actual scan of the tree and write in any directories
     we missed. */
  if (treedir)
    scan_directory(dirlist, treedir, ROOTNAME, NULL, NULL, &exclude, domd5);

  /* Now we have to buzz through and assign subdirectories. */
  {
    contextsubdir_t con;
    con.dirlist = dirlist;
    hash_iterate(dirlist, &subdir_thunk, &con);
  }  

  return 1;
}

static int check_exclude(exclude_t *exclude, const char *name)
{
  int i;

  for (i = 0; i < EXCLUDESIZE; i++) {
    if (exclude->allowed[i][0] == '\0')
      break;
    if (strncmp(name, exclude->allowed[i], strlen(exclude->allowed[i])) == 0)
      return 0;
  }

  fprintf(stderr,"File without index entry: %s\n", name);
  return (exclude->do_exclude == 0) ? 0 : 1;
}

static void scan_directory(hash_t *dirlist, char *treedir, char *dirname,
  hash_t *parentlist, char *parentdir, exclude_t *exclude, int domd5)
{
  DIR *udir;
  hash_t *dir, *filelist;
  struct dirent *ent;
  struct stat sta, sta2;
  char pathname[MAXPATHLEN];
  char dirname2[MAXPATHLEN];

  dir = hash_get(dirlist, dirname);
  if (!dir) {
    show_warning2("Unable to find directory.", dirname);
    return;
  }
  filelist = hash_get(dir, "@files");

  sprintf(pathname, "%s/%s", treedir, dirname);

  udir = opendir(pathname);
  if (!udir) {
    show_warning2("Unable to open directory.", pathname);
    return;
  }

  while ((ent = readdir(udir)) != NULL) {
    char *fname = ent->d_name;
    if (*fname == '\0' || *fname == '.')
      continue;

    sprintf(dirname2, "%s/%s", dirname, fname);
    sprintf(pathname, "%s/%s/%s", treedir, dirname, fname);

    if (lstat(pathname, &sta)) {
      show_warning2("Unable to lstat.", pathname);
      continue;
    }

    if (S_ISLNK(sta.st_mode)) {
      /* got a symlink */
      char linkname[MAXPATHLEN+1];
      int linklen;
      linklen = readlink(pathname, linkname, MAXPATHLEN);
      if (linklen <= 0) {
	show_warning2("Unable to readlink.", pathname);
      }
      else {
	linkname[linklen] = '\0';
	if (linklen && linkname[linklen-1] == '/') {
	  linklen--;
	  linkname[linklen] = '\0';
	}
	if (!stat(pathname, &sta2)) {
	  if (S_ISREG(sta2.st_mode)) {
	    hash_t *file;
	    char *cx;
	    struct tm *tmdat;
	    char datebuf[32];
	    file = hash_get(filelist, fname);
	    if (!file) {
	      if (check_exclude(exclude,dirname2))
		continue;
	      file = new_hash();
	      cx = new_string(fname);
	      hash_put(file, "rawname", cx);
	      hash_put_escurl(file, "name", "nameurl", cx);
	      hash_put_escxml(file, "namexml", cx);
	      hash_put(file, "dir", new_string_esc(dirname));
	      hash_put(file, "desc", " ");
	      hash_put(filelist, cx, file);
	    }
            hash_put(file, "islink", "1");
            hash_put(file, "islinkfile", "1");
            hash_put(file, "linkpath", new_string(linkname)); /* ### canonicalize */
            sprintf(datebuf, "%ld", (long)sta2.st_mtime);
            hash_put(file, "date", new_string(datebuf));
            tmdat = localtime(&sta2.st_mtime);
            sprintf(datebuf, "%02d-%s-%d", 
              tmdat->tm_mday, monthlist[tmdat->tm_mon], tmdat->tm_year+1900);
            hash_put(file, "datestr", new_string(datebuf));
	  }
	  else if (S_ISDIR(sta2.st_mode)) { 
	    char targetname[MAXPATHLEN+1];
	    int targetlen;
	    char *cx, *cx2;
	    hash_t *file;
	    strcpy(targetname, dirname);
	    targetlen = strlen(targetname);
	    for (cx = linkname; *cx; ) {
	      cx2 = strchr(cx, '/');
	      if (!cx2) {
		cx2 = cx + strlen(cx);
	      }
	      if (cx2-cx == 1 && cx[0] == '.') {
		/* nothing */
	      }
	      else if (cx2-cx == 2 && cx[0] == '.' && cx[1] == '.') {
		if (targetlen)
		  targetlen--;
		while (targetlen && targetname[targetlen] != '/')
		  targetlen--;
	      }
	      else {
		targetname[targetlen] = '/';
		targetlen++;
		memcpy(targetname+targetlen, cx, sizeof(char) * (cx2-cx));
		targetlen += (cx2-cx);
	      }
	      if (*cx2 == '/')
		cx2++;
	      cx = cx2;
	    }
	    targetname[targetlen] = '\0';
	    if (targetname[0] == '/')
	      continue;
	    file = hash_get(filelist, fname);
	    if (!file) {
	      char tempname[MAXPATHLEN+1];
	      sprintf(tempname, "Symlink to %s", targetname);
	      file = new_hash();
	      cx = new_string(fname);
	      hash_put(file, "name", cx);
	      hash_put_escxml(file, "namexml", cx);
	      hash_put(file, "dir", new_string(dirname));
	      hash_put(file, "desc", new_string(tempname));
	      hash_put(filelist, cx, file);
	    }
            hash_put(file, "islink", "1");
            hash_put(file, "islinkdir", "1");
	    cx = new_string(targetname);
	    hash_put(file, "linkdir", cx);
	    cx = new_string(targetname);
	    for (cx2 = cx; *cx2; cx2++)
	      if (*cx2 == '/')
		*cx2 = 'X';
	    hash_put(file, "xlinkdir", cx);
	  }
	}
      }
    }
    else if (S_ISREG(sta.st_mode)) {
      /* regular file */
      hash_t *file;
      char *cx;
      struct tm *tmdat;
      char datebuf[32];
      if (!strcmp(fname, "Index"))
	continue;
      file = hash_get(filelist, fname);
      if (!file) {
	if (check_exclude(exclude,dirname2))
	  continue;
	file = new_hash();
	cx = new_string(fname);
	hash_put(file, "rawname", cx);
	hash_put_escurl(file, "name", "nameurl", cx);
	hash_put_escxml(file, "namexml", cx);
	hash_put(file, "dir", new_string_esc(dirname));
	hash_put(file, "desc", " ");
	hash_put(filelist, cx, file);
      }
      sprintf(datebuf, "%ld", (long)sta.st_size);
      hash_put(file, "filesize", new_string(datebuf));
      sprintf(datebuf, "%ld", (long)sta.st_mtime);
      hash_put(file, "date", new_string(datebuf));
      tmdat = localtime(&sta.st_mtime);
      sprintf(datebuf, "%02d-%s-%d", 
	tmdat->tm_mday, monthlist[tmdat->tm_mon], tmdat->tm_year+1900);
      hash_put(file, "datestr", new_string(datebuf));
      if (domd5)
        hash_put(file, "md5", new_string(md5(pathname)));
    }
    else if (S_ISDIR(sta.st_mode)) {
      /* directory */
      hash_t *dir2, *file;
      char *cx, *cx2, *cx3;
      dir2 = hash_get(dirlist, dirname2);
      if (!dir2) {
	dir2 = new_hash();
	cx = new_string(dirname2);
	hash_put(dir2, "dir", cx);
	hash_put(dirlist, cx, dir2);
	hash_put(dir2, "@files", new_hash());
	cx = new_string(dirname2);
	for (cx2=cx; *cx2; cx2++)
	  if (*cx2 == '/')
	    *cx2 = 'X';
	hash_put(dir2, "xdir", cx);
  add_dir_links(dir2, dirname2);

	cx = new_string(dirname2);
	for (cx2=cx, cx3=NULL; *cx2; cx2++)
	  if (*cx2 == '/')
	    cx3 = cx2;
	if (cx3) {
	  *cx3 = '\0';
	  hash_put(dir2, "parentdir", new_string(cx));
	  for (cx2 = cx; *cx2; cx2++)
	    if (*cx2 == '/')
	      *cx2 = 'X';
	  hash_put(dir2, "xparentdir", cx);
	}
      }
      file = hash_get(filelist, fname);
      if (file) {
	cx = new_string(dirname2);
	hash_put(file, "linkdir", cx);
	cx = new_string(dirname2);
	for (cx2 = cx; *cx2; cx2++)
	  if (*cx2 == '/')
	    *cx2 = 'X';
	hash_put(file, "xlinkdir", cx);
      }
      if (parentlist && parentdir) {
	hash_t *parentfile;
	char parentname[MAXPATHLEN];
	sprintf(parentname,"%s/%s",parentdir,fname);
	parentfile = hash_get(parentlist, parentname);
	if (parentfile) {
	  cx = new_string(dirname2);
	  hash_put(parentfile, "linkdir", cx);
	  cx = new_string(dirname2);
	  for (cx2 = cx; *cx2; cx2++)
	    if (*cx2 == '/')
	      *cx2 = 'X';
	  hash_put(parentfile, "xlinkdir", cx);
	}
      }
      scan_directory(dirlist, treedir, dirname2, filelist, fname, exclude, domd5);
    }
  }

  closedir(udir);
}

static int subdir_thunk(char *id, void *dirptr, void *rock)
{
  contextsubdir_t *con = rock;
  hash_t *dir = dirptr;
  hash_t *filelist;
  int len;
  contextsubdire_t conm;
  char numbuf[16];

  filelist = hash_get(dir, "@files");
  if (filelist) {
    len = hash_length(filelist);
    if (len) {
      sprintf(numbuf, "%d", len);
      hash_put(dir, "count", new_string(numbuf));
    }
  }

  conm.con = con;
  conm.dir = dir;
  conm.dirname = id;
  conm.subdirlist = new_hash();

  hash_iterate(con->dirlist, &subdirel_thunk, &conm);
  
  hash_put(dir, "@subdirs", conm.subdirlist);
  len = hash_length(conm.subdirlist);
  if (len) {
    sprintf(numbuf, "%d", len);
    hash_put(dir, "subdircount", new_string(numbuf));
  }
  return FALSE;
}

static int subdirel_thunk(char *id, void *dirptr, void *rock)
{
  contextsubdire_t *con = rock;
  hash_t *dir = dirptr; 
  int idlen = strlen(con->dirname);

  if (!strncmp(con->dirname, id, idlen)
      && id[idlen] == '/'
      && !strchr(id+idlen+1, '/')) {
    hash_put(con->subdirlist, id, dir);
  }
  return FALSE;
}

static void tabsub(char *buf)
{
  char buf2[BUFSIZE+1];
  char *cx, *cx2;
  int len2 = 0, count = 0;
  
  for (cx=buf, cx2=buf2; *cx; cx++) {
    if (*cx == '\t') {
      count++;
      do {
	*cx2++ = ' ';
	len2++;
      } while ((len2 & 8) != 0);
    }
    else {
      *cx2++ = *cx;
      len2++;
    }
  }
  *cx2 = '\0';

  if (count) {
    strcpy(buf, buf2);
  }
}

static int bracket_count(char* buf)
{
  int count = 0;

  while (*buf != 0)
  {
    if ((*buf == '[') || (*buf == '('))
      count++;
    if ((*buf == ']') || (*buf == ')'))
      count--;
    buf++;
  }

  return count;
}

