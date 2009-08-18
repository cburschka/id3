/* id3 - an ID3 tag editor
 * Copyright (c) 1998,1999,2000 Robert Woodcock <rcw@debian.org>
 * This code is hereby licensed for public consumption under either the
 * GNU GPL v2 or greater, or Larry Wall's Artistic license - your choice.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *                                 
 * genre.h is (c) 1998 Robert Alto <badcrc@tscnet.com> and licensed only
 * under the GPL.
 */

const char version[]="0.15";

const char usage[]= "usage: id3 -[tTaAycg] `text' file1 [file2...]\n\
       id3 -d file1 [file2...]\n\
       id3 -l file1 [file2...]\n\
       id3 -L\n\
       id3 -v\n\
 -t   Modifies a Title tag\n\
 -T   Modifies a Track tag\n\
 -a   Modifies an Artist tag\n\
 -A   Modifies an Album tag\n\
 -y   Modifies a Year tag\n\
 -c   Modifies a Comment tag\n\
 -g   Modifies a Genre tag\n\
 -l   Lists an ID3 tag\n\
 -L   Lists all genres\n\
 -R   Uses an rfc822-style format for output\n\
 -d   Deletes an ID3 tag\n\
 -h   Displays this help info\n\
 -v   Prints version info\n";

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "genre.h"


struct id3 {
	char tag[3];
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	/* With ID3 v1.0, the comment is 30 chars long */
	/* With ID3 v1.1, if comment[28] == 0 then comment[29] == tracknum */
	char comment[30];
	unsigned char genre;
};

