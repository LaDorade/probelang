#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/dir.h>
#include <sys/dirent.h>
#include <sys/wait.h>

char *getFileExtension(const char *str)
{
	return strrchr(str, '.');
}

bool isCTestFile(const char *str)
{
	return strstr(str, "test.c") != NULL;
}

char **getTestFiles(const char *folder)
{
	DIR *wd = opendir(folder);
	assert(wd != NULL);
	size_t cur = 0;
	char **files = malloc(128 * sizeof(char*));

	struct dirent *entry;

	while ((entry = readdir(wd)) != NULL) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		if (entry->d_type == DT_REG) {
			if (isCTestFile(entry->d_name)) {
				files[cur] = malloc(sizeof(entry->d_name));
                files[cur] = strncat(files[cur], folder, sizeof(entry->d_name));
				files[cur] = strncat(files[cur], entry->d_name, sizeof(entry->d_name));
				cur++;
			}
		}
	}

	files[cur++] = NULL;
	closedir(wd);
	return files;
}

#define TEST_DIR "./tests/"
int main()
{
	char **files = getTestFiles(TEST_DIR);
	size_t count = 0;
	while (files[count] != NULL) {
		printf("Found test file: %s\n", files[count]);

		pid_t pid = fork(); // compilation
		if (pid < 0) {
			printf("Failed fork for compilation: %s\n", strerror(errno));
			return 1;
		}
		if (pid == 0) {
			char *args[] = {"gcc", "-Iinclude/", files[count], "src/parser.c", "src/lexer.c", "-o", "test", NULL};
			if (execvp(args[0], args) < 0) {
				printf("Error executing child proc '%s': %s\n", args[0], strerror(errno));
				exit(1);
			}
			// UNREACHABLE
		}

		int status = 0;
		wait(&status);
		if (WIFEXITED(status)) {
			printf("The process ended with exit(%d).\n", WEXITSTATUS(status));
			if (WEXITSTATUS(status) != 0) {
				return 1;
			}
		} else if (WIFSIGNALED(status)) {
			printf("The process ended with kill -%d.\n", WTERMSIG(status));
		}

		pid = fork(); // run
		if (pid < 0) {
			printf("Failed fork: %s\n", strerror(errno));
			return 1;
		}
		if (pid == 0) {
			char *args[] = {"./test", NULL};
			if (execvp(args[0], args) < 0) {
				printf("Error executing child proc '%s': %s\n", args[0], strerror(errno));
				exit(1);
			}
			// UNREACHABLE
		}

        status = 0;
		wait(&status);
		if (WIFEXITED(status)) {
			printf("The process ended with exit(%d).\n", WEXITSTATUS(status));
			if (WEXITSTATUS(status) != 0) {
				return 1;
			}
		} else if (WIFSIGNALED(status)) {
			printf("The process ended with kill -%d.\n", WTERMSIG(status));
		}
		count++;
	}

	return 0;
}
