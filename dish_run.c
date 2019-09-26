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

void print(char** tokens)
{
    for(int i = 0; tokens[i] != NULL; i++)
    {
        printf("{%s} ", tokens[i]);
    }
    printf("\n");
}

char ** parse_command(char**tokens, int nTokens, int curr_cmd_num, char * toParse)
{
    int cmd_cnt = 0, idx = 0;
    char** parsed_cmd = calloc(sizeof(*parsed_cmd), 18);
    for (int i = 0; i < nTokens; i++)
    {
        if (strncmp(tokens[i], toParse, strlen(tokens[i])) != 0)
        {
            parsed_cmd[idx] = calloc(sizeof(char), strlen(tokens[i]));
            strncpy(parsed_cmd[idx], tokens[i], strlen(tokens[i]));
            idx++;
            parsed_cmd[idx] = calloc(100, sizeof(char));
            parsed_cmd[idx] = NULL;
        }
        else if (strncmp(tokens[i], "|", strlen(tokens[i])) == 0 || tokens[i+1] == NULL)
        {
            cmd_cnt++;
            if (cmd_cnt == curr_cmd_num)
            {
                break;
            }else{
                for(int j = 0; j <= idx; j++)
                {
                    free(parsed_cmd[j]);
                }
                idx = 0;
            }
        }

    }
    return parsed_cmd;
}

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
        FILE *ifp,
		char ** const tokens,
		int nTokens,
		int verbosity)
{
	if (verbosity > 0) {
		fprintf(stderr, " + ");
		fprintfTokens(stderr, tokens, 1);
	}
    if (strcmp(tokens[0], "exit") == 0){
        exit(0);
    }
    if (strcmp(tokens[0], "pwd") == 0)
    {
        char * pwd = calloc(sizeof(char), 200);
        if (getcwd(pwd, 200) == NULL)
        {
            perror("pwd error");
        }else{
            printf("%s\n", pwd);
        }
        free(pwd);
    }
    if (strcmp(tokens[0], "cd") == 0)
    {
        if (nTokens > 1){
            chdir(tokens[1]);
        }else{
            chdir(getenv("HOME"));
        }


    }else{
        if (strcmp(tokens[nTokens - 1], "&") == 0)//if the last token is & then run background process
        {
            tokens[nTokens - 1] = 0; nTokens -= 1;
            int pid = fork();

            if(pid == 0)
            {
                int grand_nTokens;
                char *bg_tokens[MAXTOKENS];
                char linebuf[LINEBUFFERSIZE];
                printf("running background process\n");
                grand_nTokens = parseLine(ifp, bg_tokens, MAXTOKENS, linebuf, LINEBUFFERSIZE, verbosity - 3);
                execFullCommandLine(ofp, ifp, bg_tokens, grand_nTokens, verbosity);
            }
            if (pid < 0)
            {
                perror("background fork");
                exit(1);
            }
            wait(NULL);
        }
        int nPipes = count_pipe(tokens, nTokens);
        int pipefds[2*nPipes];
        for (int i = 0; i < nPipes; i++)
        {
            if (pipe(pipefds + i*2) < 0)
            {
                perror("pipe"); exit(1);
            }
        }
        int idx = 0, j = 0, current = 0;
        while(current < nPipes+1) {
            char ** command = parse_command(tokens, nTokens, current+1, "|");
            print(command);
            int pid = fork();
            if (pid < 0){
                perror("error");
                exit(1);
            }
            if (pid == 0){
                if (current < nPipes)
                {
                    //not last command
                    if (dup2(pipefds[j + 1],1) < 0)
                    {
                        perror("dup2"); exit(1);
                    }
                }
                if (current > 0)
                {
                    //not first command,
                    if (dup2(pipefds[j - 2], 0) < 0)
                    {
                        perror("dup2"); exit(1);
                    }
                }

                for (int i = 0; i < nPipes*2; i++)
                {
                    close(pipefds[i]);
                }
                if (execvp(command[0], command) < 0)
                {
                    perror("execvp"); exit(1);
                }
                for (int k = 0; command[k] != NULL; k++){
                    free(command[k]);
                }
                free(command);
            }
            for (int i = 0; command[i] == NULL; i++){
                free(command[i]);
            }
            j +=2;
            current++;
        }

        for (int i = 0; i < 2*nPipes; i++)
        {
            close(pipefds[i]);
        }
        for (int i = 0; i < nPipes+1; i++)
        {
            wait(NULL);
        }
    }



/*

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

    }*/
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
    printf("%s\n", filename);
	prompt(pfp, ifp);
	while ((nTokens = parseLine(ifp,
				tokens, MAXTOKENS,
				linebuf, LINEBUFFERSIZE, verbosity - 3)) > 0) {
		lineNo++;

		if (nTokens > 0) {

			executeStatus = execFullCommandLine(ofp, ifp, tokens, nTokens, verbosity);

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

