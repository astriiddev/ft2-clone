/* Super Nintendo BRR sample loader
**
** Note: Could use some sample rate/pitch detection
** Do NOT close the file handle!
**
** created: September 4, 2023, 3:09:15 AM by _astriid_
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../ft2_header.h"
#include "../ft2_audio.h"
#include "../ft2_sample_ed.h"
#include "../ft2_sysreqs.h"
#include "../ft2_sample_loader.h"

int16_t filterCoeff(const int32_t sample, const bool sampleNumber, const uint8_t filterType);

bool loadBRR(FILE* f, uint32_t filesize)
{
	sample_t* s = &tmpSmp;
    char* brrBuffer;
    brrBuffer = (char*)malloc(filesize * sizeof * brrBuffer);

    if (brrBuffer == NULL)
    {
        loaderMsgBox("Not enough memory!");
        return false;
    }

    int32_t sampleDataPosition = 0, fileLength = 0, 
            sampleLength = 0, loopStart = 0, loopEnd = 0;

    fileLength = (int32_t)filesize;

	if (fread(brrBuffer, 1, filesize, f) != filesize)
	{
		okBoxThreadSafe(0, "System message", "General I/O error during loading! Is the file in use?");
        free(brrBuffer);
        brrBuffer = NULL;
		return false;
	}

    const double BLOCK_RATIO = 16.00 / 9 ;

    bool    loopFlag = false, endFlag = false;
    int32_t  endFlagPos = fileLength;
    uint8_t  filterFlag = 0,
             shifter    = 0;

    static int16_t sampleOne      = 0, sampleTwo = 0 ,
                   sampleCoeffOne = 0, sampleCoeffTwo = 0;

    s->volume = 64;
    s->panning = 128;
    s->flags |= SAMPLE_16BIT;

    if(brrBuffer != NULL)
    {
        if (fileLength % 9 == 0)
        {

            sampleDataPosition = 0;
            sampleLength = (int32_t)(((uint64_t)fileLength * ((16 << 16) / 9) + 0x8000) >> 16);

            if (!allocateSmpData(s, sampleLength, true))
            {
                loaderMsgBox("Not enough memory!");
                return false;
            }

            s->flags |= LOOP_DISABLED;
            loopStart = 16;
            loopEnd = sampleLength--;
        }
        else if ((fileLength - 2) % 9 == 0)
        {
            loopStart = (int32_t)(round((double)((uint8_t)brrBuffer[0] | (uint8_t)brrBuffer[1] << 8) * BLOCK_RATIO));

            s->flags |= LOOP_FWD;

            sampleDataPosition = 2;
            sampleLength = (int32_t)((((uint64_t)filesize - 2) * ((16 << 16) / 9) + 0x8000) >> 16);

            if (!allocateSmpData(s, sampleLength, true))
            {
                loaderMsgBox("Not enough memory!");
                return false;
            }
        }
        else
        {
            free(brrBuffer);
            brrBuffer = NULL;
            return false;
        }

        int decodedSample = 0;
        for (int32_t i = sampleDataPosition; i < fileLength; i++)
        {
            s->dataPtr[decodedSample + 0] = 0;
            if ((i - sampleDataPosition) % 9 == 0)
            {
                shifter = (uint8_t)(brrBuffer[i] & ~0x0F) >> 4;

                if (shifter > 12)
                    shifter = 12;

                filterFlag = (uint8_t)(brrBuffer[i] & ~0xF3) >> 2;

                if (filterFlag > 3)
                    filterFlag = 3;

                if (brrBuffer[i] & 0x02)
                    loopFlag = true;
                else
                    loopFlag = false;

                if (brrBuffer[i] & 0x01)
                {
                    endFlag = true;
                    endFlagPos = i;

                    if (loopFlag && endFlag)
                    {
                        loopEnd = (uint32_t)(floor((double)(i + 7) * BLOCK_RATIO));

                        if (loopEnd > sampleLength)
                            loopEnd = sampleLength;
                    }
                    else
                    {
                        s->flags |= LOOP_DISABLED;
                        loopStart = 16;
                        loopEnd = sampleLength--;
                    }
                }
                else
                {
                    endFlag = false;
                    endFlagPos = fileLength;
                }
            }

            else if ((i - sampleDataPosition) % 9 != 0 && shifter <= 12)
            {
                if ((i - sampleDataPosition) > 0)
                {
                    int8_t nibble = 0;

                    sampleCoeffOne = filterCoeff(sampleTwo, 0, filterFlag);
                    sampleCoeffTwo = filterCoeff(sampleOne, 1, filterFlag);

                    nibble = ((brrBuffer[i] & ~0x0F) >> 4);
                    if (nibble > 7)
                    {
                        nibble -= 16;
                    }

                    sampleOne = ((int16_t)nibble << shifter) + (sampleCoeffOne)-(sampleCoeffTwo);

                    if (i > endFlagPos + 8)
                    {
                        s->dataPtr[decodedSample + 0] = 0;
                        s->dataPtr[decodedSample + 1] = 0;
                    }
                    else
                    {
                        s->dataPtr[decodedSample + 0] = (uint8_t)(sampleOne & ~0xFF00);
                        s->dataPtr[decodedSample + 1] = (uint8_t)((sampleOne & ~0x00FF) >> 8);
                    }

                    decodedSample += 2;

                    sampleCoeffOne = filterCoeff(sampleOne, 0, filterFlag);
                    sampleCoeffTwo = filterCoeff(sampleTwo, 1, filterFlag);

                    nibble = (brrBuffer[i] & ~0xF0);
                    if (nibble > 7)
                    {
                        nibble -= 16;
                    }

                    sampleTwo = ((int16_t)nibble << shifter) + (sampleCoeffOne)-(sampleCoeffTwo);

                    if (i > endFlagPos + 8)
                    {
                        s->dataPtr[decodedSample + 0] = 0;
                        s->dataPtr[decodedSample + 1] = 0;
                    }
                    else
                    {
                        s->dataPtr[decodedSample + 0] = (uint8_t)(sampleTwo & ~0xFF00);
                        s->dataPtr[decodedSample + 1] = (uint8_t)((sampleTwo & ~0x00FF) >> 8);
                    }

                    decodedSample += 2;

                    if (decodedSample >= (int)(sampleLength << 1))
                        break;
                }
            }
        }

        s->length = sampleLength;
        s->loopStart = loopStart;
        s->loopLength = loopEnd - loopStart;
        
        free(brrBuffer);
        brrBuffer = NULL;
    }

    return true;
}

int16_t filterCoeff(const int32_t sample, const bool sampleNumber, const uint8_t filterType)
{
    if (filterType == 1)
    {
        if (sampleNumber == 0)
            return (int16_t)((sample * ((15 << 16) / 16) + 0x8000) >> 16);
        else
            return 0;
    }

    else if (filterType == 2)
    {
        if (sampleNumber == 0)
            return (int16_t)((sample * ((61 << 16) / 32) + 0x8000) >> 16);
        else
            return (int16_t)((sample * ((15 << 16) / 16) + 0x8000) >> 16);
    }

    else if (filterType == 3)
    {
        if (sampleNumber == 0)
            return (int16_t)((sample * ((115 << 16) / 64) + 0x8000) >> 16);
        else
            return (int16_t)((sample * ((13 << 16) / 16) + 0x8000) >> 16);
    }

    else /*filterType == 0*/
        return 0;
}