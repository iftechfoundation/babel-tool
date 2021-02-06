/* babel_ifiction_functions.c babel top-level operations for ifiction
 * (c) 2006 By L. Ross Raszewski
 *
 * This code is freely usable for all purposes.
This program is released under the Perl Artistic License as specified below: 

 Preamble

The intent of this document is to state the conditions under which a
Package may be copied, such that the Copyright Holder maintains some
semblance of artistic control over the development of the package,
while giving the users of the package the right to use and distribute
the Package in a more-or-less customary fashion, plus the right to
make reasonable modifications.  Definitions

    "Package" refers to the collection of files distributed by the
    Copyright Holder, and derivatives of that collection of files
    created through textual modification.

    "Standard Version" refers to such a Package if it has not been
    modified, or has been modified in accordance with the wishes of
    the Copyright Holder as specified below.

    "Copyright Holder" is whoever is named in the copyright or
    copyrights for the package.

    "You" is you, if you're thinking about copying or distributing
    this Package.

    "Reasonable copying fee" is whatever you can justify on the basis
    of media cost, duplication charges, time of people involved, and
    so on. (You will not be required to justify it to the Copyright
    Holder, but only to the computing community at large as a market
    that must bear the fee.)

    "Freely Available" means that no fee is charged for the item
    itself, though there may be fees involved in handling the item. It
    also means that recipients of the item may redistribute it under
    the same conditions they received it.

   1. You may make and give away verbatim copies of the source form of
    the Standard Version of this Package without restriction,
    provided that you duplicate all of the original copyright
    notices and associated disclaimers.

   2. You may apply bug fixes, portability fixes and other
    modifications derived from the Public Domain or from the
    Copyright Holder. A Package modified in such a way shall still
    be considered the Standard Version.

   3. You may otherwise modify your copy of this Package in any way,
    provided that you insert a prominent notice in each changed file
    stating how and when you changed that file, and provided that
    you do at least ONE of the following:

         1. place your modifications in the Public Domain or otherwise
         make them Freely Available, such as by posting said
         modifications to Usenet or an equivalent medium, or placing
         the modifications on a major archive site such as
         uunet.uu.net, or by allowing the Copyright Holder to include
         your modifications in the Standard Version of the Package.
         2. use the modified Package only within your corporation or
         organization.  3. rename any non-standard executables so the
         names do not conflict with standard executables, which must
         also be provided, and provide a separate manual page for each
         non-standard executable that clearly documents how it differs
         from the Standard Version.
         4. make other distribution arrangements with the Copyright
         4. Holder.

   4. You may distribute the programs of this Package in object code
    or executable form, provided that you do at least ONE of the
    following:

         1. distribute a Standard Version of the executables and
         library files, together with instructions (in the manual page
         or equivalent) on where to get the Standard Version.
         2. accompany the distribution with the machine-readable
         source of the Package with your modifications.  3. give
         non-standard executables non-standard names, and clearly
         document the differences in manual pages (or equivalent),
         together with instructions on where to get the Standard
         Version.
         4. make other distribution arrangements with the Copyright
         4. Holder.

   5. You may charge a reasonable copying fee for any distribution of
    this Package. You may charge any fee you choose for support of
    this Package. You may not charge a fee for this Package
    itself. However, you may distribute this Package in aggregate
    with other (possibly commercial) programs as part of a larger
    (possibly commercial) software distribution provided that you do
    not advertise this Package as a product of your own. You may
    embed this Package's interpreter within an executable of yours
    (by linking); this shall be construed as a mere form of
    aggregation, provided that the complete Standard Version of the
    interpreter is so embedded.

   6. The scripts and library files supplied as input to or produced
    as output from the programs of this Package do not automatically
    fall under the copyright of this Package, but belong to whomever
    generated them, and may be sold commercially, and may be
    aggregated with this Package. If such scripts or library files
    are aggregated with this Package via the so-called "undump" or
    "unexec" methods of producing a binary executable image, then
    distribution of such an image shall neither be construed as a
    distribution of this Package nor shall it fall under the
    restrictions of Paragraphs 3 and 4, provided that you do not
    represent such an executable image as a Standard Version of this
    Package.

   7. C subroutines (or comparably compiled subroutines in other
    languages) supplied by you and linked into this Package in order
    to emulate subroutines and variables of the language defined by
    this Package shall not be considered part of this Package, but
    are the equivalent of input as in Paragraph 6, provided these
    subroutines do not change the language in any way that would
    cause it to fail the regression tests for the language.

   8. Aggregation of this Package with a commercial distribution is
    always permitted provided that the use of this Package is
    embedded; that is, when no overt attempt is made to make this
    Package's interfaces visible to the end user of the commercial
    distribution. Such use shall not be construed as a distribution
    of this Package.

   9. The name of the Copyright Holder may not be used to endorse or
    promote products derived from this software without specific
    prior written permission.

  10. THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
   WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.

The End



 *
 * This file depends upon babel.c (for rv), babel.h, misc.c and ifiction.c
 */

