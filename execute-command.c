// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <error.h>


#include "alloc.h"
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void check_IO (command_t cmd)
{
	int fd_in;
	int fd_out;
			
	if(cmd->input)
	{
		//cmd < file
		if((fd_in = open(cmd->input,O_RDONLY, 0666)) == -1)
			error(1,0,"cannot input file\n!");
		if (dup2(fd_in, 0) == -1)
		error(1,0, "cannot do input redirect\n");
	}
	if(cmd->output)
	{
		//cmd > file
		if((fd_out = open(cmd->output,O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
			error(1,0,"cannot output file!\n");
		if (dup2(fd_out, 1) == -1)
		error(1,0, "cannot do output redirect\n");
	}
}

void execute_simple(command_t cmd)
{
    int status;
    pid_t pid = fork();
	
    //execute simple command
    if (pid == 0) {
        check_IO(cmd);
        execvp(cmd->u.word[0], cmd->u.word);
        error(1, 0, "Invalid command");
    }
    //wait for child status
    else if (pid > 0) {
        if (waitpid(pid, &status, 0) < 0)
            abort();
        cmd->status = WEXITSTATUS(status);
    }
    // Otherwise return an error because the fork got an error
    else 
        error(1, 0, "Can't create chil process!\n");
}

void execute_and(command_t cmd, int time_travel)
{
	execute_command(cmd->u.command[0],time_travel);
	if (cmd->u.command[0]->status == 0)
	{
		//first succeed run second
		execute_command(cmd->u.command[1],time_travel);
		//set the status of the AND command
		cmd->status = cmd->u.command[1]->status;
	}
	else
	{
		//skip cmd2
		//set the status of the ANDcommand
		cmd->status = cmd->u.command[0]->status;
	} 
	//printf("ending status is : %d\n" , cmd->status);
}

void execute_or(command_t cmd, int time_travel)
{
	execute_command(cmd->u.command[0],time_travel);
	if (cmd->u.command[0]->status != 0)
	{
		//first fail run second
		execute_command(cmd->u.command[1],time_travel);
		//set the status of the OR command
		cmd->status = cmd->u.command[1]->status;
	}
	else //first succeed
	{
		//skip cmd2
		//set the status of the OR command
		cmd->status = cmd->u.command[0]->status;
	} 
	//printf("ending status is : %d\n" , cmd->status);
}


void execute_pipe(command_t cmd,int time_travel)
{
	int fd[2];
	int create_pipe = pipe(fd);
	if (create_pipe != 0)
		error(1,0, "Fail to create pipe!\n");
		
	pid_t pid = fork();
	
	int status;
	//If it is child process then close the read and 
	//redirect stdout to write descriptor of the pipe
	if (pid  ==0)
	{	//cmd1
		//child writes to pipe
		close(fd[0]);
		if (dup2(fd[1],STDOUT_FILENO) == -1)
			error(1,0,"cannot redirect output!");
		execute_command(cmd->u.command[0],time_travel);
		close(fd[1]);
		exit(cmd->u.command[0]->status);
		//should never get here
		printf("error exiting\n");
	}
	else if(pid>0)
	{	//cmd2
		// parent reads the pipe
		waitpid(pid, &status, 0);
		cmd->u.command[0]->status = status;
		close(fd[1]);
		if( dup2(fd[0],STDIN_FILENO) == -1)
			error(1, 0, "cannot redirect input");
		execute_command(cmd->u.command[1],time_travel);
		close(fd[0]);
		cmd->status = cmd->u.command[1]->status;
	}
	else
	{
		error(1,0,"cannot create child process!\n");
	}
}

void execute_sequence(command_t cmd,int time_travel)
{
	int fd[2];
		
	pid_t pid = fork();
	
	int status;
	//we mimic pipe execution without io redirection
	if (pid  ==0)
	{	
		execute_command(cmd->u.command[0],time_travel);
		exit(cmd->u.command[0]->status);
		//should never get here
		printf("error exiting\n");
	}
	else if(pid>0)
	{	
		//wait till first cmd exits
		waitpid(pid, &status, 0);
		cmd->u.command[0]->status = status;
		execute_command(cmd->u.command[1],time_travel);
		cmd->status = cmd->u.command[1]->status;
	}
	else
	{
		error(1,0,"cannot create child process!\n");
	}
}

//Subsheel requires a child process to run what is inside
//and parent to wait and get status of child
void execute_subshell(command_t cmd,int timetravel)
{
    pid_t pid = fork();
    int status;

    if (pid == 0) 
	{
        check_IO(cmd);
        execute_command(cmd->u.subshell_command, timetravel);
        exit(cmd->u.subshell_command->status);
    }
	else if (pid > 0) 
        waitpid(pid, &status, 0);
    else
        error(1, 0, "Forked process failed");

    cmd->status = WEXITSTATUS(status);
}

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  command_t cmd = c;
  switch (cmd->type)
    {
		case SIMPLE_COMMAND:
			execute_simple(cmd);
			break;
        case SUBSHELL_COMMAND:
			execute_subshell(cmd,time_travel);
            break;
        case SEQUENCE_COMMAND:
            execute_sequence(cmd, time_travel);
            break;
        case OR_COMMAND:
			execute_or(cmd, time_travel);
			break;
        case AND_COMMAND:
			execute_and(cmd,time_travel);
            break;
        case PIPE_COMMAND:
			execute_pipe(cmd,time_travel);	
            break;
        default:
            error(1, 0, "Not valid command\n");
			
		printf("Status : %d\n",cmd->status);
			
    }
 int p = time_travel;
}

//************************************************
//Create a array of strings to store input output
//************************************************

str_array* create_str_array()
{
    str_array* arr = checked_malloc(sizeof(str_array));
    arr->used = 0;
    arr->size = 9;
    arr->names = checked_malloc(8*sizeof(char*));
    return arr;
}

void increase_str_array(str_array* arr)
{
    arr->size = arr->size*2;
    arr->names=checked_realloc(arr->names,arr->size*(sizeof(char*)));
}

void insert_to_array(str_array* arr, char* tobeadded)
{
    //arr full increase size by 2times
    if(arr->used>=arr->size)
	increase_str_array(arr);
    
    arr->names[arr->used]=tobeadded;
    arr->used = arr->used+1;
}

void add_simple_to_arr(command_t cmd, str_array* this_arr)
{
    int i = 1;
    char** words = cmd->u.word;
    while(words[i])
    {
	if(words[i][0]!='-')
	{
	    insert_to_array(this_arr, words[i]);
	}i++;
    }
    
}


void create_input_arr(command_t cmd, str_array* this_arr)
{
    //add whatever input is with the command first
    if(cmd->input)
    {
	insert_to_array(this_arr, cmd->input);
    }
    //simple command add terminate
    if(cmd->type == SIMPLE_COMMAND)
    {
	add_simple_to_arr(cmd,this_arr);
    }
    //subshell call one more time
    else if(cmd->type == SUBSHELL_COMMAND)
    {
	create_input_arr(cmd->u.subshell_command, this_arr);
    }
    else
    {
	
	create_input_arr(cmd->u.command[0], this_arr);
	create_input_arr(cmd->u.command[1], this_arr);
    }
    //add original input

}

str_array* main_input_create(command_t cmd)
{
    str_array* this_arr = create_str_array();
    create_input_arr(cmd,this_arr);
        //testing printf
    int p = 0;

    return this_arr;
}

void create_output_arr(command_t cmd, str_array* this_arr)
{
    //add whatever input is with the command first
    
    //simple command add terminate
    if(cmd->type == SIMPLE_COMMAND)
    {
	//do nothing
    }
    //subshell call one more time
    else if(cmd->type == SUBSHELL_COMMAND)
    {
	create_output_arr(cmd->u.subshell_command, this_arr);
    }
    else
    {
	
	create_output_arr(cmd->u.command[0], this_arr);
	create_output_arr(cmd->u.command[1], this_arr);
    }
    //add original input
    if(cmd->output)
    {
	insert_to_array(this_arr, cmd->output);
    }
}

str_array* main_output_create(command_t cmd)
{
    str_array* this_arr = create_str_array();
    create_output_arr(cmd,this_arr);
    int p = 0;

    return this_arr;
}
//check string the same
//
bool is_equal_word(char* w1, char* w2)
{
    //printf("Compareing word: *%s*, and *%s*.\n",w1, w2);
    if (strlen(w1)!=strlen(w2))
        return false;
    else
    {
	unsigned int i = 0;
	while(i < strlen(w1))
	{
	    if(w1[i]!=w2[i])
		return false;
	    i++;
	}
        return true;
    }
}

bool is_dependent(str_array* s1, str_array* s2)
{
    char **names1 = s1->names;
    char **names2 = s2->names;
    int size1 = s1->used;
    int size2 = s2->used;
    if(size1 == 0 || size2 == 0)
	{
		//printf("Something wrong happened!\n");
		return false;
    }
	int i;
    int j;
    char* temp;
    for(i=0;i<size1;i++)
    {
		temp = names1[i];
		for(j=0;j<size2;j++)
		{
			
			if(is_equal_word(names2[j],temp))
			return true;
		}
    }
    return false;
}

bool is_dependent_with_index(str_array** stream_input_array,str_array** stream_output_array, int left, int right)
{
    if(is_dependent(stream_input_array[left], stream_output_array[right]) ||
	is_dependent(stream_input_array[right], stream_output_array[left]))
	return true;
    else 
	return false;
}
void create_strarrs_command_stream(command_stream_t s, str_array** stream_input_array ,
				   str_array** stream_output_array )
{
    command_t command;
    
    int index = 0;
    while ((command = read_command_stream (s)))
    {
	//printf("where is seg\n");
	//input with index
	stream_input_array[index] = main_input_create(command);
	//output with index
	stream_output_array[index] = main_output_create(command);
	index++;
    }
/*
    int p = 0;
    
    while(p!=s->tree_count)
    {
	int k = 0;
	printf("#%d cmd tree input has %d items:\n",k,stream_input_array[p]->used);
	while(k!=stream_input_array[p]->used)
	{
	    printf(" %s",stream_input_array[p]->names[k]);
	    k++;
	}
	printf("\n");
	k=0;
	printf("#%d cmd tree output has:\n",k);
	while(k!=stream_output_array[p]->used)
	{
	    printf(" %s",stream_output_array[p]->names[k]);
	    k++;
	}
	printf("\n");
	p++;
    }*/
 }

bool no_dependency_from_start(str_array** stream_input_array,
			      str_array** stream_output_array,
			     int travel_start,
			     int travel_end)
{
    //if it is the same tree return true
    if (travel_start == travel_end)
	return true;
    
    int totravel = travel_end;
    while(travel_start != totravel)
    {
	if(is_dependent_with_index(stream_input_array,stream_output_array,  travel_start, totravel))
	    return false;
	travel_start++;
	
    }
    return true;
}

//recursion for forking parallelization
void fork_to_parallel(int todo, int start, command_stream_t s)
{
    if(todo!=0)
    {
	pid_t pid;
	int status;
	
	pid = fork();
	if(pid < 0)//fail
	{
	    error(1,0,"fork failed");
	}
	else if( pid == 0) //child
	{
	    execute_command(s->forest[start],1);
	    _exit(0);
	}
	else //parent
	{
	  //  printf("I am doing command tree : %d \n",start);

	    
		fork_to_parallel(todo-1, start+1,s);
		waitpid(pid,&status,0);
	}
    }
}
 
 //helpers
 
 void insert_dependency(int** dep_table, int i, str_array**  stream_input_array ,str_array** stream_output_array)
 {
	int kk = 0;
	while(kk!=i)
	{
		if(is_dependent_with_index(stream_input_array,stream_output_array,kk,i)){
			dep_table[i][kk]=1;
		}
		kk++;
	}
 }
 
 void fillin_run_table(int** run_table,int** dep_table,int size){
	int i,j;
	for(i = 0;i<size;i++)
	{
		int count = 0;
		for(j=0;j<size;j++)
		{
			count = count + dep_table[i][j];
		}
		run_table[i][0] = count;
	}
 }
 
 
 

 int* to_run_table(int** run_table, int size,int *to_run_size)
 {
	int i= 0;
	int count = 0;
	while(i < size)
	{
		if(run_table[i][0]==0 && run_table[i][1]==0)
				{count++;/*printf("\n###########\nthis is %d\n################\n",i);*/}
		i++;
	}
	//printf("count is :%d\n",count);
	if (count == 0)
		return NULL;
	
	int * to_run = (int *) checked_malloc(count*sizeof(int));
	*to_run_size=count;
	int k = 0;
	int pp = 0;
	while(pp < size)
	{
		if(run_table[pp][0]==0 && run_table[pp][1]==0)
		{	
			to_run[k] = pp;
			k++;
			if(k>=count)
				break;
		}
		pp=pp+1;
	}
	
	return to_run;//no runnable
 }
 
 void run_to_run(int* to_run,int start ,int togo, command_stream_t s)
{
    if(togo!=0)
    {
//	printf("am i stuck here\n");
		pid_t pid;
		int status;
		
		pid = fork();
		if(pid < 0)//fail
		{
			error(1,0,"fork failed");
		}
		else if( pid == 0) //child
		{
			
			execute_command(s->forest[to_run[start]],1);
			_exit(0);

		}
		else //parent
		{
		  //  printf("I am doing command tree : %d \n",start);
			
			run_to_run(to_run, start + 1, togo-1,s);
			
			waitpid(pid,&status,0);
		}
    }
}
 
void execute_timetravel(command_stream_t s)
{
    int size_forest = s->tree_count;
    str_array** stream_input_array = (str_array**) checked_malloc(size_forest*sizeof(str_array*));
    str_array** stream_output_array = (str_array**) checked_malloc(size_forest*sizeof(str_array*));
    create_strarrs_command_stream(s,stream_input_array , stream_output_array);
	
	int** dep_table = (int**) checked_malloc(size_forest*sizeof(int*));
	int cc=0;
	int ck=0;
	for(;cc<size_forest;cc++)
	{
		dep_table[cc]=(int*) checked_malloc(size_forest*sizeof(int));
		int cp=0;
		for(;cp<size_forest;cp++)
		{
			dep_table[cc][cp]=0;
		}
	}
	for(cc=0;cc<size_forest;cc++)
	{
		insert_dependency(dep_table, cc,  stream_input_array , stream_output_array);
	}
	// for(cc=0;cc<size_forest;cc++)
	// {
		// for(ck=0;ck<size_forest;ck++)
		// {
			// printf("%d ",dep_table[cc][ck]);
		// }
		// printf("%c",'\n');
	// }
	int** run_table = (int**) checked_malloc(size_forest*sizeof(int*));
	for(cc=0;cc<size_forest;cc++)
	{
		run_table[cc]=(int*) checked_malloc(2*sizeof(int));
		run_table[cc][0]=run_table[cc][0]=0;
	}
	fillin_run_table(run_table,dep_table,size_forest);
	// for(cc=0;cc<size_forest;cc++)
	// {
		// printf("%c%d %d",'\n',run_table[cc][0],run_table[cc][1]);
	// }
	
	int* to_run;
	int t_r_size;
	while((to_run  = to_run_table(run_table,size_forest,&t_r_size)))
	{
		//printf("\nTo run table is\n");
			int k3;
			// for(k3=0; k3< t_r_size;k3++)
			// {
				// printf("%d\n",to_run[k3]);
			// }
		
		
		//running in parallel
		
		run_to_run( to_run, 0 ,t_r_size, s);
		//update dep table and run rable
			int ix;
			for(ix=0;ix<t_r_size;ix++)
			{
				run_table[to_run[ix]][1]=1;
				int iy;
				
				if(t_r_size > 0)
				{
					for(iy=0;iy<size_forest;iy++)
					{
						dep_table[iy][to_run[ix]]=0;
					}
				}
					//printf("omg:%d / %d\n", ix, t_r_size);
					
			}
			// printf("Do i get here: \n");
			// print new dep
			
			// printf("\ndep table is:\n");
			// for(cc=0;cc<size_forest;cc++)
			// {
				// for(ck=0;ck<size_forest;ck++)
				// {
					// printf("%d ",dep_table[cc][ck]);
				// }
				// printf("%c",'\n');
			// }
			// printf("run table is:\n");
			fillin_run_table(run_table,dep_table,size_forest);
			//print run table
			// for(cc=0;cc<size_forest;cc++)
			// {
				// printf("%c%d %d",'\n',run_table[cc][0],run_table[cc][1]);
			// }
			
			free(to_run);
	}
	
	/*while((to_run  = to_run_table(run_table,size_forest)))
	{
		int t_r_size = sizeof(to_run)/sizeof(int);
		
		//print table 
		printf("\nTo run table is\n");
		int k3;
		for(k3=0; k3< t_r_size;k3++)
		{
			printf("%d\n",to_run[k3]);
		}
		//update dep table and run rable
		int ix;
		for(;ix<=t_r_size;ix++)
		{
			run_table[to_run[ix]][1]=1;
			int iy;
			for(iy=0;iy<size_forest;iy++)
				dep_table[iy][to_run[ix]]=0;
		}
		
		printf("dep table is:\n");
		for(cc=0;cc<size_forest;cc++)
		{
			for(ck=0;ck<size_forest;ck++)
			{
				printf("%d ",dep_table[cc][ck]);
			}
			printf("%c",'\n');
		}
		printf("run table is:\n");
		fillin_run_table(run_table,dep_table,size_forest);
				for(cc=0;cc<size_forest;cc++)
		{
			printf("%c%d %d",'\n',run_table[cc][0],run_table[cc][1]);
		}
	}*/














    
    /*int travel_start = 0;
    int travel_end   = 0;
    
    
   // printf("\n\nTravelling!\n");
    int zone_count = 0;
    //For start ---end we can fork to parallelize execution
    while(travel_end<size_forest)
    {
	while(travel_end<size_forest && no_dependency_from_start(stream_input_array,stream_output_array, travel_start,travel_end))
	{
	    
	    travel_end++;
	}

	int exec_start = travel_start;
	int exec_end = travel_end;
	int execute_index ;
	if (exec_start != exec_end)
	{
	    fork_to_parallel(exec_end-exec_start,exec_start,s);
	}
	
	
	//printf("zone %d from %d -- %d valid.\n",zone_count,travel_start,travel_end-1);
	zone_count++;
	travel_start = travel_end;
    }*/
}