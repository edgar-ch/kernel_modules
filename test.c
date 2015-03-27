#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char const *argv[])
{
	int fd;
	char c;

	fd = open("/dev/pipe", O_WRONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}
	c = getchar();
	close(fd);

	return 0;
}