/* Copyright (c) 2008, XenSource Inc.
 * All rights reserved.
 *
 * XenSource proprietary code.
 */
#ifndef _VHD_LIB_H_
#define _VHD_LIB_H_

#include <string.h>
#include <endian.h>
#include <byteswap.h>

#include "vhd.h"

#if BYTE_ORDER == LITTLE_ENDIAN
  #define BE16_IN(foo)             (*(foo)) = bswap_16(*(foo))
  #define BE32_IN(foo)             (*(foo)) = bswap_32(*(foo))
  #define BE64_IN(foo)             (*(foo)) = bswap_64(*(foo))
  #define BE16_OUT(foo)            (*(foo)) = bswap_16(*(foo))
  #define BE32_OUT(foo)            (*(foo)) = bswap_32(*(foo))
  #define BE64_OUT(foo)            (*(foo)) = bswap_64(*(foo))
#else
  #define BE16_IN(foo)
  #define BE32_IN(foo)
  #define BE64_IN(foo)
  #define BE32_OUT(foo)
  #define BE32_OUT(foo)
  #define BE64_OUT(foo)
#endif

#define MIN(a, b)                  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)                  (((a) > (b)) ? (a) : (b))

#define VHD_MAX_NAME_LEN           1024

#define VHD_BLOCK_SHIFT            21
#define VHD_BLOCK_SIZE             (1ULL << VHD_BLOCK_SHIFT)

#define UTF_16                     "UTF-16"
#define UTF_16LE                   "UTF-16LE"
#define UTF_16BE                   "UTF-16BE"

static const char                  VHD_POISON_COOKIE[] = "v_poison";

typedef struct hd_ftr              vhd_footer_t;
typedef struct dd_hdr              vhd_header_t;
typedef struct vhd_bat             vhd_bat_t;
typedef struct vhd_batmap          vhd_batmap_t;
typedef struct dd_batmap_hdr       vhd_batmap_header_t;
typedef struct prt_loc             vhd_parent_locator_t;
typedef struct vhd_context         vhd_context_t;

struct vhd_bat {
	uint32_t                   spb;
	uint32_t                   entries;
	uint32_t                  *bat;
};

struct vhd_batmap {
	vhd_batmap_header_t        header;
	char                      *map;
};

struct vhd_context {
	int                        fd;
	char                      *file;

	uint32_t                   spb;
	uint32_t                   bm_secs;

	vhd_header_t               header;
	vhd_footer_t               footer;
	vhd_bat_t                  bat;
	vhd_batmap_t               batmap;
};

static inline uint32_t
secs_round_up(uint64_t bytes)
{
	return ((bytes + (VHD_SECTOR_SIZE - 1)) >> VHD_SECTOR_SHIFT);
}

static inline uint32_t
secs_round_up_no_zero(uint64_t bytes)
{
	return (secs_round_up(bytes) ? : 1);
}

static inline uint64_t
vhd_bytes_padded(uint64_t bytes)
{
	return secs_round_up_no_zero(bytes) << VHD_SECTOR_SHIFT;
}

static inline int
vhd_type_dynamic(vhd_context_t *ctx)
{
	return (ctx->footer.type == HD_TYPE_DYNAMIC ||
		ctx->footer.type == HD_TYPE_DIFF);
}

static inline int
vhd_creator_tapdisk(vhd_context_t *ctx)
{
	return !strncmp(ctx->footer.crtr_app, "tap", 3);
}

static inline size_t
vhd_parent_locator_size(vhd_parent_locator_t *loc)
{
	/*
	 * MICROSOFT_COMPAT
	 * data_space *should* be in sectors,
	 * but sometimes we find it in bytes
	 */
	if (loc->data_space < 512)
		return (loc->data_space << VHD_SECTOR_SHIFT);
	else if (loc->data_space % 512 == 0)
		return loc->data_space;
	else
		return 0;
}

void libvhd_set_log_level(int);

uint32_t vhd_time(time_t time);
size_t vhd_time_to_string(uint32_t timestamp, char *target);
uint32_t vhd_chs(uint64_t size);

uint32_t vhd_checksum_footer(vhd_footer_t *);
uint32_t vhd_checksum_header(vhd_header_t *);
uint32_t vhd_checksum_batmap(vhd_batmap_t *);

void vhd_footer_in(vhd_footer_t *);
void vhd_footer_out(vhd_footer_t *);
void vhd_header_in(vhd_header_t *);
void vhd_header_out(vhd_header_t *);
void vhd_bat_in(vhd_bat_t *);
void vhd_bat_out(vhd_bat_t *);
void vhd_batmap_header_in(vhd_batmap_t *);
void vhd_batmap_header_out(vhd_batmap_t *);

