#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
do_copy(const char *inName, FILE *in, const char *outName, FILE *out)
{
  int c;

  while ((c = fgetc(in)) != EOF) {
    if (fputc(c, out) == EOF) {
      fprintf(stderr, "error writing to %s: %s\n",
              outName, strerror(errno));
      return 1;
    }
  }

  if (ferror(in)) {
    fprintf(stderr, "i/o error reading from %s: %s\n",
            inName, strerror(errno));
    return 1;
  }

  return 0;
}

int
main(int argc, const char *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "usage: %s SRC_NAME DEST_NAME\n", argv[0]);
    exit(1);
  }

  const char *srcName = argv[1];
  const char *destName = argv[2];

  FILE *in = fopen(srcName, "r");
  if (!in) {
    fprintf(stderr, "cannot read %s: %s\n", srcName, strerror(errno));
    exit(1);
  }

  FILE *out = fopen(destName, "w");
  if (!out) {
    fprintf(stderr, "cannot write %s: %s\n", destName, strerror(errno));
    fclose(in);
    exit(1);
  }

  int status = do_copy(srcName, in, destName, out);

  fclose(in);
  fclose(out);

  return status;
}
