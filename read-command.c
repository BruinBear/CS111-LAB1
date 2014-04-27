// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "alloc.h"

//GLOBALS
command_t last_command; //points to the prev command, check if there is a command before an newline
int command_line; //  for current line number
int left_shell_count;
int size_of_str_buffer = 4096*4096;
bool is_normal_token(int c);

int get_line_number(char* str, int index);

//Checker for parenthese match
void is_parenthese_formed(char* str)
{
	int left = 0;
	int id = 0;
	char c;
	while(str[id]!=0)
	{
		if(id!=0 && str[id]=='#' && (is_normal_token(str[id-1]) || 
		str[id-1] ==';' ||  str[id-1] =='|'  || 
		str[id-1] =='&' || str[id-1] =='(' || 
		str[id-1] ==')' || str[id-1] =='<' || str[id-1] =='>' ))
		{
			error(1,0,"Not valid comment at line: %d\n",get_line_number(str,id));
		}
		
		
		c=str[id];
		if(c==')')
		{
			if(left<=0)
			{
				error(1,0,"Parenthese error at line: %d\n",get_line_number(str,id));
				exit(1);
			}
			else
				left--;
		}
		else if(c=='(')
		{
			left++;
		}
		id++;
	}
	if(left!=0)
	{
		error(1,0,"Parenthese error at line: %d\n",get_line_number(str,id));
		exit(1);
	}
	else 
		return;
}
   

// Determines whether the given character is a regular character
bool is_normal_token(int c)
{
    return (isalnum(c)	||   c == '!'   ||  c == '%' ||
           c == '+' || c == ',' || c == '-' || c == '.' ||
           c == '/' || c == ':' || c == '@' || c == '^' ||
           c == '_' || c == '"');
}
//================================================================
//Precedence Scorer
//================================================================
int get_precedence_score(command_t operator)
{
    int score;

    switch (operator->type)
    {
        case SUBSHELL_COMMAND:
            score = 0;
            break;
        case SEQUENCE_COMMAND:
            score = 1;
            break;
        case OR_COMMAND:
        case AND_COMMAND:
            score = 2;
            break;
        case PIPE_COMMAND:
            score = 3;
            break;
        default:
            error(1, 0, "Invalid command on line:%d\n", command_line);
    }

    return score;
}

//****************************************************************
//Some skip helper
//****************************************************************
void remove_white_spaces(char* str, int* i)
{
    // Skip any intial spaces
    while (str[*i] == ' ' || str[*i] == '\t') {
        *i=*i+1;
    }
	command_line = get_line_number(str,*i);
}

void remove_white_spaces_and_new_lines(char* str, int* i)
{
    while (str[*i] == ' ' || str[*i] == '\t' || str[*i] == '\n') {
        *i=*i+1;
       
    }
	command_line = get_line_number(str,*i);
	
}   

//****************************************************************
// Word array maintainance
//****************************************************************

void increase_word_size(char** word, int *size)
{
    *size *= 2;
    *word = checked_realloc(*word, *size*sizeof(*word));
}

char** increase_word_array_size(char** arr, int *size)
{
    *size *= 2; 
    arr = checked_realloc(arr, (*size)*sizeof(*arr));
    return arr;
}

//Get word if there is one, NULL if none
char* get_word(char* str, int* i)
{
    //skip spaces
    remove_white_spaces(str, i);
     
    // if it is anything else then none special token then
    if (!is_normal_token(str[*i])) {
        return NULL;
    }
    int used = 0;
    int size = 128;
    
    char* new_word = checked_malloc(size*sizeof(char));
	
    while (is_normal_token(str[*i])) {
	//check if memory is enough
        if (used >= size) {
            increase_word_size(&new_word, &size);
        }

        new_word[used] = str[*i];
        used++;
        
        *i = *i+1;
        
    }
    if (used >= size) {
        increase_word_size(&new_word, &size);
    }
	//set last bit EOF
    new_word[used] = '\0';
    return new_word;
}
   
   
   
//****************************************************************
// Stack Data structure and Functions
//****************************************************************

