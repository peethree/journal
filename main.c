#include <time.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

// cc -o main main.c
// ./main
int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-o", "journal", "main.c");
    if (!nob_cmd_run_sync(cmd)) return 1;

    // get time
    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);
  
    // day/ month initialized at 0 so + 1, year at 1900
    int day = tm_info->tm_mday + 1;
    int month = tm_info->tm_mon + 1;    
    int year = tm_info->tm_year + 1900;
    
    printf("day: %d, month: %d, year: %d\n", day, month, year);

    return 0;      
}


// journal app in c

// get time (day / year)

// take user input for the day

// save all the input in a txt file