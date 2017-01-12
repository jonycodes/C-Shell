/*
 * name: Joannier Pinales
 * I affirm that this program is entirely my own work and none of it is the work
 * of any other person
 * Description: A shell to run scripts from. The shell is executed its own
 * process, each command it's also executed in its own process so it doesn't
 * block the parent process
 * Features: Pipes between commands, redirects input and output to files
 */

/*
 * Importing libraries
 */
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

/*
 * @MAX_ARGS: max number of arguments per command
 * @MAX_CMDS: max number of commands at a time
 */
#define MAX_ARGS 20
#define MAX_CMDS 20

/*
 * Storing the command and its arguments
 */
typedef struct { char *args; } command;

/*
 * @Description: parses arguments from a command
 * @cmdline: command input
 * @args: argument output
 */
int get_args(char *cmdline, char *args[]) {
  int i = 0;

  /* if no args */
  if ((args[0] = strtok(cmdline, "\n\t ")) == NULL)
    return 0;

  /* parsing arguments */
  while ((args[++i] = strtok(NULL, "\n\t ")) != NULL) {
    if (i >= MAX_ARGS) {
      printf("Too many arguments!\n");
      exit(1);
    }
  }

  return i;
}

/*
 * @Description: parses commands from cmdline input
 * @cmdline: cmdline input
 * @cmds[]: output array of commands
 */
int get_cmds(char *cmdline, command *cmds[]) {
  int i = 0;
  char *token;

  /* storing first token */
  token = strtok(cmdline, "\n\t|");

  /* parsing through command line */
  while (token != NULL) {
    /* allocating memory for each command */
    cmds[i] = calloc(1, sizeof(command));
    cmds[i++]->args = token;
    token = strtok(NULL, "\n\t|");
  }
  return i;
}

/*
 * @Description: free memory space from each command
 * @cmds: cmdline input
 * @ncmds: number of commands
 */
void cleanCommands(command *cmds[], int ncmds) {
  int i;
  for (i = 0; i < ncmds; i++) {
    free(cmds[i]);
  }
}

/*
 * @Description: executes a child in case we need to send output to two places (file and pipe)
 * This method is beyond the scope of this assignment
 * @cmd: cmd to run
 * @args: command arguments
 * @fd: pipe
 */
void execute_child(char *cmd, char *args[], int fd[]) {
  int pid;
  /* creating new process */
  switch (pid = fork()) {
  case 0:
    /* Changing output to pipe */
    dup2(fd[1], 1);
    close(fd[0]);
    execvp(cmd, args);
    // If execlp fails
    printf("Execlp failed running ls\n");
    break;
  // Fork Failed
  case -1:
    printf("Fork failed\n");
    exit(1);
    break;
  default:
    break;
  }
}

/*
 * @Description: executes all the commands
 * @cmds: array of commands
 * @ncmds: number of commads
 */
int execute(command *cmds[], int ncmds) {
  pid_t pid;
  int child_status, argsc = 0, i = 0, j, fd[2], in = 0, inputStream,
                    outputStream;
  /* name of output file */
  char *outfilename;
  /* name of input file */
  char *infilename;


  /* executing all commands */
  while (i < ncmds) {
    /* piping i/o */
    if (pipe(fd) == -1) {
      printf("bad pipe%i\n", i);
      exit(1);
    }

    /* initiating writeflag readflag appendflag and pipeflag */
    int wflag = 0, rflag = 0, aflag = 0, pipeflag = 0;
    char *command, *args[MAX_ARGS];

    /* getting the arguments of this command*/
    argsc = get_args(cmds[i]->args, args);

    /* getting command from arguments */
    command = args[0];

    /* if user exits */
    if (!strcmp(command, "quit") || !strcmp(command, "exit")) {
      exit(-1);
    }

    /* parsing argumetns for pipe(|) redirect out(>); redirect in (<); append(>>) */
    for (j = 0; j < argsc; j++) {

      /* if write to file */
      if (!strcmp(args[j], ">")) {
        wflag = 1;
        outfilename = args[j + 1];

        /* clearing the arguments */
        args[j] = NULL;
        args[j++] = NULL;
        if (i != ncmds - 1)
          pipeflag = 1;
      } else
      /* if read from file */
      if (!strcmp(args[j], "<")) {
        rflag = 1;
        infilename = args[j + 1];

        /* clearing the arguments */
        args[j] = NULL;
        args[j++] = NULL;
      } else
      /* if append to file */
       if (!strcmp(args[j], ">>")) {
        aflag = 1;
        outfilename = args[j + 1];
        /* clearing the arguments */
        args[j] = NULL;
        args[j++] = NULL;
        if (i != ncmds - 1)
          pipeflag = 1;
      }
    }


    /* creating child process to execute command */
    switch (pid = fork()) {
    case 0:

      /* get input from previous commnad */
      if (in != 0) {
        dup2(in, 0);
        close(in);
      }

      /* read from file */
      if (rflag) {
        if (!(inputStream = open(infilename, O_RDONLY, 0644))) {
          printf("Error opening file\n");
          exit(1);
        }
        dup2(inputStream, 0);
        close(inputStream);
      }

      /* write to file */
      if (wflag) {
        // printf("wflag from cmd: %i \n", i);
        if (!(outputStream =
                  open(outfilename, O_CREAT | O_TRUNC | O_WRONLY, 0644))) {
          printf("Error opening file\n");
          exit(1);
        }
        if (pipeflag)
          execute_child(command, args, fd);
        dup2(outputStream, 1);
        close(outputStream);
      } else

      /* append to file */
      if (aflag) {
        if (!(outputStream =
                  open(outfilename, O_CREAT | O_APPEND | O_WRONLY, 0666))) {
          printf("Error opening file\n");
          exit(1);
        }
        if (pipeflag)
          execute_child(command, args, fd);
        dup2(outputStream, 1);
        close(outputStream);
      } else

      /* pipe output to next process */
      if (i < ncmds - 1) {
        dup2(fd[1], 1);
        close(fd[1]);
      }

      /* execute command */
      execvp(command, args);
      // If execlp fails
      printf("I don't know what '%s' means\n", command);
      exit(1);
      break;
    // Fork Failed
    case -1:
      printf("Fork failed\n");
      exit(1);
      break;
    default:
      break;
    }
    close(fd[1]);
    in = fd[0];
    i++;
  }

  /* closing pipe in the parent process */
  close(fd[1]);
  close(fd[0]);

  /* Wait for all the children to finish */
  while ((pid = wait(&child_status)) != -1) {}

  return 0;
}

/*
 * @Description: analyzes the command line and calls execute
 * @cmdline: string of commands
 */
int analyzeCmds(char *cmdline) {
  command *cmds[MAX_CMDS];

  /* getting the array of commands */
  int ncmds = get_cmds(cmdline, cmds);

  /* if no commands */
  if (ncmds <= 0)
    return -1;

  /* execute the array of commands */
  execute(cmds, ncmds);

  /* free the space allocated by execute */
  cleanCommands(cmds, ncmds);
  return 0;
}

/* main method */
int main(int argc, char *argv[]) {
  char cmdline[BUFSIZ];

  /* infinte loop */
  for (;;) {
    printf("COP4338$ ");
    if (fgets(cmdline, BUFSIZ, stdin) == NULL) {
      perror("fgets failed");
      exit(1);
    }

    /* analyzing command input and executing */
    if(analyzeCmds(cmdline) != 0){
      printf("No Commands\n");
    };
  }

  return 0;
}