//array implemented stack of commands
typedef struct
{
    int top;
    int Stack_size;
    command_t* forest;//pointer to the start of forest
}Stack;
//initializer
Stack init_stack()
{
    Stack new_Stack;
    new_Stack.top = -1;
    new_Stack.Stack_size = 128;
    new_Stack.forest = checked_malloc(new_Stack.Stack_size*sizeof(command_t));
    return new_Stack;
}
//increase stack size
void increase_Stack_size(Stack* orig_Stack)
{
    orig_Stack->Stack_size *= 2;
    checked_realloc(orig_Stack->forest, (orig_Stack->Stack_size)*sizeof(command_t));
}
//push command into stack
void push_on_top(Stack* orig_Stack, command_t command)
{
    orig_Stack->top++;
    if (orig_Stack->top >= orig_Stack->Stack_size) {
        increase_Stack_size(orig_Stack);
    }
    orig_Stack->forest[orig_Stack->top] = command;
}
//check if empty
bool is_stack_empty(Stack orig_Stack)
{
    return (orig_Stack.top < 0);
}
//get top element
command_t top_Stack(Stack orig_Stack)
{
    if (is_stack_empty(orig_Stack)) {
        error(1, 0, "%d: Can't access top element of empty Stack", command_line);
    }
    return orig_Stack.forest[orig_Stack.top];
}
//pop top element
command_t pop_Stack(Stack* orig_Stack)
{
    if (is_stack_empty(*orig_Stack)) {
        error(1, 0, "%d: Can't pop empty Stack", command_line );
    }

    command_t val = orig_Stack->forest[orig_Stack->top];
    orig_Stack->top--;
    return val;
}

//****************************************************************
//Create string (jingyuzhinan)
//****************************************************************
char* char_append(char* string, char new_char) 
{
  int length = strlen(string)+1;
  if(length ==size_of_str_buffer-1)
  {
	string = checked_realloc(string, 2*size_of_str_buffer);
	size_of_str_buffer*=size_of_str_buffer;
  }
  string[length-1] = new_char;
  string[length] = 0;
  return string;
}

void get_rid_of_comments(char* str)
{
	int i=0;
	while(str[i]!=0)
	{
		if (str[i]!='#')
			i++;
		else{
			if(i>0 && is_normal_token(str[i-1]))
				error(1,0,"bad comments\n");
			while(str[i]!='\n' && str[i] !=0)
			{
				str[i] = ' ';
				i++;
			}
		}
	}
	return;
}

char*
create_string(int (*get_next_byte) (void *), void *get_next_arg)
{
    char* the_string = checked_malloc(4096*4096*sizeof(char));
    *the_string = '\0';
    int b;
    //Stop reading until EOF
    while ((b = get_next_byte(get_next_arg)) > 0) 
    {
        //if ((char)b == '\r') continue;
        the_string = char_append(the_string, (char)b);
    }
	
	
    get_rid_of_comments(the_string);
	//printf("%s",the_string);
	return the_string;
}
//****************************************************************
// Data structure for the command stream
//****************************************************************


command_stream_t init_command_stream()
{
    command_stream_t new_stream = checked_malloc(sizeof(struct command_stream));
    new_stream->tree_read = 0;
	new_stream->size = 128;
    new_stream->tree_count = 0;
    
    new_stream->forest = checked_malloc(new_stream->size*sizeof(command_t));

    return new_stream;
}

void increase_command_stream_size(command_stream_t stream)
{
    command_t* c = stream->forest;
	stream->size *=2;
    command_t* temp_trees = checked_realloc(c, (stream->size)*sizeof(command_t));
    stream->forest = temp_trees;
}
//****************************************************************
//Initializer helper
//****************************************************************
// Adds the input and output files for the command
void IO_redirect(char* str, int* i, command_t new_command)
{

    if (str[*i] == '<') {
        *i = *i +1;
        new_command->input = get_word(str, i);
        remove_white_spaces(str, i);
    }

    if (str[*i] == '>') {
        *i = *i +1;
        new_command->output = get_word(str, i);
        remove_white_spaces(str, i);
    }
}
//****************************************************************
//Get line num
//****************************************************************
int
get_line_number(char* str, int index)
{
    int i;
    int line_count=1;
    for(i = 0;i!=index;i++)
    {
      if(str[i]=='\n')
	line_count++;
    }
    return line_count;
}


//****************************************************************
//All initializer & Identifiers for tree
//****************************************************************

//master initilizer
command_t init_command()
{
	command_t new_command = checked_malloc(sizeof(*new_command));

	new_command->input = NULL;
    new_command->output = NULL;
    
    new_command->status = -1;
	
    return new_command;
}

