# Component Reference

Reference for the custom components used in this project (ESP32-S3, ESP-IDF v5.5.5).
Target board: ESP32-S3 N16R8, PSRAM enabled, SD card on SDMMC 4-bit.

---

## bsp_sd

Board-specific SD card mounting over SDMMC. Depends on pin wiring, so it lives
under the `bsp_` prefix.

**Header:** `components/bsp_sd/include/bsp_sd.h`

### Types

```c
typedef struct {
    int clk, cmd, d0, d1, d2, d3;   // GPIO numbers
    int width;              // 1 or 4 (bus width)
    int max_freq_khz;       // 0 = default (20 MHz). Use SDMMC_FREQ_HIGHSPEED for 40 MHz.
    bool internal_pullup;   // true = use ESP32 internal pull-ups (test only, use external 10k in production)
} bsp_sd_config_t;
```

### Functions

| Function | Description |
|---|---|
| `esp_err_t bsp_sd_mount(const bsp_sd_config_t *cfg)` | Mounts the SD card at `/sdcard`. Prints card info (type, speed, size) on success. |
| `esp_err_t bsp_sd_unmount(void)` | Unmounts the card. **Always call before physically removing the card** to avoid filesystem corruption. |
| `sdmmc_card_t *bsp_sd_get_card(void)` | Returns the mounted card handle (for raw sector access, diagnostics, etc). NULL if not mounted. |
| `esp_err_t bsp_sd_bench(size_t total_bytes, size_t block_size)` | Writes then reads `total_bytes` in `block_size` chunks to measure throughput (MB/s). Deletes the test file afterward. For diagnostics only — don't call on every boot. |

**Mount point:** `BSP_SD_MOUNT_POINT` = `/sdcard`

### Notes
- SDSC/SDHC/SDXC all supported via standard `esp_vfs_fat_sdmmc_mount`.
- `format_if_mount_failed` is hardcoded `false` inside `bsp_sd_mount` — enable manually in source if you need to format a blank/corrupted card (wipes all data).
- Avoid GPIO 26–37 (used internally by flash/PSRAM on N16R8 octal variant) and strapping pins (0, 3, 45, 46) when choosing `clk/cmd/d0-d3`.

---

## bsp_fs

Generic POSIX file/directory operations. Not board-specific — works on any
mounted VFS filesystem (SD, SPIFFS, LittleFS), despite the `bsp_` prefix
(kept for historical/naming-consistency reasons in this project).

**Header:** `components/bsp_fs/include/bsp_fs.h`

### Functions

| Function | Description |
|---|---|
| `bool bsp_fs_exists(const char *path)` | True if path exists (file or dir). |
| `bool bsp_fs_is_dir(const char *path)` | True if path exists and is a directory. |
| `esp_err_t bsp_fs_file_size(const char *path, size_t *out_size)` | Gets file size in bytes. `ESP_ERR_NOT_FOUND` if missing. |
| `esp_err_t bsp_fs_create_file(const char *path, const void *data, size_t len)` | Creates/overwrites a file with `data`. **Parent directory must already exist** (uses `fopen(..., "wb")`). |
| `esp_err_t bsp_fs_append_file(const char *path, const void *data, size_t len)` | Appends `data` to an existing or new file. |
| `esp_err_t bsp_fs_delete_file(const char *path)` | Deletes a single file. `ESP_ERR_NOT_FOUND` if missing. |
| `esp_err_t bsp_fs_create_dir(const char *path)` | Creates one directory level. OK (no error) if it already exists. Parent must exist. |
| `esp_err_t bsp_fs_create_dir_recursive(const char *path)` | Like `mkdir -p` — creates every missing directory level in `path`. |
| `esp_err_t bsp_fs_create_file_p(const char *path, const void *data, size_t len)` | Like `create_file`, but auto-creates all missing parent directories first. **Use this instead of `create_file` unless you specifically want to fail when the parent dir is missing.** |
| `esp_err_t bsp_fs_delete_dir(const char *path)` | Removes an **empty** directory only. `ESP_FAIL` if not empty. |
| `esp_err_t bsp_fs_delete_recursive(const char *path)` | Deletes a file, or a directory and everything inside it. Refuses to delete the SD mount root (`/sdcard`) as a safety guard. |
| `esp_err_t bsp_fs_list_dir(const char *path, bsp_fs_list_cb_t cb, void *ctx)` | Walks a directory, calling `cb(name, is_dir, size, ctx)` per entry. Return `false` from the callback to stop early. |
| `int bsp_fs_list_by_ext(const char *dir, const char *ext, char names[][256], int max_count)` | Lists filenames in `dir` matching extension `ext` (case-insensitive, e.g. `".jpg"`), sorted alphabetically. Returns count found (≤ `max_count`). |
| `FILE *bsp_fs_fopen_buffered(const char *path, const char *mode, void *vbuf, size_t vbuf_size)` | Opens a file via `fopen` and immediately attaches a larger stdio buffer via `setvbuf`. **Use this instead of raw `fopen` for any sizeable read/write** — see Performance Notes below. |

