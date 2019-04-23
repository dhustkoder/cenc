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
	char** paths;
	int cnt;
};

struct pack {
	unsigned char* buffer;
	long size;
};


static void parse_files(const char* const path,
                        void(* const clbk)(const char*, const char*, void*),
                        void* user_data)
{
	DIR* const dirp = opendir(path);
	if (dirp == NULL) {
		FILE* const file = fopen(path, "rb");
		if (file != NULL) {
			fclose(file);
			char tmp[strlen(path) + 1];
			strcpy(tmp, path);
			char* const slashaddr = strrchr(tmp, '/');
			if (slashaddr != NULL) {
				*slashaddr = '\0';
				clbk(tmp, slashaddr + 1, user_data);
			} else {
				clbk("./", tmp, user_data);
			}
			return;
		} else {
			TERMINATE_EX(strerror(errno), path);
		}
	}

	struct dirent* direntp;
	while ((direntp = readdir(dirp)) != NULL) {
		if (direntp->d_type == DT_REG) {
			clbk(path, direntp->d_name, user_data);
		} else if (direntp->d_type == DT_DIR &&
		          (strcmp(direntp->d_name, ".") != 0) &&
		          (strcmp(direntp->d_name, "..") != 0)) {
			char subdir[strlen(path) + strlen(direntp->d_name) + 2];
			sprintf(subdir, "%s/%s", path, direntp->d_name);
			parse_files(subdir, clbk, user_data);
		}
	}

	closedir(dirp);
}

static void get_files_clbk(const char* const dirpath,
                           const char* const filename,
                           void* const user_data)
{
	struct file_array* const fa = (struct file_array*)user_data;

	const int idx = fa->cnt++;

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
}

static void init_file_array(const char* const* paths, 
                            const int npaths,
                            struct file_array* const fa)
{	
	fa->cnt = 0;
	fa->paths = NULL;

	PRINT_TITLE("FILE ARRAY");
	printf("Fetching files... ");
	fflush(stdout);
	for (int i = 0; i < npaths; ++i)
		parse_files(paths[i], get_files_clbk, fa);
	
	printf("%d files fetched!\n", fa->cnt);
}

static void terminate_file_array(struct file_array* const fa)
{
	for (int i = 0; i < fa->cnt; ++i)
		free(fa->paths[i]);
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

	pack->size = 0;
	pack->buffer = NULL;
	
	printf("Copying files content to package buffer... ");
	fflush(stdout);
	for (int i = 0; i < fa->cnt; ++i) {
		FILE* const file = fopen(fa->paths[i], "rb");
		if (file == NULL)
			TERMINATE_EX(strerror(errno), fa->paths[i]);
		const long size = get_file_size(file);
		pack->buffer = realloc(pack->buffer, pack->size + size);
		if (pack->buffer == NULL)
			TERMINATE(strerror(errno));
		pack->size += fread(&pack->buffer[pack->size], 1, size, file);
		fclose(file);
	}

	printf("Done!\n");
	printf("Unencrypted package size: %ld bytes\n", pack->size);
}

static void terminate_pack(struct pack* const pack)
{
	free(pack->buffer);
}



static void write_pack_to_file(const struct pack* const pack,
                               const char* const filename)
{
	FILE* const file = fopen(filename, "wb");
	if (file == NULL)
		TERMINATE(strerror(errno));
	
	printf("Writing file: %s... ", filename);
	fflush(stdout);
	fwrite(pack->buffer, sizeof(unsigned char), pack->size, file);
	printf("Done!\n");
	fclose(file);

}


int main(const int argc, const char* const* const argv)
{
	PRINT_TITLE("CENC - C FILE ENCRYPTER VERSION 0.1 BY RAFAEL MOURA (DHUSTKODER)");

	if (argc < 2) {
		printf("Usage: %s [otclient root path]\n", argv[0]);
		return 0;
	}

	struct file_array fa;
	struct pack pack;

	init_file_array(argv + 1, argc - 1, &fa);
	init_pack(&fa, &pack);
	terminate_file_array(&fa);
	
	write_pack_to_file(&pack, "unencrypted.bin");
	terminate_pack(&pack);
	
	return EXIT_SUCCESS;
}







