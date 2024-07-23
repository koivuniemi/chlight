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
	if (file->path != NULL) {
		free(file->path);
		file->path = NULL;
	}
	if (file->filename != NULL) {
		free(file->filename);
		file->path = NULL;
	}
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
					goto err_strdup_id;
			} else if (args->val == NULL) {
				args->val = strdup(argv[i]);
				if (args->val == NULL)
					goto err_strdup_val;
			}
		}
	}
	return 0;
err_strdup_id:
	fprintf(stderr, "Error: %s at strdup id", __func__);
	goto cleanup;
err_strdup_val:
	fprintf(stderr, "Error: %s at strdup val", __func__);
	goto cleanup;
cleanup:
	return 1;
}

void args_destroy(struct args* args) {
	if (args->id != NULL) {
		free(args->id);
		args->id = NULL;
	}
	if (args->val != NULL) {
		free(args->val);
		args->val = NULL;
	}
}

int get_files(struct file** files, size_t* files_cap, size_t* files_len,
			  char* dirname) {
	DIR* dir = opendir(dirname);
	if (!dir)
		goto err_opendir;
	for (struct dirent* e = readdir(dir); e != NULL; e = readdir(dir)) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		if (e->d_type == DT_DIR) {
			size_t dirname_oglen = strlen(dirname);
			strcat(dirname, "/");
			strcat(dirname, e->d_name);
			if (get_files(files, files_cap, files_len, dirname))
				goto err_get_files;
			dirname[dirname_oglen] = '\0';
		} else {
			if (*files_len >= *files_cap) {
				*files_cap += 100;
				*files = realloc(*files, sizeof(**files) * *files_cap);
				if (*files == NULL)
					goto err_realloc_files;
			}
			(*files)[*files_len].path = strdup(dirname);
			(*files)[*files_len].basename = basename((*files)[*files_len].path);
			(*files)[*files_len].filename = strdup(e->d_name);
			(*files_len)++;
		}
	}
	if (closedir(dir))
		goto err_closedir;
	return 0;
err_opendir:
	fprintf(stderr, "Error: %s at diropen\n", __func__);
	goto cleanup;
err_realloc_dirname:
	fprintf(stderr, "Error: %s at realloc dirname\n", __func__);
	goto cleanup;
err_get_files:
	fprintf(stderr, "Error: %s at get_file\n", __func__);
	goto cleanup;
err_realloc_files:
	fprintf(stderr, "Error: %s at realloc files\n", __func__);
	goto cleanup;
err_closedir:
	fprintf(stderr, "Error: %s at closedir\n", __func__);
	/* fall through */
cleanup:
	if (dir != NULL)
		closedir(dir);
	return -1;
}

int find_devices(struct file** devs) {
	size_t devs_cap = 100;
	size_t devs_len = 0;
	*devs = malloc(sizeof(**devs) * devs_cap);
	if (devs == NULL)
		goto err_malloc_devs;
	char dirname[PATH_MAX];
	strcpy(dirname, "/sys/devices");
	if (get_files(devs, &devs_cap, &devs_len, dirname))
		goto err_get_files;
	int bness_devs_len = 0;
	for (size_t i = 0; i < devs_len; i++) {
		if (strcmp((*devs)[i].filename, "brightness") == 0)
			(*devs)[bness_devs_len++] = (*devs)[i];
		else
			file_destroy(&(*devs)[i]);
	}
	*devs = realloc(*devs, sizeof(**devs) * bness_devs_len);
	if (*devs == NULL)
		goto err_realloc;
	return bness_devs_len;
err_malloc_devs:
	fprintf(stderr, "Error: %s at malloc devs\n", __func__);
	goto cleanup;
err_malloc_dirname:
	fprintf(stderr, "Error: %s at malloc dirname\n", __func__);
	goto cleanup;
err_get_files:
	fprintf(stderr, "Error: %s at get_files\n", __func__);
	goto cleanup;
err_realloc:
	fprintf(stderr, "Error: %s at realloc\n", __func__);
	/* fall through */
cleanup:
	for (size_t i = 0; i < devs_len; i++)
		file_destroy(&(*devs)[i]);
	if (*devs != NULL) {
		free(*devs);
		*devs = NULL;
	}
	return -1;
}

