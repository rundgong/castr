/*
    Copyright 2019-2021 rundgong

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <termios.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


static struct termios old_settings, new_settings;

/* Initialize new terminal i/o settings */
static void initTermios(int echo)
{
    tcgetattr(0, &old_settings); /* grab old terminal i/o settings */
    new_settings = old_settings; /* make new settings same as old settings */
    new_settings.c_lflag &= ~ICANON; /* disable buffered i/o */
    if (echo)
    {
        new_settings.c_lflag |= ECHO; /* set echo mode */
    }
    else
    {
        new_settings.c_lflag &= ~ECHO; /* set no echo mode */
    }
    tcsetattr(0, TCSANOW, &new_settings); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
static void resetTermios(void) 
{
    tcsetattr(0, TCSANOW, &old_settings);
}

/* Read 1 character - echo defines echo mode */
static char getch_(int echo) 
{
    char ch;
    initTermios(echo);
    ch = getchar();
    resetTermios();
    return ch;
}

char getch(void) 
{
    return getch_(0);
}

char getche(void) 
{
    return getch_(1);
}

#ifdef __cplusplus
}
#endif
