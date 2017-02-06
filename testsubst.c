#include <stdio.h>
#include "hash.h"
#include "read.h"
#include "subst.h"

int main(int argc, char *argv[])
{
  int ix;
  pfile_t *plan;
  char *body;

  if (argc != 2) {
    printf("Usage: %s subst.htmls\n", argv[0]);
    return -1;
  }

  plan = read_pfile(argv[1]);
  if (!plan) {
    printf("Cannot read %s.\n", argv[1]);
    return -1;
  }

  body = pfile_getbody(plan);
  substitute(body, plan->tab, NULL, stdout);

  return 0;
}

void show_warning2(char *msg, char *msg2)
{
  if (msg2)
    printf("Problem: %s: %s\n", msg, msg2);
  else
    printf("Problem: %s\n", msg);
}
