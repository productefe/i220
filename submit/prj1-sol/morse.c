#include "morse.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

typedef struct {
  char c;
  const char *code;
} TextMorse;

//<https://en.wikipedia.org/wiki/Morse_code#/media/File:International_Morse_Code.svg>
static const TextMorse char_codes[] = {
  { 'A', ".-" },
  { 'B', "-..." },
  { 'C', "-.-." },
  { 'D', "-.." },
  { 'E', "." },
  { 'F', "..-." },
  { 'G', "--." },
  { 'H', "...." },
  { 'I', ".." },
  { 'J', ".---" },
  { 'K', "-.-" },
  { 'L', ".-.." },
  { 'M', "--" },
  { 'N', "-." },
  { 'O', "---" },
  { 'P', ".--." },
  { 'Q', "--.-" },
  { 'R', ".-." },
  { 'S', "..." },
  { 'T', "-" },
  { 'U', "..-" },
  { 'V', "...-" },
  { 'W', ".--" },
  { 'X', "-..-" },
  { 'Y', "-.--" },
  { 'Z', "--.." },

  { '1', ".----" },
  { '2', "..---" },
  { '3', "...--" },
  { '4', "....-" },
  { '5', "....." },
  { '6', "-...." },
  { '7', "--..." },
  { '8', "---.." },
  { '9', "----." },
  { '0', "-----" },

  { '\0', ".-.-." }, //AR Prosign indicating End-of-message
                     //<https://en.wikipedia.org/wiki/Prosigns_for_Morse_code>
};


/** Return NUL-terminated morse code string (like "..--") for char
*  c. Returns NULL if there is no code for c.
*/
static const char *
char_to_morse(Byte c) {
  for (unsigned i = 0; i < sizeof(char_codes)/sizeof(char_codes[0]); i++) {
    if (char_codes[i].c == c) return char_codes[i].code;
  }
  return NULL;
}


/** Given a NUL-terminated morse code string (like "..--") for a
    single char, return the corresponding char. Returns < 0 if code is
    invalid.
*/
static int
morse_to_char(const char *code) {
  for (unsigned i = 0; i < sizeof(char_codes)/sizeof(char_codes[0]); i++) {
    if (strcmp(char_codes[i].code, code) == 0) return char_codes[i].c;
  }
  return -1;
}

#include "inlines.h"

/** Given an array of Bytes, a bit index is the offset of a bit
*  in the array (with MSB having offset 0).
*
*  Given a bytes[] array and some bitOffset, and assuming that
*  BITS_PER_BYTE is 8, then (bitOffset >> 3) represents the index of
*  the byte within bytes[] and (bitOffset & 0x7) gives the bit-index
*  within the byte (MSB represented by bit-index 0) and .
*
*  For example, given array a[] = {0xB1, 0xC7} which is
*  { 0b1011_0001, 0b1100_0111 } we have the following:
*
*     Bit-Offset   Value
*        0           1
*        1           0
*        2           1
*        3           1
*        4           0
*        5           0
*        6           0
*        7           1
*        8           1
*        9           1
*       10           0
*       11           0
*       12           0
*       13           1
*       14           1
*       15           1
*
*/


/** Return a mask for a Byte with the single bit at bitIndex set to 1,
*  all other bits set to 0.  Note that bitIndex == 0 represents the
*  MSB, bitIndex == 1 represents the next significant bit and so on.
*
*  For example, if bitIndex == 0, then it should return 0x80 if
*  BITS_PER_BYTE == 8 but 0x8000 if BITS_PER_BYTE == 16.  If bitIndex
*  == 2, then it should return 0x20 if BITS_PER_BYTE == 8 but 0x2000
*  if BITS_PER_BYTE == 16. Note that the code should work for any
*  value of BITS_PER_BYTE.
*/
static inline unsigned
byte_bit_mask(unsigned bitIndex)
{
  return 1 << (BITS_PER_BYTE - 1 - bitIndex);
}

