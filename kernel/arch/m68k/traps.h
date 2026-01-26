#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct __attribute__((packed)) {
    uint16_t sr;
    uint32_t pc;
    uint16_t fmtvec;
    uint16_t tail[];    // Variable length words
} exc_frame_header_t;

#define FMTVEC(format, vector_offset) \
    (unsigned short)((((format) & 0xF) << 12) | ((vector_offset) & 0x0FFF))

typedef struct __attribute__((packed)) {
    uint32_t addr;
} exc_fmt2_t;

typedef struct __attribute__((packed)) {
    uint32_t ea;
} exc_fmt3_t;

typedef struct __attribute__((packed)) {
    uint16_t ea;
    uint32_t pc;
} exc_fmt4_t;

typedef struct __attribute__((packed)) {
    uint32_t ea;
    uint16_t ssw;
    uint16_t wb3s;
    uint16_t wb2s;
    uint16_t wb1s;
    uint32_t fa;
    uint32_t wb3a;
    uint32_t wb3d;
    uint32_t wb2a;
    uint32_t wb2d;
    uint32_t wb1a;
    uint32_t wb1d_pd0;
    uint32_t pd1;
    uint32_t pd2;
    uint32_t pd3;
} exc_fmt7_t;

typedef struct __attribute__((packed)) {
    uint32_t d0,d1,d2,d3,d4,d5,d6,d7;
    uint32_t a0,a1,a2,a3,a4,a5,a6;
} saved_regs_t;

static inline uint8_t exc_format(const exc_frame_header_t *h)
{
    return (h->fmtvec >> 12) & 0xF;
}

static inline uint16_t exc_vector(const exc_frame_header_t *h)
{
    return (h->fmtvec & 0x0FFF) >> 2;
}

static inline const exc_fmt2_t *exc_as_fmt2(const exc_frame_header_t *h)
{
    return (const exc_fmt2_t *)h->tail;
}

static inline const exc_fmt3_t *exc_as_fmt3(const exc_frame_header_t *h)
{
    return (const exc_fmt3_t *)h->tail;
}

static inline const exc_fmt4_t *exc_as_fmt4(const exc_frame_header_t *h)
{
    return (const exc_fmt4_t *)h->tail;
}

static inline const exc_fmt7_t *exc_as_fmt7(const exc_frame_header_t *h)
{
    return (const exc_fmt7_t *)h->tail;
}

static inline size_t exc_frame_size_types(uint8_t fmt)
{
    switch (fmt) {
        case 0: return 8;
        case 2: return 8 + 4;
        case 3: return 8 + 4;
        case 4: return 8 + 2 + 4;
        case 7: return 60; // "30-word" stack frame
    }
}

// evt_base: Where to place the exception vector table
void traps_init(virt_addr_t evt_base);