int main (int argc, char *argv[])
{
	int deletetag=0, listtag=0, rfc822style=0, hastag=0, notdir=0;
	int v11tag=0, i, r, exitcode=0;
	int matches, lettersmatched; /* genre lazy matching */
	int newtitle=0, newartist=0, newalbum=0;
	int newyear=0, newcomment=0, newgenre=0;
	int newtrack=0;
	struct id3 oldid3, newid3;
	struct stat finfo;
	FILE *fp;
	
	memset(&newid3, 0, 127);
	newid3.genre=255;
	
	if (argc < 2) {
		fprintf(stderr, "%s", usage);
		exit(1);
	}
	
	while (1) {
		r = getopt(argc, argv, "dhlLRvt:T:a:A:y:c:g:");
		if (r == -1) break;
		switch (r) {
		case 'd': /* delete a tag */
			deletetag=1;
			break;
		case 'l': /* list a tag */
			listtag=1;
			break;
		case 'L': /* list all genres */
		        for (i=0; i<genre_count; i++)
		        	printf("%3d: %s\n", i, genre_table[i]);
		        exit(0);
		        break;
		case 'R': /* list tags in rfc822-style format */
		        rfc822style=1;
		        break;
		case 't': /* Title */
			memcpy(newid3.title, optarg, 30);
			for (i=strlen(optarg);i<30;i++) newid3.title[i]=0;
			newtitle=1;
			break;
		case 'T': /* Track */
			if (isdigit(optarg[0])) {
				newid3.comment[28] = 0;
				newid3.comment[29] = (unsigned char)atoi(optarg);
				newtrack=1;
			} else {
				fprintf(stderr, "%s: Track: %s: Expected a number.\n", argv[0], optarg);
				exit(1);
			}
			break;
		case 'a': /* Artist */
			memcpy(newid3.artist, optarg, 30);
			for (i=strlen(optarg);i<30;i++) newid3.artist[i]=0;
			newartist=1;
			break;
		case 'A': /* Album */
			memcpy(newid3.album, optarg, 30);
			for (i=strlen(optarg);i<30;i++) newid3.album[i]=0;
			newalbum=1;
			break;
		case 'y': /* Year */
			memcpy(newid3.year, optarg, 4);
			for (i=strlen(optarg);i<4;i++) newid3.year[i]=0;
			newyear=1;
			break;
		case 'c': /* Comment */
			memcpy(newid3.comment, optarg, 28);
			for (i=strlen(optarg);i<28;i++) newid3.comment[i]=0;
			newcomment=1;
			break;
		case 'g': /* Genre */
			if (isdigit(optarg[0])) { /* genre by number */
				newid3.genre = (unsigned char)atoi(optarg);
			} else { /* genre by name */
				matches=3; /* don't trip the first time */
				lettersmatched=0;
				/* lazy match - keep comparing down the list until we only get one hit */
				for (lettersmatched=0; matches>1; lettersmatched++) {
					for (i=matches=0; i<genre_count; i++) {
						/* Kludge to avoid infinite loop with "-g Fusion" */
						if (i == 84) i++;
						if (strncasecmp(genre_table[i], optarg, lettersmatched) == 0) {
							matches++;
							newid3.genre = (unsigned char)i;
						}
					}
				}
				if (matches == 0) { /* no hits - complain to user */
					fprintf(stderr, "%s: No such genre '%s'.\n", argv[0], optarg);
					exit(1);
				}	
			}	
			newgenre=1;
			break;
		case 'v': /* Version info */
			printf("This is id3 v%s.\n", version);
			exit(0);
			break;
		case 'h': /* Help info */
			printf("%s", usage);
			exit(0);
			break;
		case ':': /* need a value for an option */
		case '?': /* unknown option */
			fprintf(stderr, "%s", usage);
			exit(1);
		}
	}
	
	if (optind >= argc) {
		fprintf(stderr, "%s: Need a filename to work on.\n%s", argv[0], usage);
		exit(1);
	}
	
	for (i=optind; i<argc; i++) {
		notdir=hastag=1; /* innocent until proven guilty */
		if (newtitle || newartist || newalbum || newyear ||
			newcomment || newtrack || newgenre || deletetag) {
			fp = fopen(argv[i], "r+"); /* read/write */
		} else {
			fp = fopen(argv[i], "r"); /* read only */
		}
		if (fp == NULL) { /* file didn't open */
			fprintf(stderr, "%s: fopen: %s: ", argv[0], argv[i]);
			perror("");
			exitcode=1;
			continue;
		}
		fstat(fileno(fp), &finfo);
		if (S_ISDIR(finfo.st_mode)) notdir=0;
		
		if (fseek(fp, -128, SEEK_END) < 0) {
			/* problem rewinding */
			hastag=0;
		} else { /* we rewound successfully */ 
			if (fread(&oldid3, 128, 1, fp) < 0) {
				/* read error */
				fprintf(stderr, "%s: fread: %s: ", argv[0], argv[i]);
				perror("");
				exitcode=1;
				hastag=0;
			}
		}
		
		/* This simple detection code has a 1 in 16777216
		 * chance of misrecognizing or deleting the last 128
		 * bytes of your mp3 if it isn't tagged. ID3 ain't
		 * world peace, live with it.
		 */
		
		if (strncmp(oldid3.tag, "TAG", 3)) hastag=0;
		
		/* v1.1 tag == true if comment[28] == 0 */
		v11tag = !oldid3.comment[28];
		
		if (!hastag) {
			memset(&oldid3, 0, 127);
			oldid3.genre=255;
		}

		if (listtag) {
			if (rfc822style) {
				printf("\nFilename: %s\n", argv[i]);
				printf("Title: %-30.30s\n", oldid3.title);
				printf("Artist: %-30.30s\n", oldid3.artist);
				printf("Album: %-30.30s\n", oldid3.album);
				printf("Year: %-4.4s\n", oldid3.year);
				printf("Genre: %s (%d)\n",
					(oldid3.genre < genre_count) ?
					genre_table[oldid3.genre] : "Unknown", oldid3.genre);
				if (v11tag)
					printf("Track: %d\nComment: %-28.28s\n",
						oldid3.comment[29], oldid3.comment);
				else
					printf("Comment: %-30.30s\n", oldid3.comment);	
			} else {
				printf("%s:", argv[i]);
				if (hastag) {
					printf("\nTitle  : %-30.30s  Artist: %-30.30s\n",
						oldid3.title, oldid3.artist);
					printf("Album  : %-30.30s  Year: %-4.4s, Genre: %s (%d)\n",
						oldid3.album, oldid3.year, 
						(oldid3.genre < genre_count)
						? genre_table[oldid3.genre] : 
						"Unknown", oldid3.genre);
        				if (v11tag)
        					printf("Comment: %-28.28s    Track: %d\n", oldid3.comment, oldid3.comment[29]);
        				else
        					printf("Comment: %-30.30s\n", oldid3.comment);
				} else {
					if (notdir) {
						printf(" No ID3 tag.\n");
					} else {
						printf(" Directory.\n");
					}
				}
			}
		}
		
		if (hastag && deletetag) {
			if (ftruncate(fileno(fp), ftell(fp)-128) < 0) {
				fprintf(stderr, "%s: ftruncate: %s: ", argv[0], argv[i]);
				perror("");
				exitcode=1;
			}
			fclose(fp);
			continue;
		}
		
		strncpy(newid3.tag, "TAG", 3);
		if (!newtitle) memcpy(&newid3.title, &oldid3.title, 30);		
		if (!newartist) memcpy(&newid3.artist, &oldid3.artist, 30);
		if (!newalbum) memcpy(&newid3.album, &oldid3.album, 30);
		if (!newyear) memcpy(&newid3.year, &oldid3.year, 4);
		if (!newcomment) memcpy(&newid3.comment, &oldid3.comment, 28);
		if (!newtrack) { memcpy(&newid3.comment[28], &oldid3.comment[28], 2); }
		if (!newgenre) memcpy(&newid3.genre, &oldid3.genre, 1);
		v11tag = !newid3.comment[28];


		if (newtitle || newartist || newalbum || newyear || newcomment || newgenre || newtrack) {
			/* something did change */
			if (rfc822style) {
				printf("\nFilename: %s\n", argv[i]);
				printf("Title: %-30.30s\n", newid3.title);
				printf("Artist: %-30.30s\n", newid3.artist);
				printf("Album: %-30.30s\n", newid3.album);
				printf("Year: %-4.4s\n", newid3.year);
				printf("Genre: %s (%d)\n",
					(newid3.genre < genre_count) ?
					genre_table[newid3.genre] : "Unknown", newid3.genre);
				if (v11tag)
					printf("Track: %d\nComment: %-28.28s\n",
						newid3.comment[29], newid3.comment);
				else
					printf("Comment: %-30.30s\n", newid3.comment);	
			} else {
				printf("Title  : %-30.30s  Artist: %-30.30s\n", newid3.title, newid3.artist);
				printf("Album  : %-30.30s  Year: %-4.4s, Genre: %s (%d)\n",
					newid3.album, newid3.year,
					(newid3.genre < genre_count) ?
					genre_table[newid3.genre] : "Unknown", newid3.genre);
				if (v11tag)
					printf("Comment: %-28.28s    Track: %d\n", newid3.comment, newid3.comment[29]);
				else
					printf("Comment: %-30.30s\n", newid3.comment);
			}
			if (hastag) {
				/* seek back 128 bytes to overwrite it */
				fseek(fp, -128, SEEK_END);
			} else {
				/* new tag */
				fseek(fp, 0, SEEK_END);
			}
			if (fwrite(&newid3, 128, 1, fp) < 0) exitcode=1;
		}
		fclose(fp);
		
		/* Reset tag flags so that we don't mistakenly assume the
		 * next file also has a tag. (No need to reset hastag here,
		 * because it is always set to 1 until a non-tag is found.
		 */
		oldid3.tag[0] = '\0';
		
	}
	return exitcode;
}
