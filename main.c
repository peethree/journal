// cc -o journal main.c sqlite3.c -lsqlite3 -Wall -Wextra -Wno-cast-function-type -Wno-unused-variable -Wno-implicit-fallthrough -Wno-unused-but-set-variable
#include <time.h>
#include <stdio.h>
#include "sqlite3.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// #define NOB_IMPLEMENTATION
// #define NOB_STRIP_PREFIX
// #include "nob.h"

void update_journal(int day, int month, int year, char entry[]) 
{
    // open file in append mode, create it if it doesn't exist
    FILE *file = fopen("journal.txt", "a");
    
    if (file == NULL) {
        printf("Error opening journal file\n");
        return;
    }
    
    // write date and entry to the file
    fprintf(file, "%02d/%02d/%04d\n%s\n", day, month, year, entry);    
    fclose(file);
}

int insert_into_db(sqlite3 *db, char* todays_entry)
{
    // sql statement
    sqlite3_stmt *statement;
    char *sql = "insert into journal (text) values (?)";

    int rc = sqlite3_prepare_v2(db, sql, -1, &statement, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // bind date to placeholder (1)
    sqlite3_bind_text(statement, 1, todays_entry, -1, SQLITE_STATIC);

    // evaluate statement
    rc = sqlite3_step(statement);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    // release memorty once finalized
    sqlite3_finalize(statement);

    return rc
}

void cleanup(char* todays_entry, sqlite3 *db) 
{
    free(todays_entry);
    sqlite3_close(db);
    printf("Exiting journal...\n");
    printf("Closing database...\n");
}

int main() {   
    // nob building script (doesn't seem to work with multiple source files) but it was quite convenient for only 1

    // NOB_GO_REBUILD_URSELF(argc, argv);
    // Cmd cmd = {0};
    // cmd_append(&cmd, "cc", 
    //     "-Wall", 
    //     "-Wextra", 
    //     "-Wno-cast-function-type", 
    //     "-Wno-unused-variable", 
    //     "-Wno-implicit-fallthrough", 
    //     "-o", "journal", 
    //     "main.c", 
    //     "sqlite3.c", 
    //     "-lsqlite3");

    // if (!cmd_run_sync(cmd)) {
    //     nob_log(NOB_ERROR, "Build failed");
    //     return 1;
    // }    
    
    // db
    sqlite3 *db;
    const char* db_name = "test.db";
    int rc = sqlite3_open(db_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    printf("Opened db successfully...\n");    

    // get time
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);
  
    // month initialized at 0 so + 1, year at -1900
    int day = tm_info->tm_mday;
    int month = tm_info->tm_mon + 1;    
    int year = tm_info->tm_year + 1900;

    // dynamic buffer for user input
    char *todays_entry = NULL;
    size_t buffer_size = 0;
    printf("Write today's journal entry:\n");   
    size_t characters_read = getline(&todays_entry, &buffer_size, stdin);
    printf("%zu\n", characters_read);
 
    if (characters_read <= 1) {
        printf("No input given...\n"); 
        cleanup(todays_entry, db);        
        return 1;
    }

    // buffer for 1 char + null terminator
    char answer[sizeof(char) + 1];
    printf("Add today's entry to your journal? y/n\n");
    scanf("%s", answer);
    char *yes_answer = "y";

    // if answer is yes, update journal
    if (strcmp(answer, yes_answer) == 0) {
        update_journal(day, month, year, todays_entry);   
        printf("Adding entry to your journal\n"); 
        // add it to database as well
        insert_into_db(db, todays_entry);   
        printf("Updating database...");
    } 

    cleanup(todays_entry, db);     
    return 0;       
}

