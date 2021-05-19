#include "pwfile.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int ROWS_START_SIZE = 16;

/*parses a string literal pointed to by word and copies up to n-1 characters to record_str
 *
 *format of word:
 *"'Good evening!' ..."
 *
 *returns a string pointing to right after the closing quote. If word doesn't point to a string literal,
 *NULL is returned.
 */
char *PWF_parse_str(char *word, char *record_str, int n);
/*returns the length of the characters pointed to in str which follow
 *the rules of being an identifier (same rules as in C)
 8
 *example: PWF_get_word_length("foo+5"); //returns 3, foo contains 3 letters
 */
int PWF_get_word_length(const char *str);
/*returns number of whitespace until either the termination character or
 *non whitespace character*/
int PWF_get_whitespace_length(const char *str);
/*returns the number of characters which make up a numeric (integer or floating point)
 *literal.*/
int PWF_get_numeric_literal_length(const char *str);
/*returns the number of characters which make up a character literal.
 *i.e. the number of characters until the first character after the closing quote.
 *returns 0 if there is no literal detected.
 */
int PWF_get_char_literal_length(const char *str);
/*returns the number of non-whitespace characters until the next
 *whitespace or termination character.
 */

/*parses a char literal from word and saves it to record_char.
 *if no literal is found, no value is saved to record_char
 */
char *PWF_parse_char(char *word, char *record_char);
/*returns the length of a string until one of the delimiter character
 *or null character*/
int PWF_get_length_until(const char *str, const char *delimiters);

int PWF_compar(const void *data1, const void *data2);
int PWF_compar_num_col_compar = 0;
int PWF_compar_array_size = 0;
int *PWF_compar_col_offsets = 0;
char *PWF_compar_is_asc = 0;
char *PWF_compar_col_types = 0;

PWFILE *PWF_create(const char *args){
	int string_length;
	char *args_copy;
	int num_cols = 0;
	char *word = 0;
	int i;
	PWFILE *file = 0;
	
	string_length = strlen(args) + 1;
	args_copy = (char*) malloc(string_length);

	/*copy arguments and convert to uppercase*/
	strcpy(args_copy, args);
	for(i = 0; args_copy[i]; ++i){
		args_copy[i] = toupper(args_copy[i]);
	}
	
	file = (PWFILE*) malloc(sizeof(PWFILE));
	memset(file, 0, sizeof(PWFILE));
	
	/*figure out how many columns each record has*/
	
	word = strtok(args_copy, ", \n");
	while(word){
		/*expect the words in this order: name INT
		 *							or:	  name STR SIZE*/
		 
		num_cols++;
		
		word = strtok(0, ", \n");
		if(strcmp(word, "INT") == 0){
			/*do nothing and continue to the next word*/
		}
		else if(strcmp(word, "STR") == 0){
			/*skip the integer expected then go to the next word*/
			word = strtok(0, " \n");
			
			if(!word || atoi(word) < 2){
				/*end reading and force the function to return in the bottom condition*/
				num_cols = 0;
				break;
			}
		}
		else if(strcmp(word, "CHAR") == 0){
			
		}
		else if(strcmp(word, "SHORT") == 0){
			
		}
		else if(strcmp(word, "FLOAT") == 0){
			
		}
		else if(strcmp(word, "DOUBLE") == 0){
			
		}
		else{
			/*end reading and force the function to return*/
			num_cols = 0;
			break;
		}
		
		/*this part reads the name of the next column name, if any*/
		word = strtok(0, ", \n");
	}
	
	
	if(num_cols == 0){
		free(file);
		free(args_copy);
		return 0;
	}
	
	/*allocate memory*/
	file->num_cols = num_cols;
	file->col_names = (char**) malloc(sizeof(char*) * num_cols);
	file->col_sizes = (int*) malloc(sizeof(int) * num_cols);
	file->col_types = (char*) malloc(num_cols); /*NOT a string*/
	file->err = (char*) malloc(256); file->err[0] = 0;
	
	/*this time read the string's arguments*/
	word = 0;
	strcpy(args_copy, args);
	for(i = 0; args_copy[i]; ++i){
		args_copy[i] = toupper(args_copy[i]);
	}
	
	word = strtok(args_copy, ", \n");
	for(i = 0; i < num_cols; ++i){
		file->col_names[i] = (char*) malloc(strlen(word) + 1);
		strcpy(file->col_names[i], word);
		
		/*read the name of the type*/
		word = strtok(0, ", \n");
		
		if(strcmp(word, "INT") == 0){
			/*make the column size 4*/
			file->col_sizes[i] = 4;
			file->col_types[i] = PW_COL_TYPE_INT;
		}
		else if(strcmp(word, "STR") == 0){
			/*make the column size the next word*/
			word = strtok(0, " \n");
			file->col_sizes[i] = atoi(word);
			file->col_types[i] = PW_COL_TYPE_STR;
		}
		else if(strcmp(word, "CHAR") == 0){
			file->col_sizes[i] = 1;
			file->col_types[i] = PW_COL_TYPE_CHAR;
		}
		else if(strcmp(word, "SHORT") == 0){
			file->col_sizes[i] = 2;
			file->col_types[i] = PW_COL_TYPE_SHORT;
		}
		else if(strcmp(word, "FLOAT") == 0){
			file->col_sizes[i] = 4;
			file->col_types[i] = PW_COL_TYPE_FLOAT;
		}
		else if(strcmp(word, "DOUBLE") == 0){
			file->col_sizes[i] = 8;
			file->col_types[i] = PW_COL_TYPE_DOUBLE;
		}
		
		/*read the name of the next column, if any*/
		word = strtok(0, ", \n");
	}
	
	file->rows_size = 0;
	file->rows_length = 0;
	file->rows = 0;
	
	/*done using that string*/
	free(args_copy);
	
	return file;
}