### Callback type
```c
typedef bool (*bsp_fs_list_cb_t)(const char *name, bool is_dir, size_t size, void *ctx);
```

### Notes
- All paths are absolute and must include the mount prefix (`/sdcard/...`).
- `delete_recursive` and `list_dir` allocate a small heap buffer internally for path building; check return codes.
- `create_dir_recursive` safely no-ops on segments that already exist (including the mount point itself).

---

## jpeg_dec

JPEG decode wrapper around **`esp_new_jpeg`** (NOT the older `esp_jpeg`/tjpgd-based
library — that one is ~4-8x slower on S3 and was dropped after benchmarking).

**Header:** `components/jpeg_dec/include/jpeg_dec.h`
**Underlying library:** `espressif/esp_new_jpeg` (baseline JPEG only, no progressive support)

### Types

```c
typedef struct {
    uint16_t *pixels;   // RGB565, allocated via jpeg_calloc_align (16-byte aligned)
    uint16_t width;
    uint16_t height;
} jpeg_image_t;
```

### Functions

| Function | Description |
|---|---|
| `esp_err_t jpeg_dec_get_info(const uint8_t *data, size_t len, uint16_t *w, uint16_t *h)` | Parses JPEG header from an in-memory buffer to get dimensions, without decoding pixel data. |
| `esp_err_t jpeg_dec_buffer(const uint8_t *data, size_t len, bool swap_bytes, uint8_t *out, size_t out_size, uint16_t *w, uint16_t *h)` | Decodes JPEG bytes already in RAM into a **caller-provided** output buffer. Use this for repeated/high-frequency decodes (e.g. video frames) to avoid per-frame allocation. `out_size` must be large enough — call `jpeg_dec_get_info` first if unsure, or check against `ESP_ERR_NO_MEM`. |
| `esp_err_t jpeg_dec_file(const char *path, bool swap_bytes, jpeg_image_t *out)` | Reads a JPEG file from storage and decodes it, allocating the output buffer internally. Convenient for one-off/still images. **Caller must call `jpeg_image_free`.** |
| `void jpeg_image_free(jpeg_image_t *img)` | Frees the pixel buffer from `jpeg_dec_file`. Uses `jpeg_free_align` internally — do not mix with `heap_caps_free`. |

### `swap_bytes` parameter
Controls RGB565 byte order (`RGB565_LE` vs `RGB565_BE`) to match what the LCD
panel driver expects. **Determined empirically per panel** — if colors come out
scrambled/wrong, flip this flag. For this project's ILI9341 panel: `true`.

### Notes
- **Baseline JPEG only.** Progressive JPEGs will fail at `jpeg_dec_parse_header` (check with `ffprobe` or PIL: `Image.open(f).info.get('progressive', 0)`).
- Output buffers from this library must be allocated with `jpeg_calloc_align(size, 16)` and freed with `jpeg_free_align()` — handled internally by `jpeg_dec_file`.
- The library also supports `scale` and `clipper` config (not currently exposed by this wrapper) — useful later for decoding directly to a smaller resolution instead of decoding full-size then resizing.
- `jpeg_dec_buffer`/`jpeg_dec_file` currently open and close a decoder handle per call. For a video pipeline decoding many frames per second, consider keeping one handle open across frames to reduce per-call overhead (not yet implemented).

---

## Performance Notes (lessons learned)

- **`fopen`/`fread` without `setvbuf` is very slow on ESP-IDF.** The default
  stdio buffer is tiny (~128 bytes), causing hundreds of small `read()` calls
  per file read — this was the root cause of SD reads being capped at
  ~0.5 MB/s regardless of card speed (confirmed across 3 different cards;
  raw sector reads reached 6+ MB/s on the same hardware). Always open files
  for sizeable I/O via `bsp_fs_fopen_buffered()` with a 16 KB+ buffer.
- Static/global buffers used with `setvbuf` are **not thread-safe** — fine for
  single-task test code, but each concurrent task needs its own buffer.
- Large local arrays (e.g. `char names[64][256]`) must **not** be stack-allocated
  inside `app_main` or any task function — the default main task stack
  (~3.5 KB) will overflow silently and corrupt unrelated memory, causing
  crashes in seemingly unrelated code. Use `static`, global, or heap allocation
  for anything over a few hundred bytes.
- JPEG decode speed with `esp_new_jpeg` is CPU-bound: set `CPU_FREQ` to 240 MHz
  and build with `-O2` for best results. Also verify source JPEGs are encoded
  with `4:2:0` chroma subsampling (`yuvj420p` in ffmpeg) — other subsampling
  modes decode noticeably slower.