//Simple
command_t init_simple_command(char* str, int* i)
{
    command_t new_command = init_command();
	
    int buffer_size = 10;
    int counter = 0;

    // Allocate memory for the array of words
    char** words =  checked_malloc(buffer_size*sizeof(*words));
    char* new_word;
    
   
    // Retrieve an array of words for the simple command
    while ((new_word = get_word(str, i))) {
        if (counter >= buffer_size) {
            words = increase_word_array_size(words, &buffer_size);
        }

        words[counter] = new_word;
        counter++;
    }
	
	//setting boundary
    if (counter >=buffer_size) {
        words = increase_word_array_size(words, &buffer_size);
    }
    words[counter] = NULL;

    // Remove white spaces
    remove_white_spaces(str, i);

	
	// Updates
    IO_redirect(str, i, new_command);// Store I/O information into the command
    new_command->u.word = words;
    new_command->type = SIMPLE_COMMAND;

    return new_command;
}

//Sequence
command_t init_sequence_command(char* str, int* i)
{
  char* strqq=str;
    command_t new_command = init_command();
    new_command->type = SEQUENCE_COMMAND;
	//*i=*i+1;
    return new_command;
}

//PIPE or OR
command_t init_pipe_or_command(char* str, int* i)
{
    command_t new_command = init_command();

    if (str[*i+1]=='|') {
        new_command->type = OR_COMMAND;
        *i=*i+2;
    }
	else
	{
        new_command->type = PIPE_COMMAND;
        *i=*i+1;
    }
    return new_command;
}


//True if the command is a and, or, pipe command
bool is_command(command_t last_command)
{
    if (!last_command) {
        return false;
    }

    enum command_type type = last_command->type;

    return (type == AND_COMMAND  ||
           type == OR_COMMAND   ||
           type == PIPE_COMMAND);

}
//AND
command_t create_and_command(char* str, int *i)
{
    command_t new_command = init_command();

    // Determine if it is  &&  or single& with error
    if (str[*i+1] == '&')
    {
        new_command->type = AND_COMMAND; 
        *i=*i+2;
    }
    else
    {
        error(1, 0, "%d: Invalid & command",get_line_number(str, *i)); 
    }
	if(str[*i]=='\n')
		*i=*i+1;    

    return new_command;
}

//SUBSHELL
command_t create_subshell_command(char* str, int* i)
{
    command_t new_command = init_command();
    new_command->type = SUBSHELL_COMMAND;

    char** word = checked_malloc(sizeof(char *));
    char* w = checked_malloc(sizeof(char));
	//indicator for left right shell
    w[0] = str[*i];
    word[0] = w;
    new_command->u.word = word;
	*i=*i+1;
	remove_white_spaces(str, i);
	//if end try IO If space try skip
    if (w[0] == ')') 
	{
        IO_redirect(str, i, new_command);
    }

	remove_white_spaces(str, i);

	
    return new_command;
}



