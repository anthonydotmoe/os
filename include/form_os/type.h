#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint32_t virt_pages; //  virtual addr/length in pages
typedef uint32_t virt_bytes; //  virtual addr/length in bytes
typedef uint32_t phys_pages; // physical addr/length in pages
typedef uint32_t phys_bytes; // physical addr/length in bytes

typedef int endpoint_t;     // Process identifier

// Structure for virtual copying by means of a vector with requests.
struct virt_addr {
    endpoint_t proc_nr_e; // NONE for phys, otherwise process endpoint
    virt_bytes offset;
};

#define phys_cp_req vir_cp_req
struct vir_cp_req {
    struct virt_addr src;
    struct virt_addr dst;
    phys_bytes count;
};
