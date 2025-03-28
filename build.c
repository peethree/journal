#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

// cc -o build build.c
// avoids recompiling sqlite3 every time, instead uses static object file
int main(int argc, char** argv) {   
    // NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    cmd_append(&cmd, "cc", 
        "-Wall", 
        "-Wextra", 
        "-o", "journal", 
        "./src/main.c", 
        "./src/sqlite3.o",   
        "-lsqlite3");

    if (!cmd_run_sync(cmd)) {
        nob_log(NOB_ERROR, "Build failed");
        return 1;
    }    

    return 0;
}