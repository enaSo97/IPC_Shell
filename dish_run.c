#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>

#include "dish_run.h"
#include "dish_tokenize.h"


/**
 * Print a prompt if the input is coming from a TTY
 */
static void prompt(FILE *pfp, FILE *ifp)
{
	if (isatty(fileno(ifp))) {
		fputs(PROMPT_STRING, pfp);
	}
}

/**
 * Actually do the work
 */

int count_pipe(char ** tokens, int nTokens)
{
    int pipes = 0;
    for (int i = 0; i < nTokens; i++)
    {
        if (strncmp(tokens[i], "|", strlen(tokens[i])) == 0)
        {
            pipes++;
        }
    }
    return pipes;
}
int execFullCommandLine(
		FILE *ofp,
		char ** const tokens,
		int nTokens,
		int verbosity)
{
	if (verbosity > 0) {
		fprintf(stderr, " + ");
		fprintfTokens(stderr, tokens, 1);
	}
    int pipefds[2];
	/** Now actually do something with this command, or command set */
    /** Now actually do something with this command, or command set */
    int pipe_nums = count_pipe(tokens, nTokens);

    int status, child_exit_pid, current = 0, idx = 0;
    char **parse= calloc(10000, sizeof(char*));
    while(current < nTokens) {
        if (strncmp(tokens[current], "|", strlen(tokens[current])) != 0)
        {
            parse[idx] = calloc(strlen(tokens[current]) + 10, sizeof(char));
            strcpy(parse[idx], tokens[current]);
            printf("%s ", parse[current]);
            current++;
            idx++;
        }else break;
    }
    parse[idx] = calloc(100, sizeof(char)); parse[idx] = NULL;

    current++;
    idx = 0;
    char** parse2 = calloc(10000, sizeof(char*));
    while(current < nTokens) {
        parse2[idx] = calloc(strlen(tokens[current]) + 10, sizeof(char));
        strcpy(parse2[idx], tokens[current]);
        printf("%s ", parse2[idx]);
        current++;idx++;
    }
    parse2[idx] = calloc(100, sizeof(char)); parse2[idx] = NULL;



    current = 0;
    if (strcmp(tokens[current], "exit") == 0){
        exit(0);
    }
    if(pipe(pipefds) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    int pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(-1);
    }
    if (pid == 0)
    {
        //child process
        close(STDOUT_FILENO);  //closing stdout
        dup(pipefds[1]);         //replacing stdout with pipe write
        close(pipefds[0]);       //closing pipe read
        close(pipefds[1]);

        execvp(parse[0], parse);
        perror("execvp of ls failed");
        exit(1);
    }
    int pid2 = fork();
    if (pid2 == 0)
    {
        close(STDIN_FILENO);   //closing stdin
        dup(pipefds[0]);         //replacing stdin with pipe read
        close(pipefds[1]);       //closing pipe write
        close(pipefds[0]);

        execvp(parse2[0],parse2);
        perror("execvp of wc failed");
        exit(1);
    }
    else{
        //parent process, waiting for a child to finish

        child_exit_pid = waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == 0)
            {
                printf("Child(<%d>) exited -- Success (<%d>)\n", child_exit_pid, status);
            }else{
                printf("Child(<%d>) exited -- Failed (<%d>)\n", child_exit_pid, status);
            }
        }else{
            printf("Child(<%d>) crashed!\n", child_exit_pid);
        }

    }
    current++;

    //}
	return 1;
}

/**
 * Load each line and perform the work for it
 */
int
runScript(
		FILE *ofp, FILE *pfp, FILE *ifp,
		const char *filename, int verbosity
	)
{
	char linebuf[LINEBUFFERSIZE];
	char *tokens[MAXTOKENS];
	int lineNo = 1;
	int nTokens, executeStatus = 0;

	fprintf(stderr, "SHELL PID %ld\n", (long) getpid());

	prompt(pfp, ifp);
	while ((nTokens = parseLine(ifp,
				tokens, MAXTOKENS,
				linebuf, LINEBUFFERSIZE, verbosity - 3)) > 0) {
		lineNo++;

		if (nTokens > 0) {

			executeStatus = execFullCommandLine(ofp, tokens, nTokens, verbosity);

			if (executeStatus < 0) {
				fprintf(stderr, "Failure executing '%s' line %d:\n    ",
						filename, lineNo);
				fprintfTokens(stderr, tokens, 1);
				return executeStatus;
			}
		}
		prompt(pfp, ifp);
	}

	return (0);
}


/**
 * Open a file and run it as a script
 */
int
runScriptFile(FILE *ofp, FILE *pfp, const char *filename, int verbosity)
{
	FILE *ifp;
	int status;

	ifp = fopen(filename, "r");
	if (ifp == NULL) {
		fprintf(stderr, "Cannot open input script '%s' : %s\n",
				filename, strerror(errno));
		return -1;
	}

	status = runScript(ofp, pfp, ifp, filename, verbosity);
	fclose(ifp);
	return status;
}

