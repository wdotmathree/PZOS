#pragma once
#ifndef ARCH_X86_64_DEFINES_H
#define ARCH_X86_64_DEFINES_H

#ifndef __arch_x86_64__
#error "This file should only be included on x86_64 architecture."
#endif

#define KERNEL_CS (5 << 3)
#define KERNEL_DS (6 << 3)
#define TSS (7 << 3)

#endif
