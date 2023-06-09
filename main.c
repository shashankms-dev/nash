#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include "term_control.h"

#define TOKEN_BUFSIZE 64
#define TOKEN_DELIMITER " \t\r\n\a"

char *gen_prompt(void) {
    uid_t euid = geteuid();
    struct passwd *pws = getpwuid(euid);
    char *username = pws->pw_name;

    char hostname[160];

    FILE *fp = fopen("/etc/hostname", "r");
    fscanf(fp, "%s", hostname);
    fclose(fp);

    size_t length = strlen(username) + strlen(hostname) + 5;
    char *prompt = malloc(sizeof( char[length] ));
    memset(prompt, 0, length);

    strcat(prompt, "[");
    strcat(prompt, username);
    strcat(prompt, "@");
    strcat(prompt, hostname);
    strcat(prompt, "]");

    if(euid == 0) {
        strcat(prompt, "#");
    }
    else {
        strcat(prompt, "$");
    }

    return prompt;
}

char *readline(void) {
	char *line = NULL;
	ssize_t bufsize = 0;
	
	if (getline(&line, &bufsize, stdin) == -1) {
		if (feof(stdin)) {
			exit(EXIT_SUCCESS);
		}
		else {
			perror("nash");
			exit(EXIT_FAILURE);
		}
	}

	return line;
}

char **tokenize(char *line) {
	int bufsize = TOKEN_BUFSIZE, count = 0;
	char **tokens = (char **) malloc(bufsize * sizeof(char*));
	char *token;

	if (tokens == NULL) {
		fprintf(stderr, "nash: Fatal memory allocation error \n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, TOKEN_DELIMITER);
	while(token != NULL) {
		tokens[count++] = token;

		if(count >= bufsize) {
			bufsize += TOKEN_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));

			if (tokens == NULL ) {
				fprintf(stderr, "nash: Fatal memory allocation error \n");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, TOKEN_DELIMITER);
	}
	tokens[count] = NULL;
	return tokens;
}

int launch(char **args) {
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0) {
		if (execvp(args[0], args) == -1) {
			perror("nash");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0) {		// Error forking
		perror("nash");
	}
	else {
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		}while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

int shell_cd(char **args);
int shell_help(char **args);
int shell_exit(char **args);

char *builtin_str[] = {
	"cd",
	"help",
	"exit",
};

int (*builtin_func[]) (char **) = {
	&shell_cd,
	&shell_help,
	&shell_exit
};

int num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

int shell_cd(char **args) {
	if (args[1] != NULL && chdir(args[1]) != 0) {
		perror("nash");
	}

	return 1;
}

int shell_help(char **args) {
	printf("nash: a simple, fast shell for Unix-like systems\n");
	printf("Built-in commands:\n");

	for(int i = 0; i < num_builtins(); i++) {
		printf("  %s\n", builtin_str[i]);
	}

	return 1;
}

int shell_exit(char **args) {
	return 0;
}

int execute(char **args) {
	if (args[0] == NULL) {
		return 1;
	}

	for (int i = 0; i < num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}

	return launch(args);
}

void loop(void) {
	char *prompt;
	char *line;
	char **args;
	int status = 0;

    prompt = gen_prompt();

	do {
		printf("%s%s%s ", TC_B_GRN, prompt, TC_NRM);
		line = readline();
		args = tokenize(line);
		status = execute(args);
		
		free(line);
		free(args);
    } while(status);
    
    free(prompt);
}
 
int main(int argc, char **argv)
{
	// Load config files, if any.
	
	// Run command loop.
	loop();
	
	// Perform any shutdown/cleanup.
	
	return EXIT_SUCCESS;
}
