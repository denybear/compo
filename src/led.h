/** @file led.h
 *
 * @brief This file defines prototypes of functions inside process.c
 *
 */

void led_ui_instruments ();
void led_ui_pages ();
void led_ui_bars (int, int);
void led_ui_bar (int, int, int);
void led_ui_instrument (int);
void led_ui_page (int);
uint8_t led_ui_select (int, int);

int led_filename (int, int, int);
int filename_led_off (int);
int led_filefunct (int, int, int);
int filefunct_led_off (int);
