#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "sqlite3.h"
#include "raylib.h"
#include "resource_dir.h"
#include "../nob.h"

typedef enum {
    ADD_ENTRY = 1,
    READ_ENTRY,
    EXIT
} Options;

typedef enum {
    SELECT_DATE = 0,
    SELECT_MONTH = 1,
} SQLStatement;

typedef struct {
    char** items;
    int capacity;
    int count;
    SQLStatement statement;
} Placeholder;

typedef struct {
    char** items;
    int capacity;
    int count;
} DatabaseHits;

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

char* get_todays_entry() 
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

int get_specific_date_db_result(sqlite3 *db, char** date, char** entry, char *sql)
{
    sqlite3_stmt *statement; 

    int rc = sqlite3_prepare_v2(db, sql, -1, &statement, 0);
    if (rc != SQLITE_OK) { // sqlite_ok = 0
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // bind entry to placeholder
    sqlite3_bind_text(statement, 1, *date, -1, SQLITE_STATIC);

    // execute and check for results
    rc = sqlite3_step(statement);
    if (rc == SQLITE_ROW) {
        // text column from result
        const char *content = (const char*)sqlite3_column_text(statement, 0);           
        *entry = strdup(content);    

        printf("Retrieved entry...\n"); 
        return 0;
    } 
    else if (rc == SQLITE_DONE) {
        printf("No rows found for date: %s\n", *date);
        *entry = NULL;        
        sqlite3_finalize(statement);
        return 1;
    } 
    else {
        fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(statement);
        return 1;
    }

    // release memory once finalized
    sqlite3_finalize(statement);
    return rc;
}

int get_specific_month_db_result(sqlite3 *db, char** date, DatabaseHits *db_hits, char* sql)
{
    sqlite3_stmt *statement; 
    int row_count = 0;

    // prepare statement
    int rc = sqlite3_prepare_v2(db, sql, -1, &statement, 0);
    if (rc != SQLITE_OK) { 
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // bind placeholder
    rc = sqlite3_bind_text(statement, 1, *date, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to bind placeholder: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(statement);
        return rc;
    }
    
    // step through rows
    while (sqlite3_step(statement) == SQLITE_ROW) {
        row_count++;           
        char *content = (char*)sqlite3_column_text(statement, 0);
        if (content) {
            // copy string
            size_t len = strlen(content) + 1;  // +1 for null terminator
            char *copy = malloc(len);
            
            if (copy) {
                memcpy(copy, content, len);                
                // Verify the copy worked
                // printf("Row %d: copying '%s' to db_hits\n", row_count, copy);
                nob_da_append(db_hits, copy);
            } else {
                fprintf(stderr, "Failed to allocate memory for row %d\n", row_count);
            }
        }
    }

    // finalize statement
    rc = sqlite3_finalize(statement);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error finalizing statement: %s\n", sqlite3_errmsg(db));
    }
    return rc;
}

// TODO:
int read_from_db(sqlite3 *db, char** date, char** entry, int statement_type, DatabaseHits *db_hits)
{
    // printf("%s\n", *date);       
    char *sql;   
    int rc; 

    switch (statement_type)
    {
    case SELECT_DATE:
        // statement for specific date
        sql = "select text from journal where DATE(timestamp) = ?";
        rc = get_specific_date_db_result(db, date, entry, sql);
        break;
    case SELECT_MONTH:        
        // search for month
        // statement = "select * from journal where timestamp like ...(?)...";  
        sql = "select text from journal where STRFTIME('%Y-%m', timestamp) = ?";
        rc = get_specific_month_db_result(db, date, db_hits, sql);
        break;
    default:
        printf("statement type not given\n");
        return 69;
        break;
    }
    return rc;
}

// construct the value for the timestamp first inside raylib based on user input
char* construct_date(Placeholder *placeholder) 
{
    char *return_string = malloc(sizeof(char) * placeholder->count + 1);
    if (!return_string){
        return NULL;
    }

    // memset(return_string, 0, placeholder->count + 1);

    for (int i = 0; i < placeholder->count; i++) {
        if (placeholder->items[i]) {
            return_string[i] = *(placeholder->items[i]); 
        }
    }

    // set the statement type depending on how many chars the user gave
    // 2025/04'\0' 8 chars
    if (placeholder->count > 8) {
        placeholder->statement = SELECT_DATE;
    } else {
        placeholder->statement = SELECT_MONTH;
    }

    return return_string;
}

void DrawTextWrapped(const char *text, Rectangle rec, int fontSize, float spacing, Color color) {
    if (!text) return;
    
    // copy text
    char *textCopy = strdup(text);
    if (!textCopy) return;
    
    // (current) drawing position
    int posY = rec.y;
    
    char *savePtr;
    char *line = strtok_r(textCopy, "\n", &savePtr);
    
    while (line != NULL && posY < rec.y + rec.height) {
        // if the whole line fits in the textarea width
        if (MeasureText(line, fontSize) <= rec.width) {
            DrawText(line, rec.x, posY, fontSize, color);
            posY += fontSize + spacing;
        } else {
            // line wrapping
            int startPos = 0;
            int textLen = strlen(line);
            char buffer[1024] = {0};
            
            while (startPos < textLen && posY < rec.y + rec.height) {
                int buffPos = 0;
                int lastSpace = -1;
                
                // fill buffer until it would exceed width
                for (int i = startPos; i < textLen; i++) {
                    buffer[buffPos++] = line[i];
                    buffer[buffPos] = '\0';
                    
                    // track spaces' position for word wrapping
                    if (line[i] == ' ') {
                        lastSpace = i;
                    }
                    
                    // check if buffer exceeds width
                    if (MeasureText(buffer, fontSize) > rec.width) {
                        if (lastSpace > startPos) {
                            // break at last space
                            buffer[buffPos - (i - lastSpace)] = '\0';
                            DrawText(buffer, rec.x, posY, fontSize, color);
                            startPos = lastSpace + 1; // skip space
                        } else if (startPos == i) {
                            // single character is already too wide - we have to break it
                            buffer[1] = '\0';
                            DrawText(buffer, rec.x, posY, fontSize, color);
                            startPos = i + 1;
                        } else {
                            // no space found but we're not at the start of a word
                            // yhis means the whole word doesn't fit, so go to next line
                            // without drawing anything
                            buffer[0] = '\0';
                            startPos = i; // try again from this position on the next line
                        }
                        break;
                    }
                    
                    // end of text reached
                    if (i == textLen - 1) {
                        DrawText(buffer, rec.x, posY, fontSize, color);
                        startPos = textLen;
                    }
                }
                
                posY += fontSize + spacing;
            }
        }
        
        // next line
        line = strtok_r(NULL, "\n", &savePtr);
    }
    
    free(textCopy);
}

// TODO: refactoring inside this function
void init_raylib(sqlite3 *db) 
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 800, "journal");
    SearchAndSetResourceDir("resources");

    // init empty dynamic arrays
    Placeholder placeholder = { 0 }; 
    DatabaseHits database_hits = { 0 };

    bool enterPressed = false;
    bool read_from_db_yet = false;
    char *entry = NULL;
    char *date = NULL;
    Rectangle textArea = {10, 100, GetScreenWidth() - 20, GetScreenHeight() - 20};
    bool db_result = false;    
    float blink_time = 0.0f;
    float blink_interval = 0.33f;
    bool blink_on = false;
    int statement_type;
    
    // raylib loop
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        float dt = GetFrameTime();

        // refactor this to seperate functions 
        int unicodeIntPressed = GetCharPressed();
        if (unicodeIntPressed > 0) {
            char* newChar = malloc(8); 
            if (newChar) {
                snprintf(newChar, 8, "%c", unicodeIntPressed);
                nob_da_append(&placeholder, newChar);
            }
        }

        // remove the last char added to items on keypress
        if (IsKeyPressed(KEY_BACKSPACE)) {            
            if (placeholder.count > 0) placeholder.count--;            
        }                     
        
        // user input
        if (!enterPressed) {
            DrawText("which date's entry would you like to view?", 0, 0, 60, RED); 
            DrawText("example: 2025-03-30", 0, 60, 60, RED);
            
            // draw blinking "> " to indicate to the user you can type something     
            blink_time += dt;

            if (blink_time >= blink_interval ) {
                blink_on = !blink_on;
                blink_time = 0.0f;
            }

            if (blink_on) DrawText(">", 275, 200, 60, RED);

            // draw the items in placeholder
            for (int i = 0; i < placeholder.count; i++) {
                if (placeholder.items[i]) DrawText(placeholder.items[i], 300 + (i * 50), 200, 60, RED);
            }
        }
        
        // construct a string from each individual item in the placeholder array
        date = construct_date(&placeholder);

        // save that string when enter is pressed and process it
        if (IsKeyPressed(KEY_ENTER)) {
            if (!enterPressed) enterPressed = !enterPressed;     
        }
        
        // upon enter press get the statement type
        if (enterPressed) {
            if (date) statement_type = placeholder.statement;     
        }   

        // process the date here, prevent infinite db reads with flag
        // Process the database query ONCE when conditions are met
        if (enterPressed && date && !read_from_db_yet) {
            // Set flag to prevent further DB reads in future frames
            read_from_db_yet = true;
            
            if (statement_type == SELECT_DATE) {
                if (read_from_db(db, &date, &entry, statement_type, &database_hits) == 0 && entry != NULL) {
                    db_result = true;
                }
            } else if (statement_type == SELECT_MONTH) {
                if (read_from_db(db, &date, &entry, statement_type, &database_hits) == 0 && database_hits.count > 0) {
                    db_result = true;
                }
            }
        }    
              
        // single result
        if (db_result && statement_type == SELECT_DATE) {
            DrawTextWrapped(entry, textArea, 30, 5, WHITE);
            DrawText("Press ESC to exit.\n", 400, 460, 60, WHITE);


        // multiple results
        // TODO: fix text wrapping function to work with input from dynamic array, add the date above as well 
        // or maybe prefix it and add scrolling support 
        } else if (db_result && statement_type == SELECT_MONTH) {
            for (int i = 0; i < database_hits.count; i++) {
                if (database_hits.items[i]) {
                    DrawText(database_hits.items[i], 10, 0 + (i * 100), 40, WHITE);
                }
            }
            // all the way at the bottom when done scrolling
            // DrawText("Press ESC to exit.\n", x, y, 60, WHITE);
        } else if (!db_result && read_from_db_yet) {
            DrawText("Cannot find this entry...\n", 400, 400, 60, RED);
            DrawText("Press ESC to exit.\n", 400, 460, 60, WHITE);
        }  

        EndDrawing();
    }   
    CloseWindow(); 
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
        printf("Choose an option... (press 1, 2, 3)\n");
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
            todays_entry = get_todays_entry();              
            add_entry(db, day, month, year, todays_entry);
            break;
        case (READ_ENTRY):      
            init_raylib(db);                   
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