int vhd_validate_footer(vhd_footer_t *footer);
int vhd_validate_header(vhd_header_t *header);
int vhd_validate_batmap_header(vhd_batmap_t *batmap);
int vhd_validate_batmap(vhd_batmap_t *batmap);
int vhd_validate_platform_code(uint32_t code);

int vhd_open(vhd_context_t *, const char *file, int flags);
void vhd_close(vhd_context_t *);
int vhd_create(const char *name, uint64_t bytes, int type);
int vhd_create_fixed(const char *name, uint64_t bytes, int type);
int vhd_snapshot(const char *snapshot, const char *parent);
int vhd_snapshot_fixed(const char *snapshot, const char *parent);
int vhd_snapshot_raw(const char *snapshot, const char *parent);
int vhd_snapshot_fixed_raw(const char *snapshot, const char *parent);

off64_t vhd_position(vhd_context_t *);
int vhd_seek(vhd_context_t *, off64_t, int);
int vhd_read(vhd_context_t *, void *, size_t);
int vhd_write(vhd_context_t *, void *, size_t);

int vhd_offset(vhd_context_t *, uint32_t, uint32_t *);

int vhd_end_of_headers(vhd_context_t *ctx, off64_t *off);
int vhd_end_of_data(vhd_context_t *ctx, off64_t *off);
int vhd_batmap_header_offset(vhd_context_t *ctx, off64_t *off);

int vhd_get_header(vhd_context_t *);
int vhd_get_footer(vhd_context_t *);
int vhd_get_bat(vhd_context_t *);
int vhd_get_batmap(vhd_context_t *);

void vhd_put_header(vhd_context_t *);
void vhd_put_footer(vhd_context_t *);
void vhd_put_bat(vhd_context_t *);
void vhd_put_batmap(vhd_context_t *);

int vhd_has_batmap(vhd_context_t *);
int vhd_batmap_test(vhd_context_t *, vhd_batmap_t *, uint32_t);
void vhd_batmap_set(vhd_context_t *, vhd_batmap_t *, uint32_t);
void vhd_batmap_clear(vhd_context_t *, vhd_batmap_t *, uint32_t);
int vhd_file_size_fixed(vhd_context_t *);
int vhd_get_phys_size(vhd_context_t *, off64_t *);
int vhd_set_phys_size(vhd_context_t *, off64_t);

int vhd_bitmap_test(vhd_context_t *, char *, uint32_t);
void vhd_bitmap_set(vhd_context_t *, char *, uint32_t);
void vhd_bitmap_clear(vhd_context_t *, char *, uint32_t);

int vhd_parent_locator_count(vhd_context_t *);
int vhd_parent_locator_get(vhd_context_t *, char **);
int vhd_parent_locator_read(vhd_context_t *, vhd_parent_locator_t *, char **);
int vhd_parent_locator_write_at(vhd_context_t *, const char *,
				off64_t, uint32_t, vhd_parent_locator_t *);

int vhd_header_decode_parent(vhd_context_t *, vhd_header_t *, char **);
int vhd_change_parent(vhd_context_t *child, char *parent_path);

int vhd_read_footer(vhd_context_t *, vhd_footer_t *);
int vhd_read_footer_at(vhd_context_t *, vhd_footer_t *, off64_t);
int vhd_read_footer_strict(vhd_context_t *, vhd_footer_t *);
int vhd_read_header(vhd_context_t *, vhd_header_t *);
int vhd_read_header_at(vhd_context_t *, vhd_header_t *, off64_t);
int vhd_read_bat(vhd_context_t *, vhd_bat_t *);
int vhd_read_batmap(vhd_context_t *, vhd_batmap_t *);
int vhd_read_bitmap(vhd_context_t *, uint32_t block, char **bufp);
int vhd_read_block(vhd_context_t *, uint32_t block, char **bufp);

int vhd_write_footer(vhd_context_t *, vhd_footer_t *);
int vhd_write_footer_at(vhd_context_t *, vhd_footer_t *, off64_t);
int vhd_write_header(vhd_context_t *, vhd_header_t *);
int vhd_write_header_at(vhd_context_t *, vhd_header_t *, off64_t);
int vhd_write_bat(vhd_context_t *, vhd_bat_t *);
int vhd_write_batmap(vhd_context_t *, vhd_batmap_t *);
int vhd_write_bitmap(vhd_context_t *, uint32_t block, char *bitmap);
int vhd_write_block(vhd_context_t *, uint32_t block, char *data);

int vhd_io_read(vhd_context_t *, char *, uint64_t, uint32_t);
int vhd_io_write(vhd_context_t *, char *, uint64_t, uint32_t);

#endif