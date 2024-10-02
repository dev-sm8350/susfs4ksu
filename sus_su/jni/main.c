#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#define FIFO_SIZE 1024
#define EXTRA_PATH_ENV "/data/adb/ksu/bin:/data/data/com.termux/files/usr/bin"
#define SUS_SU_CONF_FILE "/system/bin/sus_su_drv_path"

static const char *sus_su_token = "!@#$SU_IS_SUS$#@!-pRE6W9BKXrJr1hEKyvDq0CvWziVKbatT8yzq06fhtrEGky2tVS7Q2QTjhtMfVMGV";

extern char **environ;

static char **get_new_envp(void) {
    const char *additional_paths = EXTRA_PATH_ENV;

	// Count existing environment variables
    int env_count = 0;
    while (environ[env_count] != NULL) {
        env_count++;
    }

	// Allocate space for new environment (existing + custom PATH + NULL terminator)
    char **new_environ = malloc((env_count + 2) * sizeof(char *));
    if (new_environ == NULL) {
        perror("malloc");
        return NULL;
    }

	// Copy existing environment variables
    for (int i = 0; i < env_count; i++) {
        new_environ[i] = environ[i];
    }
	
	// Get current 'PATH' value
	const char *orig_path_env = getenv("PATH");
    if (orig_path_env == NULL) {
        perror("PATH environment variable not found");
        return NULL;
    }
	
	// Calc size of new 'PATH'
    size_t new_path_env_size = strlen(orig_path_env) + strlen(additional_paths) + 7; /* 5 for 'PATH=', 1 for ':', 1 for '\0' */

	// Allocate buffer for new 'PATH'
    char *new_path_env_buf = malloc(new_path_env_size);
    if (new_path_env_buf == NULL) {
        perror("Failed to allocate memory\n");
        return NULL;
    }
	
    snprintf(new_path_env_buf, new_path_env_size, "PATH=%s:%s", orig_path_env, additional_paths);
	
	new_environ[env_count] = new_path_env_buf;
	new_environ[env_count + 1] = NULL; // NULL-terminated array

	return new_environ;
}

static int req_root_shell(void) {
	int fd;
    char read_buf[FIFO_SIZE];
	char drv_path[256] = {0};
    ssize_t bytes;
	int res;

	fd = open(SUS_SU_CONF_FILE, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        printf("Failed to open '%s'\n", SUS_SU_CONF_FILE);
        return 1;
    }
	
	res = read(fd, drv_path, 255);
	if (res < 0) {
        perror("Failed to read\n");
		return 1;
	}

	close(fd);

    fd = open(drv_path, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    bytes = write(fd, sus_su_token, sizeof(sus_su_token));
    if (bytes < 0) {
        perror("Failed to write to device");
        close(fd);
        return 1;
    }

    close(fd);
	
	if (getuid() != 0) {
        printf("Failed to get root shell\n");
		return 1;
	}

	printf("Thanks for using sus-su!\n");
    return 0;
}

int main(int argc, char *argv[]) {
    char *args[argc+1];
    char sh[] = "/system/bin/sh";
	char **new_envp = get_new_envp();

	if (!new_envp) {
		printf("Failed to get new envp\n");
	}

    for(int i=1; i<argc; i++) {
        args[i] = argv[i]; 
    }

    args[0] = sh;
    args[argc] = NULL;

	if (!req_root_shell()) {
		return execve(sh, args, new_envp);
	}
    
    return 1;
}


