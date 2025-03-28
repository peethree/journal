# journal

#### diary that saves an entry into a txt file as well as a database.

<br>

##### usage:

##### For ease of use, I'd advice making a shell script (bash in my case) in /usr/local/bin/ that runs the program from its actual path and saving it as /usr/local/bin/journal so the program can be run simply by writing "journal" in the terminal.

##### Upon running the program, user is queried for an entry. The user can then type and once done typing, the program will ask to confirm if it should be saved to the journal.

##### Answer "y" to save, "n" to exit the program. Or you can cancel it by pressing ctrl + C.
##### Now also saves the entry into a sqlite3 db.

<br>

###### I will add more things to it if I feel it's necessary. Probably something to avoid copying the date if multiple entries were written on a single day. Maybe a command to read some of the entries would be nice too.