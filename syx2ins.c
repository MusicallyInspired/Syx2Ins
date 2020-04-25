/********************************************************************
*	SYX2INS conversion utility   v1.00								*
*	by Brandon Blume												*
*	July 11, 2015													*
*																	*
*	Command line tool to pull/generate the instrument patch list	*
*	from a custom MT-32 programmed SYX dump file into 				*
*	Cakewalk/Sonar's INS format (MIDI instrument list).				*
*																	*
*	You're free to do with it as you please. This program could		*
*	probably be vastly improved to be more efficient, but it works.	*
*	It's also missing error handling for a couple instances where	*
*	MIDI command data might not exist partway through the SYX file.	*
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM(a) (sizeof(a) / sizeof(*a))

int main (int argc, char *argv[])
{
	float nVersion = 1.00;

    unsigned char sysexPattern[5] = { 0xF0, 0x41, 0x10, 0x16, 0x12 };       //This is an array of the MT-32 Sysex send code bytes
    unsigned char timbreSet[6] = { 0xF0, 0x41, 0x10, 0x16, 0x12, 0x08 };    //Beginning of MT-32 Sysex set custom Timbre bytes (08 xx xx)
    unsigned char patchSet[6] = { 0xF0, 0x41, 0x10, 0x16, 0x12, 0x05 };    //Beginning of MT-32 Sysex set Patch bytes (05 xx xx)
    char stockPatches[128][11];
    char customTimbres[64][11];
    char newPatches[128][11];

    char gameTitle[21];             //Make array 1 element larger than the contents to make room for NULL character
    gameTitle[20] = '\0';           //Terminate end of string with a NULL character to avoid garbage characters when printing string later
    char *dot, *syxFilename;        //Pointers for handling existence and absence of file extensions in the program

    printf( "Syx2Ins  v%.2f    by Brandon Blume, July 2015\n\n", nVersion );

    if (argc != 3) /* argc should be 3 for correct execution */
    {
        /* We print argv[0] assuming it is the program name */
        printf( "usage:  %s  syxfile  insfile\n", argv[0] );
        return 0;
    }
    else
    {
        /* Set up log file */
        FILE *logFile = fopen( "log.txt", "w");
        fprintf(logFile, "Syx2Ins v%.2f  Log File    by Brandon Blume, July 2015\n======================\nInput file: \"%s\"\n\n", nVersion, argv[1]);

        /* We assume argv[1] is a filename to open */
        FILE *filePointer = fopen( argv[1], "rb" );

        /* Variables for adding missing file extension to first argument */
        char *fileExt;
        int filenameLen;

        dot = strrchr(argv[1], '.');

        if(!dot && filePointer == 0) /* check for file extension in argument */
        {
            //printf("No extension found in first argument and file does not exist.\nAdding SYX extension...\n");
            fprintf(logFile, "No extension found in first argument and file does not exist.\nAdding SYX extension...\n");

            filenameLen = strlen(argv[1]);
            fileExt = malloc(filenameLen+5);
            strncpy(fileExt, argv[1], filenameLen);
            strcpy(fileExt + filenameLen, ".SYX");

            filePointer = fopen( fileExt, "rb" );
            if (filePointer == 0 )
            {
                printf("Failed.\n\nFile \"%s\" does not exist.\n", fileExt );
                fprintf(logFile, "Failed.\n\nFile \"%s\" does not exist.\n", fileExt);
                return 0;
            }
            //printf("Success! \"%s\" file found!\n\n", fileExt);
            fprintf(logFile, "Success! \"%s\" file found!\n\n", fileExt);
        }
        else if( filePointer == 0)
        {
            printf("File \"%s\" does not exist.\n", argv[1]);
            printf("File \"%s\" does not exist.\n", argv[1]);
            fprintf(logFile, "File \"%s\" does not exist.\n", argv[1]);
            return 0;
        }

        /* Successful file open, whether it had a dot or not, one was added */

        /* Store entire file contents in memory then close the file to avoid tampering with it */
        fseek(filePointer, 0, SEEK_END);
        long fsize = ftell(filePointer);
        fseek(filePointer, 0, SEEK_SET);

        unsigned char *buffer = malloc(fsize + 1);
        fread(buffer, fsize, 1, filePointer);
        fclose(filePointer);

        buffer[fsize] = 0; //Terminate end of buffer with NULL

        /* Start searching for Roland send SysEx Command */
        if( memcmp( buffer, sysexPattern, NUM( sysexPattern ) ) )
        {
            /* File exists, but no point in continuing since there is no Roland sysex header */
            printf( "Not a valid MT-32 SysEx file.\nAborting...\n" );
            fprintf(logFile, "MT-32 sysex header not found. Not a valid MT-32 SysEx file.\nAborting...\n" );
        }
        else
        {
            printf( "MT-32 sysex header found!\n" );
            fprintf(logFile, "MT-32 sysex header found!\n" );

            /* Proceed to next check: look for MT-32's 'write to dispay' command bytes
            (0x20, 0x00, 0x00) and, if found, copy the following string and put it into
            the gameTitle array to use for naming the patch list in the INS output file.
            If not found, generate gameTitle array based on filename instead. */

            if ( buffer[5] == 0x20 && buffer[6] == 0x00 && buffer[7] == 0x00 )
            {
                //printf( "Custom title text found! Generating list title name...\n\nString contents (20 characters):\n" );
                //printf(" 1........10........20 \n"); //Mark positions for 20 LCD characters
                //printf( "\[" );  //Contain LCD message string with square brackets
                fprintf(logFile, "Custom title text found! Generating list title name...\n");

                int i;

                for ( i = 0; i < 20; i++ )
                {
                    gameTitle[i] = buffer[i+8];
                    /* Display empty space characters (" ") instead of NULL ('\0') for display and log output only,
                    doesn't actually change the value in the buffer or gameTitle arrays */
                    /*if(buffer[i+8] == '\0')
                    {
                        //printf(" ");
                        fprintf(logFile, " ");
                    }
                    else
                    {
                        //printf( "%c", buffer[i+8] );
                        fprintf(logFile, "%c", buffer[i+8] );
                    }*/
                }
                //printf( "]\n\n" );

                /* We don't want preceding empty spaces in front of the text string
                so we remove them for the gameTitle array */
                if ( gameTitle[0] == 0x20)
                {
                    //printf("Preceding spaces found in text. Removing...\n");
                    fprintf(logFile, "Preceding spaces found in title. Removing...\n");

                    i = 0;
                    while ( gameTitle[0] == 0x20 )
                    {
                        while ( i < NUM(gameTitle) - 1)
                        {
                            gameTitle[i] = gameTitle[i+1];
                            i++;
                        }
                        i = 0;
                    }
                }
                //else
                //{
                    /* No blank spaces found in LCD message string so no need to attempt to remove any */
                    //printf("No preceding spaces found!\n");
                    //fprintf(logFile, "No preceding spaces found!\n");
                //}
                //printf( "Done generating gameTitle!\n\n");
                //printf("gameTitle:\n\[%s]\n\n", gameTitle );
            }
            else
            {
                //printf("No custom title text found.\nGenerating list title after filename instead...\n\n");
                fprintf(logFile, "No custom title text found.\nGenerating list title after filename instead...\n\n");

                int len;
                dot = 0;

                /* Get position of the last dot in the SYX filename */
                dot = strrchr(argv[1], '.');
                /* If a dot was found, calculate the length to this point */
                if(dot) len = dot - argv[1];
                else len = strlen(argv[1]);
                /* Allocate a buffer big enough to hold the result. */
                syxFilename = malloc(len+1);

                int i;
                for(i = 0; i < len+1; i++)
                {
                    syxFilename[i] = 0;
                }
                /* Build the new name */
                strncpy(syxFilename, argv[1], len);
                syxFilename[len+1] = 0;
                strcpy(gameTitle, syxFilename);

                //printf("argv\[1]:\n\[%s]\nand gameTitle:\n\[%s]\n\n",argv[1],gameTitle);
            }

            fprintf(logFile, "\[%s]\n\n", gameTitle );

            /* ********************************************************************* */
            /* Prepare timbre and patch names for INS file */

            FILE *insFile;

            dot = strrchr(argv[2], '.');

            if(!dot) /* If there's no extension given, create it automatically */
            {
                //printf("No extension given for output INS file.\nAdding...\n\n");
                fprintf(logFile, "No extension given for output INS file.\nAdding .INS extension...\n");

                filenameLen = strlen(argv[2]);
                fileExt = malloc(filenameLen+5);
                strncpy(fileExt, argv[2], filenameLen);
                strcpy(fileExt + filenameLen, ".INS");
                insFile = fopen(fileExt, "r");
                if(insFile != 0)
                {
                    printf("\nFile \"%s.INS\" already exists.\nAborting...\n", argv[2]);
                    fprintf(logFile, "\nFile \"%s.INS\" already exists.\nAborting...", argv[2]);
                    return 0;
                }
                insFile = fopen(fileExt, "w");
            }
            else
            {
                //printf("Extension given. Continuing normally...\n");

                insFile = fopen(argv[2], "r");
                if(insFile != 0)
                {
                    printf("\nFile \"%s\" already exists.\nAborting...\n", argv[2]);
                    fprintf(logFile, "\nFile \"%s\" already exists.\nAborting...", argv[2]);
                    return 0;
                }
                insFile = fopen(argv[2], "w");
            }

            /* Create list of default MT-32 patch names */
            /* Group A stock patch names */
            strcpy(stockPatches[0], "AcouPiano1");
            strcpy(stockPatches[1], "AcouPiano2");
            strcpy(stockPatches[2], "AcouPiano3");
            strcpy(stockPatches[3], "ElecPiano1");
            strcpy(stockPatches[4], "ElecPiano2");
            strcpy(stockPatches[5], "ElecPiano3");
            strcpy(stockPatches[6], "ElecPiano4");
            strcpy(stockPatches[7], "Honkytonk");
            strcpy(stockPatches[8], "Elec Org 1");
            strcpy(stockPatches[9], "Elec Org 2");
            strcpy(stockPatches[10], "Elec Org 3");
            strcpy(stockPatches[11], "Elec Org 4");
            strcpy(stockPatches[12], "Pipe Org 1");
            strcpy(stockPatches[13], "Pipe Org 2");
            strcpy(stockPatches[14], "Pipe Org 3");
            strcpy(stockPatches[15], "Accordion");
            strcpy(stockPatches[16], "Harpsi 1");
            strcpy(stockPatches[17], "Harpsi 2");
            strcpy(stockPatches[18], "Harpsi 3");
            strcpy(stockPatches[19], "Clavi 1");
            strcpy(stockPatches[20], "Clavi 2");
            strcpy(stockPatches[21], "Clavi 3");
            strcpy(stockPatches[22], "Celesta 1");
            strcpy(stockPatches[23], "Celesta 2");
            strcpy(stockPatches[24], "Syn Brass1");
            strcpy(stockPatches[25], "Syn Brass2");
            strcpy(stockPatches[26], "Syn Brass3");
            strcpy(stockPatches[27], "Syn Brass4");
            strcpy(stockPatches[28], "Syn Bass 1");
            strcpy(stockPatches[29], "Syn Bass 2");
            strcpy(stockPatches[30], "Syn Bass 3");
            strcpy(stockPatches[31], "Syn Bass 4");
            strcpy(stockPatches[32], "Fantasy");
            strcpy(stockPatches[33], "Harmo Pan");
            strcpy(stockPatches[34], "Chorale");
            strcpy(stockPatches[35], "Glasses");
            strcpy(stockPatches[36], "Soundtrack");
            strcpy(stockPatches[37], "Atmosphere");
            strcpy(stockPatches[38], "Warm Bell");
            strcpy(stockPatches[39], "Funny Vox");
            strcpy(stockPatches[40], "Echo Bell");
            strcpy(stockPatches[41], "Ice Rain");
            strcpy(stockPatches[42], "Oboe 2001");
            strcpy(stockPatches[43], "Echo Pan");
            strcpy(stockPatches[44], "DoctorSolo");
            strcpy(stockPatches[45], "Schooldaze");
            strcpy(stockPatches[46], "BellSinger");
            strcpy(stockPatches[47], "SquareWave");
            strcpy(stockPatches[48], "Str Sect 1");
            strcpy(stockPatches[49], "Str Sect 2");
            strcpy(stockPatches[50], "Str Sect 3");
            strcpy(stockPatches[51], "Pizzicato");
            strcpy(stockPatches[52], "Violin 1");
            strcpy(stockPatches[53], "Violin 2");
            strcpy(stockPatches[54], "Cello 1");
            strcpy(stockPatches[55], "Cello 2");
            strcpy(stockPatches[56], "Contrabass");
            strcpy(stockPatches[57], "Harp 1");
            strcpy(stockPatches[58], "Harp 2");
            strcpy(stockPatches[59], "Guitar 1");
            strcpy(stockPatches[60], "Guitar 2");
            strcpy(stockPatches[61], "Elec Gtr 1");
            strcpy(stockPatches[62], "Elec Gtr 2");
            strcpy(stockPatches[63], "Sitar");
            /* Group B stock patch names */
            strcpy(stockPatches[64], "Acou Bass1");
            strcpy(stockPatches[65], "Acou Bass2");
            strcpy(stockPatches[66], "Elec Bass1");
            strcpy(stockPatches[67], "Elec Bass2");
            strcpy(stockPatches[68], "Slap Bass1");
            strcpy(stockPatches[69], "Slap Bass2");
            strcpy(stockPatches[70], "Fretless 1");
            strcpy(stockPatches[71], "Fretless 2");
            strcpy(stockPatches[72], "Flute 1");
            strcpy(stockPatches[73], "Flute 2");
            strcpy(stockPatches[74], "Piccolo 1");
            strcpy(stockPatches[75], "Piccolo 2");
            strcpy(stockPatches[76], "Recorder");
            strcpy(stockPatches[77], "Pan Pipes");
            strcpy(stockPatches[78], "Sax 1");
            strcpy(stockPatches[79], "Sax 2");
            strcpy(stockPatches[80], "Sax 3");
            strcpy(stockPatches[81], "Sax 4");
            strcpy(stockPatches[82], "Clarinet 1");
            strcpy(stockPatches[83], "Clarinet 2");
            strcpy(stockPatches[84], "Oboe");
            strcpy(stockPatches[85], "Engl Horn");
            strcpy(stockPatches[86], "Bassoon");
            strcpy(stockPatches[87], "Harmonica");
            strcpy(stockPatches[88], "Trumpet 1");
            strcpy(stockPatches[89], "Trumpet 2");
            strcpy(stockPatches[90], "Trombone 1");
            strcpy(stockPatches[91], "Trombone 2");
            strcpy(stockPatches[92], "Fr Horn 1");
            strcpy(stockPatches[93], "Fr Horn 2");
            strcpy(stockPatches[94], "Tuba");
            strcpy(stockPatches[95], "Brs Sect 1");
            strcpy(stockPatches[96], "Brs Sect 2");
            strcpy(stockPatches[97], "Vibe 1");
            strcpy(stockPatches[98], "Vibe 2");
            strcpy(stockPatches[99], "Syn Mallet");
            strcpy(stockPatches[100], "Wind Bell");
            strcpy(stockPatches[101], "Glock");
            strcpy(stockPatches[102], "Tube Bell");
            strcpy(stockPatches[103], "Xylophone");
            strcpy(stockPatches[104], "Marimba");
            strcpy(stockPatches[105], "Koto");
            strcpy(stockPatches[106], "Sho");
            strcpy(stockPatches[107], "Shakuhachi");
            strcpy(stockPatches[108], "Whistle 1");
            strcpy(stockPatches[109], "Whistle 2");
            strcpy(stockPatches[110], "Bottleblow");
            strcpy(stockPatches[111], "Breathpipe");
            strcpy(stockPatches[112], "Timpani");
            strcpy(stockPatches[113], "MelodicTom");
            strcpy(stockPatches[114], "Deep Snare");
            strcpy(stockPatches[115], "Elec Perc1");
            strcpy(stockPatches[116], "Elec Perc2");
            strcpy(stockPatches[117], "Taiko");
            strcpy(stockPatches[118], "Taiko Rim");
            strcpy(stockPatches[119], "Cymbal");
            strcpy(stockPatches[120], "Castanets");
            strcpy(stockPatches[121], "Triangle");
            strcpy(stockPatches[122], "Orche Hit");
            strcpy(stockPatches[123], "Telephone");
            strcpy(stockPatches[124], "Bird Tweet");
            strcpy(stockPatches[125], "OneNoteJam");
            strcpy(stockPatches[126], "WaterBells");
            strcpy(stockPatches[127], "JungleTune");

            /* We will fill in all the spaces in the new INS patch list (newPatches) with
            the stock list first before searching for the custom timbre names to imitate
            the default MT-32 patch listing, this way if the custom SysEx data doesn't
            replace every patch listing (they usually don't) they won't show up empty in
            our new list */
            int j;
            for(j = 0; j < 127; j++)
                strcpy(newPatches[j], stockPatches[j]);

            unsigned long ii;
            int nTimbre = 0;
            int k = 0;

            printf("Cataloging custom timbre names...\n");
            fprintf(logFile, "Cataloging custom timbre names...\n");

            /* Search for "timbre set" command pattern matches in buffer (08 xx xx), position = ii */
            for ( ii = 0; ii <= fsize - sizeof(timbreSet); ii++ )
            {
                if ( buffer[ii] == timbreSet[0] )
                {
                    if ( ( memcmp( &buffer[ii], timbreSet, sizeof( timbreSet ) ) ) == 0 )
                    {
                        /* Pattern match found, proceed to copy timbre name */

                        //printf("Timbre sysex pattern found! (%d)\n", nTimbre);
                        //fprintf(logFile, "Timbre sysex pattern found! (%d)\n", nTimbre);

                        /* We advance the pointer position to the length of the command bytes + 2
                        for the extra required two bytes to get to the beginning of the timbre name */
                        ii += sizeof(timbreSet) + 2;
                        //fprintf(logFile, "Position of discovered instrument name in buffer: %lu\n", ii);

                        /* Copy the custom patch name for the file into the custom timbre list */
                        //strncpy(customTimbres[nTimbre], &buffer[ii], 10);

                        for(k = 0; k < 10; k++)
                        {
                            //printf("%d", k);
                            customTimbres[nTimbre][k] = buffer[ii+k];
                        }
                        customTimbres[nTimbre][10] = 0;
                        //printf("%s\n", customTimbres[nTimbre]);
                        nTimbre++;
                    }
                }
            }

            /* Begin algorithm for discovering custom Patch list names. This is different from
            the 64-instrument custom timbre list we just generated as that list is usually not
            necessarily in the same order as the full 128-instrument Patch list. The final Patch
            list is generated from three sources (or groups of timbres):
            Default Preset Group A 0-63
            Default Preset Group B 64-127
            Custom Timbre Group 0-63 */

            /* First step: iterate through the SYX file to find instances of
            the "05 xx xx" byte codes (there are 4, 32 instruments each) */
            printf("Generating final instrument list...\n\n");
            fprintf(logFile, "Generating final instrument list...\n\n");

            int nPatch = 0;
            int i = 0;
            for ( ii = 0; ii <= fsize - sizeof(patchSet); ii++ )
            {
                if ( buffer[ii] == patchSet[0] )
                {
                    if ( ( memcmp( &buffer[ii], patchSet, sizeof( patchSet ) ) ) == 0 )
                    {
                        /* Pattern found */

                        //printf("Patch set pattern found! (%d)\n", nPatch);
                        //fprintf(logFile, "Patch set pattern found! (%d)\n", nPatch);


                        /* We advance the pointer position to the length of the command bytes + 2
                        for the extra required two bytes to get to the actual patch data after the pattern and identifying code */
                        ii += sizeof(patchSet) + 2;

                        /* Second step: Now that we have found the first point of data, we need to iterate through
                        all 32 sets of patch information in this iterated instance of the "05 xx xx" pattern (128 altogether).
                        Every 8 bytes is a new entry for a total of 256 bytes per data block */

                        for(i = 0; i < 32; i++)
                        {
                            /* These conditional statements determine whether the instrument is from Preset Group A,
                            Preset Group B, or the Custom Timbre memory group */
                            if (buffer[ii] == 0x00 && buffer[ii+2] != 0x00)
                            {
                                //printf("buffer\[ii] = Group A!\n");
                                strcpy( newPatches[nPatch], stockPatches[ buffer[ii + 1] ]);
                            }
                            else if(buffer[ii] == 0x01)
                            {
                                //printf("buffer\[ii] = Group B!\n");
                                strcpy( newPatches[nPatch], stockPatches[ buffer[ii+1] + 64 ] );
                            }
                            else if(buffer[ii] == 0x02)
                            {
                                //printf("buffer\[ii] = Custom Timbre!\n");
                                strcpy( newPatches[nPatch], customTimbres[ buffer[ii + 1] ] );
                            }

                            fprintf(logFile, "%s\n", newPatches[nPatch]);
                            printf(".");
                            nPatch++;

                            ii += 8;
                        }
                    }
                }
            }
            printf("\n\nGenerating INS file...\n\n");
            fprintf(logFile, "\nGenerating INS file...\n\n");

            /* Begin generating INS file */

            fprintf(insFile, "\n; ----------------------------------------------------------------------\n\n.Patch Names\n\n\n");
            fprintf(insFile, "\[%s", gameTitle);
            fprintf(insFile, " Patch Bank]\n");

            /* We iterate through the stored patch list in newPatches and print them to our new INS file */
            for ( i = 0; i < 128; i++)
                fprintf(insFile, "%d=%s\n", i, newPatches[i]);

            fprintf(insFile, "\n; ----------------------------------------------------------------------\n\n.Note Names\n\n");
            fprintf(insFile, "\n; ----------------------------------------------------------------------\n\n.Instrument Definitions\n\n");
            fprintf(insFile, "\n\[%s Patch Bank]\nPatch\[*]=%s Patch Bank\n", gameTitle, gameTitle);

            fclose(insFile);
        }
        printf("DONE!\n");
        fprintf(logFile, "DONE!\n");
        fclose(logFile);
    }
    return 0;
}
