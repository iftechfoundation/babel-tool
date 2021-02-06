/* babel.c   The babel command line program
 * (c) 2006 By L. Ross Raszewski
 *
 * This code is freely usable for all purposes.
 *
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
 * This file depends upon misc.c and babel.h
 *
 * This file exports one variable: char *rv, which points to the file name
 * for an ifiction file.  This is used only by babel_ifiction_verify
 */

#include "babel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
int chdir(const char *);
char *getcwd(char *, int);
#ifdef __cplusplus
}
#endif

char *fn;

/* checked malloc function */
void *my_malloc(int, char *);

/* babel performs several fundamental operations, which are specified
   by command-line objects. Each of these functions corresponds to
   a story function (defined in babel_story_functions.c) or an
   ifiction function (defined in babel_ifiction_functions.c) or both.
   These are the types of those functions.
*/

typedef void (*story_function)(void);
typedef void (*ifiction_function)(char *);
typedef void (*multi_function)(char **, char *, int);
/* This structure tells babel what to do with its command line arguments.
   if either of story or ifiction are NULL, babel considers this command line
   option inappropriate for that type of file.
*/
struct function_handler {
        char *function;         /* the textual command line option */
        story_function story;   /* handler for story files */
        ifiction_function ifiction; /* handler for ifiction files */
        char *desc;             /* Textual description for help text */
        };

struct multi_handler {
        char *function;
        char *argnames;
        multi_function handler;
        int nargsm;
        int nargsx;
        char *desc;
        };
/* This is an array of function_handler objects which specify the legal
   arguments.  It is terminated by a function_handler with a NULL function
 */
static struct function_handler functions[] = {
        { "-ifid", babel_story_ifid, babel_ifiction_ifid, "Deduce IFID"},
        { "-format", babel_story_format, NULL, "Deduce story format" },
        { "-ifiction", babel_story_ifiction, NULL, "Extract iFiction file" },
        { "-meta", babel_story_meta, NULL, "Print story metadata" },
        { "-identify", babel_story_identify, NULL, "Describe story file" },
        { "-cover", babel_story_cover, NULL, "Extract cover art" },
        { "-story", babel_story_story, NULL, "Extract story file (ie. from a blorb)" },
        { "-verify", NULL, babel_ifiction_verify, "Verify integrity of iFiction file" },
        { "-lint", NULL, babel_ifiction_lint, "Verify style of iFiction file" },
        { "-fish", babel_story_fish, babel_ifiction_fish, "Extract all iFiction and cover art"},
        { "-unblorb", babel_story_unblorb, NULL, "As -fish, but also extract story files"},
        { NULL, NULL, NULL }
        };
static struct multi_handler multifuncs[] = {
        { "-blorb", "<storyfile> <ifictionfile> [<cover art>]", babel_multi_blorb, 2, 3, "Bundle story file and (sparse) iFiction into blorb" },
        { "-blorbs", "<storyfile> <ifictionfile> [<cover art>]", babel_multi_blorb1, 2, 3, "Bundle story file and (sparse) iFiction into sensibly-named blorb" },
        { "-complete", "<storyfile> <ifictionfile>", babel_multi_complete, 2, 2, "Create complete iFiction file from sparse iFiction" },
        { NULL, NULL, NULL, 0, 0, NULL }
};

