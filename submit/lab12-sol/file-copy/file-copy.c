#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** wrapper around fgetc() to force error */
static int
fgetc_err(FILE *stream)
{
  static int is_ok = 1;
  int c = (is_ok) ? fgetc(stream) : EOF;
  is_ok = 0;
  errno = EBADF;
  return c;
}

static int
do_copy(const char *inName, FILE *in, const char *outName, FILE *out)
{
  int c;

  while ((c = fgetc_err(in)) != EOF) {
    if (fputc(c, out) == EOF) {
      fprintf(stderr, "cannot write %s: %s\n",
              outName, strerror(errno));
      return 1;
    }
  }

  fprintf(stderr, "i/o error reading from %s: %s\n",
          inName, strerror(errno));
  return 1;
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
