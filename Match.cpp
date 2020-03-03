/*
 * A small regex pattern matcher only (no replace function is implemented)
 *
 * Pattern input is in hex-ascii. regex symbols are in clear text
 * Supported patterns:
 *  *: match previous character any number of time
 *  +: match previous character 1 or more times
 *  ?: match any character
 *  {min-max}: ignore atleast min to max no of characters
 *  (a|b|c): match one character with a or b or c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tstamp.h"
static const size_t PTR_SIZE = sizeof(char *);
typedef struct
{
    size_t min;
    size_t max;
}minmax_t;
typedef struct
{
    union
    {
        char *cClassPtr;
        char cClarArray[PTR_SIZE];
    };
    size_t size;
}charClass_t;
typedef struct
{
    char type;
    union
    {
        char letter;
        minmax_t minmax;
        charClass_t *charClass;
    };
}patType_t;
typedef struct
{
    patType_t *pat;
    size_t size;
}patterns_t;

// Pattern IDs
#define PATT_END    -2
#define UNKNOWN     -1
#define NORMAL      0
#define QUESTION    1
#define STAR        2
#define BRACE       3
#define PLUS        4
#define CHARCLASS   5
#define DOT         6

// byte ascii to hex
static unsigned char batoh(const unsigned char s)
{
    if (s >= '0' && s <= '9')
    {
        return (s - '0');
    }
    else if (s >= 'a' && s <= 'f')
    {
        return (s - 'a' + 0xa);
    }
    else{   printf("invalid %x ", s);   }
    return -1;
}

// word-ascii to hex
static unsigned char watoh(const char *s)
{
    unsigned char c;
    c = batoh(s[0]) << 4 | batoh(s[1]);
    return c;
}

// parse pattern string and compute pattern size
static size_t getPatternSize(const char *pat)
{
    size_t pIndex = 0;
    size_t patSize = 0;
    while (pat[pIndex] != 0)
    {
        char c = pat[pIndex];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
        {
            pIndex += 2;
            patSize++;
        }
        else
        {
            char ch = pat[pIndex++];
            switch (ch)
            {
            case '?':
            case '*':
            case '+':
            case '.':
                patSize++;
                break;
            case '{':
                while (pat[pIndex++] != '}')
                {
                }
                patSize++;
                break;
            case '(':
                while (pat[pIndex++] != ')')
                {
                }
                patSize++;
                break;


            }

        }

    }


    return patSize;
}

// skip c in str and return no of c found sequentially
static int skipChar(const char* str, char c)
{
    int skippedChars = 0;
    while (str[skippedChars] == c)
    {
        skippedChars++;
    }
    return skippedChars;
}

// get decimal int from string
static int getNextInt(const char* str, size_t *count)
{
    size_t index = 0;
    int num = 0;
    while (str[index] >= '0' && str[index] <= '9')
    {
        num *= 10;
        num += (str[index] - '0');
        index++;
    }
    *count = index;
    return num;
}

// process () pattern in pat string and load charClass pattern structure in pats
static int processParenthesis(const char *pat, patType_t *pats)
{
    size_t sizeC = 0;
    size_t pIndexLocal = 0; 
    size_t pIndex = 0;
    // find no of chars
    while (pat[pIndexLocal] != ')')
    {
        char ch = pat[pIndexLocal];
        if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))
        {
            sizeC++;
            pIndexLocal += 2;   // skip word
        }
        else
        {
            pIndexLocal++;      // skip byte
        }
    }
    // sizeC contains charClass size. Allocate memory and setup pointer
    charClass_t *charClass = (charClass_t*)malloc(sizeof(charClass_t));
    char *cClass = NULL;
    // if size is bigger than array, allocate separate memory else use current array for small no of letters
    if (sizeC > PTR_SIZE)
    {
        charClass->cClassPtr = (char *)malloc(sizeC * sizeof(char));
        cClass = charClass->cClassPtr;
    }
    else
    {
        cClass = charClass->cClarArray;
    }
    // load charClass with characters
    for (size_t i = 0; i < sizeC; i++)
    {
        char c = watoh(&pat[pIndex]);
        pIndex += 3;
        cClass[i] = c;
    }
    charClass->size = sizeC;
    pats->type = CHARCLASS;
    pats->charClass = charClass;
    return (int)pIndex;
}

// process pattern string and prepare patType_t array representing each letter in pattern string
static patType_t *processPattern(const char *pat, size_t *patSize)
{
    size_t pIndex = 0;
    size_t size = getPatternSize(pat);
    // +1 to add END marker at pattern end
    patType_t *pats = (patType_t *)calloc(size+1, sizeof(patType_t));
    int pTypeIndex = 0;
    // get valid hex ascii
    while (pat[pIndex] != 0)
    {
        char c = pat[pIndex];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
        {
            char ch = watoh(&pat[pIndex]);
            pIndex += 2;  // point to next set of hex ascii
            pats[pTypeIndex].letter = ch;
            pats[pTypeIndex].type = NORMAL;
        }
        else
        {
            char ch = pat[pIndex++];
            switch (ch)
            {
            case '?':
            {
                pats[pTypeIndex].type = QUESTION;
                pats[pTypeIndex].letter = ch;
            }
            break;
            case '*':
            {
                pats[pTypeIndex].type = STAR;
                pats[pTypeIndex].letter = ch;
            }
            break;
            case '.':
            {
                pats[pTypeIndex].type = DOT;
                pats[pTypeIndex].letter = ch;
            }
            break;
            case '+':
            {
                pats[pTypeIndex].type = PLUS;
                pats[pTypeIndex].letter = ch;
            }
            break;
            case '{':
            {
                size_t offset = 0;
                size_t min = getNextInt(&pat[pIndex], &offset);
                pIndex += offset;
                pIndex += skipChar(&pat[pIndex], ' ');   // skip spaces, if any
                pIndex += skipChar(&pat[pIndex], '-');   // skip dash
                pIndex += skipChar(&pat[pIndex], ' ');   // skip spaces, if any
                size_t max = getNextInt(&pat[pIndex], &offset);
                pIndex += offset;
                pIndex += skipChar(&pat[pIndex], '}');   // skip closing brace
                pats[pTypeIndex].type = BRACE;
                pats[pTypeIndex].minmax.min = min;
                pats[pTypeIndex].minmax.max = max;


            }
            break;
            case '(':
            {
                pIndex += (size_t)processParenthesis(&pat[pIndex], &pats[pTypeIndex]);
            }
            break;


            default:
            {
                pats[pTypeIndex].type = UNKNOWN;
                pats[pTypeIndex].letter = ch;
            }
            break;

            }
        }
        pTypeIndex++;

    }
    // add PATT_END to mark end of pattern
    pats[pTypeIndex].type = PATT_END;
    pats[pTypeIndex].letter = 0;

    *patSize = pTypeIndex+1;
    return pats;
}


void printPattern(patType_t *pat, size_t size)
{
    char * charClass = NULL;
    for (size_t i = 0; i < size; i++)
    {
        switch (pat[i].type)
        {
        case BRACE:
            printf("{%zd-%zd}", pat[i].minmax.min, pat[i].minmax.max);
            break;
        case CHARCLASS:
            if (pat[i].charClass->size > PTR_SIZE)
            {
                charClass = pat[i].charClass->cClassPtr;
            }
            else
            {
                charClass = pat[i].charClass->cClarArray;
            }
            printf("(");
            for (size_t j = 0; j < pat[i].charClass->size; j++)
            {
                printf("%c", charClass[j]);
                if (j != pat[i].charClass->size - 1)
                {
                    printf("|");
                }
            }
            printf(")");
            break;
        default:
            printf("%c", pat[i].letter);
        }
    }
}

// match single char
static int matchChar(patType_t *pat, char c)
{
    switch (pat->type)
    {
    case NORMAL:
        if (pat->letter == c)
        {
            return 0;
        }
        break;
    case QUESTION:
    case DOT:
        return 0;
    }

    return -1;
}

// match remaining pattern in case of brace pattern
static int matchRemainingPattern(patType_t *pat, const char* str)
{
    int patIndex = 0;
    int index = 0;
    // till end of patterns or end of string
    while (pat[patIndex].type != PATT_END && str[index] != 0)
    {
        if(matchChar(&pat[patIndex], str[index]) == 0)
        {
            patIndex++;
            index++;
        }
        else
        {
            return -1;
        }

    }
    return 0;
}

static int matchBrace(patType_t *pat, const char* str)
{
    size_t sizeS = strlen(str);
    if (sizeS < pat->minmax.min)
    {
        return -1;      // not enough string
    }
    int index = (int)pat->minmax.min;
    char nextPatChar = pat[1].letter;   // char till we need to skip
    /* only matching nextPatChar will not work. We may have to match the whole pattern
     * and in case of match failure, attempt again by skipping one char till we skip
     * pat->minmax.max no of chars as nextPatChar or couple of patternchars may appear in
     * the pattern min-max part which could have been skipped to get a proper match
     * for example: a{2-8}bcd should match abcbcd but will not in our case as we are returning
     * after first match of b.
     * this function needs to know patSize or something else for it to match full pattern
     *
     * Not an elegant solution. puts a limitation that there can be no other pattern char after 
     * brace pattern.
     */
    while (str[index] != 0 && index <= (int)pat->minmax.max)
    {
        if (matchRemainingPattern(&pat[1], &str[index]) == 0)
        {
            return index;
        }
        else
        {
            index++;
        }
        //if (str[index] == nextPatChar)
        //{
        //    return index;   // next char found ie brace condition matched
        //}
        //else
        //{
        //    index++;
        //}
    }
    return 0;
}

