#ifndef PWFILE_H
#define PWFILE_H

/*Ryan Homsani
 *
 *This library is a simple library that stores values in rows and columns, like a
 *spreadsheet.
 *A PWFILE keeps track of all the column attributes in the file, while a PWRECORD file stores
 *the data of each row in the file.
 *We use PWF_create() to define a PWFILE and its columns.
 *
 *EXAMPLE:
 *PWFILE *player_data;
 *player_data = PWF_create("name STR 16, id INT, hp SHORT, mp SHORT, sex CHAR");
 *
 *In the formatted string argument, the name goes first then the data type. There is no
 *case sensitivity for column names and data types so "nAme StR 16" would behave
 *identically.
 *
 *Columns have no rules for what theyre called, but to be safe, use the same naming convention
 *that you use to name variables.
 *
 *There are six types: STR, INT, SHORT, CHAR, DOUBLE, FLOAT
 *With STR is a string data type, and its length needs to be defined.
 *
 *By convention, most of the functions return 0 upon success or -1 when something
 *failed. If the function returns a pointer, a null pointer is returned if something
 *failed. Use PWF_get_file_error() to check for errors. The functions give the reason
 *why they failed.
 */

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

typedef struct PWFILE{
	short num_cols;
	char **col_names;
	int *col_sizes;
	char *col_types;
	int rows_size;
	int rows_length;
	char **rows;
	char *err;
} PWFILE;

enum PWColType{
	PW_COL_TYPE_INT = 0,
	PW_COL_TYPE_STR,
	PW_COL_TYPE_CHAR,
	PW_COL_TYPE_SHORT,
	PW_COL_TYPE_FLOAT,
	PW_COL_TYPE_DOUBLE
};

/*this acts as an iterator*/
typedef struct PWRECORD{
	PWFILE *file;
	
	int num_rows;
	int current_row;
	int *rows;
} PWRECORD;

/* Returns a new instance of PWFILE. Expects a string in the following format:
 * variable_name TYPE variable_name TYPE...
 * 
 *
 * string represented as STR SIZE where SIZE is the string length
 * The string length must be greater than 4
 *
 * Example usage:
 * PWFILE *file = pwfile_create("name STR 16, hp INT, max_hp INT, mp INT, max_mp INT");
 *
 * If there is a syntax error in args, the function will return a null pointer or 0
 *
 */
PWFILE *PWF_create(const char *args);

/*returns a record which references the first row in the file
 *
 *will return an allocated address if the file has any entries
 *else will return null pointer
 */
PWRECORD *PWF_begin(PWFILE *file);

/*get the last record allocated in the file
 *will return an allocated address if the file has any entries
 *else will return null pointer
 */
PWRECORD *PWF_last(PWFILE *file);

/*get all records in the file that matches the criteria stated in args.
 *
 *ellipsis are used for actual values to compare the record columns to,
 *rather than to write it as a literal in the string. Use "..." for ellipsis
 *
 *example usage:
 *PWFILE *file = PWF_create("NAME STR 16, HP INT, MP INT");
 * //...
 *PWRECORD *record;
 *int mp = 10000;
 *record = PWF_find(file, "name == 'JohnOmok', mp >= ...", mp);
 *
 *returns a null pointer if there are no records that match the criteria
 */
PWRECORD *PWF_find(PWFILE *file, const char *args, ...);

/*navigation through a PWFILE, treating the record like an iterator.
 *record will point to its next record. Use PWF_end to make sure that
 *it points to a valid record*/
void PWF_next(PWRECORD *record);

/*returns 1 if the record doesn't point to any record.
 *returns 0 if the record points to a record.*/
int PWF_end(PWRECORD *record);

/*call when the record is no longer in use to free
 *the memory it is allocating.
 */
void PWF_free_record(PWRECORD *record);

/*Adds a record in the given file.
 *For each value to enter, put the name first, then the value. If you want to
 *use a variable value instead of a string literal, use an ellipsis and place
 *the variable in the next function argument.
 *
 *Example usage, using above file definition
 *PWF_add(file, "name 'MAP1ENOOBIE', hp 50, max_hp 50, mp ..., max_mp 5", mp);
 *
 *MUST make sure that the identifiers and types in the string are correct, else
 *PWF_add will not add the record.
 *
 *returns 0 when successful and -1 upon failure. Check for syntax errors in the argument.
 */
int PWF_add(PWFILE *file, const char *args, ...);

/*Edits the columns of a given record with the column names provided in col_names,
 *and assigns the columns with the following values passed into the function
 *
 *returns the number of columns assigned a value.
 */
int PWF_edit_record(PWRECORD *record, const char *col_names, ...);

/*Gets values from a record.
 *the arguments put in the ellipsis should be pointers to their types, with
 *the exception of char*
 *
 *returns 0 on success or -1 on failure.
 */
int PWF_get(PWRECORD *record, const char *col_names, ...);

/*removes the record that record points to in its pointer to PWFILE*/
void PWF_remove(PWRECORD *record); 

