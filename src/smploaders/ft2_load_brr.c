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

static int32_t block2sampSize(const int64_t in);
static int16_t fixedMath16_16(const int32_t in, const int64_t numerator, const int32_t denominator);
static void filterCoeff(const int16_t sample1, const int16_t sample2, int16_t* sampCoeff1, int16_t* sampCoeff2, const int8_t filterType);

bool loadBRR(FILE* f, uint32_t filesize)
{
    sample_t* s = &tmpSmp;
    char* brrBuffer;

    int32_t sampleDataPosition = 0, fileLength = 0,
        sampleLength = 0, loopStart = 0, loopEnd = 0;

    brrBuffer = (char*)malloc(filesize * sizeof * brrBuffer);

    if (brrBuffer == NULL)
    {
        loaderMsgBox("Not enough memory!");
        return false;
    }

    fileLength = (int32_t)filesize;

    if (fread(brrBuffer, 1, filesize, f) != filesize)
    {
        okBoxThreadSafe(0, "System message", "General I/O error during loading! Is the file in use?", NULL);
        free(brrBuffer);
        brrBuffer = NULL;
        return false;
    }

    if (brrBuffer != NULL)
    {
        int16_t* ptr16 = NULL;

        bool loopFlag = false, endFlag = false;

        int32_t  endFlagPos = fileLength;

        uint8_t  filterFlag = 0, shifter = 0;

        int16_t sampleOne = 0, sampleTwo = 0,
            sampleCoeffOne = 0, sampleCoeffTwo = 0;

        s->volume = 64;
        s->panning = 128;
        s->flags |= SAMPLE_16BIT;

        if (fileLength % 9 == 0)
        {
            sampleDataPosition = 0;
            sampleLength = block2sampSize(fileLength);

            s->flags |= LOOP_DISABLED;
            loopStart = 16;
            loopEnd = sampleLength;
        }
        else if ((fileLength - 2) % 9 == 0)
        {
            const int16_t loopStartBlock = (int16_t)((uint8_t)brrBuffer[0] | ((uint8_t)brrBuffer[1] << 8));
            loopStart = block2sampSize(loopStartBlock);

            sampleDataPosition = 2;
            sampleLength = block2sampSize(fileLength - 2);

            s->flags |= LOOP_FWD;
        }
        else
        {
            free(brrBuffer);
            brrBuffer = NULL;
            return false;
        }

        if (!allocateSmpData(s, sampleLength, true))
        {
            loaderMsgBox("Not enough memory!");
            return false;
        }

        ptr16 = (int16_t*)s->dataPtr;

        for (int32_t i = sampleDataPosition; i < fileLength; i++)
        {
            uint8_t in_byte = (uint8_t)brrBuffer[i];

            if ((i - sampleDataPosition) % 9 == 0)
            {
                shifter = (uint8_t)(in_byte & ~0x0F) >> 4;
                if (shifter > 12) shifter = 12;

                filterFlag = (uint8_t)(in_byte & ~0xF3) >> 2;
                if (filterFlag > 3) filterFlag = 3;

                if ((loopFlag = in_byte & 0x02))
                    s->flags |= LOOP_FWD;
                else
                {
                    s->flags &= ~LOOP_FWD;
                    s->flags |= LOOP_DISABLED;
                }

                if (in_byte & 0x01)
                {
                    endFlag = true;
                    endFlagPos = i;

                    if (loopFlag && endFlag)
                    {
                        loopEnd = block2sampSize(i + 7);

                        if (loopEnd > sampleLength)
                            loopEnd = sampleLength;
                    }
                    else
                    {
                        s->flags &= ~LOOP_FWD;
                        s->flags |= LOOP_DISABLED;
                        loopStart = 16;
                        loopEnd = sampleLength;
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

                    nibble = ((in_byte & ~0x0F) >> 4);
                    if (nibble > 7) nibble ^= 0xF0;

                    filterCoeff(sampleTwo, sampleOne, &sampleCoeffOne, &sampleCoeffTwo, filterFlag);
                    sampleOne = ((int16_t)nibble << shifter) + sampleCoeffOne - sampleCoeffTwo;

                    *ptr16++ = sampleOne;

                    nibble = (in_byte & ~0xF0);
                    if (nibble > 7) nibble ^= 0xF0;

                    filterCoeff(sampleOne, sampleTwo, &sampleCoeffOne, &sampleCoeffTwo, filterFlag);
                    sampleTwo = ((int16_t)nibble << shifter) + sampleCoeffOne - sampleCoeffTwo;

                    *ptr16++ = sampleTwo;
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

/* converts number of bytes to number of samples as per BRR's 16 sample to 9 byte ratio*/
static int32_t block2sampSize(const int64_t in)
{
    /* equivalent to (int32) round(in  * 16.0 / 9.0) */
    return (int32_t)((in * ((16 << 16) / 9) + 0x8000) >> 16);
}

/* S-DSP chip uses 16.16 fixed math for its filter coefficients during decoding */
static int16_t fixedMath16_16(const int32_t in, const int64_t numerator, const int32_t denominator)
{
    /* equivalent to (int16) (in * numerator / denominator) */
    return (int16_t)((in * (numerator << 16) / denominator) >> 16);
}

static void filterCoeff(const int16_t sample1, const int16_t sample2, int16_t* sampCoeff1, int16_t* sampCoeff2, const int8_t filterType)
{
    switch (filterType)
    {
    case 1:
        *sampCoeff1 = fixedMath16_16(sample1, 15, 16);
        *sampCoeff2 = 0;
        break;

    case 2:
        *sampCoeff1 = fixedMath16_16(sample1, 61, 32);
        *sampCoeff2 = fixedMath16_16(sample2, 15, 16);
        break;

    case 3:
        *sampCoeff1 = fixedMath16_16(sample1, 115, 64);
        *sampCoeff2 = fixedMath16_16(sample2, 13, 16);
        break;

    default:
        *sampCoeff1 = 0;
        *sampCoeff2 = 0;
        break;
    }
}
