#include "crc32.h"
#include <stdio.h>
#include <stdlib.h>

// ---------------------------- reverse --------------------------------

// Reverses (reflects) bits in a 32-bit word.
tChecksum reverse(tChecksum x) {
   x = ((x & 0x55555555) <<  1) | ((x >>  1) & 0x55555555);
   x = ((x & 0x33333333) <<  2) | ((x >>  2) & 0x33333333);
   x = ((x & 0x0F0F0F0F) <<  4) | ((x >>  4) & 0x0F0F0F0F);
   x = (x << 24) | ((x & 0xFF00) << 8) |
       ((x >> 8) & 0xFF00) | (x >> 24);
   return x;
}

// ----------------------------- crc32a --------------------------------

/* This is the basic CRC algorithm with no optimizations. It follows the
logic circuit as closely as possible. */

tChecksum crc32a(const unsigned char* const message) {
   int i, j;
   tChecksum byte, crc;

   i = 0;
   crc = 0xFFFFFFFF;
   while (message[i] != 0) {
      byte = message[i];            // Get next byte.
      byte = reverse(byte);         // 32-bit reversal.
      for (j = 0; j <= 7; j++) {    // Do eight times.
         if ((int)(crc ^ byte) < 0)
              crc = (crc << 1) ^ 0x04C11DB7;
         else crc = crc << 1;
         byte = byte << 1;          // Ready next msg bit.
      }
      i = i + 1;
   }
   return reverse(~crc);
}

// ----------------------------- crc32b --------------------------------

/* This is the basic CRC-32 calculation with some optimization but no
table lookup. The the byte reversal is avoided by shifting the crc reg
right instead of left and by using a reversed 32-bit word to represent
the polynomial.
   When compiled to Cyclops with GCC, this function executes in 8 + 72n
instructions, where n is the number of bytes in the input message. It
should be doable in 4 + 61n instructions.
   If the inner loop is strung out (approx. 5*8 = 40 instructions),
it would take about 6 + 46n instructions. */

tChecksum crc32b(const unsigned char* const message) {
   int i, j;
   tChecksum byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (message[i] != 0) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

// ----------------------------- crc32c --------------------------------

/* This is derived from crc32b but does table lookup. First the table
itself is calculated, if it has not yet been set up.
Not counting the table setup (which would probably be a separate
function), when compiled to Cyclops with GCC, this function executes in
7 + 13n instructions, where n is the number of bytes in the input
message. It should be doable in 4 + 9n instructions. In any case, two
of the 13 or 9 instrucions are load byte.
   This is Figure 14-7 in the text. */

tChecksum crc32c(const unsigned char* const message, const size_t size) {
    int i, j;
    tChecksum byte, crc, mask;
    static tChecksum table[256];

    /* Set up the table, if necessary. */

    if (table[1] == 0) {
        for (byte = 0; byte <= 255; byte++) {
            crc = byte;
            for (j = 7; j >= 0; j--) {    // Do eight times.
                mask = -(crc & 1);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
            table[byte] = crc;
        }
    }

    /* Through with table setup, now calculate the CRC. */

    i = 0;
    crc = 0xFFFFFFFF;
    while (i < size) {
        byte = message[i];
        crc = (crc >> 8) ^ table[(crc ^ byte) & 0xFF];
        i = i + 1;
    }
    return ~crc;
}

// ---------------------------- crc32cx --------------------------------

/* This is crc32b modified to load the message a fullword at a time.
It assumes the message is word aligned and consists of an integral
number of words before the 0-byte that marks the end of the message.
   This works only on a little-endian machine.
   Not counting the table setup (which would probably be a separate
function), this function should be doable in 3 + 22w instructions, where
w is the number of fullwords in the input message. This is equivalent to
3 + 5.5n instructions, where n is the number of bytes. 1.25 of those 5.5
instructions are loads.
   This is Exercise 1 in the text. C.f. Christopher Dannemiller,
who got it from Linux Source base,
www.gelato.unsw.edu.au/lxr/source/lib/crc32.c, lines 105-111. */

tChecksum crc32cx(const unsigned char* message) {
   int j;
   tChecksum byte, crc, mask, word;
   static tChecksum table[256];

   /* Set up the table, if necessary. */

   if (table[1] == 0) {
      for (byte = 0; byte <= 255; byte++) {
         crc = byte;
         for (j = 7; j >= 0; j--) {    // Do eight times.
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
         }
         table[byte] = crc;
      }
   }

   /* Through with table setup, now calculate the CRC. */

   crc = 0xFFFFFFFF;
   while (((word = *(tChecksum *)message) & 0xFF) != 0) {
      crc = crc ^ word;
      crc = (crc >> 8) ^ table[crc & 0xFF];
      crc = (crc >> 8) ^ table[crc & 0xFF];
      crc = (crc >> 8) ^ table[crc & 0xFF];
      crc = (crc >> 8) ^ table[crc & 0xFF];
      message = message + 4;
   }
   return ~crc;
}