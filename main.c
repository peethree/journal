#include <time.h>
#include <stdio.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

bool FileExists(char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;  
    }
    // file doesn't exist
    return false;  
}

void update_journal(int day, int month, int year, char entry[])
{
    // make sure file exists, make it otherwise
    if (!FileExists("journal.txt")) {
        printf("Journal doesn't exist.\n");
        printf("Creating new journal file...\n");

        FILE *file = fopen("journal.txt", "w");
        fprintf(file, "%02d/%02d/%04d\n", day, month, year);
        fprintf(file, "%s\n", entry);
        fclose(file); 
    } else {
        FILE *file = fopen("journal.txt", "a"); 
        fprintf(file, "%02d/%02d/%04d\n", day, month, year);
        fprintf(file, "%s\n", entry);
        fclose(file);  
    }
}

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

    // printf("day: %d, month: %d, year: %d\n", day, month, year);

    // buffer size
    char todays_entry[1000];
    printf("Write today's journal entry\n");   
    // get user input 
    fgets(todays_entry, sizeof(todays_entry), stdin);  

    char answer[sizeof(char*)];
    printf("Add today's entry to your journal? y/n\n");
    scanf("%s", answer);
    char *yes_answer = "y";

    // if answer is yes, update journal
    if (strcmp(answer, yes_answer) == 0) {
        update_journal(day, month, year, todays_entry);   
        printf("Adding entry to your journal\n");     
    } else {
        printf("Exiting journal\n");
        return 1;
    }        
    printf("Exiting journal\n");
    return 0;       
}