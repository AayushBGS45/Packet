/*
 * input_util.h — Safe user-input helpers
 *
 * All functions handle:
 *   - Non-numeric input (letters, symbols).
 *   - Out-of-range values.
 *   - Empty lines.
 *   - EOF (Ctrl+Z on Windows).
 *
 * None of the functions call exit() on bad input — they return false so
 * the caller can present a proper error message and loop back to the menu.
 */

#ifndef INPUT_UTIL_H
#define INPUT_UTIL_H

#include <stdbool.h>

/**
 * input_flush_line — discard characters up to and including the next
 *                    newline (or EOF).  Call after a failed scanf to clear
 *                    the input buffer.
 */
void input_flush_line(void);

/**
 * input_read_int — read one integer from stdin.
 *
 * @param prompt  Text to display before reading (pass "" to skip).
 * @param out     Receives the parsed integer on success.
 * @return        true on success, false on invalid input or EOF.
 */
bool input_read_int(const char *prompt, int *out);

/**
 * input_read_int_range — read an integer and check it is in [lo, hi].
 *
 * @return true if the value was read AND is in range.
 */
bool input_read_int_range(const char *prompt, int lo, int hi, int *out);

/**
 * input_read_router_id — read a router index and validate it is in
 *                        [0, num_routers).
 *
 * @param prompt       Displayed before reading.
 * @param num_routers  Upper bound (exclusive).
 * @param out          Receives the router index.
 * @return             true on success.
 */
bool input_read_router_id(const char *prompt, int num_routers, int *out);

/**
 * input_read_double — read one double from stdin.
 *
 * @return true on success.
 */
bool input_read_double(const char *prompt, double *out);

/**
 * input_read_string — read a line of text (up to max_len - 1 chars).
 *
 * @param prompt   Displayed before reading.
 * @param buf      Output buffer.
 * @param max_len  Size of buf (including NUL).
 * @return         true on success.
 */
bool input_read_string(const char *prompt, char *buf, int max_len);

#endif /* INPUT_UTIL_H */
