#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <libelf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <link.h>

#include <phdr_data.h>

static struct phdr_data *phdr_data;


static int phdrCallback(struct dl_phdr_info *info, size_t size, void *data)
{
	phdr_data_add(&phdr_data, info);

	return 0;
}

void  __attribute__((constructor))at_startup(void)
{
	char *kcov_solib_path;
	void *p;
	ssize_t written;
	size_t sz;
	int fd;

	kcov_solib_path = getenv("KCOV_SOLIB_PATH");
	if (!kcov_solib_path)
		return;

	fd = open(kcov_solib_path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "kcov-solib: Can't open %s\n", kcov_solib_path);
		return;
	}

	phdr_data = phdr_data_new();

	dl_iterate_phdr(phdrCallback, NULL);

	p = phdr_data_marshal(phdr_data, &sz);
	written = write(fd, p, sz);

	if (written != sz)
		fprintf(stderr, "kcov-solib: Can't write to solib FIFO (%d)\n", written);

	free(p);

	close(fd);

	// Clear from the environment
	putenv("KCOV_SOLIB_PATH");
}
