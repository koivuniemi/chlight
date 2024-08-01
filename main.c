/*
 * MIT License
 *
 * Copyright (c) 2024 Lauri Koivuniemi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <dirent.h>
#include <libgen.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define BUFF_SIZE 4096

const char HELP[] =
	"Usage: chlight [option] [index|name] [brightness]\n"
	"Options:\n"
	"\t-h --help      | Displays this information\n"
	"\t-v --verbose   | Verbose\n"
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


void args_create(struct args* args, const int argc, char** const argv) {
	args->flags = 0;
	args->id = NULL;
	args->val = NULL;
	for (size_t i = 1; i < argc; i++) {
		if (!strncmp("-h", argv[i], 2) || !strncmp("--help", argv[i], 6)) {
			args->flags |= FLAG_H;
			continue;
		}
		if (!strncmp("-v", argv[i], 2) || !strncmp("--verbose", argv[i], 9)) {
			args->flags |= FLAG_V;
			continue;
		}
		if (args->id == NULL)
			args->id = argv[i];
		else if (args->val == NULL)
			args->val = argv[i];
	}
}

int get_devs(char** devs, int* devs_cap, int* devs_len, char* dirname) {
	int ret = 1;
	DIR* dir = opendir(dirname);
	if (!dir) {
		fprintf(stderr, "Error: %s at diropen\n", __func__);
		goto out;
	}
	for (struct dirent* e = readdir(dir); e != NULL; e = readdir(dir)) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		if (e->d_type == DT_DIR) {
			size_t dirname_oglen = strlen(dirname);
			strcat(dirname, "/");
			strcat(dirname, e->d_name);
			if (get_devs(devs, devs_cap, devs_len, dirname)) {
				fprintf(stderr, "Error: %s at get_file\n", __func__);
				goto out;
			}
			dirname[dirname_oglen] = '\0';
		} else if (strcmp(e->d_name, "brightness") == 0) {
			devs[(*devs_len)++] = strdup(dirname);
		}
	}
	ret = 0;
out:
	if (dir)
		closedir(dir);
	return ret;
}

int readfilenstr(char* dest, const size_t n, const char* const path) {
	int fdata_len = -1;
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "Error: %s at fopen\n", __func__);
		goto out;
	}
	fseek(file, 0, SEEK_END);
	int fdata_size = ftell(file);
	if (fseek(file, 0, SEEK_SET)) {
		fprintf(stderr, "Error: %s at fseek\n", __func__);
		goto out;
	}
	fdata_len = fread(dest, 1, n - 1, file);
	dest[fdata_len] = '\0';
out:
	if (file)
		fclose(file);
	return fdata_len;
}

int iswspace(const char c) {
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

void devs_print_info(char** const devs,
		     const size_t devs_len,
		     const struct args* const args) {
	char path[PATH_MAX];
	char buff0[BUFF_SIZE];
	char buff1[BUFF_SIZE];
	int dname_strmaxlen = 0;
	int bness_strmaxlen = 0;
	for (int i = 0; i < devs_len; i++) {
		int dname_len = strlen(basename(devs[i]));
		if (args->flags & FLAG_V)
			dname_len = strlen(devs[i]);
		if (dname_strmaxlen < dname_len)
			dname_strmaxlen = dname_len;
		snprintf(path, PATH_MAX, "%s/%s", devs[i], "brightness");
		if (readfilenstr(buff0, BUFF_SIZE, path) == -1) {
			fprintf(stderr, "Error: %s at strmaxlen\n", __func__);
			return;
		}
		int bness_len = strlen(strtrimr(buff0));
		if (bness_strmaxlen < bness_len)
			bness_strmaxlen = bness_len;
	}
	for (int i = 0; i < devs_len; i++) {
		char* dname = basename(devs[i]);
		if (args->flags & FLAG_V)
			dname = devs[i];
		snprintf(path, PATH_MAX, "%s/%s", devs[i], "brightness");
		if (readfilenstr(buff0, BUFF_SIZE, path) == -1) {
			fprintf(stderr, "Error: %s at bness\n", __func__);
			return;
		}
		char* bness = strtrimr(buff0);
		snprintf(path, PATH_MAX, "%s/%s", devs[i], "max_brightness");
		if (readfilenstr(buff1, BUFF_SIZE, path) == -1) {
			fprintf(stderr, "Error: %s at maxbness\n", __func__);
			return;
		}
		char* maxbness = strtrimr(buff1);
		printf("%3d %-*s %-*s %s\n",
		       i + 1,
		       dname_strmaxlen, dname,
		       bness_strmaxlen, bness,
		       maxbness);
	}
}

int main(int argc, char** argv) {
	int ret = 1;
	struct args args = {};
	args_create(&args, argc, argv);
	if (args.flags & FLAG_H) {
		puts(HELP);
		ret = 0;
		goto out;
	}


	char* devs[BUFF_SIZE];
	int devs_cap = 0;
	int devs_len = 0;
	char dirname[PATH_MAX] = "/sys/devices";
	if (get_devs(devs, &devs_cap, &devs_len, dirname)) {
		fprintf(stderr, "Failed to find devices\n");
		goto out;
	}
	if (devs_len == 0) {
		fprintf(stderr, "No devices found\n");
		goto out;
	}
	if (args.id == NULL) {
		devs_print_info(devs, devs_len, &args);
		ret = 0;
		goto out;
	}
	if (args.val == NULL) {
		fprintf(stderr, "Missing second parameter\n");
		goto out;
	}


	int id = strtol(args.id, NULL, 10);
	if (id < 0 || id > devs_len) {
		fprintf(stderr, "Id out of range\n");
		goto out;
	}
	char* target_dev;
	if (id == 0) {
		int i = 0;
		for (; i < devs_len; i++) {
			if (strstr(basename(devs[i]), args.id) != NULL)
				break;
		}
		if (i == devs_len) {
			fprintf(stderr, "Could not match with device name\n");
			goto out;
		}
		target_dev = devs[i];
	} else {
		target_dev = devs[id - 1];
	}
	char bness_path[PATH_MAX];
	snprintf(bness_path, PATH_MAX, "%s/%s", target_dev, "brightness");


	uid_t euid = geteuid();
	if (seteuid(0)) {
		fprintf(stderr, "Insufficient privileges (try sudo)\n");
		goto out;
	}
	FILE* file = fopen(bness_path, "w");
	if (file == NULL) {
		goto out;
		fprintf(stderr, "Failed to open (%s)\n", bness_path);
	}
	if (fwrite(args.val, 1, strlen(args.val), file) == 0) {
		fprintf(stderr, "Failed to write to (%s)\n", bness_path);
		goto out;
	}
	if (fclose(file)) {
		fprintf(stderr, "Failed to close (%s)\n", bness_path);
		goto out;
	}
	if (seteuid(euid)) {
		fprintf(stderr, "Failed to drop user privileges\n");
		goto out;
	}


	char bness[BUFF_SIZE];
	if (readfilenstr(bness, BUFF_SIZE, bness_path) == -1) {
		fprintf(stderr, "Failed to read (%s)\n", bness_path);
		goto out;
	}
	if (args.flags & FLAG_V)
		printf("%s %s", target_dev, bness);
	else
		printf("%s %s", basename(target_dev), bness);


	ret = 0;
out:
	for (int i = 0; i < devs_len; i++)
		free(devs[i]);
	return ret;
}