/** Given a power-of-2 powerOf2, return log2(powerOf2) */
static inline unsigned
get_log2_power_of_2(unsigned powerOf2)
{
  unsigned count = 0;
  unsigned value = 1;
  
  while (value != powerOf2) {
    value = value << 1;
    count++;
  }
  
  return count;
}

/** Given a bitOffset return the bitIndex part of the bitOffset. */
static inline unsigned
get_bit_index(unsigned bitOffset)
{
  return bitOffset & (BITS_PER_BYTE - 1);
}

/** Given a bitOffset return the byte offset part of the bitOffset */
static inline unsigned
get_byte_offset(unsigned bitOffset)
{
  unsigned log2 = get_log2_power_of_2(BITS_PER_BYTE);
  return bitOffset >> log2;
}

/** Return bit at offset bitOffset in array[]; i.e., return
*  (bits(array))[bitOffset]
*/
static inline unsigned
get_bit_at_offset(const Byte array[], unsigned bitOffset)
{
  unsigned byteOffset = get_byte_offset(bitOffset);
  unsigned bitIndex = get_bit_index(bitOffset);
  unsigned mask = byte_bit_mask(bitIndex);
  
  return (array[byteOffset] & mask) != 0;
}

/** Set bit selected by bitOffset in array to bit. */
static inline void
set_bit_at_offset(Byte array[], unsigned bitOffset, unsigned bit)
{
  unsigned byteOffset = get_byte_offset(bitOffset);
  unsigned bitIndex = get_bit_index(bitOffset);
  unsigned mask = byte_bit_mask(bitIndex);
  
  if (bit) {
    array[byteOffset] = array[byteOffset] | mask;
  } else {
    array[byteOffset] = array[byteOffset] & ~mask;
  }
}

/** Set count bits in array[] starting at bitOffset to bit.  Return
*  bit-offset one beyond last bit set.
*/
static inline unsigned
set_bits_at_offset(Byte array[], unsigned bitOffset,
                   unsigned bit, unsigned count)
{
  for (unsigned i = 0; i < count; i++) {
    set_bit_at_offset(array, bitOffset + i, bit);
  }
  return bitOffset + count;
}

/** Return count of run of identical bits starting at bitOffset
*  in bytes[nBytes].
*  Returns 0 when bitOffset outside bytes[nBytes].
*/
static inline unsigned
run_length(const Byte bytes[], unsigned nBytes, unsigned bitOffset)
{
  unsigned maxBitOffset = nBytes * BITS_PER_BYTE;
  
  if (bitOffset >= maxBitOffset) {
    return 0;
  }
  
  unsigned initialBit = get_bit_at_offset(bytes, bitOffset);
  unsigned count = 1;
  unsigned currentOffset = bitOffset + 1;
  
  while (currentOffset < maxBitOffset) {
    unsigned currentBit = get_bit_at_offset(bytes, currentOffset);
    if (currentBit != initialBit) {
      break;
    }
    count++;
    currentOffset++;
  }
  
  return count;
}



/** Convert text[nText] into a binary encoding of morse code in
*  morse[].  It is assumed that array morse[] is initially all zero
*  and is large enough to represent the morse code for all characters
*  in text[].  The result in morse[] should be terminated by the
*  morse prosign AR.  Any sequence of non-alphanumeric characters in
*  text[] should be treated as a *single* inter-word space.  Leading
*  non alphanumeric characters in text are ignored.
*
*  Returns count of number of bytes used within morse[].
*/
int
text_to_morse(const Byte text[], unsigned nText, Byte morse[])
{
  unsigned textIndex = 0;
  unsigned morseBitOffset = 0;
  
  while (textIndex < nText && !isalnum(text[textIndex])) {
    textIndex++;
  }
  
  int done = 0;
  while (!done) {
    Byte c = (textIndex < nText) ? toupper(text[textIndex]) : '\0';
    
    if (textIndex >= nText || c == '\0') {
      c = '\0';
      done = 1;
    }
    
    const char *code = char_to_morse(c);
    
    if (code != NULL) {
      unsigned codeIndex = 0;
      while (code[codeIndex] != '\0') {
        char symbol = code[codeIndex];
        
        if (symbol == '.') {
          morseBitOffset = set_bits_at_offset(morse, morseBitOffset, 1, 1);
          morseBitOffset = set_bits_at_offset(morse, morseBitOffset, 0, 1);
        } else if (symbol == '-') {
          morseBitOffset = set_bits_at_offset(morse, morseBitOffset, 1, 3);
          morseBitOffset = set_bits_at_offset(morse, morseBitOffset, 0, 1);
        }
        
        codeIndex++;
      }
      
      morseBitOffset = set_bits_at_offset(morse, morseBitOffset, 0, 2);
      
      textIndex++;
    } else {
      morseBitOffset = set_bits_at_offset(morse, morseBitOffset, 0, 4);
      
      textIndex++;
      while (textIndex < nText && !isalnum(text[textIndex])) {
        textIndex++;
      }
    }
  }
  
  unsigned nBytesUsed = get_byte_offset(morseBitOffset);
  if (get_bit_index(morseBitOffset) != 0) {
    nBytesUsed++;
  }
  
  return nBytesUsed;
}