static int matchPlus(char patChar, patType_t *pat, const char* str)
{
    int index = 0;
    if (str[index] == patChar)
    {
        index++;
    }
    else
    {
        return -1;
    }
    index += skipChar(&str[index], patChar);
    return index;
}
static int matchStar(patType_t *patPrev, patType_t *patNext, const char* str)
{
    int index = 0;
    index = skipChar(str, patPrev->letter);
    return index;
}

static int matchCharClass(patType_t *pat, const char* str)
{
    char *charClass = NULL;
    if (pat->charClass->size > PTR_SIZE)
    {
        charClass = pat->charClass->cClassPtr;
    }
    else
    {
        charClass = pat->charClass->cClarArray;
    }
    for (size_t i = 0; i < pat->charClass->size; i++)
    {
        if (*str == charClass[i])
        {
            return 0;
        }

    }
    return -1;
}

// match complete pattern starting in str. return success/failure and no of letters matched
static int matchPattern(patType_t *pat, const char *str, int *charsMatched)
{
    size_t index = 0;
    size_t patIndex = 0;
    while (str[index] != 0)
    {
        if (pat[patIndex + 1].type == STAR)
        {
            index += matchStar(&pat[patIndex], &pat[patIndex + 2], &str[index]);
            patIndex += 2;
        }
        else if (pat[patIndex + 1].type == PLUS)
        {
            int result = matchPlus(pat[patIndex].letter, &pat[patIndex + 2], &str[index]);
            if (result == -1)
            {
                return -1;      // couldn't find atleast a single match
            }
            index += (size_t)result;
            patIndex += 2;
        }
        else if (pat[patIndex].type == BRACE)
        {
            int result = matchBrace(&pat[patIndex], &str[index]);
            if (result == -1)
            {
                return -1;  // match failed
            }
            patIndex += 1;
            index += (size_t)result;
        }
        else if (pat[patIndex].type == CHARCLASS)
        {
            if (matchCharClass(&pat[patIndex], &str[index]) == -1)
            {
                return -1;
            }
            patIndex++;
            index++;
        }
        else if (matchChar(&pat[patIndex], str[index]) != -1)
        {
            patIndex++;
            index++;
        }
        else
        {
            return -1;
        }
        // -1 because we added PATT_END to mark end of pattern
        if (pat[patIndex].type == PATT_END)
        {
            // full pattern match
            *charsMatched = (int)index;
            return 0;
        }

    }
    return -1;
}

