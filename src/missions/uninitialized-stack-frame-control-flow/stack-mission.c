/*
 * SPDX-License-Identifier: BSD-2-Clause-DARPA-SSITH-ECATS-HR0011-18-C-0016
 * Copyright (c) 2020 SRI International
 */

#include <sel4/assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <printf.h>
#include <microkit.h>

#define SERIAL_CHANNEL 1
uintptr_t serial_to_client_vaddr;
uintptr_t client_to_serial_vaddr;

// Helper functions since we don't have a C library
// ------------------------------------------------ //
static char getchar() {
    microkit_ppcall(SERIAL_CHANNEL, microkit_msginfo_new(1, 0));
    return ((char *)serial_to_client_vaddr)[0];
}

static int isxdigit(int c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

static int digittoint(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;  // Not a valid hex digit
}

static int isspace(int c) {
    return c == ' '  ||  // space
           c == '\t' ||  // horizontal tab
           c == '\n' ||  // newline
           c == '\v' ||  // vertical tab
           c == '\f' ||  // form feed
           c == '\r';    // carriage return
}

static void errx(int err, const char *msg) {
    printf("ERROR: %s\n", msg);
    microkit_internal_crash(err);  // Crash the component with a specific error code
}

static void exit(int status) {
    microkit_internal_crash(status);
}
// ------------------------------------------------ //

void
success(void)
{
	printf("Exploit successful, yum!\n");
	exit(42);
}

void
no_cookies(void)
{
	printf("No cookies??\n");
	exit(1);
}

#pragma weak init_pointer
void
init_pointer(void *p)
{
}

static void __attribute__((noinline))
init_cookie_pointer(void)
{
	void *pointers[12];
	void (* volatile cookie_fn)(void);

	for (size_t i = 0; i < sizeof(pointers) / sizeof(pointers[0]); i++)
		init_pointer(&pointers[i]);
	cookie_fn = no_cookies;
}

static void __attribute__((noinline))
get_cookies(void)
{
	alignas(void *) char cookies[sizeof(void *) * 32];
	char *cookiep;
	int ch, cookie;

	printf("Cookie monster is hungry, provide some cookies!\n");
	printf("'=' skips the next %zu bytes\n", sizeof(void *));
	printf("'-' skips to the next character\n");
	printf("XX as two hex digits stores a single cookie\n");
	printf("> ");

	cookiep = cookies;
	for (;;) {
		ch = getchar();

		if (ch == '\n' || ch == '\r' || ch == -1)
			break;

		if (isspace(ch))
			continue;

		if (ch == '-') {
			cookiep++;
			continue;
		}

		if (ch == '=') {
			cookiep += sizeof(void *);
			continue;
		}

		if (isxdigit(ch)) {
			cookie = digittoint(ch) << 4;
			ch = getchar();
			if (ch == -1)
				errx(1, "Half-eaten cookie, yuck!");
			if (!isxdigit(ch))
				errx(1, "Malformed cookie");
			cookie |= digittoint(ch);
			*cookiep++ = cookie;
			continue;
		}

		errx(1, "Malformed cookie");
	}
}

static void __attribute__((noinline))
eat_cookies(void)
{
	void *pointers[12];
	void (* volatile cookie_fn)(void);

	for (size_t i = 0; i < sizeof(pointers) / sizeof(pointers[0]); i++)
		init_pointer(&pointers[i]);
	cookie_fn();
}

void
init(void)
{
	init_cookie_pointer();
	get_cookies();
	eat_cookies();
}

void notified(microkit_channel channel) {
    switch (channel) {
        case SERIAL_CHANNEL: {
            char ch = ((char *)serial_to_client_vaddr)[0];
            microkit_dbg_putc(ch);
            break;
        }
    }
}
