#include "bcd.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>

enum {
  BCD_MASK = (1 << BCD_BITS) - 1,
};

static unsigned
get_bcd_digit(Bcd bcd, int pos)
{
  return (bcd >> (pos * BCD_BITS)) & BCD_MASK;
}

static void
set_bcd_digit(Bcd *bcd, int pos, unsigned digit)
{
  *bcd |= ((Bcd)digit << (pos * BCD_BITS));
}

static int
is_valid_bcd(Bcd bcd)
{
  for (int i = 0; i < MAX_BCD_DIGITS; i++) {
    if (get_bcd_digit(bcd, i) > 9) return 0;
  }
  return 1;
}

static BcdError
shift_left_digits(Bcd n, int shift_digits, Bcd *shifted)
{
  if (shift_digits < 0) return OVERFLOW_ERR;

  if (shift_digits >= MAX_BCD_DIGITS) {
    if (n == 0) {
      *shifted = 0;
      return NO_ERR;
    }
    return OVERFLOW_ERR;
  }

  Bcd result = 0;

  for (int i = 0; i < MAX_BCD_DIGITS; i++) {
    unsigned digit = get_bcd_digit(n, i);
    if (digit > 9) return BAD_VALUE_ERR;
    if (digit == 0) continue;
    if (i + shift_digits >= MAX_BCD_DIGITS) return OVERFLOW_ERR;
    set_bcd_digit(&result, i + shift_digits, digit);
  }

  *shifted = result;
  return NO_ERR;
}

static BcdError
bcd_multiply_digit(Bcd n, unsigned bcd_digit, Bcd *bcd)
{
  if (!is_valid_bcd(n) || bcd_digit > 9) return BAD_VALUE_ERR;

  Bcd result = 0;
  unsigned carry = 0;

  for (int i = 0; i < MAX_BCD_DIGITS; i++) {
    unsigned digit = get_bcd_digit(n, i);
    unsigned prod = digit * bcd_digit + carry;
    unsigned out_digit = prod % 10;
    carry = prod / 10;
    set_bcd_digit(&result, i, out_digit);
  }

  if (carry != 0) return OVERFLOW_ERR;

  *bcd = result;
  return NO_ERR;
}

BcdError
binary_to_bcd(Binary value, Bcd *bcd)
{
  Bcd result = 0;
  int pos = 0;

  do {
    if (pos >= MAX_BCD_DIGITS) return OVERFLOW_ERR;
    unsigned digit = value % 10;
    set_bcd_digit(&result, pos, digit);
    value /= 10;
    pos++;
  } while (value != 0);

  *bcd = result;
  return NO_ERR;
}

BcdError
bcd_to_binary(Bcd bcd, Binary *binary)
{
  Binary result = 0;

  for (int i = MAX_BCD_DIGITS - 1; i >= 0; i--) {
    unsigned digit = get_bcd_digit(bcd, i);
    if (digit > 9) return BAD_VALUE_ERR;
    result = result * 10 + digit;
  }

  *binary = result;
  return NO_ERR;
}

BcdError
str_to_bcd(const char *s, const char **p, Bcd *bcd)
{
  Bcd result = 0;
  int digits = 0;
  const char *q = s;

  while (isdigit((unsigned char)*q)) {
    if (digits >= MAX_BCD_DIGITS) return OVERFLOW_ERR;
    result = (result << BCD_BITS) | (unsigned)(*q - '0');
    q++;
    digits++;
  }

  if (p != NULL) *p = q;
  *bcd = result;
  return NO_ERR;
}

BcdError
bcd_to_str(Bcd bcd, char buf[], size_t buf_size, int *len)
{
  for (int i = 0; i < MAX_BCD_DIGITS; i++) {
    if (get_bcd_digit(bcd, i) > 9) return BAD_VALUE_ERR;
  }

  int first = MAX_BCD_DIGITS - 1;
  while (first > 0 && get_bcd_digit(bcd, first) == 0) {
    first--;
  }

  int needed = first + 1;
  if (len != NULL) *len = needed;

  if (buf_size < (size_t)needed + 1) return OVERFLOW_ERR;

  for (int i = first; i >= 0; i--) {
    buf[first - i] = '0' + get_bcd_digit(bcd, i);
  }
  buf[needed] = '\0';

  return NO_ERR;
}

BcdError
bcd_add(Bcd n, Bcd m, Bcd *sum)
{
  Bcd result = 0;
  unsigned carry = 0;

  for (int i = 0; i < MAX_BCD_DIGITS; i++) {
    unsigned dn = get_bcd_digit(n, i);
    unsigned dm = get_bcd_digit(m, i);

    if (dn > 9 || dm > 9) return BAD_VALUE_ERR;

    unsigned s = dn + dm + carry;
    unsigned out_digit = s % 10;
    carry = s / 10;
    set_bcd_digit(&result, i, out_digit);
  }

  if (carry != 0) return OVERFLOW_ERR;

  *sum = result;
  return NO_ERR;
}

BcdError
bcd_multiply(Bcd n, Bcd m, Bcd *prod)
{
  if (!is_valid_bcd(n) || !is_valid_bcd(m)) return BAD_VALUE_ERR;

  Bcd result = 0;

  for (int i = 0; i < MAX_BCD_DIGITS; i++) {
    unsigned mdigit = get_bcd_digit(m, i);
    Bcd partial;
    Bcd shifted;
    Bcd temp_sum;

    BcdError err = bcd_multiply_digit(n, mdigit, &partial);
    if (err != NO_ERR) return err;

    err = shift_left_digits(partial, i, &shifted);
    if (err != NO_ERR) return err;

    err = bcd_add(result, shifted, &temp_sum);
    if (err != NO_ERR) return err;

    result = temp_sum;
  }

  *prod = result;
  return NO_ERR;
}
