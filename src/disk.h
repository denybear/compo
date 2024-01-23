/** @file disk.h
 *
 * @brief This file defines prototypes of functions inside disk.c
 *
 */

int get_files_in_directory (char *, uint8_t *);
int load (uint8_t, char *);
int save (uint8_t, char *);
int save_to_midi (uint8_t, char *, int);
void get_colors_from_ui ();
void set_colors_to_ui ();
