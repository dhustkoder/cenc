#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>


#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))


static const char* const main_files[] = {
	"init.lua",
	"otclientrc.lua"
};

static const char* const main_dirs[] = {
	"data"
};

struct file_array {
	int nfiles;
	FILE** files;
};


static void terminate(const char* const error_msg)
{
	fprintf(stderr, "ABORTING CENC: %s\n", error_msg);
	exit(EXIT_FAILURE);
}

static void parse_dir_files(const char* dirpath,
                            void(*clbk)(const struct dirent*, void*),
                            void* user_data)
{
	DIR* dirp = opendir(dirpath);
	if (dirp == NULL)
		terminate(strerror(errno));

	struct dirent* direntp;
	while ((direntp = readdir(dirp)) != NULL) {
		if (direntp->d_type == DT_REG) {
			clbk(direntp, user_data);
		} else if (direntp->d_type == DT_DIR &&
		           (strcmp(direntp->d_name, ".") != 0) &&
			   (strcmp(direntp->d_name, "..") != 0)) {
			char subdir[strlen(dirpath) + strlen(direntp->d_name) + 1];
			sprintf(subdir, "%s/%s", dirpath, direntp->d_name);
			parse_dir_files(subdir, clbk, user_data);
		}
	}
}

static void cnt_files_clbk(const struct dirent* direntp, void* user_data)
{
	int* n = (int*)&user_data;
	*n += 1;
	printf("FILE FOUND: %s\n", direntp->d_name);
}

static void init_file_array(const char* const rootpath, struct file_array* const fa)
{	
	fa->nfiles = 0;
	fa->files = NULL;

	printf("FETCHING FILES TO PACK...\n");
	for (unsigned i = 0; i < ARRAY_SIZE(main_dirs); ++i) {
		char path[strlen(rootpath) + strlen(main_dirs[i]) + 1];
		sprintf(path, "%s/%s", rootpath, main_dirs[i]);
		parse_dir_files(path, cnt_files_clbk, &fa->nfiles);
	}
	
	printf("TOTAL FILES: %d\n", fa->nfiles);
}

static void terminate_file_array(struct file_array* const fa)
{
	for (int i = 0; i < fa->nfiles; ++i)
		fclose(fa->files[i]);

	free(fa->files);
}


int main(const int argc, const char* const* const argv)
{
	if (argc < 2) {
		printf("Usage: %s [otclient root path]\n", argv[0]);
		return 0;
	}

	const char* const rootpath = argv[1];
	
	struct file_array fa;

	init_file_array(rootpath, &fa);

	//terminate_file_array(&fa);

	return 0;
}







