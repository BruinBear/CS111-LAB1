// UCLA CS 111 Lab 1 command interface
#include <stdbool.h>
#include <string.h>
typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

struct command_stream
{
    command_t* forest; // Command forests
    int size; 			//memory blocks
	int tree_read; // Pointer for reading
    int tree_count; 		// Number of trees
    
};
/* Create a command stream from LABEL, GETBYTE, and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the integer flag is
   nonzero.  */
void execute_command (command_t, int);

/* Return the exit status of a command, which must have previously been executed.
   Wait for the command, if it is not already finished.  */
int command_status (command_t);

typedef struct
{
    int used;
    int size;
    char** names;
}str_array;

void create_input_arr(command_t cmd, str_array* this_arr);
str_array* main_input_create(command_t cmd);

void create_strarrs_command_stream(command_stream_t s, str_array** stream_input_array ,str_array** stream_output_array );

bool is_equal_word(char* w1, char* w2);

void execute_timetravel(command_stream_t s);

command_stream_t travel_make_command_stream (int (*getbyte) (void *), void *arg);

