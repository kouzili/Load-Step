#ifndef _Cache_helpers_a64_H
#define _Cache_helpers_a64_H

extern void dcache_civac(void *addr);
extern void dcache_cis_level1(unsigned long set_num);
extern void dcache_cis_level2(unsigned long set_num);
extern void dcache_cis_level_all(unsigned long set_num);
extern void dcache_ci_all_level1(void);
extern void dcache_ci_all_level2(void);

extern void dcache_cleaninv_range(void *addr, unsigned long size);
extern void dcache_clean_range(void *addr, unsigned long size);
extern void dcache_inv_range(void *addr, unsigned long size);
extern void dcache_clean_range_pou(void *addr, unsigned long size);

extern void icache_inv_all(void);
extern void icache_inv_range(void *addr, unsigned long size);
extern void icache_inv_user_range(void *addr, unsigned long size);

extern void dcache_op_louis(unsigned long op_type);
extern void dcache_op_all(unsigned long op_type);

extern void dcache_op_level1(unsigned long op_type);
extern void dcache_op_level2(unsigned long op_type);
extern void dcache_op_level3(unsigned long op_type);

#endif /*_Cache_helpers_a64_H*/
