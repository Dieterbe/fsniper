#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <wordexp.h>
#include "keyvalcfg.h"
#include "watchnode.h"

extern struct keyval_section *config;

/* recursively add watches */
void recurse_add(int fd, char* directory, struct watchnode* node)
{
  struct stat dir_stat;
  struct dirent* entry;
	char* path;
  DIR* dir = opendir(directory);

	if (dir == NULL)
	{
		return;
	}

	while ( (entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0)
			continue;
		if (strcmp(entry->d_name, "..") == 0)
			continue;
		path = malloc(strlen(directory) + strlen(entry->d_name) + 2);
		sprintf(path, "%s/%s", directory, entry->d_name);
		if (stat(path, &dir_stat) == -1) {
			continue;
		}
		if (S_ISDIR(dir_stat.st_mode))
		{
			printf("adding watch for: %s\n", path);
			node->wd = inotify_add_watch(fd, path, IN_CREATE);
			node->path = strdup(path);
			node->next = malloc(sizeof(struct watchnode));
			node->next->next = NULL;
			node = node->next;
			recurse_add(fd, path, node);
		}
		free(path);
	}
}

/* parses the config and adds inotify watches to the fd */
struct watchnode* add_watches(int fd, struct keyval_section* config)
{
	struct keyval_section* child;
	char* directory;
	wordexp_t wexp;
	struct watchnode* firstnode;
	struct watchnode* node = malloc(sizeof(struct watchnode));
	node->next = NULL;
	firstnode = node;

	for (child = config->children; child; child = child->next)
	{
		wordexp(child->name, &wexp, 0);
		directory = strdup(wexp.we_wordv[0]);
		node->wd = inotify_add_watch(fd, directory, IN_CREATE);
		node->path = strdup(wexp.we_wordv[0]);
		node->next = malloc(sizeof(struct watchnode));
		node->next->next= NULL;
		node = node->next;
		if (keyval_pair_find(child->keyvals, "recurse")->value != NULL && strcmp(keyval_pair_find(child->keyvals, "recurse")->value, "true") == 0)
			recurse_add(fd, directory, node);
		wordfree(&wexp);
		free(directory);
	}
	return firstnode;
}
