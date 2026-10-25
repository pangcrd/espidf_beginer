#pragma once

/* v1.0.1 defaults = the board dai ca actually tested with Arduino
 * (NOT the S31 shield ext-4 map from v1.0.0). v1.0.0 defaulted to
 * shield I2C0 SDA GPIO1 / SCL GPIO2 / RST GPIO38 / INT GPIO39, which
 * is why a v1.0.0 run on this board got no ACK at all: the driver was
 * talking on the wrong pins. This file is the SINGLE place to change
 * pins / address / transform. Do NOT duplicate these values in main
 * or in the LVGL example: the example only calls
 * ft6336g_set_display_size() / set_transform() when it knows the real
 * display size, it never re-inverts coordinates.
 *
 * Board map used here (dai ca, 2026-10-25, Arduino-proven):
 *   I2C0 SDA = GPIO17, SCL = GPIO18
 *   RST = GPIO16, INT = GPIO9
 *   Address 0x38 (confirmed on Arduino, same as here)
 * Shield ext-4 users: change these back to SDA 1 / SCL 2 / RST 38 /
 * INT 39 and set FT6336G_I2C_INTERNAL_PULLUP to 0 (shield already has
 * 4.7k pull-ups to 3V3).
 *
 * Bring-up speed is 100 kHz on purpose, to match the Arduino Wire
 * default that already proved this board. Raise it to 400000 only
 * after poll works at 100 kHz. Datasheet Table 2-2 allows 10..400 kHz.
 */

#ifndef FT6336G_I2C_PORT
#define FT6336G_I2C_PORT 0
#endif

#ifndef FT6336G_SDA_GPIO
#define FT6336G_SDA_GPIO 16
#endif

#ifndef FT6336G_SCL_GPIO
#define FT6336G_SCL_GPIO 15
#endif

/* 7-bit slave address. */
#ifndef FT6336G_I2C_ADDR
#define FT6336G_I2C_ADDR 0x38
#endif

/* 100 kHz first (Arduino parity). Datasheet max is 400 kHz. */
#ifndef FT6336G_I2C_SPEED_HZ
#define FT6336G_I2C_SPEED_HZ 400000
#endif

/* 1 = enable the ESP32 internal pull-ups on SDA/SCL. Arduino Wire
 * enables them, and this board has no guaranteed external pull-ups
 * (unlike the shield). An internal pull-up in parallel with an
 * external 4.7k is harmless, so leaving this at 1 is the safe first
 * choice; set it to 0 only if the board is known to have its own. */
#ifndef FT6336G_I2C_INTERNAL_PULLUP
#define FT6336G_I2C_INTERNAL_PULLUP 1
#endif

/* -1 = pin not wired / reset handled externally. */
#ifndef FT6336G_RST_GPIO
#define FT6336G_RST_GPIO 18
#endif

/* -1 = polling only (this driver polls; INT is not used as an IRQ yet). */
#ifndef FT6336G_INT_GPIO
#define FT6336G_INT_GPIO 17
#endif

/* Display size in pixels. 0 = unknown: coordinates pass through raw and
 * inversion is a no-op, because x = width-1-x needs a real width.
 * The LVGL example fills this from the real display at runtime. */
#ifndef FT6336G_DISP_WIDTH
#define FT6336G_DISP_WIDTH 0
#endif

#ifndef FT6336G_DISP_HEIGHT
#define FT6336G_DISP_HEIGHT 0
#endif

/* Coordinate transform, applied in exactly one place: ft6336g_read().
 * Leave all three at 0 first, touch the panel, and only then flip the
 * axis that is actually reversed. Never flip the same axis here and
 * again in the LVGL example. */
#ifndef FT6336G_SWAP_XY
#define FT6336G_SWAP_XY 0
#endif

#ifndef FT6336G_INVERT_X
#define FT6336G_INVERT_X 0
#endif

#ifndef FT6336G_INVERT_Y
#define FT6336G_INVERT_Y 0
#endif

#ifndef FT6336G_I2C_TIMEOUT_MS
#define FT6336G_I2C_TIMEOUT_MS 100
#endif