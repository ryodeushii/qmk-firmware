/*
Copyright 2023 @ Nuphy <https://nuphy.com/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#define    BREATHE_TAB_LEN     128

#define    SIDE_BRIGHT_MAX     6
#define    SIDE_SPEED_MAX      4
#define    SIDE_COLOUR_MAX     10

#define    SIDE_LINE           5
#define    LEFT_SIDE           1
#define    RIGHT_SIDE          2
#define    SYS_MARK            10

#define    SIDE_BLINK_LIGHT    128
#define    SIDE_INDEX          100

#define    RF_LED_LINK_PERIOD  500
#define    RF_LED_PAIR_PERIOD  250

// #define    CAPS_LED            55
// #define    LSHIFT_LED          71


//----------------------------------------------------------------
//         breathe data table
//----------------------------------------------------------------

const uint8_t breathe_data_tab[BREATHE_TAB_LEN] =
{
    0,      1,      2,      3,      4,      5,      6,      7,
//    8,      9,      10,     12,     14,     16,     18,     20,
//    22,     24,     27,     30,     33,     36,     39,     42,
//    45,     49,     53,     57,     61,     65,     69,     73,
    8,      9,      10,     11,     12,     13,     14,     15,
    16,     17,     18,     19,     20,     21,     22,     23,
    25,     27,     29,     31,     33,     36,     39,     42,
    45,     49,     53,     57,     61,     65,     69,     73,
    77,     81,     85,     89,     94,     99,     104,    109,
    114,    119,    124,    129,    134,    140,    146,    152,
    158,    164,    170,    176,    182,    188,    194,    200,
    206,    213,    220,    227,    234,    241,    248,    255,

    255,    248,    241,    234,    227,    220,    213,    206,
    200,    194,    188,    182,    176,    170,    164,    158,
    152,    146,    140,    134,    129,    124,    119,    114,
    109,    104,    99,     94,     89,     85,     81,     77,
    73,     69,     65,     61,     57,     53,     49,     45,
    42,     39,     36,     33,     30,     27,     24,     22,
//    20,     18,     16,     14,     12,     10,     9,      8,
    7,      6,      5,      4,      3,      2,      1,      0,
};


const uint8_t colour_lib[SIDE_COLOUR_MAX][3] = 
{
    {RGB_RED},
    {RGB_ORANGE},
    {RGB_YELLOW},
    {RGB_GREEN},
    {RGB_SPRINGGREEN},
    {RGB_CYAN},
    {RGB_BLUE},
    {RGB_PURPLE},
    {RGB_MAGENTA},
    {RGB_WHITE}
};
