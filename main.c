#include <dirent.h>
#include <libgen.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


const char HELP[] =
	"Usage: chlight [option] [index|name] [brightness|max]\n"
	"Options:\n"
	"\t-h --help      | Displays this information\n"
	"\t-v             | Verbose\n"
	"Examples:\n"
	"\tchlight        | List devices | index device brightness max_brightness\n"
	"\tchlight 1 1000 | Index 1 device changed to brightness 1000\n"
	"\tchlight kbd 3  | Device with 'kbd' in its name changed to 3";


enum flags {
	FLAG_H = 1 << 0,
	FLAG_V = 1 << 1
};

struct args {
	enum flags flags;
	char* id;
	char* val;
};

struct file {
	char* path;
	char* basename;
	char* filename;
};


void file_destroy(struct file* file) {
	if (file->path)
		free(file->path);
	if (file->filename)
		free(file->filename);
}

int args_create(struct args* args, const int argc, char** const argv) {
	args->flags = 0;
	args->id = NULL;
	args->val = NULL;
	for (size_t i = 1; i < argc; i++) {
		if (!strncmp("-h", argv[i], 2)) {
			args->flags |= FLAG_H;
		} else if (!strncmp("-v", argv[i], 2)) {
			args->flags |= FLAG_V;
		} else {
			if (args->id == NULL) {
				args->id = strdup(argv[i]);
				if (args->id == NULL)
					return 1;
			} else if (args->val == NULL) {
				args->val = strdup(argv[i]);
				if (args->val == NULL)
					return 1;
			}
		}
	}
	return 0;
}

void args_destroy(struct args* args) {
	if (args->id != NULL)
		free(args->id);
	if (args->val != NULL)
		free(args->val);
}

int get_files(struct file** files,
			  size_t* files_cap,
			  size_t* files_len,
			  const char* const dirname) {
	DIR* dir = opendir(dirname);
	if (!dir)
		return -1;
	for (struct dirent* e = readdir(dir); e != NULL; e = readdir(dir)) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		if (e->d_type == DT_DIR) {
			char* path = malloc(strlen(dirname) + strlen(e->d_name) + 2);
			if (path == NULL)
				return -1;
			sprintf(path, "%s/%s", dirname, e->d_name);
			if (get_files(files, files_cap, files_len, path))
				return -1;
			free(path);
		} else {
			if (*files_len >= *files_cap) {
				*files_cap += 100;
				*files = realloc(*files, sizeof(**files) * *files_cap);
				if (*files == NULL)
					return -1;
			}
			(*files)[*files_len].path = strdup(dirname);
			(*files)[*files_len].basename = basename(strdup(dirname));
			(*files)[*files_len].filename = strdup(e->d_name);
			(*files_len)++;
		}
	}
	closedir(dir);
	return 0;
}

int find_devices(struct file** devs) {
	size_t devs_cap = 100;
	size_t devs_len = 0;
	*devs = malloc(sizeof(**devs) * devs_cap);
	if (devs == NULL)
		return -1;
	if (get_files(devs, &devs_cap, &devs_len, "/sys/devices"))
		return -1;
	int bness_devs_len = 0;
	for (size_t i = 0; i < devs_len; i++) {
		if (strcmp((*devs)[i].filename, "brightness") == 0)
			(*devs)[bness_devs_len++] = (*devs)[i];
	}
	*devs = realloc(*devs, sizeof(**devs) * bness_devs_len);
	if (*devs == NULL)
		return -1;
	return bness_devs_len;
}

char* read_file_to_str(const char* const path) {
	char* fdata = NULL;
	FILE* file = fopen(path, "rb");
	if (file == NULL)
		return NULL;
	fseek(file, 0, SEEK_END);
	int fdata_size = ftell(file);
	if (fseek(file, 0, SEEK_SET))
		goto close_file;
	fdata = malloc(fdata_size);
	if (fdata == NULL)
		goto close_file;
	int fdata_len = fread(fdata, 1, fdata_size, file);
	fdata = realloc(fdata, fdata_len + 1);
	if (fdata == NULL)
		goto close_file;
	fdata[fdata_len] = '\0';
close_file:
	if (fclose(file))
		return NULL;
	return fdata;
}

int iswspace(char c) {
	if (c == ' ') return 0;
	if (c == '\t') return 0;
	if (c == '\n') return 0;
	return 1;
}