#include "babel.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifndef THREE_LETTER_EXTENSIONS
#define IFICTION_EXT ".iFiction"
#else
#define IFICTION_EXT ".ifi"
#endif

void *my_malloc(int, char *);

struct IFiction_Info
{
 char ifid[256];
 int wmode;
};

static void write_story_to_disk(struct XMLTag *xtg, void *ctx)
{
 char *b, *ep;
 char *begin, *end;
 char buffer[TREATY_MINIMUM_EXTENT];
 int32 l, j;
 if (ctx) { }

 if (strcmp(xtg->tag,"story")==0)
 {
 begin=xtg->begin;
 end=xtg->end;
 l=end-begin+1;
 b=(char *)my_malloc(l,"XML buffer");
 memcpy(b,begin,l-1);
 b[l]=0;
 j=ifiction_get_IFID(b,buffer,TREATY_MINIMUM_EXTENT);
 if (!j)
 {
  fprintf(stderr,"No IFID found for this story\n");
  free(b);
  return;
 }
 ep=strtok(buffer,",");
 while(ep)
 {
  char buf2[256];
  FILE *f;
  sprintf(buf2,"%s%s",ep,IFICTION_EXT);
  f=fopen(buf2,"w");

  if (!f ||
  fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!-- Metadata extracted by Babel -->"
        "<ifindex version=\"1.0\" xmlns=\"http://babel.ifarchive.org/protocol/iFiction/\">\n"
        " <story>",
        f)==EOF ||
  fputs(b,f)==EOF ||
  fputs("/<story>\n</ifindex>\n",f)==EOF
  )
  {
   fprintf(stderr,"Error writing to file %s\n",buf2);
  } else
   printf("Extracted %s\n",buf2);
  if (f) fclose(f);

  ep=strtok(NULL,",");
 }

 free(b);
 }
}

void babel_ifiction_ifid(char *md)
{
 char output[TREATY_MINIMUM_EXTENT];
 int i;
 char *ep;
 i=ifiction_get_IFID(md,output,TREATY_MINIMUM_EXTENT);
 if (!i)

 {
  fprintf(stderr,"Error: No IFIDs found in iFiction file\n");
  return;
 }
 ep=strtok(output,",");
 while(ep)
 {
  printf("IFID: %s\n",ep);
  ep=strtok(NULL,",");
 }

}

static char isok;

static void examine_tag(struct XMLTag *xtg, void *ctx)
{
 struct IFiction_Info *xti=(struct IFiction_Info *)ctx;

 if (strcmp("ifid",xtg->tag)==0 && strcmp(xti->ifid,"UNKNOWN")==0)
 {
 memcpy(xti->ifid,xtg->begin,xtg->end-xtg->begin);
 xti->ifid[xtg->end-xtg->begin]=0;
 }

}
static void verify_eh(char *e, void *ctx)
{
 if (*((int *)ctx) < 0) return;
 if (*((int *)ctx) || strncmp(e,"Warning",7))
  { isok=0;
   fprintf(stderr, "%s\n",e);
  }
}



void babel_ifiction_fish(char *md)
{
 int i=-1;
 ifiction_parse(md,write_story_to_disk,NULL,verify_eh,&i);
}

void deep_ifiction_verify(char *md, int f)
{
 struct IFiction_Info ii;
 int i=0;
 ii.wmode=0;
 isok=1;
 strcpy(ii.ifid,"UNKNOWN");
 ifiction_parse(md,examine_tag,&ii,verify_eh,&i);
 if (f&& isok) printf("Verified %s\n",ii.ifid);
}
void babel_ifiction_verify(char *md)
{
 deep_ifiction_verify(md,1);

}


void babel_ifiction_lint(char *md)
{
 struct IFiction_Info ii;
 int i=1;
 ii.wmode=1;
 isok=1;
 strcpy(ii.ifid,"UNKNOWN");
 ifiction_parse(md,examine_tag,&ii,verify_eh,&i);
 if (isok) printf("%s conforms to iFiction style guidelines\n",ii.ifid);
}


