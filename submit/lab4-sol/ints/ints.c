#include <assert.h>
#include <stdio.h>

int
main() {
  assert(sizeof(short) == 2);
  assert(sizeof(int) == 4);
  assert(sizeof(long) == 8);

  const unsigned short vals[] = {
    0x1234, 0x5678, 0x9abc, 0xdef0, 0x4321, 0x8765, 0xcba9, 0x0fed,
  };

  const size_t n_vals = sizeof(vals)/sizeof(vals[0]);

  const void *base = vals;
  const void *end = &vals[n_vals];

  const unsigned short *shorts = base;
  const unsigned short *shorts_end = end;

  for (const unsigned short *p = shorts; p < shorts_end; p++) {
    printf("shorts[%td] = 0x%04x\n", p - shorts, *p);
  }
  printf("\n");

  const unsigned char *chars = base;
  const unsigned char *chars_end = end;

  for (const unsigned char *p = chars + 1; p < chars_end; p += 3) {
    printf("chars[%td] = 0x%02x\n", p - chars, *p);
  }
  printf("\n");

  const unsigned int *ints = base;
  const unsigned int *ints_end = end;

  for (const unsigned int *p = ints; p < ints_end; p += 2) {
    printf("ints[%td] = 0x%08x\n", p - ints, *p);
  }
  printf("\n");

  const unsigned long *longs = base;
  const unsigned long *longs_end = end;

  for (const unsigned long *p = longs; p < longs_end; p++) {
    printf("longs[%td] = 0x%016lx\n", p - longs, *p);
  }
  printf("\n");
}