//****************************************************************
// Try build a command starting from str[i]
//****************************************************************
command_t get_next_command(char* str, int* i,bool command_before)
{
	char c = str[*i];
	for(;;) {
		if(str[*i]==0)
				return NULL;
		if(str[*i]==25)
			*i=*i+1;
        // Skip spaces
        remove_white_spaces(str,i);
		while(!command_before && str[*i]=='\n')
			*i = *i +1;
		//check newline + ')'
		int i_temp = *i;
		remove_white_spaces_and_new_lines(str,&i_temp);
		if(str[i_temp]==')')
			*i=i_temp;
		
		command_line=get_line_number(str,*i);
        //Next not space
        char t = str[*i];
        

        if (str[*i] == '#')
        {
			//printf("%d number\n",*i);
            // Ignore comment
			again:
            while (str[*i] != '\n' && str[*i] !=0) {
				(*i)=(*i)+1;
            }
			//printf("%d number\n",*i);
			//printf("%d char\n",str[*i]);
			if(str[*i]==0)
			{
				//printf("did i return here\n");
				return NULL;
			}
            if (str[*i] != '\n')
            	(*i)=(*i)+1;
			if(str[*i]=='#')
				goto again;
			command_line=get_line_number(str,*i);
		    // If there is no command before this comment then remove all new
		    // lines and white spaces after the comment jingyu zhinan
		    if (!command_before) {
		        remove_white_spaces_and_new_lines(str,i);
		    }
		    command_line=get_line_number(str,*i);
		    continue;
        }
        if (str[*i] == '\n') 
		{
            if(left_shell_count==0)
			{
				if (is_command(last_command)) 
				{
					*i = *i +1;
					command_line=get_line_number(str,*i);
					continue;
				}
				else
				{
					remove_white_spaces_and_new_lines(str,i);
					return NULL;
				}
			}	
			else
			{
				command_line=get_line_number(str,*i);
					return last_command=init_sequence_command(str,i);
			}
		}
        // If reach the end we return NULL  jingyu zhinan
        if (str[*i] <=0) {
        remove_white_spaces_and_new_lines(str,i);
            return NULL;
        }
		command_line=get_line_number(str,*i);
        if(str[*i] == ' ' || str[*i] == '\t')
			remove_white_spaces(str,i);
		else if (is_normal_token(str[*i])) {
            last_command=init_simple_command(str, i);
            return last_command;
        }
        else if (str[*i] == '|')
        {
            // Determine if it is a pipe or an or statement
            return last_command = init_pipe_or_command(str, i);
        }
        else if(str[*i] == ';'){
			command_line=get_line_number(str,*i);
			*i=*i+1;
			if(str[*i] == ';')
				error(1,0,"bad seq formation\n");
			remove_white_spaces(str,i);
			//printf("start sequence at line %d before is :%c# after is :%c#\n",command_line,str[*i-1],str[*i+1]);
        	if(str[*i] != ';')
			{
				//printf("what is coming %d\n",str[*i]);
				if(str[*i] == ';')
					error(1,0, "false seq formation\n");
				//end or  a newline
				if ( str[*i] == '\n' || str[*i] == 0 || str[*i] == '#')
					return NULL;
				else if(str[*i] != '\n')
					return	last_command=init_sequence_command(str,i);
			}
			else
			{
				error(1,0,"bad formation");
			}
		}
        else if (str[*i] == '&')
        {
            return last_command=create_and_command(str, i);
        }
        else if (str[*i] == '(' || str[*i] == ')')
        {
			command_t temp;
			if(str[*i] == '(')
			{
				left_shell_count++;
				temp = create_subshell_command(str, i);
				remove_white_spaces_and_new_lines(str,i);
			}
			else
			{
				left_shell_count--;
				temp = create_subshell_command(str, i);
			}
            return last_command=temp;
        }
		else
        {
            error(1, 0, "%d: Unrecognized symbol received - \'%d\'", get_line_number(str, *i), str[*i]);
            return last_command=init_command();
        }
   }
}