int main(int argc, char **argv)
{
 char *todir=".";
 char cwd[512];
 int ok=1,i, l, ll;
 FILE *f;
 char *md=NULL;
 /* Set the input filename.  Note that if this is invalid, babel should
   abort before anyone notices
 */
 fn=argv[2];

 if (argc < 3) ok=0;
 /* Detect the presence of the "-to <directory>" argument.
  */
 if (ok && argc >=5 && strcmp(argv[argc-2], "-to")==0)
 {
  todir=argv[argc-1];
  argc-=2;
 }
 if (ok) for(i=0;multifuncs[i].function;i++)
          if (strcmp(argv[1],multifuncs[i].function)==0 &&
              argc>= multifuncs[i].nargsm+2 &&
              argc <= multifuncs[i].nargsx+2)
          {

   multifuncs[i].handler(argv+2, todir, argc-2);
   exit(0);
          }

 if (argc!=3) ok=0;

 /* Find the apropriate function_handler */
 if (ok) {
 for(i=0;functions[i].function && strcmp(functions[i].function,argv[1]);i++);
 if (!functions[i].function) ok=0;
 else  if (strcmp(fn,"-")) {
   f=fopen(argv[2],"r");
   if (!f) ok=0;
  }
 }

 /* Print usage error if anything has gone wrong */
 if (!ok)
 {
  printf("%s: Treaty of Babel Analysis Tool (%s, %s)\n"
         "Usage:\n", argv[0],BABEL_VERSION, TREATY_COMPLIANCE);
  for(i=0;functions[i].function;i++)
  {
   if (functions[i].story)
    printf(" babel %s <storyfile>\n",functions[i].function);
   if (functions[i].ifiction)
    printf(" babel %s <ifictionfile>\n",functions[i].function);
   printf("   %s\n",functions[i].desc);
  }
  for(i=0;multifuncs[i].function;i++)
  {
   printf("babel %s %s\n   %s\n",
           multifuncs[i].function,
           multifuncs[i].argnames,
           multifuncs[i].desc);
  }

  printf ("\nFor functions which extract files, add \"-to <directory>\" to the command\n"
          "to set the output directory.\n"
          "The input file can be specified as \"-\" to read from standard input\n"
          "(This may only work for .iFiction files)\n");
  return 1;
 }

 /* For story files, we end up reading the file in twice.  This
    is unfortunate, but unavoidable, since we want to be all
    cross-platformy, so the first time we read it in, we
    do the read in text mode, and the second time, we do it in binary
    mode, and there are platforms where this makes a difference.
 */
 ll=0;
 if (strcmp(fn,"-"))
 {
  fseek(f,0,SEEK_END);
  l=ftell(f)+1;
  fseek(f,0,SEEK_SET);
  md=(char *)my_malloc(l,"Input file buffer");
  fread(md,1,l-1,f);
  md[l-1]=0;
 }
 else
  while(!feof(stdin))
  {
   char *tt, mdb[1024];
   int ii;
   ii=fread(mdb,1,1024,stdin);
   tt=(char *)my_malloc(ll+ii,"file buffer");
   if (md) { memcpy(tt,md,ll); free(md); }
   memcpy(tt+ll,mdb,ii);
   md=tt;
   ll+=ii;
   if (ii<1024) break;
  }


  if (strstr(md,"<?xml version=") && strstr(md,"<ifindex"))
  { /* appears to be an ifiction file */
   char *pp;
   pp=strstr(md,"</ifindex>");
   if (pp) *(pp+10)=0;
   getcwd(cwd,512);
   chdir(todir);
   l=0;
   if (functions[i].ifiction)
    functions[i].ifiction(md);
   else
    fprintf(stderr,"Error: option %s is not valid for iFiction files\n",
             argv[1]);
   chdir(cwd);
 }

 if (strcmp(fn,"-"))
 {
 free(md);
 fclose(f);
 }
 if (l)
 { /* Appears to be a story */
   char *lt;
   if (functions[i].story)
   {
    if (strcmp(fn,"-")) lt=babel_init(argv[2]);
    else { lt=babel_init_raw(md,ll);
           free(md);
         }

    if (lt)
    {
     getcwd(cwd,512);
     chdir(todir);
     if (!babel_get_authoritative() && strcmp(argv[1],"-format"))
      printf("Warning: Story format could not be positively identified. Guessing %s\n",lt);
     functions[i].story();

     chdir(cwd);
    }
    else if (strcmp(argv[1],"-ifid")==0) /* IFID is calculable for all files */
    {
     babel_md5_ifid(cwd,512);
     printf("IFID: %s\n",cwd);
    }
    else
     fprintf(stderr,"Error: Did not recognize format of story file\n");
    babel_release();
   }
   else
    fprintf(stderr,"Error: option %s is not valid for story files\n",
             argv[1]);
  }    

 return 0;
}