/** Convert AR-prosign terminated binary Morse encoding in
*  morse[nMorse] into text in text[].  It is assumed that array
*  text[] is large enough to represent the decoding of the code in
*  morse[].  Leading zero bits in morse[] are ignored.  Encodings
*  representing word separators are output as a space ' ' character.
*
*  Returns count of number of bytes used within text[], < 0 on error.
*/
int
morse_to_text(const Byte morse[], unsigned nMorse, Byte text[])
{
  #define MAX_MORSE_LEN 6
  char code[MAX_MORSE_LEN + 1];
  unsigned codeIndex = 0;
  
  unsigned textIndex = 0;
  unsigned morseBitOffset = 0;
  unsigned maxBitOffset = nMorse * BITS_PER_BYTE;
  
  int endOfMessage = 0;
  
  while (morseBitOffset < maxBitOffset && 
         get_bit_at_offset(morse, morseBitOffset) == 0) {
    morseBitOffset++;
  }
  
  while (morseBitOffset < maxBitOffset && !endOfMessage) {
    unsigned onesCount = run_length(morse, nMorse, morseBitOffset);
    
    if (onesCount == 0) {
      break;
    }
    
    if (onesCount == 1) {
      code[codeIndex++] = '.';
    } else if (onesCount == 3) {
      code[codeIndex++] = '-';
    } else {
      return -1;
    }
    
    morseBitOffset += onesCount;
    
    if (codeIndex > MAX_MORSE_LEN) {
      return -1;
    }
    
    unsigned zerosCount = run_length(morse, nMorse, morseBitOffset);
    morseBitOffset += zerosCount;
    
    if (zerosCount == 1) {
      continue;
    } else if (zerosCount == 3) {
      code[codeIndex] = '\0';
      int ch = morse_to_char(code);
      
      if (ch == '\0') {
        endOfMessage = 1;
      } else if (ch < 0) {
        return -1;
      } else {
        text[textIndex++] = (Byte)ch;
      }
      
      codeIndex = 0;
    } else if (zerosCount >= 7) {
      if (codeIndex > 0) {
        code[codeIndex] = '\0';
        int ch = morse_to_char(code);
        
        if (ch < 0) {
          return -1;
        }
        text[textIndex++] = (Byte)ch;
        codeIndex = 0;
      }
      
      int hasMoreData = 0;
      for (unsigned i = morseBitOffset; i < maxBitOffset; i++) {
        if (get_bit_at_offset(morse, i)) {
          hasMoreData = 1;
          break;
        }
      }
      if (hasMoreData) {
        text[textIndex++] = ' ';
      }
    } else {
      if (zerosCount == 0) {
        if (codeIndex > 0) {
          code[codeIndex] = '\0';
          int ch = morse_to_char(code);
          
          if (ch == '\0') {
            endOfMessage = 1;
          } else if (ch < 0) {
            return -1;
          } else {
            text[textIndex++] = (Byte)ch;
          }
          codeIndex = 0;
        }
        break;
      }
      
    }
  }
  
  text[textIndex] = '\0';
  
  return textIndex;
}
