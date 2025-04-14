#include <stdio.h>
#include <ctype.h>

#define WORD_WIDTH 12
#define WORD_HEIGHT 7

// Number of letters to print before newline, adjust as needed 
#define WRAP_ON 10

struct LetterMap {
    char letter;
    char *strings[WORD_HEIGHT];
};

struct LetterMap letter_map[] = {
    { '!', { "   !!!   "
           , "   !!!   "
           , "   !!!   "
           , "   !!!   "
           , "   !!!   "
           , "         "
           , "   !!!   " }
    },
    { '"', { "   \"\"\"   "
           , "   \"\"\"   "
           , "         "
           , "         "
           , "         "
           , "         "
           , "         " }
    },
    { '#', { " ##   ## "
           , " ##   ## "
           , "#########"
           , " ##   ## "
           , "#########"
           , " ##   ## "
           , " ##   ## " }
    },
    { '$', { " $$$$$$$ "
           , "$$  $  $$"
           , "$$  $    "
           , " $$$$$$$ "
           , "    $  $$"
           , "$$  $  $$"
           , " $$$$$$$ " }
    },
    { '%', { "       %%"
           , " %%   %% "
           , "     %%  "
           , "   %%%   "
           , "  %%     "
           , " %%   %% "
           , "%%       " }
    },
    { '&', { "  &&&&&  "
           , " &     & "
           , "&&       "
           , "&  &   & "
           , " &   &&  "
           , "&   &  & "
           , " &&&    &" }
    },
    { '\'', { "   '''   "
            , "   '''   "
            , "         "
            , "         "
            , "         "
            , "         "
            , "         " }
    },
    { '(', { "    ((   "
           , "   ((    "
           , "  ((     "
           , "  ((     "
           , "  ((     "
           , "   ((    "
           , "    ((   " }
    },
    { ')', { "  ))     "
           , "   ))    "
           , "    ))   "
           , "    ))   "
           , "    ))   "
           , "   ))    "
           , "  ))     " }
    },
    { '*', { "         "
           , "         "
           , "   ***   "
           , "  *****  "
           , "   ***   "
           , "         "
           , "         " }
    },
    { '+', { "         "
           , "    +    "
           , "    +    "
           , "+++++++++"
           , "    +    "
           , "    +    "
           , "         " }
    },
    { ',', { "         "
           , "         "
           , "         "
           , "         "
           , "         "
           , "   ,,,   "
           , "  ,,,    " }
    },
    { '-', { "         "
           , "         "
           , "         "
           , "---------"
           , "         "
           , "         "
           , "         " }
    },
    { '.', { "         "
           , "         "
           , "         "
           , "         "
           , "         "
           , "   ...   "
           , "   ...   " }
    },
    { '/', { "       / "
           , "      /  "
           , "     /   "
           , "    /    "
           , "   /     "
           , "  /      "
           , " /       " }
    },
    { '0', { " 0000000 "
           , "00     00"
           , "00   0 00"
           , "00  0  00"
           , "00 0   00"
           , "00     00"
           , " 0000000 " }
    },
    { '1', { "    11   "
           , "   111   "
           , "  1 11   "
           , "    11   "
           , "    11   "
           , "    11   "
           , " 1111111 " }
    },
    { '2', { " 2222222 "
           , "22     22"
           , "       22"
           , "     22  "
           , "   22    "
           , " 22      "
           , "222222222" }
    },
    { '3', { " 3333333 "
           , "33     33"
           , "       33"
           , "    3333 "
           , "       33"
           , "33     33"
           , " 3333333 " }
    },
    { '4', { "44    44 "
           , "44    44 "
           , "44    44 "
           , "444444444"
           , "      44 "
           , "      44 "
           , "      44 " }
    },
    { '5', { "555555555"
           , "55       "
           , "55       "
           , "55555555 "
           , "       55"
           , "55     55"
           , " 5555555 " }
    },
    { '6', { "  666666 "
           , " 66      "
           , "66       "
           , "66666666 "
           , "66     66"
           , "66     66"
           , " 6666666 " }
    },
    { '7', { "777777777"
           , "      77 "
           , "     77  "
           , "    77   "
           , "   77    "
           , "  77     "
           , " 77      " }
    },
    { '8', { " 8888888 "
           , "88     88"
           , "88     88"
           , " 8888888 "
           , "88     88"
           , "88     88"
           , " 8888888 " }
    },
    { '9', { " 9999999 "
           , "99     99"
           , "99     99"
           , " 9999999 "
           , "      99 "
           , "     99  "
           , "   99    " }
    },
    { ':', { "   :::   "
           , "   :::   "
           , "         "
           , "         "
           , "         "
           , "   :::   "
           , "   :::   " }
    },
    { ';', { "   ;;;   "
           , "   ;;;   "
           , "         "
           , "         "
           , "         "
           , "   ;;;   "
           , "  ;;;    " }
    },
    { '<', { "         "
           , "     <<  "
           , "   <<    "
           , " <<      "
           , "   <<    "
           , "     <<  "
           , "         " }
    },
    { '=', { "         "
           , "         "
           , "========="
           , "         "
           , "========="
           , "         "
           , "         " }
    },
    { '>', { "         "
           , "  >>     "
           , "    >>   "
           , "      >> "
           , "    >>   "
           , "  >>     "
           , "         " }
    },
    { '?', { " ??????? "
           , "??     ??"
           , "       ??"
           , "     ??? "
           , "   ??    "
           , "         "
           , "   ??    " }
    },
    { '@', { " @@@@@@@ "
           , "@       @"
           , "@   @@@@@"
           , "@  @    @"
           , "@   @@@@@"
           , "@        "
           , " @@@@@@@ " }
    },
    { 'A', { "  AAAAA  "
           , " AA   AA "
           , "AA     AA"
           , "AAAAAAAAA"
           , "AA     AA"
           , "AA     AA"
           , "AA     AA" }
    },
    { 'B', { "BBBBBBBB "
           , "BB     BB"
           , "BB     BB"
           , "BBBBBBBB "
           , "BB     BB"
           , "BB     BB"
           , "BBBBBBBB " }
    },
    { 'C', { " CCCCCCC "
           , "CC     CC"
           , "CC       "
           , "CC       "
           , "CC       "
           , "CC     CC"
           , " CCCCCCC " }
    },
    { 'D', { "DDDDDDDD "
           , "DD     DD"
           , "DD     DD"
           , "DD     DD"
           , "DD     DD"
           , "DD     DD"
           , "DDDDDDDD " }
    },
    { 'E', { "EEEEEEEEE"
           , "EE       "
           , "EE       "
           , "EEEEEEEE "
           , "EE       "
           , "EE       "
           , "EEEEEEEEE" }
    },
    { 'F', { "FFFFFFFFF"
           , "FF       "
           , "FF       "
           , "FFFFFF   "
           , "FF       "
           , "FF       "
           , "FF       " }
    },
    { 'G', { " GGGGGGG "
           , "GG     GG"
           , "GG       "
           , "GG       "
           , "GG    GGG"
           , "GG     GG"
           , " GGGGGGG " }
    },
    { 'H', { "HH     HH"
           , "HH     HH"
           , "HH     HH"
           , "HHHHHHHHH"
           , "HH     HH"
           , "HH     HH"
           , "HH     HH" }
    },
    { 'I', { "IIIIIIIII"
           , "   III   "
           , "   III   "
           , "   III   "
           , "   III   "
           , "   III   "
           , "IIIIIIIII" }
    },
    { 'J', { "JJJJJJJJJ"
           , "    JJJ  "
           , "    JJJ  "
           , "    JJJ  "
           , "    JJJ  "
           , "JJ  JJJ  "
           , " JJJJJ   " }
    },
    { 'K', { "KK     KK"
           , "KK     KK"
           , "KK    KK "
           , "KKKKKK   "
           , "KK    KK "
           , "KK     KK"
           , "KK     KK" }
    },
    { 'L', { "LL       "
           , "LL       "
           , "LL       "
           , "LL       "
           , "LL       "
           , "LL       "
           , "LLLLLLLLL" }
    },
    { 'M', { "MM     MM"
           , "MMM   MMM"
           , "MM MMM MM"
           , "MM  M  MM"
           , "MM     MM"
           , "MM     MM"
           , "MM     MM" }
    },
    { 'N', { "NN     NN"
           , "NNN    NN"
           , "NN N   NN"
           , "NN  N  NN"
           , "NN   N NN"
           , "NN    NNN"
           , "NN     NN" }
    },
    { 'O', { " OOOOOOO "
           , "OO     OO"
           , "OO     OO"
           , "OO     OO"
           , "OO     OO"
           , "OO     OO"
           , " OOOOOOO " }
    },
    { 'P', { "PPPPPPPP "
           , "PP     PP"
           , "PP     PP"
           , "PPPPPPPP "
           , "PP       "
           , "PP       "
           , "PP       " }
    },
    { 'Q', { " QQQQQQQ "
           , "QQ     QQ"
           , "QQ     QQ"
           , "QQ     QQ"
           , "QQ     QQ"
           , " QQQQQQQ "
           , "        Q" }
    },
    { 'R', { "RRRRRRRR "
           , "RR     RR"
           , "RR     RR"
           , "RRRRRRRR "
           , "RR   RR  "
           , "RR    RR "
           , "RR     RR" }
    },
    { 'S', { " SSSSSSS "
           , "SS     SS"
           , "SS       "
           , " SSSSSSS "
           , "       SS"
           , "SS     SS"
           , " SSSSSSS " }
    },
    { 'T', { "TTTTTTTTT"
           , "   TTT   "
           , "   TTT   "
           , "   TTT   "
           , "   TTT   "
           , "   TTT   "
           , "   TTT   " }
    },
    { 'U', { "UU     UU"
           , "UU     UU"
           , "UU     UU"
           , "UU     UU"
           , "UU     UU"
           , "UU     UU"
           , " UUUUUUU " }
    },
    { 'V', { "VV     VV"
           , "VV     VV"
           , "VV     VV"
           , " VV   VV "
           , " VV   VV "
           , "  VV VV  "
           , "   VVV   " }
    },
    { 'W', { "WW     WW"
           , "WW     WW"
           , "WW     WW"
           , "WW  W  WW"
           , "WW WWW WW"
           , "WWW   WWW"
           , "WW     WW" }
    },
    { 'X', { "XX     XX"
           , " XX   XX "
           , "  XX XX  "
           , "   XXX   "
           , "  XX XX  "
           , " XX   XX "
           , "XX     XX" }
    },
    { 'Y', { "YY     YY"
           , " YY   YY "
           , "  YY YY  "
           , "   YYY   "
           , "   YYY   "
           , "   YYY   "
           , "   YYY   " }
    },
    { 'Z', { "ZZZZZZZZZ"
           , "     ZZ  "
           , "    ZZ   "
           , "   ZZ    "
           , "  ZZ     "
           , " ZZ      "
           , "ZZZZZZZZZ" }
    },
    { '[', { "   [[[[[ "
           , "   [[[   "
           , "   [[[   "
           , "   [[[   "
           , "   [[[   "
           , "   [[[   "
           , "   [[[[[ " }
    },
    { '\\', { " \\       "
            , "  \\      "
            , "   \\     "
            , "    \\    "
            , "     \\   "
            , "      \\  "
            , "       \\ " }
    },
    { ']', { " ]]]]]   "
           , "   ]]]   "
           , "   ]]]   "
           , "   ]]]   "
           , "   ]]]   "
           , "   ]]]   "
           , " ]]]]]   " }
    },
    { '^', { "    ^    "
           , "   ^ ^   "
           , "  ^   ^  "
           , " ^     ^ "
           , "         "
           , "         "
           , "         " }
    },
    { '_', { "         "
           , "         "
           , "         "
           , "         "
           , "         "
           , "         "
           , "_________" }
    },
    { '`', { "   ``    "
           , "    ``   "
           , "         "
           , "         "
           , "         "
           , "         "
           , "         " }
    },
    { '{', { "    {    "
           , "    {    "
           , "    {    "
           , "   {     "
           , "    {    "
           , "    {    "
           , "    {    " }
    },
    { '|', { "    |    "
           , "    |    "
           , "    |    "
           , "    |    "
           , "    |    "
           , "    |    "
           , "    |    " }
    },
    { '}', { "    }    "
           , "    }    "
           , "    }    "
           , "     }   "
           , "    }    "
           , "    }    "
           , "    }    " }
    },
    { '~', { "         "
           , "         "
           , " ~~~~   ~"
           , "~   ~~~~ "
           , "         "
           , "         "
           , "         " }
    },
    { ' ', { "         "
           , "         "
           , "         "
           , "         "
           , "         "
           , "         "
           , "         " }
    },
    {'\a', { "\a",  "\a",  "\a",  "\a",  "\a",  "\a",  "\a" } },
};

char *bad_letter[] = { "         "
                     , "    �    "
                     , "  �   �  "
                     , "�   ?   �"
                     , "  �   �  "
                     , "    �    "
                     , "         " };

int letter_map_size = sizeof(letter_map) / sizeof(*letter_map);

void print_big_lookup(char c, int line)
{
    char cased = toupper(c);
    for (int i = 0; i < letter_map_size; i++) {
        if (cased == letter_map[i].letter) {
            printf("%s ", letter_map[i].strings[line]);
            return;
        }
    }
    printf("%s ", bad_letter[line]);
}

void print_big(char *input)
{
    printf("\n");
    char *init_input = input;
    int len = strlen(input);

    for (int i = 0; i <= len / WRAP_ON; i++) {
        for (int j = 0; j < WORD_HEIGHT; j++) {
            input = init_input;
            for (int k = i * WRAP_ON; input[k] != '\0' && k < (i + 1) * WRAP_ON; k++) {
                print_big_lookup(input[k], j);
            }
            printf("\n");
        }
        if (i + 1 <= len / WRAP_ON) printf("\n");
    }
}

