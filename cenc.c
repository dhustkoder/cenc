#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>


#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#define PRINT_TITLE(title) \
	printf("\n\n\n%s %s %s\n\n",\
	       &"::::::::::::::::::::::::::::::::::::::::"[(strlen(title)/2)],\
	       title,\
	       &"::::::::::::::::::::::::::::::::::::::::"[(strlen(title)/2)])

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

struct pack {
	unsigned char* buffer;
	long size;
};


static void terminate(const char* const error_msg)
{
	fprintf(stderr, "ABORTING CENC: %s\n", error_msg);
	exit(EXIT_FAILURE);
}

static void parse_dir_files(const char* dirpath,
                            void(*clbk)(const char*, const char*, void*),
                            void* user_data)
{
	DIR* dirp = opendir(dirpath);
	if (dirp == NULL)
		terminate(strerror(errno));

	struct dirent* direntp;
	while ((direntp = readdir(dirp)) != NULL) {
		if (direntp->d_type == DT_REG) {
			clbk(dirpath, direntp->d_name, user_data);
		} else if (direntp->d_type == DT_DIR &&
		          (strcmp(direntp->d_name, ".") != 0) &&
		          (strcmp(direntp->d_name, "..") != 0)) {
			char subdir[strlen(dirpath) + strlen(direntp->d_name) + 1];
			sprintf(subdir, "%s/%s", dirpath, direntp->d_name);
			parse_dir_files(subdir, clbk, user_data);
		}
	}

	closedir(dirp);
}

static void cnt_files_clbk(const char* dirpath,
                           const char* filename,
                           void* user_data)
{
	((void)dirpath);
	((void)filename);
	struct file_array* const fa = (struct file_array*)user_data;
	fa->nfiles += 1;
}

static void open_files_clbk(const char* dirpath,
                            const char* filename,
                            void* user_data)
{
	static int open_idx = 0;
	
	char fullpath[strlen(dirpath) + strlen(filename) + 1];
	sprintf(fullpath, "%s/%s", dirpath, filename);
	
	struct file_array* const fa = (struct file_array*)user_data;
	fa->files[open_idx] = fopen(fullpath, "rb");
	if (fa->files[open_idx] == NULL)
		terminate(strerror(errno));

	open_idx++;
}

static void init_file_array(const char* const rootpath, struct file_array* const fa)
{	
	fa->nfiles = ARRAY_SIZE(main_files);
	fa->files = NULL;

	PRINT_TITLE("INITIALIZING FILE ARRAY");
	printf("Finding files...\n");

	for (unsigned i = 0; i < ARRAY_SIZE(main_dirs); ++i) {
		char path[strlen(rootpath) + strlen(main_dirs[i]) + 1];
		sprintf(path, "%s/%s", rootpath, main_dirs[i]);
		parse_dir_files(path, cnt_files_clbk, fa);
	}
	
	printf("Total files: %d\n", fa->nfiles);

	fa->files = malloc(sizeof(FILE*) * fa->nfiles);
	if (fa->files == NULL)
		terminate(strerror(errno));

	printf("Fetching files into RAM...\n");
	
	for (unsigned i = 0; i < ARRAY_SIZE(main_files); ++i)
		open_files_clbk(rootpath, main_files[i], fa);
	
	for (unsigned i = 0; i < ARRAY_SIZE(main_dirs); ++i) {
		char path[strlen(rootpath) + strlen(main_dirs[i]) + 1];
		sprintf(path, "%s/%s", rootpath, main_dirs[i]);
		parse_dir_files(path, open_files_clbk, fa);
	}

	printf("Files fetched.\n");
}

static void terminate_file_array(struct file_array* const fa)
{
	for (int i = 0; i < fa->nfiles; ++i)
		fclose(fa->files[i]);

	free(fa->files);
}


static long get_file_size(FILE* file)
{
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

static void init_pack(const struct file_array* const fa,
                      struct pack* const pack)
{
	PRINT_TITLE("INITIALIZING BINARY PACKAGE");

	printf("Counting unencrypted package size...\n");
	pack->size = 0;
	for (int i = 0; i < fa->nfiles; ++i)
		pack->size += get_file_size(fa->files[i]);

	printf("Total unecrypted package size expected: %ld bytes\n", pack->size);

	pack->buffer = malloc(sizeof(unsigned char) * pack->size);
	
	printf("Copying files content to package area...\n");
	unsigned char* bufp = pack->buffer;
	for (int i = 0; i < fa->nfiles; ++i) {
		const long size = get_file_size(fa->files[i]);
		bufp += fread(bufp, sizeof(unsigned char), size, fa->files[i]);
	}

	printf("Unencrypted package completed.\n");
}

static void terminate_pack(struct pack* const pack)
{
	free(pack->buffer);
}


static void write_pack_to_file(const struct pack* const pack,
                               const char* const filename)
{
	FILE* file = fopen(filename, "wb");
	if (file == NULL)
		terminate(strerror(errno));
	fwrite(pack->buffer, sizeof(unsigned char), pack->size, file);
	fclose(file);
}

int main(const int argc, const char* const* const argv)
{
	PRINT_TITLE("CENC - C FILE ENCRYPTER VERSION 0.1 BY RAFAEL MOURA (DHUSTKODER)");

	if (argc < 2) {
		printf("Usage: %s [otclient root path]\n", argv[0]);
		return 0;
	}

	const char* const rootpath = argv[1];	
	struct file_array fa;
	struct pack pack;

	printf("Root Path: %s\n", rootpath);
	printf("Main Files:\n");
	for (unsigned i = 0; i < ARRAY_SIZE(main_files); ++i)
		printf("\t%s/%s\n", rootpath, main_files[i]);

	printf("Main Directories:\n");
	for (unsigned i = 0; i < ARRAY_SIZE(main_dirs); ++i)
		printf("\t%s/%s\n", rootpath, main_dirs[i]);


	init_file_array(rootpath, &fa);
	init_pack(&fa, &pack);
	terminate_file_array(&fa);

	printf("Saving unencrypted pack to file... ");
	write_pack_to_file(&pack, "unecrypted.bin");
	printf("done!\n");
	
	terminate_pack(&pack);
	return EXIT_SUCCESS;
}







