/* loongarch/check.h - LSX optimized filter functions
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 * All rights reserved.
 * Contributed by Jin Bo <jinbo@loongson.cn>
 *
 * This code is released under the libpng license.  See LICENSE, below.
 */
#if defined(__loongarch_sx)
#  define PNG_TARGET_CODE_IMPLEMENTATION "loongarch/loongarch_lsx_init.c"
#  define PNG_TARGET_IMPLEMENTS_FILTERS
#  define PNG_TARGET_ROW_ALIGNMENT 16
#endif /* __loongarch_sx */