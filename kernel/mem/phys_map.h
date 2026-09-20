#pragma once

#define PHYS_MAP_REGION_MAX 64

struct phys_region
{
    unsigned long long base;
    unsigned long long length;
    unsigned int type;
};

void phys_map_init(void);

unsigned int phys_map_region_count(void);

const struct phys_region *phys_map_region(unsigned int index);

unsigned long long phys_map_total_available(void);

void phys_map_print(void);

const char *phys_map_type_name(unsigned int type);
