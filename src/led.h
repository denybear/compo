/** @file led.h
 *
 * @brief This file defines prototypes of functions inside process.c
 *
 */

int color_ui_cursor ();
void led_ui_instruments ();
void led_ui_pages ();
void led_ui_bars (int, int);
void led_ui_bar (int, int, int);
void led_ui_instrument (int);
void led_ui_page (int);
uint8_t led_ui_select (int, int);
void refresh_ui_bars ();
