# journal

#### diary that saves an entry into a txt file as well as a database.

<br>

##### usage:

##### For ease of use, I'd advice making a shell script (bash in my case) in /usr/local/bin/ that runs the program from its actual path and saving it as /usr/local/bin/journal so the program can be run simply by writing "journal" in the terminal.

##### Upon running the program, user is met with a menu of choices inside the terminal. the user can either add an entry. If chosen, one can then type a message and once done writing (enter is pressed), the program will ask to confirm if it should be saved or not. Answer "y" to save, "n" to exit the program. Or cancel the program by pressing ctrl + C.

##### Now also saves the entry into a sqlite db.

##### Implemented Raylib to read text from the sqlite db. This can be done if the user has chosen for the 2nd menu option: "Read entry" (I know this is very dumb use case hehehe). If given input exists (in my case the db has id, text and timestamp fields (for now . . .)), the program will output the text inside the raylib window. This window can be exited by pressing 'ESC'.

##### The last menu option is exit, which does as it describes . . . 

##### ps. this leaks memory all over the place 8-)=

<br>