int freePattern(patType_t *pats, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (pats[i].type == CHARCLASS)
        {
            if(pats[i].charClass->size > PTR_SIZE)
            {
                free(pats[i].charClass->cClassPtr);
            }
            free(pats[i].charClass);
        }
    }
    free(pats);
    return 0;
}

// interface function to match patterns
int matchPatterns(patType_t *pat, size_t size, const char *str, int *charsMatched)
{
    int index = 0;
    while (str[index] != 0)
    {
        if (matchPattern(pat, &str[index], charsMatched) == 0)
        {
            return index;
        }
        index++;
    }

    return -1;
}


int main()
{
    tstamp ts;
    const char *patX = "4a53????41414141";
    const char *strX = "sd,mfns.d,mfns.ad,mfnas.,dmfnasd,.mfnas.,sadmnfas.d,mfnas.d,mfnJSabcdAAAA";
    const char *pat1 = "646f63756d656e742e777269746528273c494652414d45205352433d22687474703a2f2f{7-15}2f6c696e6b2e68746d6c22";
    const char *str1 = "abcdefghdocument.write(\'<IFRAME SRC=\"http://abcdefg/link/link.html\"abcd";
    const char *pat2 = "6869.6a*6b6c6d6e6f707172";
    const char *str2 = "hijhijklmnopqahijjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjklmnopqr";
    const char *pat3 = "646174613d606d732d6974733a6d68746d6c3a66696c653a2f2f(63|64|65|66|67|68)3a5c";
    const char *str3 = "123456789data=`ms-its:mhtml:file://c:\\";
    size_t patSize = 0;
    const char *pat = pat3;
    const char *str = str3;
    ts.start();
    patType_t *pats = processPattern(pat, &patSize);
    ts.stop();
    printf("process: %lld usec\n", ts.elapsed());
    int charsMatched = 0;
    ts.start();
    int result = matchPatterns(pats, patSize, str, &charsMatched);
    ts.stop();
    printf("match: %lld usec\n", ts.elapsed());
    if (result != -1)
    {
        printf("match found:: ");
        printPattern(pats, patSize);
        printf(" matched %s\nat index: %d. Chars Matched: %d\n", str, result, charsMatched);
    }
    else
    {
        printf("match not found:: ");
        printPattern(pats, patSize);
        printf(" didn't match %s \n", str);

    }
    freePattern(pats, patSize);

}