int readfilenstr(char* dest, const size_t n, const char* const path) {
	FILE* file = fopen(path, "rb");
	if (file == NULL)
		goto err_fopen;
	fseek(file, 0, SEEK_END);
	int fdata_size = ftell(file);
	if (fseek(file, 0, SEEK_SET))
		goto err_fseek;
	int fdata_len = fread(dest, 1, n - 1, file);
	dest[fdata_len] = '\0';
	if (fclose(file))
		goto err_fclose;
	return fdata_len;
err_fopen:
	fprintf(stderr, "Error: %s at fopen\n", __func__);
	goto cleanup;
err_fseek:
	fprintf(stderr, "Error: %s at fclose\n", __func__);
	goto cleanup;
err_fclose:
	fprintf(stderr, "Error: %s at fclose\n", __func__);
	goto cleanup;
cleanup:
	if (file != NULL)
		fclose(file);
	return -1;
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
	const size_t BUFF_SIZE = 200;
	char buff[BUFF_SIZE];
	size_t buff_len = 0;
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
		if (readfilenstr(buff, BUFF_SIZE, path) == -1)
			goto err_readfilen_strmaxlen;
		int bness_len = strlen(strtrimr(buff));
		if (bness_strmaxlen < bness_len)
			bness_strmaxlen = bness_len;
	}
	for (size_t i = 0; i < devs_len; i++) {
		printf("%3zu ", i + 1);
		char* dname = (*devs)[i].basename;
		if (args->flags & FLAG_V)
			dname = (*devs)[i].path;
		printf("%-*s ", dname_strmaxlen, dname);
		char path[PATH_MAX];
		snprintf(path, PATH_MAX, "%s/%s", (*devs)[i].path, "brightness");
		if (readfilenstr(buff, BUFF_SIZE, path) == -1)
			goto err_readfilen_bness;
		const char* const bness = strtrimr(buff);
		printf("%-*s ", bness_strmaxlen, bness);
		snprintf(path, PATH_MAX, "%s/%s", (*devs)[i].path, "max_brightness");
		if (readfilenstr(buff, BUFF_SIZE, path) == -1)
			goto err_readfilen_maxbness;
		const char* const maxbness = strtrimr(buff);
		printf("%s\n", maxbness);
	}
	return;
err_readfilen_strmaxlen:
	fprintf(stderr, "Error: %s at strmaxlen\n", __func__);
	return;
err_readfilen_bness:
	fprintf(stderr, "Error: %s at bness\n", __func__);
	return;
err_readfilen_maxbness:
	fprintf(stderr, "Error: %s at maxbness\n", __func__);
}

int main(int argc, char** argv) {
	int ret = 0;
	struct file* devs = NULL;
	int devs_len = 0;


	struct args args = {};
	if (args_create(&args, argc, argv))
		goto err_args_create;


	if (args.flags & FLAG_H) {
		puts(HELP);
		goto cleanup;
	}


	devs_len = find_devices(&devs);
	if (devs_len == -1)
		goto err_find_devices;
	if (devs_len == 0)
		goto err_devs_len;
	if (args.id == NULL) {
		print_devs_info(&devs, devs_len, &args);
		goto cleanup;
	}
	if (args.val == NULL)
		goto err_args_val;


	int id = strtol(args.id, NULL, 10);
	struct file target_dev;
	if (id < 0 || id > devs_len)
		goto err_id_range;
	if (id == 0) {
		int i = 0;
		for (; i < devs_len; i++) {
			if (strstr(devs[i].basename, args.id) != NULL)
				break;
		}
		if (i == devs_len)
			goto err_dev_match;
		target_dev = devs[i];
	} else {
		target_dev = devs[id - 1];
	}
	char bness_path[PATH_MAX];
	snprintf(bness_path, PATH_MAX, "%s/%s", target_dev.path, "brightness");


	uid_t euid = geteuid();
	if (seteuid(0))
		goto err_seteuid_to_root;
	FILE* file = fopen(bness_path, "w");
	if (file == NULL)
		goto err_fopen;
	if (fwrite(args.val, 1, strlen(args.val), file) == 0)
		goto err_fwrite;
	if (fclose(file))
		goto err_fclose;
	if (seteuid(euid))
		goto err_seteuid_to_user;


	char bness[200];
	if (readfilenstr(bness, 200, bness_path) == -1)
		goto err_readfilen;
	printf("%s: %s", target_dev.basename, bness);


	goto cleanup;
err_args_create:
	fprintf(stderr, "Failed to handle command line arguments\n");
	goto err_cleanup;
err_find_devices:
	fprintf(stderr, "Failed to find devices\n");
	goto err_cleanup;
err_devs_len:
	fprintf(stderr, "No devices found\n");
	goto err_cleanup;
err_args_val:
	fprintf(stderr, "Missing second parameter\n");
	goto err_cleanup;
err_id_range:
	fprintf(stderr, "Id out of range\n");
	goto err_cleanup;
err_dev_match:
	fprintf(stderr, "Could not match with device name\n");
	goto err_cleanup;
err_seteuid_to_root:
	fprintf(stderr, "Insufficient privileges\n");
	goto err_cleanup;
err_fopen:
	fprintf(stderr, "Failed to open (%s)\n", bness_path);
	goto err_cleanup;
err_fwrite:
	fprintf(stderr, "Failed to write to (%s)\n", bness_path);
	goto err_cleanup;
err_fclose:
	fprintf(stderr, "Failed to close (%s)\n", bness_path);
	goto err_cleanup;
err_seteuid_to_user:
	fprintf(stderr, "Failed to drop user privileges\n");
	goto err_cleanup;
err_readfilen:
	fprintf(stderr, "Failed to read (%s)\n", bness_path);
	/* fall through */
err_cleanup:
	ret = 1;
	/* fall through */
cleanup:
	for (int i = 0; i < devs_len; i++)
		file_destroy(&devs[i]);
	if (devs != NULL) {
		free(devs);
		devs = NULL;
	}
	args_destroy(&args);
	return ret;
}
