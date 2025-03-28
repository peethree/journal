#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

int main(int argc, char** argv) {     

    // NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    cmd_append(&cmd, "cc", 
        "-Wall", 
        "-Wextra", 
        "-Wno-cast-function-type", 
        "-Wno-unused-variable", 
        "-Wno-implicit-fallthrough", 
        "-o", "journal", 
        "./src/main.c", 
        "./src/sqlite3.c", 
        "-lsqlite3");

    if (!cmd_run_sync(cmd)) {
        nob_log(NOB_ERROR, "Build failed");
        return 1;
    }    
    return 0;
}