int PWF_add(PWFILE *file, const char *args, ...){
	int record_length = 0;
	int i;
	char **new_rows = 0;	/*used to resize the file row array*/
	char *word = 0;			/*copies one word at a time from args_copy*/
	char *args_copy = 0;	/*copy of args, made all uppercase*/
	int col_offset = 0;		/*offset of column assigned in the record*/
	int num_cols_assigned = 0;
	char *record_str;
	
	int placeholder_int;
	double placeholder_double;
	int sscanf_result;
	va_list list;
	
	if(!file){
		return -1;
	}
	if(file->num_cols == 0){
		return -1;
	}
	
	va_start(list, args);
	
	for(i = 0; i < file->num_cols; ++i){
		record_length += file->col_sizes[i];
	}
	
	/*check if there is space left on the rows*/
	if(file->rows_size == 0){
		/*extend the rows size to the default start size*/
		file->rows_size = ROWS_START_SIZE;
		
		file->rows = (char**) malloc(sizeof(char*) * ROWS_START_SIZE);
		
		for(i = 0; i < file->rows_size; ++i){
			file->rows[i] = (char*) malloc(record_length);
			memset(file->rows[i], 0, record_length);
		}
	}
	else if(file->rows_length >= file->rows_size){
		/*extend the rows size to double the length*/
		new_rows = (char**) malloc(sizeof(char*) * file->rows_size * 2);
		
		/*move the old rows to the new array*/
		for(i = 0; i < file->rows_size; ++i){
			new_rows[i] = file->rows[i];
		}
		
		free(file->rows);
		
		for(i = file->rows_size; i < file->rows_size * 2; ++i){
			/*allocate memory for the new rows*/
			new_rows[i] = (char*) malloc(record_length);
			memset(new_rows[i], 0, record_length);		
		}
		
		file->rows = new_rows;
		
		file->rows_size *= 2;
	}
	
	/*now begin copying the string arguments to the next record*/
	args_copy = (char*) malloc(strlen(args) + 1);
	strcpy(args_copy, args);
	
	/*copy one word at a time*/
	word = strtok(args_copy, ", \n");
	
	while(word){
		col_offset = 0;
		
		/*make identifier all uppercase*/
		for(i = 0; word[i]; ++i){
			word[i] = toupper(word[i]);
		}
		
		/*expect identifier for first word*/
		for(i = 0; i < file->num_cols; ++i){
			if(strcmp(word, file->col_names[i]) == 0){
				break;
			}
			else{
				col_offset += file->col_sizes[i];
			}
		}
		
		/*if the word doesnt match any column identifier*/
		if(col_offset >= record_length){
			sprintf(file->err, "[PWF_add] Error: No column name in file matches \"%s\".", word);
			num_cols_assigned = -1;
			break;
		}
		
		/*expect value for second word, but don't use space for delimiting value yet*/
		word = strtok(0, "\n");
		
		if(!word){
			sprintf(file->err, "[PWF_add] Error: No value assigned to column \"%s\".", file->col_names[i]);
			num_cols_assigned = -1;
			break;
		}
		
		word += PWF_get_whitespace_length(word);
		
		/*if an argument is passed through ellipsis, copy ellipsis value to column*/
		if(strcspn(word, ", \n") == 3 && strncmp(word, "...", 3) == 0){
			switch(file->col_types[i]){
			case PW_COL_TYPE_CHAR:
				*(char*)(file->rows[file->rows_length] + col_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_SHORT:
				*(short*)(file->rows[file->rows_length] + col_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_INT:
				*(int*)(file->rows[file->rows_length] + col_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_FLOAT:
				*(float*)(file->rows[file->rows_length] + col_offset) = va_arg(list, double);
				break;
			case PW_COL_TYPE_DOUBLE:
				*(double*)(file->rows[file->rows_length] + col_offset) = va_arg(list, double);
				break;
			case PW_COL_TYPE_STR:
				record_str = (char*)(file->rows[file->rows_length] + col_offset);
				strncpy(record_str, va_arg(list, const char*), file->col_sizes[i] - 1);
				break;
			}
			
			/*separate the char literal from the whole string*/
			strtok(word, ", ");
			/*move to the next word*/
			word = strtok(0, ", \n");
		}
		
		/*else copy literal value in string to column*/
		else{
			if(file->col_types[i] == PW_COL_TYPE_CHAR){
				/*expect either single quoted character or integer*/
				/*single quote literal case*/
				if(PWF_get_char_literal_length(word) > 0){
					word = PWF_parse_char(word, (char*)(file->rows[file->rows_length] + col_offset));
					/*if the literal was not read correctly*/
					if(!word){
						sprintf(file->err, "[PWF_add] Error: Character literal syntax error for column \"%s\".", file->col_names[i]);
						num_cols_assigned = -1;
						break;
					}
				}
				/*numeric literal case*/
				else{
					sscanf_result = sscanf(word, "%d", &placeholder_int);
					/*if the literal could not be read*/
					if(sscanf_result != 1){
						sprintf(file->err, "[PWF_add] Error: Could not read value assigned for column \"%s\".", file->col_names[i]);
						num_cols_assigned = -1;
						break;
					}
					*(char*)(file->rows[file->rows_length] + col_offset) = placeholder_int;
				}
				/*separate the char literal from the whole string*/
				word += PWF_get_length_until(word, ", \n");
				word = strtok(word, ", \n");
			}
			else if(file->col_types[i] == PW_COL_TYPE_SHORT){
				/*expect a plain number*/
				sscanf_result = sscanf(word, "%d", &placeholder_int);
				if(sscanf_result != 1){
					sprintf(file->err, "[PWF_add] Error: Could not read value assigned for column \"%s\".", file->col_names[i]);
					num_cols_assigned = -1;
					break;
				}
				
				*(short*)(file->rows[file->rows_length] + col_offset) = placeholder_int;
				strtok(word, ", \n");
				word = strtok(0, ", \n");
			}
			else if(file->col_types[i] == PW_COL_TYPE_INT){
				sscanf_result = sscanf(word, "%d", &placeholder_int);
				if(sscanf_result != 1){
					sprintf(file->err, "[PWF_add] Error: Could not read value assigned for column \"%s\".", file->col_names[i]);
					num_cols_assigned = -1;
					break;
				}
				
				*(int*)(file->rows[file->rows_length] + col_offset) = placeholder_int;
				strtok(word, ", \n");
				word = strtok(0, ", \n");
			}
			else if(file->col_types[i] == PW_COL_TYPE_FLOAT){
				sscanf_result = sscanf(word, "%lf", &placeholder_double);
				if(sscanf_result != 1){
					sprintf(file->err, "[PWF_add] Error: Could not read value assigned for column \"%s\".", file->col_names[i]);
					num_cols_assigned = -1;
					break;
				}
				
				*(float*)(file->rows[file->rows_length] + col_offset) = placeholder_double;
				strtok(word, ", \n");
				word = strtok(0, ", \n");
			}
			else if(file->col_types[i] == PW_COL_TYPE_DOUBLE){
				sscanf_result = sscanf(word, "%lf", &placeholder_double);
				if(sscanf_result != 1){
					sprintf(file->err, "[PWF_add] Error: Could not read value assigned for column \"%s\".", file->col_names[i]);
					num_cols_assigned = -1;
					break;
				}
				
				*(double*)(file->rows[file->rows_length] + col_offset) = placeholder_double;
				strtok(word, ", \n");
				word = strtok(0, ", \n");
			}
			else if(file->col_types[i] == PW_COL_TYPE_STR){
				record_str = (char*)(file->rows[file->rows_length] + col_offset);
				word = PWF_parse_str(word, record_str, file->col_sizes[i]);
				
				if(!word){
					sprintf(file->err, "[PWF_add] Error: Opening/Closing quote not found for column \"%s\".", file->col_names[i]);
					num_cols_assigned = -1;
					break;
				}
				
				word = strtok(word, ", \n");
			}
		}
		
		num_cols_assigned++;
	}
	
	free(args_copy);
	va_end(list);
	
	if(num_cols_assigned == -1){
		return -1;
	}
	else{
		file->rows_length++;
		return 0;
	}
}

PWRECORD *PWF_begin(PWFILE *file){
	PWRECORD *record = 0;
	int i;
	
	if(!file){
		return 0;
	}
	
	if(file->rows_length == 0){
		return 0;
	}
	
	record = (PWRECORD*) malloc(sizeof(PWRECORD));
	
	record->file = file;
	record->num_rows = file->rows_length;
	record->current_row = 0;
	record->rows = (int*) malloc(sizeof(int) * file->rows_length);
	
	for(i = 0; i < file->rows_length; ++i){
		record->rows[i] = i;
	}
	
	return record;
}

PWRECORD *PWF_last(PWFILE *file){
	PWRECORD *record = 0;
	
	if(!file){
		return 0;
	}
	
	if(file->rows_length == 0){
		return 0;
	}
	
	record = (PWRECORD*) malloc(sizeof(PWRECORD));
	
	record->file = file;
	record->num_rows = 1;
	record->current_row = 0;
	record->rows = (int*) malloc(sizeof(int));
	
	record->rows[0] = file->rows_length - 1;
	
	return record;
}

void PWF_next(PWRECORD *record){
	record->current_row += 1;
}

int PWF_end(PWRECORD *record){
	if(!record || record->current_row >= record->num_rows){
		return 1;
	}
	return 0;
}

void PWF_free_record(PWRECORD *record){
	if(record){
		free(record->rows);
		free(record);
	}
}

void pw_update_str(PWRECORD *record, const char *col_destination, const char *value){
	PWFILE *file = record->file;
	
	/*determine which column the col_name corresponds to and what the offset
	 *is from the beginning of the record*/
	int col_number = -1;
	int col_offset = 0;
	int i;
	int row_number;
	
	for(i = 0; i < file->num_cols; ++i){
		if(strcmp(col_destination, file->col_names[i]) == 0){
			col_number = i;
			break;
		}
	}
	
	if(col_number == -1){
		return;
	}
	
	for(i = 0; i < col_number; ++i){
		col_offset += file->col_sizes[i];
	}
	
	row_number = record->rows[record->current_row];
	
	/*if the string value is too long for the file column, truncate*/
	if(strlen(value) + 1 >= file->col_sizes[col_number]){
		memcpy(file->rows[row_number], value, file->col_sizes[col_number] - 1);
	}
	else{
		strcpy(file->rows[row_number], value);
	}
}
	
void pw_update_int(PWRECORD *record, const char *col_destination, int value){
	PWFILE *file = record->file;
	
	/*determine which column the col_name corresponds to and what the offset
	 *is from the beginning of the record*/
	int col_number = -1;
	int col_offset = 0;
	int i;
	int row_number;
	
	for(i = 0; i < file->num_cols; ++i){
		if(strcmp(col_destination, file->col_names[i]) == 0){
			col_number = i;
			break;
		}
	}
	
	if(col_number == -1){
		return;
	}
	
	for(i = 0; i < col_number; ++i){
		col_offset += file->col_sizes[i];
	}
	
	row_number = record->rows[record->current_row];
	
	memcpy(file->rows[row_number] + col_offset, &value, sizeof(int));
}
	
const char *pw_get_str(PWRECORD *record, const char *col_name){
	PWFILE *file = record->file;
	
	/*determine which column the col_name corresponds to and what the offset
	 *is from the beginning of the record*/
	int col_number = -1;
	int col_offset = 0;
	int i;
	int row_number;
	
	for(i = 0; i < file->num_cols; ++i){
		if(strcmp(col_name, file->col_names[i]) == 0){
			col_number = i;
			break;
		}
	}
	
	if(col_number == -1){
		return "";
	}
	
	for(i = 0; i < col_number; ++i){
		col_offset += file->col_sizes[i];
	}
	
	row_number = record->rows[record->current_row];
	
	return file->rows[row_number] + col_offset;
}
		
int pw_get_int(PWRECORD *record, const char *col_name){
	PWFILE *file = record->file;
	
	/*determine which column the col_name corresponds to and what the offset
	 *is from the beginning of the record*/
	int col_number = -1;
	int col_offset = 0;
	int i;
	int row_number;
	
	for(i = 0; i < file->num_cols; ++i){
		if(strcmp(col_name, file->col_names[i]) == 0){
			col_number = i;
			break;
		}
	}
	
	if(col_number == -1){
		return 0;
	}
	
	for(i = 0; i < col_number; ++i){
		col_offset += file->col_sizes[i];
	}
	
	row_number = record->rows[record->current_row];
	
	return *(int*)(file->rows[row_number] + col_offset);
}

void PWF_close(PWFILE *file){
	if(!file){
		return;
	}
	
	int i;
	for(i = 0; i < file->num_cols; ++i){
		free(file->col_names[i]);
	}
	free(file->col_names);
	free(file->col_sizes);
	free(file->col_types);
	
	for(i = 0; i < file->rows_size; ++i){
		free(file->rows[i]);
	}
	free(file->rows);
	
	free(file);
}

PWRECORD *pw_find_by_str(PWFILE *file, const char *col_name, const char *where){
	/*number of records whose columns under col_name matches where*/
	int num_matches = 0;
	int col_num = -1;
	int col_offset = 0;
	int i;
	PWRECORD *record = 0;
	
	/*find the column number of col_name and its offset*/
	for(i = 0; i < file->num_cols; ++i){
		if(strcmp(col_name, file->col_names[i]) == 0){
			col_num = i;
			break;
		}
	}
	
	if(col_num == -1){
		return 0;
	}
	
	for(i = 0; i < col_num; ++i){
		col_offset += file->col_sizes[i];
	}
	
	/*find out how many matches there are...
	 *...
	 *yea i know, it's inefficient to iterate through the rows twice*/
	for(i = 0; i < file->rows_length; ++i){
		if(strcmp(file->rows[i] + col_offset, where) == 0){
			num_matches++;
		}
	}
	
	if(num_matches == 0){
		return 0;
	}
	
	record = (PWRECORD*) malloc(sizeof(PWRECORD));
	record->file = file;
	record->current_row = 0;
	record->num_rows = 0;
	record->rows = 0;
	
	record->num_rows = num_matches;
	record->rows = (int*) malloc(sizeof(int) * num_matches);
	
	num_matches = 0;
	
	for(i = 0; num_matches < record->num_rows; ++i){
		if(strcmp(file->rows[i] + col_offset, where) == 0){
			record->rows[num_matches] = i;
			num_matches++;
		}
	}
	
	return record;
}

PWRECORD *pw_find_by_int(PWFILE *file, const char *col_name, int where){
	/*number of records whose columns under col_name matches where*/
	int num_matches = 0;
	int col_num = -1;
	int col_offset = 0;
	PWRECORD *record = 0;
	int i;
	
	/*find the column number of col_name and its offset*/
	for(i = 0; i < file->num_cols; ++i){
		if(strcmp(col_name, file->col_names[i]) == 0){
			col_num = i;
			break;
		}
	}
	
	if(col_num == -1){
		return 0;
	}
	
	for(i = 0; i < col_num; ++i){
		col_offset += file->col_sizes[i];
	}
	
	/*find out how many matches there are...
	 *...
	 *yea i know, it's inefficient to iterate through the rows twice*/
	for(i = 0; i < file->rows_length; ++i){
		if(memcmp(file->rows[i] + col_offset, &where, sizeof(int)) == 0){
			num_matches++;
		}
	}
	
	if(num_matches == 0){
		return 0;
	}
	record = (PWRECORD*) malloc(sizeof(PWRECORD));
	record->file = file;
	record->current_row = 0;
	record->num_rows = 0;
	record->rows = 0;
	
	
	record->num_rows = num_matches;
	record->rows = (int*) malloc(sizeof(int) * num_matches);
	
	num_matches = 0;
	
	for(i = 0; num_matches < record->num_rows; ++i){
		if(memcmp(file->rows[i] + col_offset, &where, sizeof(int)) == 0){
			record->rows[num_matches] = i;
			num_matches++;
		}
	}
	
	return record;
}

/*save length of string (including null character), (int)
 *save initializer string (with null character),
 *save number of records (int),
 *save each record*/
void PWF_save(PWFILE *file, const char *filename){
	int record_length = 0;
	int init_strlen = 0;
	char *init_str = 0;
	int i;
	char num_buf[16];
	
	/*open the file*/
	FILE *out_file = fopen(filename, "wb");
	
	if(!out_file){
		sprintf(file->err, "[PWF_save] Error: cannot save file to \"%s\".", filename);
		return;
	}
	
	/*find the length of the initialization string (not including null character,
	 *what strlen() would return)
	 */
	for(i = 0; i < file->num_cols; ++i){
		init_strlen += strlen(file->col_names[i]); /*identifier*/
		init_strlen++; /*space*/
		
		/*type*/
		switch(file->col_types[i]){
		case PW_COL_TYPE_CHAR:
			init_strlen += 4;
			break;
		case PW_COL_TYPE_SHORT:
			init_strlen += 5;
			break;
		case PW_COL_TYPE_FLOAT:
			init_strlen += 5;
			break;
		case PW_COL_TYPE_INT:
			init_strlen += 3;
			break;
		case PW_COL_TYPE_DOUBLE:
			init_strlen += 6;
			break;
		case PW_COL_TYPE_STR:
			init_strlen += 3;
			
			init_strlen++; /*space*/
			
			sprintf(num_buf, "%d", file->col_sizes[i]);
			init_strlen += strlen(num_buf); /*number of digits in the string size value*/
			break;
		default:
			init_strlen += 6;
			break;
		}
		
		init_strlen += 2; /*comma and space*/
	}
	
	init_strlen -= 2; /*remove trailing comma and space*/
	
	/*the +2 is to account for the trailing comma and space
	 *which will be later removed from the string*/
	init_str = (char*) malloc(init_strlen + 1 + 2); 
	init_str[0] = '\0';
	
	/*initialize string now*/
	for(i = 0; i < file->num_cols; ++i){
		strcat(init_str, file->col_names[i]); /*identifier*/
		strcat(init_str, " "); /*space*/
		
		/*type*/
		switch(file->col_types[i]){
		case PW_COL_TYPE_CHAR:
			strcat(init_str, "CHAR");
			break;
		case PW_COL_TYPE_SHORT:
			strcat(init_str, "SHORT");
			break;
		case PW_COL_TYPE_FLOAT:
			strcat(init_str, "FLOAT");
			break;
		case PW_COL_TYPE_INT:
			strcat(init_str, "INT");
			break;
		case PW_COL_TYPE_DOUBLE:
			strcat(init_str, "DOUBLE");
			break;
		case PW_COL_TYPE_STR:
			strcat(init_str, "STR");
			
			strcat(init_str, " "); /*space*/
			
			sprintf(num_buf, "%d", file->col_sizes[i]);
			strcat(init_str, num_buf); /*number of digits in the string size value*/
			break;
		default:
			strcat(init_str, "XXXXXX");
			break;
		}
		
		strcat(init_str, ", "); /*comma and space*/
	}
	
	/*remove trailing comma and space*/
	init_str[strlen(init_str) - 2] = 0;
	
	/*print string length and initializer string*/
	fwrite(&init_strlen, sizeof(int), 1, out_file);
	fwrite(init_str, init_strlen, 1, out_file);
	
	free(init_str);
	
	/*find the row length*/
	for(i = 0; i < file->num_cols; ++i){
		record_length += file->col_sizes[i];
	}
	
	/*write the number of rows*/
	fwrite(&file->rows_length, sizeof(int), 1, out_file);
	/*write the rows*/
	for(i = 0; i < file->rows_length; ++i){
		fwrite(file->rows[i], record_length, 1, out_file);
	}
	
	fclose(out_file);
}

PWFILE *PWF_load(const char *filename){
	FILE *in_file = fopen(filename, "rb");
	int init_strlen = 0;
	char *init_str = 0;
	PWFILE *file = 0;
	
	int record_length = 0;
	int num_rows;
	
	int i;
	
	/*if the file does not exist*/
	if(!in_file){
		return 0;
	}
	
	/*file contains the length of the initializer string, then the initializer string*/
	init_strlen = 0;
	fread(&init_strlen, sizeof(int), 1, in_file);
	
	if(init_strlen == 0 || init_strlen < 0 || init_strlen > 32767){
		fclose(in_file);
		return 0;
	}
	
	init_str = (char*) malloc(init_strlen + 1);
	fread(init_str, init_strlen, 1, in_file);
	init_str[init_strlen] = '\0';
	
	/*create the database instance*/
	file = PWF_create(init_str);
	free(init_str);
	
	/*initialize the rows*/
	/*find row width*/
	for(i = 0; i < file->num_cols; ++i){
		record_length += file->col_sizes[i];
	}
	
	/*find number of rows*/
	fread(&num_rows, sizeof(int), 1, in_file);
	file->rows_length = num_rows;
	file->rows_size = ROWS_START_SIZE;
	/*allocate the appropriate amount of memory*/
	while(file->rows_size < file->rows_length){
		file->rows_size *= 2;
	}
	
	file->rows = (char**) malloc(sizeof(char*) * file->rows_size);
	for(i = 0; i < file->rows_size; ++i){
		file->rows[i] = (char*) malloc(record_length);
		memset(file->rows[i], 0, record_length);
	}
	
	/*read the file for existing rows*/
	for(i = 0; i < file->rows_length; ++i){
		fread(file->rows[i], record_length, 1, in_file);
	}
	
	fclose(in_file);
	return file;
}

void PWF_remove(PWRECORD *record){
	PWFILE *file = record->file;
	int row_num = record->rows[record->current_row];
	int row_end = file->rows_length;
	
	char *tmp_row = file->rows[row_num];
	int i;
	
	/*move the rows in front back by one and place the removed row
	 *in the front*/
	for(i = row_num; i < row_end - 1; ++i){
		file->rows[i] = file->rows[i + 1];
	}
	file->rows[row_end - 1] = tmp_row;
	
	/*move the rows back by one in the iterator as well*/
	for(i = record->current_row + 1; i < record->num_rows; ++i){
		record->rows[i]--;
	}
	
	file->rows_length--;
	record->current_row++;
}

int PWF_add_record(PWFILE *file, const char *col_names, ...){
	int record_length = 0;
	int i;
	int col_offset = 0;
	char *col_names_copy = 0;
	char *col_name = 0;
	char *record;
	va_list list;
	char **temp_rows;
	
	/*values passed into ellipses*/
	const char *col_value_str;
	
	int num_cols_assigned = 0;
	
	if(!file){
		return -1;
	}
	
	/*initialize variable arguments*/
	va_start(list, col_names);
	
	/*initialize record length*/
	for(i = 0; i < file->num_cols; ++i){
		record_length += file->col_sizes[i];
	}
	
	/*initialize copy to use with strtok*/
	col_names_copy = (char*) malloc(strlen(col_names) + 1);
	strcpy(col_names_copy, col_names);
	/*make uppercase*/
	for(i = 0; col_names_copy[i]; ++i){
		col_names_copy[i] = toupper(col_names_copy[i]);
	}
	
	/*check if there is space left on the rows*/
	if(file->rows_size == 0){
		/*extend the rows size to the default start size*/
		file->rows_size = ROWS_START_SIZE;
		
		file->rows = (char**) malloc(sizeof(char*) * ROWS_START_SIZE);
		
		for(i = 0; i < file->rows_size; ++i){
			file->rows[i] = (char*) malloc(record_length);
			memset(file->rows[i], 0, record_length);
		}
	}
	else if(file->rows_length >= file->rows_size){
		/*extend the rows size to double the length*/
		temp_rows = (char**) malloc(sizeof(char*) * file->rows_size * 2);
		
		/*move the old rows to the new array*/
		for(i = 0; i < file->rows_size; ++i){
			temp_rows[i] = file->rows[i];
		}
		
		free(file->rows);
		
		for(i = file->rows_size; i < file->rows_size * 2; ++i){
			/*allocate memory for the new rows*/
			temp_rows[i] = (char*) malloc(record_length);
			memset(temp_rows[i], 0, record_length);		
		}
		
		file->rows = temp_rows;
		
		file->rows_size *= 2;
	}
	
	record = file->rows[file->rows_length];
	
	/*begin copying the values to the correct columns*/
	col_name = strtok(col_names_copy, ", ");
	
	while(col_name){
		/*search for the column in the file with col_name*/
		
		/*find the columnn and its offset by comparing column name*/
		col_offset = 0;
		for(i = 0; i < file->num_cols; ++i){
			if(strcmp(col_name, file->col_names[i]) == 0){
				break;
			}
			else{
				col_offset += file->col_sizes[i];
			}
		}
		
		if(col_offset >= record_length){
			sprintf(file->err, "[PWF_add_record] Error: no column from file has name \"%s\".", col_name);
			num_cols_assigned = -1;
			break;
		}
		
		else{
			/*check the data type
			 *note that i is the index for the matching row*/
			switch(file->col_types[i]){
			case PW_COL_TYPE_CHAR:
				*(char*)(record + col_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_SHORT:
				*(short*)(record + col_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_INT:
				*(int*)(record + col_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_FLOAT:
				*(float*)(record + col_offset) = va_arg(list, double);
				break;
			case PW_COL_TYPE_DOUBLE:
				*(double*)(record + col_offset) = va_arg(list, double);
				break;
			case PW_COL_TYPE_STR:
				col_value_str = va_arg(list, const char*);
				memset(record + col_offset, 0, file->col_sizes[i]);
				strncpy(record + col_offset, col_value_str, file->col_sizes[i] - 1);
				break;
			}
			num_cols_assigned++;
		}
		
		col_name = strtok(0, ", ");
	}
	
	va_end(list);
	
	free(col_names_copy);
	
	if(num_cols_assigned == -1){
		return num_cols_assigned;
	}
	else{
		file->rows_length++;
		return 0;
	}
}

int PWF_edit_record(PWRECORD *record, const char *args, ...){
	char *args_copy = NULL;
	char *word = NULL;
	char *current_row = NULL;
	int i;
	int column_number;
	int column_offset;
	int placeholder_int;
	double placeholder_double;
	int sscanf_result;
	int sscanf_fail = 0;
	int other_error = 0;
	va_list list;
	
	if(!record){
		return -1;
	}
	
	if(!(record->file)){
		return -1;
	}
	
	current_row = record->file->rows[record->rows[record->current_row]];
	if(!current_row){
		sprintf(record->file->err, "Error: [PWF_edit_record] The record does not point to any row in the file.");
		return -1;
	}
	
	va_start(list, args);
	
	/*make copy of argument string*/
	args_copy = (char*) malloc(strlen(args) + 1);
	strcpy(args_copy, args);
	
	word = strtok(args_copy, " \n");
	/*while there are arguments left*/
	while(word){
		/*make column name in CAPS*/
		for(i = 0; word[i]; ++i){
			word[i] = toupper(word[i]);
		}
		
		/*find which column the column name matches*/
		for(column_number = 0; column_number < record->file->num_cols; ++column_number){
			if(strcmp(word, record->file->col_names[column_number]) == 0){
				break;
			}
		}
		
		/*if it doesn't match, return with error*/
		if(column_number == record->file->num_cols){
			sprintf(record->file->err, "Error: [PWF_edit_record] No column name in file matches \"%s\".", word);
			break;
		}
		
		/*get the offset of the column data*/
		for(column_offset = 0, i = 0; i < column_number; ++i){
			column_offset += record->file->col_sizes[i];
		}
		
		/*find whether the value is passed in string or in ellipsis*/
		word = strtok(NULL, " \n");
		if(!word){
			sprintf(record->file->err, "Error: [PWF_edit_record] No value assigned to column \"%s\".", record->file->col_names[column_number]);
			other_error = 1;
			break;
		}
		
		/*if passed through ellipsis*/
		if(PWF_get_length_until(word, ", \n") == 3 && strncmp(word, "...", 3) == 0){
			switch(record->file->col_types[column_number]){
			case PW_COL_TYPE_CHAR:
				*(char*)(current_row + column_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_SHORT:
				*(short*)(current_row + column_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_INT:
				*(int*)(current_row + column_offset) = va_arg(list, int);
				break;
			case PW_COL_TYPE_FLOAT:
				*(float*)(current_row + column_offset) = va_arg(list, double);
				break;
			case PW_COL_TYPE_DOUBLE:
				*(double*)(current_row + column_offset) = va_arg(list, double);
				break;
			case PW_COL_TYPE_STR:
				strncpy(current_row + column_offset, va_arg(list, const char*), record->file->col_sizes[column_number]);
				break;
			}
			word = strtok(NULL, ", \n");
		}
		/*else parse the string for values*/
		else{
			/*after parse, word should point to the character directly after the literal*/
			switch(record->file->col_types[column_number]){
			case PW_COL_TYPE_CHAR:
				/*single quote literal case*/
				if(PWF_get_char_literal_length(word) > 0){
					word = PWF_parse_char(word, (char*)(current_row + column_offset));
				}
				/*numeric literal case*/
				else{
					sscanf_result = sscanf(word, "%d", &placeholder_int);
					/*if the literal could not be read*/
					if(sscanf_result != 1){
						sprintf(record->file->err, "[PWF_edit_record] Error: Could not read value assigned for column \"%s\".", record->file->col_names[column_number]);
						sscanf_fail = 1;
					}
					*(char*)(current_row + column_offset) = placeholder_int;
				}
				word += PWF_get_length_until(word, ", \n");
				word = strtok(word, ", \n");
				break;
			case PW_COL_TYPE_SHORT:
				/*expect a plain number*/
				sscanf_result = sscanf(word, "%d", &placeholder_int);
				if(sscanf_result != 1){
					sprintf(record->file->err, "[PWF_edit_record] Error: Could not read value assigned for column \"%s\".", record->file->col_names[column_number]);
					sscanf_fail = 1;
				}
				*(short*)(current_row + column_offset) = placeholder_int;
				strtok(word, ", \n");
				word = strtok(NULL, ", \n");
				break;
			case PW_COL_TYPE_INT:
				sscanf_result = sscanf(word, "%d", &placeholder_int);
				if(sscanf_result != 1){
					sprintf(record->file->err, "[PWF_edit_record] Error: Could not read value assigned for column \"%s\".", record->file->col_names[column_number]);
					sscanf_fail = 1;
				}
				*(int*)(current_row + column_offset) = placeholder_int;
				strtok(word, ", \n");
				word = strtok(NULL, ", \n");
				break;
			case PW_COL_TYPE_FLOAT:
				sscanf_result = sscanf(word, "%lf", &placeholder_double);
				if(sscanf_result != 1){
					sprintf(record->file->err, "[PWF_edit_record] Error: Could not read value assigned for column \"%s\".", record->file->col_names[column_number]);
					sscanf_fail = 1;
				}
				*(float*)(current_row + column_offset) = placeholder_double;
				
				strtok(word, ", \n");
				word = strtok(NULL, ", \n");
				break;
			case PW_COL_TYPE_DOUBLE:
				sscanf_result = sscanf(word, "%lf", &placeholder_double);
				if(sscanf_result != 1){
					sprintf(record->file->err, "[PWF_edit_record] Error: Could not read value assigned for column \"%s\".", record->file->col_names[column_number]);
					sscanf_fail = 1;
				}
				*(double*)(current_row + column_offset) = placeholder_double;
				strtok(word, ", \n");
				word = strtok(NULL, ", \n");
				break;
			case PW_COL_TYPE_STR:
				word = PWF_parse_str(word, current_row + column_offset, record->file->col_sizes[column_number]);
				if(!word){
					sprintf(record->file->err, "[PWF_edit_record] Error: Opening/Closing quote not found for column \"%s\".", record->file->col_names[column_number]);
					other_error = 1;
				}
				word = strtok(word, ", \n");
				break;
			}
		}
		if(sscanf_fail || other_error){
			break;
		}
	/*end of while*/
	}
	
	/*free memory*/
	free(args_copy);
	va_end(list);
	
	if(column_number == record->file->num_cols || sscanf_fail || other_error){
		return -1;
	}
	return 0;
}

int PWF_get(PWRECORD *record, const char *col_names, ...){
	char *current_row = record->file->rows[record->rows[record->current_row]];
	char *col_name = 0;
	char *col_names_copy; /*used for strtok*/
	int i;
	int record_length = 0;
	int offset;
	int error = 0;
	
	void *param;
	
	/*initialize va_list*/
	va_list list;
	va_start(list, col_names);
	
	/*initialize string copy*/
	col_names_copy = (char*) malloc(strlen(col_names) + 1);
	strcpy(col_names_copy, col_names);
	/*make uppercase*/
	for(i = 0; col_names_copy[i]; ++i){
		col_names_copy[i] = toupper(col_names_copy[i]);
	}
	
	/*find record length*/
	for(i = 0; i < record->file->num_cols; ++i){
		record_length += record->file->col_sizes[i];
	}
	
	col_name = strtok(col_names_copy, ", \n");
	
	while(col_name){
		offset = 0;
		
		for(i = 0; i < record->file->num_cols; ++i){
			if(strcmp(col_name, record->file->col_names[i]) == 0){
				break;
			}
			else{
				offset += record->file->col_sizes[i];
			}
		}
		
		if(offset >= record_length){
			/*then the column name was likely mispelled, for it wasn't found*/
			sprintf(record->file->err, "[PWF_get] Error: no column name matches \"%s\".", col_name);
			error = 1;
			break;
		}
		else{
			param = va_arg(list, void*);
			
			switch(record->file->col_types[i]){
			case PW_COL_TYPE_CHAR:
				*(char*)(param) = *(char*)(current_row + offset);
				break;
			case PW_COL_TYPE_SHORT:
				*(short*)(param) = *(short*)(current_row + offset);
				break;
			case PW_COL_TYPE_INT:
				*(int*)(param) = *(int*)(current_row + offset);
				break;
			case PW_COL_TYPE_FLOAT:
				*(float*)(param) = *(float*)(current_row + offset);
				break;
			case PW_COL_TYPE_DOUBLE:
				*(double*)(param) = *(double*)(current_row + offset);
				break;
			case PW_COL_TYPE_STR:
				strncpy((char*)(param), (char*)(current_row + offset), record->file->col_sizes[i] - 1);
				break;
			default:
				break;
			}
		}
		if(error){
			break;
		}
		col_name = strtok(0, ", \n");
	}
	
	free(col_names_copy);
	va_end(list);
	
	if(error){
		return -1;
	}
	return 0;
}

PWRECORD *PWF_find(PWFILE *file, const char *args, ...){
	/*find out which column names are needed*/
	int i, ii;
	char *tmp;
	int cmp_length = 0;
	int cmp_size = 16;
	char *cmp_data;
	
	int num_compare_cols = 0;
	int compare_col_array_size = 16;
	char *col_comparisons;	/*store what comparison is made to each column*/
	int *cols_compared;				/*column number is stored*/
	int *col_offsets;		/*the offsets of each column in the records*/
	void *tmp_array;
	
	char *args_copy;
	
	PWRECORD *record = 0;
	va_list list;
	
	int record_length = 0;
	int placeholder_int;
	double placeholder_double;
	int sscanf_result;
	
	int is_record_match = 1;
	int num_record_matches = 0;
	int record_array_size = 16;
	
	enum comp_operator{
		EQUAL_TO,
		INEQUAL_TO,
		LESS_THAN,
		GREATER_THAN,
		LESS_THAN_OR_EQUAL_TO,
		GREATER_THAN_OR_EQUAL_TO
	};
	
	va_start(list, args);
	
	cmp_data = (char*) malloc(cmp_size);
	
	col_comparisons = (char*) malloc(compare_col_array_size);
	cols_compared = (int*) malloc(compare_col_array_size * sizeof(int));
	col_offsets = (int*) malloc(compare_col_array_size * sizeof(int));
	
	for(i = 0; i < file->num_cols; ++ i){
		record_length += file->col_sizes[i];
	}
	
	/*make copy of arguments,*/
	args_copy = (char*) malloc(strlen(args) + 1);
	strcpy(args_copy, args);
	
	/*column name is expected, tmp now points to the column name*/
	tmp = args_copy + PWF_get_whitespace_length(args_copy);
	
	while(*tmp){
		/*convert the column name to upper case*/
		for(i = 0; i < PWF_get_word_length(tmp); ++i){
			tmp[i] = toupper(tmp[i]);
		}
		/*check if it matches one of the column names of the file*/
		for(i = 0; i < file->num_cols; ++i){
			if(strncmp(tmp, file->col_names[i], strlen(file->col_names[i])) == 0){
				/*resize the cols_compared and col_comparison and col_offsets array if necessary*/
				if(num_compare_cols >= compare_col_array_size){
					tmp_array = malloc(compare_col_array_size + 16);
					memcpy(tmp_array, col_comparisons, compare_col_array_size);
					free(col_comparisons);
					col_comparisons = (char*)(tmp_array);
					
					tmp_array = malloc(sizeof(int) * (compare_col_array_size + 16));
					memcpy(tmp_array, cols_compared, sizeof(int) * compare_col_array_size);
					free(cols_compared);
					cols_compared = (int*)(tmp_array);
					
					tmp_array = malloc(sizeof(int) * (compare_col_array_size + 16));
					memcpy(tmp_array, col_offsets, sizeof(int) * compare_col_array_size);
					free(col_offsets);
					col_offsets = (int*)(tmp_array);

					compare_col_array_size += 16; 
				}
				
				
				/*read the comparison*/
				
				/*tmp now points to the part after the column name, presumably the operator*/
				tmp = tmp + PWF_get_word_length(tmp);
				tmp = tmp + PWF_get_whitespace_length(tmp);
				
				if(strncmp(tmp, "==", 2) == 0){
					col_comparisons[num_compare_cols] = EQUAL_TO;
					tmp += 2;
				}
				else if(strncmp(tmp, "!=", 2) == 0){
					col_comparisons[num_compare_cols] = INEQUAL_TO;
					tmp += 2;
				}
				else if(strncmp(tmp, "<=", 2) == 0){
					col_comparisons[num_compare_cols] = LESS_THAN_OR_EQUAL_TO;
					tmp += 2;
				}
				else if(strncmp(tmp, ">=", 2) == 0){
					col_comparisons[num_compare_cols] = GREATER_THAN_OR_EQUAL_TO;
					tmp += 2;
				}
				else if(tmp[0] == '<'){
					col_comparisons[num_compare_cols] = LESS_THAN;
					tmp++;
				}
				else if(tmp[0] == '>'){
					col_comparisons[num_compare_cols] = GREATER_THAN;
					tmp++;
				}
				else{
					/*handle situation where no operator is found*/
					sprintf(file->err, "[PWF_find] Error: no operator found at \"%s\".", tmp);
					num_compare_cols = 0;
					break;
				}
				
				/*read the value either from argument string or from ellipsis*/
				/*resize the col_data array if necessary*/
				if(cmp_size < cmp_length + file->col_sizes[i]){
					
					tmp_array = malloc(cmp_size + 16 + file->col_sizes[i]);
					memcpy(tmp_array, cmp_data, cmp_length);
					free(cmp_data);
					cmp_data = (char*)(tmp_array);
					cmp_size += 16 + file->col_sizes[i];
				}
				
				/*tmp points to part after operator*/
				tmp = tmp + PWF_get_whitespace_length(tmp);
				
				/*case where an ellipsis is passed*/
				if(strncmp(tmp, "...", 3) == 0){
					switch(file->col_types[i]){
					case PW_COL_TYPE_CHAR:
						*(char*)(cmp_data + cmp_length) = va_arg(list, int);
						break;
					case PW_COL_TYPE_SHORT:
						*(short*)(cmp_data + cmp_length) = va_arg(list, int);
						break;
					case PW_COL_TYPE_INT:
						*(int*)(cmp_data + cmp_length) = va_arg(list, int);
						break;
					case PW_COL_TYPE_FLOAT:
						*(float*)(cmp_data + cmp_length) = va_arg(list, double);
						break;
					case PW_COL_TYPE_DOUBLE:
						*(double*)(cmp_data + cmp_length) = va_arg(list, double);
						break;
					case PW_COL_TYPE_STR:
						strncpy(cmp_data + cmp_length, va_arg(list, const char*), file->col_sizes[i] - 1);
						break;
					}
					tmp += 3;
				}
				/*if there is a literal, read the literal value*/
				else{
					switch(file->col_types[i]){
					case PW_COL_TYPE_CHAR:
						if(PWF_get_char_literal_length(tmp) > 0){
							tmp = PWF_parse_char(tmp, cmp_data +cmp_length);
							//this bypasses the validity for parse_char
							sscanf_result = 1;
						}
						else{
							sscanf_result = sscanf(tmp, "%d", &placeholder_int);
							*(char*)(cmp_data + cmp_length) = placeholder_int;
							tmp += PWF_get_numeric_literal_length(tmp);
						}
						break;
					case PW_COL_TYPE_SHORT:
						sscanf_result = sscanf(tmp, "%d", &placeholder_int);
						*(short*)(cmp_data + cmp_length) = placeholder_int;
						tmp += PWF_get_numeric_literal_length(tmp);
						break;
					case PW_COL_TYPE_INT:
						sscanf_result = sscanf(tmp, "%d", &placeholder_int);
						*(int*)(cmp_data + cmp_length) = placeholder_int;
						tmp += PWF_get_numeric_literal_length(tmp);
						break;
					case PW_COL_TYPE_FLOAT:
						/*for floats, replace any character after the literal with a null character*/
						placeholder_int = tmp[PWF_get_numeric_literal_length(tmp)];
						tmp[PWF_get_numeric_literal_length(tmp)] = '\0';
						sscanf_result = sscanf(tmp, "%lf", &placeholder_double);
						*(float*)(cmp_data + cmp_length) = placeholder_double;
						/*replace the null character with its original character in string*/
						tmp[PWF_get_numeric_literal_length(tmp)] = placeholder_int;
						tmp += PWF_get_numeric_literal_length(tmp);
						break;
					case PW_COL_TYPE_DOUBLE:
						placeholder_int = tmp[PWF_get_numeric_literal_length(tmp)];
						tmp[PWF_get_numeric_literal_length(tmp)] = '\0';
						sscanf_result = sscanf(tmp, "%lf", &placeholder_double);
						*(double*)(cmp_data + cmp_length) = placeholder_double;
						tmp += PWF_get_numeric_literal_length(tmp);
						tmp[PWF_get_numeric_literal_length(tmp)] = placeholder_int;
						tmp += PWF_get_numeric_literal_length(tmp);
						break;
					case PW_COL_TYPE_STR:
						tmp = PWF_parse_str(tmp, cmp_data + cmp_length, file->col_sizes[i]);
						/*if tmp is a null pointer due to error in string literal syntax,
						 *handle it outside switch*/
						break;
					}
					/*if tmp is a null pointer due to error in syntax*/
					if(!tmp){
						sprintf(file->err, "[PWF_find] Error: Opening/closing quote not found for column \"%s\".", file->col_names[i]);
						num_compare_cols = 0;
						break;
					}
					if(sscanf_result != 1){
						sprintf(file->err, "[PWF_find] Error: Could not read value assigned for column \"%s\".", file->col_names[i]);
						num_compare_cols = 0;
						break;
					}
				}
				cmp_length += file->col_sizes[i];
				
				/*keep track of which column it is*/
				cols_compared[num_compare_cols] = i;

				/*get the offset of the column in the record (will be used when comparing record entries)*/
				col_offsets[num_compare_cols] = 0;
				for(i = 0; i < cols_compared[num_compare_cols]; ++i){
					col_offsets[num_compare_cols] += file->col_sizes[i];
				}

				num_compare_cols++;
				break;
			}
		}
		if(i == file->num_cols){
			/*handle situation where no column name matched with argument*/
			num_compare_cols = 0;
			sprintf(file->err, "[PWF_find] Error: no column name matches \"%s\".", tmp);
		}
		
		/*if some error in syntax or column misspelling made*/
		if(num_compare_cols == 0){
			free(cmp_data);
			free(col_comparisons);
			free(cols_compared);
			free(col_offsets);
			free(args_copy);
			va_end(list);

			return NULL;
		}

		/*skip comma if there is any*/
		/*start at next column name*/
		while(*tmp && PWF_get_word_length(tmp) == 0){
			tmp++;
		}
	}
	
	record = (PWRECORD*) malloc(sizeof(PWRECORD));
	record->file = file;
	record->current_row = 0;
	record->num_rows = 0;
	record->rows = (int*) malloc(record_array_size * sizeof(int));
	
	/*check if all the values are correct
	cmp_length = 0;
	for(i = 0; i < num_compare_cols; ++i){
		printf("cols_compared[%d] = %d\n",i, cols_compared[i]);
		printf("col_offsets[%d] = %d\n", i, col_offsets[i]);
		printf("col_comparisons[%d] = %d\n", i, col_comparisons[i]);
		printf("value compared is ");
		
		switch(file->col_types[cols_compared[i]]){
		case PW_COL_TYPE_CHAR:
			printf("%d", *(char*)(cmp_data + cmp_length));
			break;
		case PW_COL_TYPE_SHORT:
			printf("%d", *(short*)(cmp_data + cmp_length));
			break;
		case PW_COL_TYPE_INT:
			printf("%d", *(int*)(cmp_data + cmp_length));
			break;
		case PW_COL_TYPE_FLOAT:
			printf("%f", *(float*)(cmp_data + cmp_length));
			break;
		case PW_COL_TYPE_DOUBLE:
			printf("%lf", *(double*)(cmp_data + cmp_length));
			break;
		case PW_COL_TYPE_STR:
			printf("%s", cmp_data + cmp_length);
			break;
		}
		
		cmp_length += file->col_sizes[cols_compared[i]];
	}
	*/
	
	/*iterate through the whole file and see which records match*/
	/*i = row number, ii = compared col number*/
	for(i = 0; i < file->rows_length; ++i){
		//check each column
		cmp_length = 0;
		is_record_match = 1;

		for(ii = 0; ii < num_compare_cols; ++ii){
			switch(col_comparisons[ii]){
			case EQUAL_TO:
				/*do string comparison*/
				if(file->col_types[cols_compared[ii]] == PW_COL_TYPE_STR){
					if(strcmp(cmp_data + cmp_length, file->rows[i] + col_offsets[ii]) != 0){
						is_record_match = 0;
					}
				}
				/*do memory comparison*/
				else {
					if(memcmp(cmp_data + cmp_length, file->rows[i] + col_offsets[ii], file->col_sizes[cols_compared[ii]]) != 0){
						is_record_match = 0;
					}
				}
				break;
			case INEQUAL_TO:
				/*do string comparison*/
				if(file->col_types[cols_compared[ii]] == PW_COL_TYPE_STR){
					if(strcmp(cmp_data + cmp_length, file->rows[i] + col_offsets[ii]) == 0){
						is_record_match = 0;
					}
				}
				/*do memory comparison*/
				else {
					if(memcmp(cmp_data + cmp_length, file->rows[i] + col_offsets[ii], file->col_sizes[cols_compared[ii]]) == 0){
						is_record_match = 0;
					}
				}
				break;
			case LESS_THAN:
				switch(file->col_types[cols_compared[ii]]){
				case PW_COL_TYPE_CHAR:
					if(*(char*)(file->rows[i] + col_offsets[ii]) >= *(char*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_SHORT:
					if(*(short*)(file->rows[i] + col_offsets[ii]) >= *(short*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_INT:
					if(*(int*)(file->rows[i] + col_offsets[ii]) >= *(int*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_FLOAT:
					if(*(float*)(file->rows[i] + col_offsets[ii]) >= *(float*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_DOUBLE:
					if(*(double*)(file->rows[i] + col_offsets[ii]) >= *(double*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_STR:
					if(strcmp(file->rows[i] + col_offsets[ii], cmp_data + cmp_length) >= 0){
						is_record_match = 0;
					}
					break;
				}
				break;
			case GREATER_THAN:
				switch(file->col_types[cols_compared[ii]]){
				case PW_COL_TYPE_CHAR:
					if(*(char*)(file->rows[i] + col_offsets[ii]) <= *(char*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_SHORT:
					if(*(short*)(file->rows[i] + col_offsets[ii]) <= *(short*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_INT:
					if(*(int*)(file->rows[i] + col_offsets[ii]) <= *(int*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_FLOAT:
					if(*(float*)(file->rows[i] + col_offsets[ii]) <= *(float*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_DOUBLE:
					if(*(double*)(file->rows[i] + col_offsets[ii]) <= *(double*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_STR:
					if(strcmp(file->rows[i] + col_offsets[ii], cmp_data + cmp_length) <= 0){
						is_record_match = 0;
					}
					break;
				}
				break;
			case LESS_THAN_OR_EQUAL_TO:
				switch(file->col_types[cols_compared[ii]]){
				case PW_COL_TYPE_CHAR:
					if(*(char*)(file->rows[i] + col_offsets[ii]) > *(char*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_SHORT:
					if(*(short*)(file->rows[i] + col_offsets[ii]) > *(short*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_INT:
					if(*(int*)(file->rows[i] + col_offsets[ii]) > *(int*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_FLOAT:
					if(*(float*)(file->rows[i] + col_offsets[ii]) > *(float*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_DOUBLE:
					if(*(double*)(file->rows[i] + col_offsets[ii]) > *(double*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_STR:
					if(strcmp(file->rows[i] + col_offsets[ii], cmp_data + cmp_length) > 0){
						is_record_match = 0;
					}
					break;
				}
				break;
			case GREATER_THAN_OR_EQUAL_TO:
				switch(file->col_types[cols_compared[ii]]){
				case PW_COL_TYPE_CHAR:
					if(*(char*)(file->rows[i] + col_offsets[ii]) < *(char*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_SHORT:
					if(*(short*)(file->rows[i] + col_offsets[ii]) < *(short*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_INT:
					if(*(int*)(file->rows[i] + col_offsets[ii]) < *(int*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_FLOAT:
					if(*(float*)(file->rows[i] + col_offsets[ii]) < *(float*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_DOUBLE:
					if(*(double*)(file->rows[i] + col_offsets[ii]) < *(double*)(cmp_data + cmp_length)){
						is_record_match = 0;
					}
					break;
				case PW_COL_TYPE_STR:
					if(strcmp(file->rows[i] + col_offsets[ii], cmp_data + cmp_length) < 0){
						is_record_match = 0;
					}
					break;
				}
				break;
			}
			if(!is_record_match){
				break;
			}
			
			cmp_length += file->col_sizes[cols_compared[ii]];
		}
		if(is_record_match){
			/*add this current record, resize array if necessary*/
			if(record->num_rows >= record_array_size){
				tmp_array = malloc((record_array_size + 16) * sizeof(int));
				memcpy(tmp_array, record->rows, record->num_rows * sizeof(int));
				free(record->rows);
				record->rows = (int*)(tmp_array);
			}

			record->rows[record->num_rows] = i;
			record->num_rows++;
		}
	}
	
	free(col_comparisons);
	free(cols_compared);
	free(cmp_data);
	free(col_offsets);
	free(args_copy);
	
	va_end(list);
	
	if(record->num_rows == 0){
		free(record->rows);
		free(record);
		record = 0;
	}
	
	return record;
}

int PWF_sort(PWFILE *file, const char *args){
	/*int PWF_compar_num_col_compar = 0;
	 *int PWF_compar_array_size = 0;
	 *int *PWF_compar_col_offsets = 0;
	 *char *PWF_compar_is_asc = 0;
	 *char *PWF_compar_col_types = 0;*/
	/*make copy of arguments and put in all capital letters*/
	char *args_copy = 0;
	void *tmp_array;
	const char *current_arg;
	int error = 0;
	int i;
	
	/*initialize global scope arrays here and deallocate at end of function.*/
	PWF_compar_num_col_compar = 0;
	PWF_compar_array_size = 16;
	PWF_compar_col_offsets = (int*) malloc(sizeof(int) * PWF_compar_array_size);
	PWF_compar_is_asc = (char*) malloc(PWF_compar_array_size);
	PWF_compar_col_types = (char*) malloc(PWF_compar_array_size);
	
	args_copy = (char*) malloc(strlen(args) + 1);
	strcpy(args_copy, args);
	for(i = 0; i < strlen(args); ++i){
		args_copy[i] = toupper(args[i]);
	}
	
	current_arg = strtok(args_copy, " \n");
	
	while(current_arg){
		/*check if array needs to be resized*/
		if(PWF_compar_num_col_compar <= PWF_compar_array_size){
			PWF_compar_array_size += 16;
			/*col offsets*/
			tmp_array = calloc(PWF_compar_array_size, 1);
			memcpy(tmp_array, PWF_compar_col_offsets, sizeof(int) * PWF_compar_num_col_compar);
			free(PWF_compar_col_offsets);
			PWF_compar_col_offsets = (int*)(tmp_array);
			
			/*is ascending*/
			tmp_array = malloc(PWF_compar_array_size);
			memcpy(tmp_array, PWF_compar_is_asc, PWF_compar_num_col_compar);
			free(PWF_compar_is_asc);
			PWF_compar_is_asc = (char*)(tmp_array);
			
			/*col types*/
			tmp_array = malloc(PWF_compar_array_size);
			memcpy(tmp_array, PWF_compar_col_types, PWF_compar_num_col_compar);
			free(PWF_compar_col_types);
			PWF_compar_col_types = (char*)(tmp_array);
		}
		
		/*get the column type and offset*/
		for(i = 0; i < file->num_cols; ++i){
			if(strcmp(current_arg, file->col_names[i]) == 0){
				break;
			}
			PWF_compar_col_offsets[PWF_compar_num_col_compar] += file->col_sizes[i];
		}
		/*if no column type matched*/
		if(i == file->num_cols){
			sprintf(file->err, "[PWF_sort] Error: No column name matches \"%s\".", current_arg);
			error = 1;
			break;
		}
		PWF_compar_col_types[PWF_compar_num_col_compar] = file->col_types[i];
		
		/*check if column is ascending*/
		current_arg = strtok(NULL, " \n");
		/*if order does not exist*/
		if(!current_arg){
			sprintf(file->err, "[PWF_sort] Error: Order of sort not given.");
			error = 1;
			break;
		}
		
		if(strncmp(current_arg, "DESCENDING", PWF_get_word_length(current_arg)) == 0){
			PWF_compar_is_asc[PWF_compar_num_col_compar] = 0;
		}
		else if(strncmp(current_arg, "ASCENDING", PWF_get_word_length(current_arg)) == 0){
			PWF_compar_is_asc[PWF_compar_num_col_compar] = 1;
		}
		else{
			sprintf(file->err, "[PWF_sort] Error: Undefined order \"%s\".", current_arg);
			error = 1;
			break;
		}
		
		PWF_compar_num_col_compar++;
		current_arg = strtok(NULL, ", \n");
	}
	
	if(!error){
		qsort(file->rows, file->rows_length, sizeof(char*), PWF_compar);
	}
	
	free(args_copy);
	PWF_compar_num_col_compar = 0;
	PWF_compar_array_size = 0;
	free(PWF_compar_col_offsets);
	free(PWF_compar_is_asc);
	free(PWF_compar_col_types);
	
	if(error){
		return -1;
	}
	return 0;
}

const char *PWF_get_file_error(PWFILE *file){
	return file->err;
}

/*parses a string literal pointed to by word and copies up to n-1 characters to record_str*/
char *PWF_parse_str(char *word, char *record_str, int n){
	/*index used for char *word*/
	int j = 0;
	/*index for record_str*/
	int str_index = 0;
	int close_quote_found = 0;
	
	record_str[0] = 0;
	
	/*skip the first single quote*/
	if(word[0] == '\''){
		j++;
	}
	else{
		/*return the pointer to end of string if there is no opening quote*/
		return NULL;
	}
	
	while(word[j]){
		/*break out of loop if end quote is met*/
		if(word[j] == '\''){
			close_quote_found = 1;
			break;
		}
		if(str_index < n - 1){
			/*escape sequence*/
			if(word[j] == '\\'){
				j++;
				switch(word[j]){
				case '\\':
					record_str[str_index] = '\\';
					break;
				case '\'':
					record_str[str_index] = '\'';
					break;
				case '"':
					record_str[str_index] = '"';
					break;
				case '0':
					record_str[str_index] = '\0';
					break;
				case 'n':
					record_str[str_index] = '\n';
					break;
				case 'r':
					record_str[str_index] = '\r';
					break;
				case 't':
					record_str[str_index] = '\t';
					break;
				default:
					break;
				}
				j++;
			}
			/*normal character*/
			else{
				record_str[str_index] = word[j];
				j++;
			}
			str_index++;
		}
		/*in the case of the string literal being longer than n, ignore the characters of the rest
		 *of the string literal until the end quote is found*/
		else{
			if(word[j] == '\\'){
				j += 2;
			}
			else{
				j++;
			}
		}
	}
	
	record_str[str_index] = '\0';
	
	/*return the character after the close quote*/
	if(close_quote_found){
		return word + j + 1;
	}
	/*if there is no close quote, word+j points to the end of the string*/
	else{
		return NULL;
	}
}

int PWF_get_word_length(const char *str){
	int word_length = 0;
	
	if((str[0] >= 'A' && str[0] <= 'Z')
		|| (str[0] >= 'a' && str[0] <= 'z')
		|| str[0] == '_'){
		
		while((str[word_length] >= 'A' && str[word_length] <= 'Z')
			|| (str[word_length] >= 'a' && str[word_length] <= 'z')
			|| (str[word_length] >= '0' && str[word_length] <= '9')
			|| str[word_length] == '_'){
			
			word_length++;
		}
	}
	return word_length;
}

/*returns the number of characters that is whitespace
 *returns 0 if no whitespace detected*/
int PWF_get_whitespace_length(const char *str){
	int whitespace_length = 0;
	
	while(str[whitespace_length] == '\n'
		|| str[whitespace_length] == '\r'
		|| str[whitespace_length] == '\t'
		|| str[whitespace_length] == ' '){
		
		whitespace_length++;
	}
	return whitespace_length;
}

int PWF_get_numeric_literal_length(const char *str){
	int word_length = 0;
	
	if((str[0] >= '0' && str[0] <= '9') || (str[0] == '-')){
		while((str[word_length] >= 'A' && str[word_length] <= 'Z')
			|| (str[word_length] >= 'a' && str[word_length] <= 'z')
			|| (str[word_length] >= '0' && str[word_length] <= '9')
			|| str[word_length] == '.'){
			
			word_length++;
		}
	}
	return word_length;
}

int PWF_get_char_literal_length(const char *str){
	int word_length = 0;
	
	if(!str){
		return 0;
	}
	if(str[0] != '\''){
		return 0;
	}
	/*in the case of an escape sequence*/
	if(str[1] == '\\' && str[2] && str[3] == '\''){
		return 4;
	}
	/*normal character*/
	if(str[1] && str[2] == '\''){
		return 3;
	}
	return 0;
}

char *PWF_parse_char(char *word, char *record_char){
	int word_length = 0;
	
	if(!word){
		return NULL;
	}
	if(word[0] != '\''){
		return NULL;
	}
	/*in the case of an escape sequence*/
	if(word[1] == '\\' && word[2] && word[3] == '\''){
		switch(word[2]){
		case '\\':
			*record_char = '\\';
			break;
		case '\'':
			*record_char = '\'';
			break;
		case '"':
			*record_char = '"';
			break;
		case '0':
			*record_char = '\0';
			break;
		case 'n':
			*record_char = '\n';
			break;
		case 'r':
			*record_char = '\r';
			break;
		case 't':
			*record_char = '\t';
			break;
		default:
			break;
		}
		return word + 4;
	}
	/*normal character*/
	if(word[1] && word[2] == '\''){
		*record_char = word[1];
		return word + 3;
	}
	return NULL;
}

/*data1 and data2 converted from char** 
 */
int PWF_compar(const void *data1, const void *data2){
	int i;
	int result = 0;
	double dresult = 0.0;
	for(i = 0; i < PWF_compar_array_size && result == 0; ++i){
		switch(PWF_compar_col_types[i]){
		case PW_COL_TYPE_CHAR:
			result = *(char*)(*(char**)(data2) + PWF_compar_col_offsets[i]) - *(char*)(*(char**)(data1) + PWF_compar_col_offsets[i]);
			break;
		case PW_COL_TYPE_SHORT:
			result = *(short*)(*(char**)(data2) + PWF_compar_col_offsets[i]) - *(short*)(*(char**)(data1) + PWF_compar_col_offsets[i]);
			break;
		case PW_COL_TYPE_INT:
			result = *(int*)(*(char**)(data2) + PWF_compar_col_offsets[i]) - *(int*)(*(char**)(data1) + PWF_compar_col_offsets[i]);
			break;
		case PW_COL_TYPE_FLOAT:
			dresult = *(float*)(*(char**)(data2) + PWF_compar_col_offsets[i]) - *(float*)(*(char**)(data1) + PWF_compar_col_offsets[i]);
			if(dresult < 0.0){
				result = -1;
			}
			if(dresult > 0.0){
				result = 1;
			}
			break;
		case PW_COL_TYPE_DOUBLE:
			dresult = *(double*)(*(char**)(data2) + PWF_compar_col_offsets[i]) - *(double*)(*(char**)(data1) + PWF_compar_col_offsets[i]);
			if(dresult < 0.0){
				result = -1;
			}
			if(dresult > 0.0){
				result = 1;
			}
			break;
		case PW_COL_TYPE_STR:
			result = strcmp((char*)(*(char**)(data2) + PWF_compar_col_offsets[i]), (char*)(*(char**)(data1) + PWF_compar_col_offsets[i]));
			break;
		}
		
		if(PWF_compar_is_asc[i]){
			result = -result;
		}
	}
	
	return result;
}

int PWF_get_length_until(const char *str, const char *delimiters){
	int i = 0;
	int j;
	
	while(str[i]){
		j = 0;
		while(delimiters[j]){
			if(str[i] == delimiters[j]){
				return i;
			}
			j++;
		}
		i++;
	}
	
	return i;
}
