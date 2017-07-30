/*
    Initial author: Convery (tcn@ayria.se)
    Started: 30-07-2017
    License: MIT
    Notes:
        Provided as a hack for linux games.
*/

#include <stdio.h>

void Suspendprocess()
{
    printf("Hello world!\n");
}

__attribute__((constructor))
void Appmain()
{
    Suspendprocess();
}
