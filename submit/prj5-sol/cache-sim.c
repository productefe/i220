#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "cache-sim.h"
#include "memalloc.h"

typedef struct {
    bool valid;
    bool dirty;
    MemAddr block_addr;
} Line;

typedef struct {
    Line *lines;
    unsigned count;
} Set;

struct CacheSimImpl {
    unsigned s;
    unsigned b;
    unsigned E;
    unsigned num_sets;
    Replacement repl;
    Set *sets;
};

static MemAddr get_block_addr(CacheSim *cache, MemAddr addr) {
    return addr & ~((1UL << cache->b) - 1);
}

static unsigned get_set_index(CacheSim *cache, MemAddr addr) {
    return (addr >> cache->b) & ((1UL << cache->s) - 1);
}

static int find_line(CacheSim *cache, Set *set, MemAddr block_addr) {
    for (unsigned i = 0; i < set->count; i++) {
        if (set->lines[i].valid && set->lines[i].block_addr == block_addr) {
            return i;
        }
    }
    return -1;
}

static void move_to_front(Set *set, unsigned idx) {
    if (idx == 0) return;

    Line temp = set->lines[idx];

    for (unsigned i = idx; i > 0; i--) {
        set->lines[i] = set->lines[i - 1];
    }

    set->lines[0] = temp;
}

static void insert_front(Set *set, unsigned E, Line line) {
    unsigned limit = set->count;

    if (limit < E) {
        set->count++;
    } else {
        limit = E - 1;
    }

    for (unsigned i = limit; i > 0; i--) {
        set->lines[i] = set->lines[i - 1];
    }

    set->lines[0] = line;
}

static unsigned pick_victim(CacheSim *cache, Set *set) {
    if (cache->repl == MRU_R) {
        return 0;
    }

    if (cache->repl == LRU_R) {
        return set->count - 1;
    }

    return rand() % cache->E;
}

CacheSim *new_cache_sim(const CacheParams *params) {
    CacheSim *cache = malloc_chk(sizeof(CacheSim));

    cache->s = params->n_set_index_bits;
    cache->b = params->n_blk_offset_bits;
    cache->E = params->n_lines_per_set;
    cache->repl = params->replacement;
    cache->num_sets = 1U << cache->s;

    cache->sets = malloc_chk(cache->num_sets * sizeof(Set));

    for (unsigned i = 0; i < cache->num_sets; i++) {
        cache->sets[i].lines = calloc_chk(cache->E, sizeof(Line));
        cache->sets[i].count = 0;
    }

    return cache;
}

void free_cache_sim(CacheSim *cache) {
    for (unsigned i = 0; i < cache->num_sets; i++) {
        free(cache->sets[i].lines);
    }

    free(cache->sets);
    free(cache);
}

CacheResult cache_sim_result(CacheSim *cache, MemAddr access_addr, bool is_write) {
    CacheResult result = { 0 };

    result.access_addr = access_addr;

    MemAddr block = get_block_addr(cache, access_addr);
    unsigned set_idx = get_set_index(cache, access_addr);
    Set *set = &cache->sets[set_idx];

    int idx = find_line(cache, set, block);

    if (idx >= 0) {
        result.status = CACHE_HIT;

        if (is_write) {
            set->lines[idx].dirty = true;
        }

        move_to_front(set, (unsigned)idx);
        return result;
    }

    Line new_line;
    new_line.valid = true;
    new_line.dirty = is_write;
    new_line.block_addr = block;

    if (set->count < cache->E) {
        result.status = CACHE_MISS_WITHOUT_REPLACE;
        insert_front(set, cache->E, new_line);
        return result;
    }

    unsigned victim = pick_victim(cache, set);

    result.status = CACHE_MISS_WITH_REPLACE;
    result.replace_addr = set->lines[victim].block_addr;
    result.is_dirty = set->lines[victim].dirty;

    for (unsigned i = victim; i > 0; i--) {
        set->lines[i] = set->lines[i - 1];
    }

    set->lines[0] = new_line;

    return result;
}