char* strtrimr(char* str) {
	char* end = str + strlen(str) - 1;
	while (!iswspace(*end))
		end--;
	*(end + 1) = '\0';
	return str;
}

void print_devs_info(struct file** const devs,
					 const size_t devs_len,
					 const struct args* const args) {
	int dname_strmaxlen = 0;
	int bness_strmaxlen = 0;
	for (size_t i = 0; i < devs_len; i++) {
		int dname_len = strlen((*devs)[i].basename);
		if (args->flags & FLAG_V)
			dname_len = strlen((*devs)[i].path);
		if (dname_strmaxlen < dname_len)
			dname_strmaxlen = dname_len;
		char path[PATH_MAX];
		snprintf(path, PATH_MAX, "%s/%s", (*devs)[i].path, "brightness");
		int bness_len = strlen(strtrimr(read_file_to_str(path)));
		if (bness_strmaxlen < bness_len)
			bness_strmaxlen = bness_len;
	}
	for (size_t i = 0; i < devs_len; i++) {
		char* dname = (*devs)[i].basename;
		if (args->flags & FLAG_V)
			dname = (*devs)[i].path;
		char path[PATH_MAX];
		snprintf(path, PATH_MAX, "%s/%s", (*devs)[i].path, "brightness");
		const char* const bness = strtrimr(read_file_to_str(path));
		snprintf(path, PATH_MAX, "%s/%s", (*devs)[i].path, "max_brightness");
		const char* const mbness = strtrimr(read_file_to_str(path));
		printf("%3zu %-*s %-*s %s\n",
			i + 1,
			dname_strmaxlen, dname,
			bness_strmaxlen, bness,
			mbness);
	}
}

int main(int argc, char** argv) {
	int ret = 0;
	struct file* devs = NULL;
	int devs_len = 0;


	struct args args = {};
	if (args_create(&args, argc, argv)) {
		fprintf(stderr, "Error while handling command line arguments\n");
		goto exit_error;
	}


	if (args.flags & FLAG_H) {
		puts(HELP);
		goto exit_success;
	}


	devs_len = find_devices(&devs);
	if (devs_len == -1) {
		fprintf(stderr, "Error while finding devices\n");
		goto exit_error;
	}
	if (devs_len == 0) {
		fprintf(stderr, "No devices found\n");
		goto exit_error;
	}
	if (args.id == NULL) {
		print_devs_info(&devs, devs_len, &args);
		goto exit_success;
	}
	if (args.val == NULL) {
		fprintf(stderr, "Missing second parameter\n");
		goto exit_error;
	}


	int id = strtol(args.id, NULL, 10);
	struct file target_dev;
	if (id < 0 || id > devs_len) {
		fprintf(stderr, "Id out of range\n");
		goto exit_error;
	}
	if (id == 0) {
		int i = 0;
		for (; i < devs_len; i++) {
			if (strstr(devs[i].basename, args.id) != NULL)
				break;
		}
		if (i == devs_len) {
			fprintf(stderr, "Could not match with device name\n");
			goto exit_error;
		}
		target_dev = devs[i];
	} else {
		target_dev = devs[id - 1];
	}
	char bness_path[PATH_MAX];
	snprintf(bness_path, PATH_MAX, "%s/%s", target_dev.path, "brightness");


	uid_t euid = geteuid();
	if (seteuid(0)) {
		fprintf(stderr, "Insufficient privileges\n");
		goto exit_error;
	}
	FILE* file = fopen(bness_path, "w");
	if (file == NULL) {
		fprintf(stderr, "Failed to write to (%s)\n", bness_path);
		goto exit_error;
	}
	fwrite(args.val, 1, strlen(args.val), file);
	if (fclose(file)) {
		fprintf(stderr, "Failed to close (%s)\n", bness_path);
		goto exit_error;
	}
	if (seteuid(euid)) {
		fprintf(stderr, "Failed to drop user privileges\n");
		goto exit_error;
	}


	char* fdata = read_file_to_str(bness_path);
	if (fdata == NULL) {
		fprintf(stderr, "Failed to read (%s)\n", bness_path);
		goto exit_error;
	}
	printf("%s: %s", target_dev.basename, fdata);
	free(fdata);


	goto exit_success;
exit_error:
	ret = 1;
exit_success:
	for (int i = 0; i < devs_len; i++)
		file_destroy(&devs[i]);
	if (devs != NULL)
		free(devs);
	args_destroy(&args);
	return ret;
}
