/* alan.c  Treaty of Babel module for ALAN files
 * 2006 By L. Ross Raszewski
 *
 * This file depends on treaty_builder.h
 *
 * This file is public domain, but note that any changes to this file
 * may render it noncompliant with the Treaty of Babel
 */

#define FORMAT alan
#define HOME_PAGE "http://www.alanif.se/"
#define FORMAT_EXT ".acd,.a3c"
#define NO_METADATA
#define NO_COVER

#include "treaty_builder.h"
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>


typedef unsigned char byte;

static int32 read_alan_int_at(unsigned char *from)
{
  return ((unsigned long int) from[3])| ((unsigned long int)from[2] << 8) |
    ((unsigned long int) from[1]<<16)| ((unsigned long int)from[0] << 24);
}

static int32 get_story_file_IFID(void *story_file, int32 extent, char *output, int32 output_extent)
{
  if (story_file || extent)
    { }
  ASSERT_OUTPUT_SIZE(6);
  strcpy(output,"ALAN-");
  return INCOMPLETE_REPLY_RV;
}


static bool magic_word_found(byte *sf) {
  return memcmp(sf,"ALAN",4) == 0;
}


static bool correct_crc(byte *sf, int32 size_in_awords) {
  /* Size of AcodeHeader is 50 Awords = 200 bytes */
  int32 crc = 0;

  for (int i=50*4;i<(size_in_awords*4);i++)
    crc+=sf[i];

  return (crc == read_alan_int_at(sf+46*4));
}


/*
  The claim algorithm for Alan files is:
  * For Alan 3, check for the magic word
  * load the file length in blocks
  * check that the file length is correct
  * For alan 2, each word between byte address 24 and 81 is a
  word address within the file, so check that they're all within
  the file
  * Locate the checksum and verify that it is correct
  */
static int32 claim_story_file(void *story_file, int32 extent_in_bytes)
{
  byte *sf = (byte *) story_file;
  int32 size_in_awords, i;

  if (extent_in_bytes < 160)
    /* File is shorter than any Alan file header */
    return INVALID_STORY_FILE_RV;

  if (!magic_word_found(sf))
    { /* Identify Alan 2.x */
      int32 crc = 0;

      size_in_awords=read_alan_int_at(sf+4);
      if (size_in_awords > extent_in_bytes/4) return INVALID_STORY_FILE_RV;
      for (i=24;i<81;i+=4)
        if (read_alan_int_at(sf+i) > extent_in_bytes/4) return INVALID_STORY_FILE_RV;
      for (i=160;i<(size_in_awords*4);i++)
        crc+=sf[i];
      if (crc!=read_alan_int_at(sf+152)) return INVALID_STORY_FILE_RV;
      return VALID_STORY_FILE_RV;
    }
  else
    { /* Identify Alan 3 */
      size_in_awords=read_alan_int_at(sf+3*4); /* hdr.size @ 3 */

      if (!correct_crc(sf, size_in_awords))
        return INVALID_STORY_FILE_RV;

      return VALID_STORY_FILE_RV;
    }
}
