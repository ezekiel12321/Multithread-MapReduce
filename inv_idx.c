#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kvlist.h"
#include "mr.h"

/**
 * `toLowerStr` is a helper function that turns the string lowercased
 */
char *toLowerStr(char *s) {
  for (char *p = s; *p; p++) {
    *p = tolower(*p);
  }
  return s;
}

/**
 * `mapper` is a function to be passed to `map_reduce`
 *
 * `pair->value` has one line of the text.
 * `mapper` needs to read the line into words and and
 * add ($WORD, $FILENAME) to the `output` list.
 */
void mapper(kvpair_t *pair, kvlist_t *output) {
  char delim[] = "\t\r\n !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
  char *token;
  char *saveptr;

  // use `strtok_r` to split the line into words and add pairs to `output`
  token = strtok_r(pair->value, delim, &saveptr);
  if (token == NULL) {
    return;
  }
  kvlist_append(output, kvpair_new(toLowerStr(token), pair->key));
  while ((token = strtok_r(NULL, delim, &saveptr)) != NULL) {
    kvlist_append(output, kvpair_new(toLowerStr(token), pair->key));
  }
}

/**
 * `reducer` is a function to be passed to `map_reduce`
 *
 * `key` contains the string key to be aggregated.
 * `lst` is a list of pairs. All keys in this list is the same as `key`.
 *
 * `reducer` shows the filename that the words show up in
 *
 */
bool filename_not_checked(char *filename, size_t filename_len,
                          char *filenames_string, size_t fs_len) {
  int *s = (int *)malloc((filename_len + fs_len) * sizeof(int));
  s[0] = 0;
  int border = 0;
  for (size_t i = 1; i < filename_len; i++) {
    while (border > 0 && filename[i] != filename[border]) {
      border = s[border - 1];
    }
    if (filename[i] == filename[border]) {
      border++;
    } else {
      border = 0;
    }
    // printf("Letter: %c\n", filename[i]);
    // printf("Border: %d\n", border);
    // printf("\n");
    s[i] = border;
  }
  // printf("%d\n", s[0]);
  for (size_t i = 0; i < fs_len; i++) {
    while (border > 0 && filenames_string[i] != filename[border]) {
      border = s[border - 1];
    }
    if (filenames_string[i] == filename[border]) {
      border++;
    } else {
      border = 0;
    }
    // printf("Letter: %c\n", filenames_string[i]);
    // printf("Border: %d\n", border);
    // printf("\n");
    s[i + filename_len] = border;
    if (((size_t)s[i + filename_len] == filename_len &&
         (size_t)s[i - 1 + filename_len] != filename_len) &&
        (filenames_string[i + 1] == ' ' || filenames_string[i + 1] == '\0')) {
      if (filenames_string[i - filename_len] != ' ') {
        continue;
      }
      free(s);
      return false;
    }
  }
  free(s);
  return true;
}

void reducer(char *key, kvlist_t *lst, kvlist_t *output) {
  int offset = 1;

  char *filenames_string = (char *)calloc(20, sizeof(char));
  size_t fs_cap = 15;

  filenames_string[0] = ' ';
  filenames_string[1] = '\0';

  // iterate through lst
  kvlist_iterator_t *itor = kvlist_iterator_new(lst);
  for (;;) {
    kvpair_t *pair = kvlist_iterator_next(itor);
    if (pair == NULL) {
      break;
    }
    // find the string, if it is there:
    size_t filename_len = strlen(pair->value);
    size_t fs_len = strlen(filenames_string);
    if (filename_not_checked(pair->value, filename_len, filenames_string,
                             fs_len)) {
      for (size_t i = 0; i < filename_len; i++) {
        if ((size_t)offset == fs_cap) {
          fs_cap *= 2;
          filenames_string = (char *)realloc(filenames_string, fs_cap);
        }
        filenames_string[offset] = pair->value[i];
        offset++;
      }
      filenames_string[offset++] = ' ';
      filenames_string[offset] = '\0';
    }
  }
  kvlist_iterator_free(&itor);

  // turn `sum` back to a string and add to output
  kvlist_append(output, kvpair_new(key, filenames_string));

  free(filenames_string);
}

int main(int argc, char **argv) {
  // parse arguments
  if (argc < 4) {
    fprintf(stderr, "Usage: %s num_mapper num_reducer file...\n", argv[0]);
    return 1;
  }
  int num_mapper = strtol(argv[1], NULL, 10);
  int num_reducer = strtol(argv[2], NULL, 10);
  if (num_mapper <= 0 || num_reducer <= 0) {
    fprintf(stderr, "num_mapper and num_reducer must be positive\n");
    return 1;
  }

  // input is a list of words
  kvlist_t *input = kvlist_new();

  // for each file
  for (int i = 3; i < argc; i++) {
    char *filename = argv[i];
    // open the file
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
      kvlist_free(&input);
      err(1, "%s", filename);
    }
    // variables required for reading the file by line
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    // read the file by line
    // int line_num = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
      // char buffer[1000];
      // sprintf(buffer, "%s:%02d", filename, line_num++);
      // kvlist_append(input, kvpair_new(strdup(buffer), line));
      kvlist_append(input, kvpair_new(filename, line));
    }
    // cleanup
    fclose(fp);
    if (line) {
      free(line);
    }
    line = NULL;
  }

  // create output list
  kvlist_t *output = kvlist_new();

  // call map_reduce
  map_reduce(mapper, num_mapper, reducer, num_reducer, input, output);
  // map_reduce(mapper, num_mapper, num_reducer, input, output);

  // print out the result
  kvlist_print(1, output);

  // cleanup
  kvlist_free(&input);
  kvlist_free(&output);
  return 0;
}
