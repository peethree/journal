#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "sqlite3.h"
#include "raylib.h"
#include "resource_dir.h"

typedef enum {
    ADD_ENTRY = 1,
    READ_ENTRY,
    EXIT
} Options;

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

    // bind entry to placeholder (1 in this case, if there were more placeholders -- the next would be 2 and so on)
    sqlite3_bind_text(statement, 1, todays_entry, -1, SQLITE_STATIC);

    // evaluate statement
    rc = sqlite3_step(statement);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    // release memory once finalized
    sqlite3_finalize(statement);
    return rc;
}

char* todays_entry_string() 
{    
    char* todays_entry = NULL;

    // if the buffer is not big enough, buffer_size will be increased by getline.
    size_t buffer_size = 256;
    printf("Write today's journal entry:\n");   
    printf("> "); 
    size_t characters_read = getline(&todays_entry, &buffer_size, stdin);
    // printf("%zu\n", characters_read);

    if (characters_read <= 1) {
        printf("No input given...\n"); 
        free(todays_entry);        
        return NULL;
    }

    return todays_entry;
}

void add_entry(sqlite3 *db, int day, int month, int year, char* todays_entry)
{
    if (todays_entry == NULL) {
        return;
    }

    // buffer for 1 char + null terminator  
    char answer[sizeof(char) + 1];

    while (1) {              
        printf("Add today's entry to your journal? y/n\n");
        printf("> ");

        if (fgets(answer, sizeof(answer), stdin) == NULL) {
            continue;
        }   
        
        // flush extra input if it's bigger than the buffer
        if (strchr(answer, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF); 
        }

        // remove newline from fgets answer to improve strcmp logic below
        char *newline = strchr(answer, '\n');
        if (newline) *newline = '\0';

        // if answer is yes, update journal
        if (strcmp(answer, "y") == 0) {
            update_journal(day, month, year, todays_entry);   
            printf("Adding entry to your journal\n");

            // add it to database as well
            insert_into_db(db, todays_entry);   
            printf("Updating database...\n");
            break;
        } else if (strcmp(answer, "n") == 0) {
            printf("Not adding your entry to your journal...\n");
            break;            
        } else {
            printf("Invalid input...\n");
        }        
    }  
}


int main() {    
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
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
  
    // month initialized at 0 so + 1, year at -1900
    int day = tm_info->tm_mday;
    int month = tm_info->tm_mon + 1;    
    int year = tm_info->tm_year + 1900;
    // printf("%d, %d, %d\n", day, month, year);

    // menu options
    char *option = NULL;
    size_t option_buffer = 8;
    char *todays_entry = NULL;
    bool exit = false;

    while (!exit) {
        printf("1. Add entry\n");
        printf("2. Read entry\n");
        printf("3. Exit\n");
        printf("Choose an option.\n");
        printf("> ");

        size_t options_read = getline(&option, &option_buffer, stdin);
        if (options_read < 1) {
            printf("Invalid input.\n");
            continue;
        }

        char *endptr;
        // convert user input to an integer to be compared against in the following switch statement
        int menu_option = strtol(option, &endptr, 10);        

        switch (menu_option) {
            case ADD_ENTRY:  
                todays_entry = todays_entry_string();              
                add_entry(db, day, month, year, todays_entry);
                break;
            case (READ_ENTRY):      
                // case read entry
                // Tell the window to use vsync and work on high DPI displays
                SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

                // Create the window and OpenGL context
                InitWindow(1280, 800, "Hello Raylib");

                // Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
                SearchAndSetResourceDir("resources");

                // Load a texture from the resources directory
                Texture wabbit = LoadTexture("wabbit_alpha.png");
                
                // game loop
                while (!WindowShouldClose())		// run the loop untill the user presses ESCAPE or presses the Close button on the window
                {
                    // drawing
                    BeginDrawing();

                    // Setup the back buffer for drawing (clear color and depth buffers)
                    ClearBackground(BLACK);

                    // draw some text using the default font
                    DrawText("Hello Raylib", 200,200,20,WHITE);

                    // draw our texture to the screen
                    DrawTexture(wabbit, 400, 200, WHITE);
                    
                    // end the frame and get ready for the next one  (display frame, poll input, etc...)
                    EndDrawing();
                }

                // cleanup
                // unload our texture so it can be cleaned up
                UnloadTexture(wabbit);

                // destroy the window and cleanup the OpenGL context
                CloseWindow();
                exit = true;      
                    
                break;
            case (EXIT):
                printf("Exiting...\n");
                exit = true;
                break;
            default:
                printf("Invalid option.\n");
                break;
        }
    }  
  


    printf("Closing database...\n");
    sqlite3_close(db); 

    free(option);
    free(todays_entry);
        
    return 0;       
}

