#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>


#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#define PRINT_TITLE(title) \
	printf("\n%s %s %s\n",\
	       &"::::::::::::::::::::::::::::::::::::::::"[(strlen(title)/2)],\
	       title,\
	       &"::::::::::::::::::::::::::::::::::::::::"[(strlen(title)/2)])


#define TERMINATE_EX(error_msg, subject) { \
	fprintf(stderr, "CENC FATAL: LINE %d :: %s :: %s\n", __LINE__, subject, error_msg); \
	exit(EXIT_FAILURE); \
}

#define TERMINATE(error_msg) TERMINATE_EX(error_msg, "STRERROR")



struct file_array {
	FILE** files;
	char** paths;
	int cnt;
};

struct pack {
	unsigned char* buffer;
	long size;
};


static void parse_dir_files(const char* dirpath,
                            void(*clbk)(const char*, const char*, void*),
                            void* user_data)
{
	DIR* dirp = opendir(dirpath);
	if (dirp == NULL)
		TERMINATE_EX(strerror(errno), dirpath);

	struct dirent* direntp;
	while ((direntp = readdir(dirp)) != NULL) {
		if (direntp->d_type == DT_REG) {
			clbk(dirpath, direntp->d_name, user_data);
		} else if (direntp->d_type == DT_DIR &&
		          (strcmp(direntp->d_name, ".") != 0) &&
		          (strcmp(direntp->d_name, "..") != 0)) {
			char subdir[strlen(dirpath) + strlen(direntp->d_name) + 2];
			sprintf(subdir, "%s/%s", dirpath, direntp->d_name);
			parse_dir_files(subdir, clbk, user_data);
		}
	}

	closedir(dirp);
}

static void open_files_clbk(const char* const dirpath,
                            const char* const filename,
                            void* const user_data)
{
	struct file_array* const fa = (struct file_array*)user_data;

	const int idx = fa->cnt++;

	fa->files = realloc(fa->files, sizeof(FILE*) * fa->cnt);
	if (fa->files == NULL)
		TERMINATE(strerror(errno));

	fa->paths = realloc(fa->paths, sizeof(char*) * fa->cnt);
	if (fa->paths == NULL)
		TERMINATE(strerror(errno));
                                           /* / */
	const int pathlen = strlen(dirpath) + 1 + strlen(filename);
	fa->paths[idx] = malloc(pathlen + 1);
	if (fa->paths[idx] == NULL)
		TERMINATE(strerror(errno));

	sprintf(&fa->paths[idx][0], "%s/%s", dirpath, filename);
	fa->paths[idx][pathlen] = '\0';
	
	fa->files[idx] = fopen(fa->paths[idx], "rb");
	if (fa->files[idx] == NULL)
		TERMINATE_EX(strerror(errno), fa->paths[idx]);
}

static void init_file_array(const char* const* paths, 
                            const int npaths,
                            struct file_array* const fa)
{	
	fa->cnt = 0;
	fa->files = NULL;
	fa->paths = NULL;

	PRINT_TITLE("FILE ARRAY");
	printf("Fetching files... ");

	for (int i = 0; i < npaths; ++i)
		parse_dir_files(paths[i], open_files_clbk, fa);
	
	printf("%d files fetched!\n", fa->cnt);
}

static void terminate_file_array(struct file_array* const fa)
{
	for (int i = 0; i < fa->cnt; ++i) {
		fclose(fa->files[i]);
		free(fa->paths[i]);
	}

	free(fa->files);
	free(fa->paths);
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
	PRINT_TITLE("BINARY PACKAGE");

	printf("Counting unencrypted package required buffer size... ");
	pack->size = 0;
	for (int i = 0; i < fa->cnt; ++i)
		pack->size += get_file_size(fa->files[i]);

	printf("%ld bytes counted!\n", pack->size);

	pack->buffer = malloc(sizeof(unsigned char) * pack->size);
	
	printf("Copying files content to package buffer... ");
	unsigned char* bufp = pack->buffer;
	for (int i = 0; i < fa->cnt; ++i) {
		const long size = get_file_size(fa->files[i]);
		bufp += fread(bufp, sizeof(unsigned char), size, fa->files[i]);
	}

	printf("Done!\n");
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
		TERMINATE(strerror(errno));
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

	const char* const * paths = &argv[1];
	struct file_array fa;
	struct pack pack;

	init_file_array(paths, argc - 1, &fa);
	init_pack(&fa, &pack);
	terminate_file_array(&fa);

	printf("Saving unencrypted pack to file... ");
	write_pack_to_file(&pack, "unecrypted.bin");
	printf("Done!\n");
	
	terminate_pack(&pack);
	return EXIT_SUCCESS;
}