//****************************************************************
//Build up a tree from one stream
//****************************************************************jingyu zhinan
command_t get_next_stream(char* str,bool* valid_stream,int *itr,int travel)
{
	bool in_subshell = false;
    bool command_before = false;
    
    command_t curr_command;

    Stack operand_Stack = init_stack();
    Stack operator_Stack = init_stack();
    
    // While we are getting new command
    while ((curr_command = get_next_command(str,itr, command_before))) {
        command_before = true;
        last_command = curr_command;

        if ((curr_command->type == SIMPLE_COMMAND)) 
        {
            push_on_top(&operand_Stack, curr_command);
        }
        else // operator
        {
            // If it's a sequence command and we are not in a subshell then jingyu zhinan
            // break from this loop
            if (curr_command->type == SEQUENCE_COMMAND && !in_subshell && travel )
				break;
			// Shell
            if (curr_command->type == SUBSHELL_COMMAND ) 
            {
				char **word = curr_command->u.word;
				// if left shell just push on top
                if (**word == '(') {
                    in_subshell = true;
                    push_on_top(&operator_Stack, curr_command);
                }
				else // right shell needs to pop everything till left shell
				{
                    in_subshell = false;
                    command_t next_command = pop_Stack(&operator_Stack);
					//assembly till left shell
                    while (next_command->type != SUBSHELL_COMMAND) {
                        command_t right_command = pop_Stack(&operand_Stack);
                        command_t left_command = pop_Stack(&operand_Stack);

                        next_command->u.command[0] = left_command;
                        next_command->u.command[1] = right_command;

                        push_on_top(&operand_Stack, next_command);

                        if (is_stack_empty(operator_Stack)) {
                            error(1, 0, "Missing open bracket on: %d\n", command_line);
                        }
						else
						{
                            next_command = pop_Stack(&operator_Stack);
                        }
                    }
  
                    command_t subshell_command = pop_Stack(&operand_Stack);
                    curr_command->u.subshell_command = subshell_command;
                    push_on_top(&operand_Stack, curr_command);
                }
            }
			//if operator stack is empty
			else if (is_stack_empty(operator_Stack)) 
            {
                push_on_top(&operator_Stack, curr_command);
            }
			//if higher precedence score than next just push
            else if(get_precedence_score(curr_command) > get_precedence_score(top_Stack(operator_Stack)))
            {
                push_on_top(&operator_Stack, curr_command);
            }
			
		//otherwise we need to pop two elements and assemble the command hello world
		//with the top of operator
		//then push back to operand stack
            else
            {
                command_t right_command = pop_Stack(&operand_Stack);
                command_t left_command = pop_Stack(&operand_Stack);
                command_t operator_command = pop_Stack(&operator_Stack);

                operator_command->u.command[0] = left_command;
                operator_command->u.command[1] = right_command;

                push_on_top(&operand_Stack, operator_command);
                push_on_top(&operator_Stack, curr_command);
            }
        }
    }
    
    //keep asemble commands in the same fashion
    while (!is_stack_empty(operator_Stack)) {
        command_t right_command = pop_Stack(&operand_Stack);
        command_t left_command = pop_Stack(&operand_Stack);
        command_t operator_command = pop_Stack(&operator_Stack);

        operator_command->u.command[0] = left_command;
        operator_command->u.command[1] = right_command;

        push_on_top(&operand_Stack, operator_command);

    }

    if (is_stack_empty(operand_Stack)) {
        error(1, 0, "Too many operators ondada: %d\n", command_line);
    }
    command_t new_stream = pop_Stack(&operand_Stack);

    if (!is_stack_empty(operand_Stack))
    {
        error(1, 0, "Too many operators onmewmwe: %d\n", command_line);
    }

    //move i to next command stream;
    while (str[*itr] == ' ' || str[*itr] == '\t' || str[*itr] == '\n') {
        *itr = *itr +1;
    }

    //no more stream coming in
    if (str[*itr] <=0) {
        *valid_stream = false;
    }
    return new_stream;
}
//****************************************************************
//Make forest
//****************************************************************
command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
	
  	//Initilize Input
	char* command_str = create_string(get_next_byte,get_next_byte_argument);

	is_parenthese_formed(command_str);
	left_shell_count=0;
	int p = 0;
	remove_white_spaces_and_new_lines(command_str, &p);
	if(command_str[p] == 0)
		error(1,0,"Empty input\n");
		
	//initialize command stream
	command_stream_t new_stream = init_command_stream();
	
	command_t tree;
	command_line = 1;
	int itr = 0;// for position in the strin
	bool valid_stream = true; // Determines whether we have reached the end of the stream
	// Get tree
    while (valid_stream) {
        tree = get_next_stream(command_str,&valid_stream,&itr,0);
        // Need more tree holes!
        if (new_stream->tree_count >= new_stream->size) {
            increase_command_stream_size(new_stream);
        }
        // Plant tree
        new_stream->forest[new_stream->tree_count] = tree;
        new_stream->tree_count++;
        command_line++;
    }
    return new_stream;

}

//travel stream
command_stream_t
travel_make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
	
  	//Initilize Input
	char* command_str = create_string(get_next_byte,get_next_byte_argument);

	is_parenthese_formed(command_str);
	left_shell_count=0;
	int p = 0;
	remove_white_spaces_and_new_lines(command_str, &p);
	if(command_str[p] == 0)
		error(1,0,"Empty input\n");
		
	//initialize command stream
	command_stream_t new_stream = init_command_stream();
	
	command_t tree;
	command_line = 1;
	int itr = 0;// for position in the strin
	bool valid_stream = true; // Determines whether we have reached the end of the stream
	// Get tree
    while (valid_stream) {
        tree = get_next_stream(command_str,&valid_stream,&itr,1);
        // Need more tree holes!
        if (new_stream->tree_count >= new_stream->size) {
            increase_command_stream_size(new_stream);
        }
        // Plant tree
        new_stream->forest[new_stream->tree_count] = tree;
        new_stream->tree_count++;
        command_line++;
    }
    return new_stream;

}

command_t
read_command_stream (command_stream_t s)
{
    //while not all tree is not inspected 
	/* Pi, The ratio of the circumference of a circle to its diameter
	and this is just the biginning. forever. without ever repeating. which means that the contained within this string of 
	decimals is every single other number. your birth date. combination to your locker
	your ssn. it's all in there somewhere and if you convert these decimals into letters. you would have everyword
	that ever existed in every possible combination. The first syllable you spoke as a baby. the name
	of your latest crush. your entire life story from beginning to end. everything we ever sa or do.
	all the world's infinite possiblities rest within this one simple circle*/
    if (s->tree_read < s->tree_count) 
        return s->forest[s->tree_read++];
    else
        return NULL;
}

