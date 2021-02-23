#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define SEESH_RL_BUFSIZE 512
#define SEESH_TOK_BUFSIZE 64
#define SEESH_TOK_DELIM " \t\r\n\a"

/* Function declarations for built-in commands */
int seesh_cd(char **args);
int seesh_help(char **args);
int seesh_exit(char **args);
int seesh_pwd(char **args);
int seesh_set(char **args);
int seesh_unset(char **args);

/* List of built-in commands */
char *builtin_str[] = {
	"cd",
	"help",
	"exit",
	"pwd",
	"set",
	"unset"
};

int (*builtin_func[]) (char **) = {
	&seesh_cd,
	&seesh_help,
	&seesh_exit,
	&seesh_pwd,
	&seesh_set,
	&seesh_unset
};

int seesh_num_builtins(){
	return sizeof(builtin_str) / sizeof(char *);
}

/* Implement the built-in functions */

/* 
	Destroys the environment variable passed in
	Errors if the argument is a null pointer, points to an empty string, or points to a string that contains "="
*/
int seesh_unset(char **args)
{
	char *variable = args[1];
	if (unsetenv(variable))
		perror("SEEsh: error");
	
	return 1;
}

/* 
	Sets the environment variable passed in
	Errors if the name argument is a null pointer, points to an empty string, or points to a string that contains '='
*/
int seesh_set(char **args)
{
	if (setenv(args[1], args[2] == NULL ? "" : args[2], 1)) {
		perror("SEEsh: Error\n");
	}

	return 1;
}

int seesh_pwd(char **args)
{
	char pwd[SEESH_RL_BUFSIZE];
	if (getcwd(pwd, sizeof(pwd)) != NULL)
		printf("%s\n", pwd);
	else
		perror("SEESH: pwd error");

	return 1;
}

int seesh_cd(char **args)
{
	if (args[1] == NULL) {
		fprintf(stderr, "SEEsh: expected argument to \"cd\"\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("SEEsh: Error");
		}
	}
	return 1;
}

int seesh_help(char **args)
{
	printf("Welcome to Ashley Van Spankeren's SEEsh.\n");
	printf("Type program names and arguments then hit enter.\n");
	printf("The following are built-in:\n");
	
	for (int i = 0; i < seesh_num_builtins(); i++){
		printf("	%s\n", builtin_str[i]);
	}
	
	return 1;
}

int seesh_exit(char **args)
{
	printf("Goodbye!\n");
	return 0;
}

int seesh_launch(char **args)
{
	pid_t pid;
	int status;
	
	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0], args) == -1) {
			perror("SEEsh: Error");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error forking
		perror("SEEsh: Error");
	} else {
		// Parent process
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	
	return 1;
}

int seesh_execute(char **args)
{
	if (args[0] == NULL){
		// No command entered
		return 1;
	}
	
	for (int i = 0; i < seesh_num_builtins(); i++){
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}
	
	return seesh_launch(args);
}

char **seesh_split_line(char *line)
{
	int bufsize = SEESH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;
	
	if (!tokens) {
		fprintf(stderr, "SEEsh: allocation error\n");
		exit(EXIT_FAILURE);
	}
	
	token = strtok(line, SEESH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;
		
		if (position >= bufsize) {
			bufsize += SEESH_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				fprintf(stderr, "SEEsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
		
		token = strtok(NULL, SEESH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

char *seesh_read_line(void)
{
	int bufsize = SEESH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "SEEsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, SIG_IGN);
  	while (1) {

		// Read a character
		c = getchar();

		// If we hit EOF, replace it with a null character and return.
		if (c == EOF){
			free(buffer);
			exit(EXIT_SUCCESS);
		}
		if (c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		// If we have exceeded the buffer, error and exit.
		if (position >= bufsize) {
			fprintf(stderr, "SEEsh: command exceeded allowable length\n");
			exit(EXIT_FAILURE);
		}

  	}
}

void seesh_loop(void)
{
	char *line;
	char **args;
	int status;
	
	do {
		printf("? ");
		line = seesh_read_line();
		args = seesh_split_line(line);
		status = seesh_execute(args);
		
		free(line);
		free(args);
	} while (status);
}

int main(int argc, char **argv)
{
	// Load config files if they exist.
	char *home = getenv("HOME");
	char *rc_file = "/.SEEshrc";
	const size_t len_home = strlen(home);
    const size_t len_rc = strlen(rc_file);
    char *rc_path = malloc(len_home + len_rc + 1);
    // in real code you would check for errors in malloc here
	if (rc_path){
		memcpy(rc_path, home, len_home);
		memcpy(rc_path + len_home, rc_file, len_rc + 1); 
	}

	char *line = NULL;
    size_t len = 0;
	char **args;
	// If .SEEshrc exists, read and execute its contents
	if (access(rc_path, R_OK) != -1) {
		FILE *rc = fopen(rc_path, "r");
		if (rc == NULL) {
			exit(EXIT_FAILURE);
		}
		while (getline(&line, &len, rc) != -1) {
			args = seesh_split_line(line);
			seesh_execute(args);
			free(args);
		}
		free(line);
		fclose(rc);
	}
	free(rc_path);
	// Run command loop.
	seesh_loop();

	// Perform any shutdown/cleanup

	return EXIT_SUCCESS;
}