/*frees the memory used by the file*/
void PWF_close(PWFILE *file);

/*file io
 *will return null pointer if the file doesn't exist
 */
PWFILE *PWF_load(const char *filename);
/*saves to a file called filename*/
void PWF_save(PWFILE *file, const char *filename);
/*sorts PWFILE using the syntax:
 *<col_name> <ascending/descending>, <col_name2> <ascending/descending>, ...
 *returns 0 when there are no errors, else returns -1*/
int PWF_sort(PWFILE *file, const char *args);

const char *PWF_get_file_error(PWFILE *file);

/*DEPRECATED FUNCTIONS*/
/* Find a certain record in the database by matching the given value in a specific column
 *
 * Example usage:
 * PWRECORD *record = pw_find_by_str(file, "name", "JohnOmok");
 *
 * Returns a null pointer or 0 if there is no result
 *
 *NOW DEPRECATED BECAUSE pwfile_find IS NOW WORKING!!!
 */
PWRECORD *pw_find_by_str(PWFILE *file, const char *col_name, const char *where);
PWRECORD *pw_find_by_int(PWFILE *file, const char *col_name, int where);
/*change the values in the record (deprecated)*/
void pw_update_str(PWRECORD *record, const char *col_destination, const char *value);
void pw_update_int(PWRECORD *record, const char *col_destination, int value);
/*get the value in a record (deprecacted)*/
const char *pw_get_str(PWRECORD *record, const char *col_name);
int pw_get_int(PWRECORD *record, const char *col_name);
/*Deprecated as of April 18, 2019. Will be replaced with PWF_add*/
/*Adds a record to the file with values passed to the given column names
 *
 *example usage:
 *pwfile_add_record(file, "name max_hp max_mp", "Harute", 30000, 2000);
 *
 *returns 0 when successful and -1 upon failure
 */
int PWF_add_record(PWFILE *file, const char *col_names, ...);

/*Ryan Homsani
 *
 *Originally created on July 19, 2018
 *
 *October 1, 2018 - October 2, 2018
 *
 *Testing is needed for functions pwfile_add_record, pwfile_edit_record,
 *pwfile_get, pwfile_last, pwfile_find
 *
 *Later implement col_types in type PWFILE so there can be char, short, float, double
 *	-Doing so, the way that the program determines the type must change from determining whether the
 *   column has a size of 4
 *
 *TODO: convert string arguments to all uppercase to reduce ambiguity and the
 *tediousness of typing in CAPS for types
 *
 *functions modified to comply with above standard:
 *pwfile_create - types and caps
 *pwfile_add	- types and caps
 *pwfile_save	- types and caps
 *pwfile_add_record - types and caps
 *pwfile_edit_record - types and caps
 *pwfile_get	- types and caps
 *
 *October 6, 2018
 *completed all the above, EXCEPT pwfile_find
 *
 *October 9, 2018
 *Have not spent time on this library due to my increase in productivity in
 *coming up with how the M3PW online game will work. Would be VERY nice to
 *show to my high school physics teacher.
 *That aside, the next thing I'll implement is sorting records. Records will only
 *have to be sorted once if they aren't already yet. Add keyword KEY to the syntax
 *when creating the file:
 *pw_create("id INT KEY, name STR 16, hp SHORT, mp SHORT");
 *if there is the KEY after the identifier, the file will be sorted by that column.
 *
 *October 23, 2018
 *I am going to start working on the pwfile_find function, and sort function, and add function
 *for sorted items...:
 *syntax: PWRECORD *pwfile_find(PWFILE *file, const char *args, ...);
 *		  void      pwfile_sort(PWFILE *file, const char *col_names);
 *		  void      pwfile_add_sorted
 *
 *October 28, 2018
 *WHEW, I just finished implementing the pwfile_find function and this source
 *file sure is getting big...
 *It works but still should be debugged a bit just to be safe. Also, I'm 100%
 *sure that not all allocated memory in that funtion is freed so I'd better
 *look at that next time I edit this file.
 *
 *November 7, 2018
 *OK, pwfile_sort has been implemented. If time permits, maybe I should add an array type or something?
 *The only array there is is the string type, and that limits functionality. Oh,
 *the functions are 100% free from memory leaks.
 *
 *Some time in January/February 2019
 *There is a new member char *err in the PWFILE that records the error
 *made in the syntax of a function.
 *
 *April 17, 2019
 *Function names are now prefixed with PWF_ instead of pwfile_ just for brevity.
 *To make it easier to find errors in syntax of a function call, they
 *will now follow the convention of returning 0 on success and -1 on
 *failure.
 *
 *APRIL 20, 2019
 *PWRECORD.err has been removed as it will likely
 *cost too much computational power. All record errors
 *are directed to their respective PWFILE.
 *
 *July 26, 2019
 *Reworking the comments in this header file to be more readable by myself and
 *other people who might want to use this code.
 *
 *September 20, 2019
 *Added the break statement in PWF_compar() for the PW_COL_TYPE_INT case. For
 *some reason it wasn't there.
 */


#endif