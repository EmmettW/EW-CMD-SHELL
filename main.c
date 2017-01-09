/***************************************************************************//**
      
      Emmett Wesolowski
      12/4/2016
      optional makefile
      
*******************************************************************************/

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a" 

/*
  Function Declarations for builtin shell commands:
 */
int shell_cd(char **args);
int shell_exit(char **args);
char * parse_file(char *args);
void multi_exec(char *args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "xscript", //execute script
  "exit"
};

int (*builtin_func[]) (char **) = {
  &shell_cd,
  &xscript,
  &shell_exit
};

int shell_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int shell_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "shell: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("shell");
    }
  }
  return 1;
}

/**
  Builtin command: multi
  @param list of args, args[0] is "multi" command, args[1] is the path and filename. <path/file_name>
  @return 1 if the shell should continue running, 0 if it should terminate
*/
char* parse_file(char* filename) {
    char *source = NULL;
    FILE *fp = fopen(filename, "r");
    if (fp != NULL) {
      /* Go to the end of the file. */
      if (fseek(fp, 0L, SEEK_END) == 0) {
          /* Get the size of the file. */
          long bufsize = ftell(fp);
          if (bufsize == -1) { /* Error */ }
  
          /* Allocate our buffer to that size. */
          source = malloc(sizeof(char) * (bufsize + 1));
  
          /* Go back to the start of the file. */
          if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }
  
          /* Read the entire file into memory. */
          size_t newLen = fread(source, sizeof(char), bufsize, fp);
          if ( ferror( fp ) != 0 ) {
              fputs("Error reading file", stderr);
          } else {
              source[newLen++] = '\0'; /* Just to be safe. */
              return source;
          } 
      }
      fclose(fp);
    } 
  free(source); 
  return NULL;
}

/**
  return 0 and exit the shell
 */
int shell_exit(char **args)
{
  return 0;
}

/**
  Launch a program and wait for it to terminate.
  args Null terminated list of arguments (including program).
  always returns 1, to continue execution.
 */
int shell_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("shell");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("shell");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int shell_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < shell_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return shell_launch(args);
}

#define shell_RL_BUFSIZE 1024
/**
   reads the input from the command line
 */
char *shell_read_line(void)
{
  int bufsize = shell_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += shell_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/**
  tokenize the line
   returns Null-terminated array of tokens
 */
char **shell_split_line(char *line)
{
  int bufsize = SHELL_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, SHELL_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += SHELL_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, SHELL_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

void multi_exec(char* args) {
  char* clist[1024*2];
  char* entry;
  char **cmd;

  // get the first entry from the input, seperated by
  // '\n' character 
  entry = strtok(args, "\n");
  clist[0]=malloc(sizeof(entry));
  clist[0]=entry;

  // the following loop gets the rest of the data until the
  // end of the input 
  int j =1;
  while ((entry = strtok(NULL, "\n")) != NULL)
  {
    clist[j]=malloc(sizeof(entry));
    clist[j]=entry;
    j++;
  }

  j=0;
  while(clist[j]!=NULL)
  {
    cmd = shell_split_line(clist[j++]);
    shell_execute(cmd);
  }
}

/**
   Loop to get input and execute it.
 */
void shell_loop(void)
{
  char *line;
  char **args;
  int status;
  size_t buff_size = 100;
  char cwd_buff[buff_size];

  do {
    getcwd(cwd_buff, buff_size);
    printf("%s ? ", cwd_buff);
    line = shell_read_line();
    args = shell_split_line(line);
    if(strcmp(args[0],"multi") == 0) {
      char* string = parse_file(args[1]);
      multi_exec(string);
      free(string);
    }
    else {
      status = shell_execute(args);
    }
    
    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.
	printf("      - Welcome to myShell -\n");
  // Run command loop.
  shell_loop();


  return EXIT_SUCCESS;
}
