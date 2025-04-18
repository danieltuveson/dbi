#include <stdio.h>
#include "dbi.h"

int main(int argc, char *argv[])
{
    if (argc > 2) {
        printf("Error: too many arguments");
        return 1;
    } else if (argc == 2) {
        return dbi_run_file(argv[1]);
    }
    return dbi_run_file(NULL);
}

