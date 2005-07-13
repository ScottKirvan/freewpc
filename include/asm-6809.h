#ifndef _ASM_6809_H
#define _ASM_6809_H

#define BITS_PER_BYTE 8
#define BITS_PER_WORD 16

#define CC_CARRY 		0x1
#define CC_OVERFLOW 	0x2
#define CC_ZERO 		0x4
#define CC_NEGATIVE 	0x8
#define CC_IRQ 		0x10
#define CC_HALF 		0x20
#define CC_FIRQ 		0x40
#define CC_E 			0x80

#ifndef __SASM__
extern inline void __lda (uint8_t i)
{
	asm __volatile__ ("\tlda %0" :: "g" (i) : "d");
}

extern inline void __ldb (uint8_t i)
{
	asm __volatile__ ("\tldb %0" :: "g" (i) : "d");
}

extern inline void __sta (uint8_t *i)
{
	asm __volatile__ ("\tsta %0" : "=m" (*i));
}

extern inline void __stb (uint8_t *i)
{
	asm __volatile__ ("\tstb %0" : "=m" (*i));
}

extern inline void __bytecopy (uint8_t *dst, uint8_t src)
{
	__lda (src);
	__sta (dst);
}

#if 0
extern inline uint8_t __highbyte (uint16_t val)
{
	asm __volatile__ ("\ttfr a,b\n\tclra\n");
}


extern inline uint8_t __lowbyte (uint16_t val)
{
	asm __volatile__ ("\tclra\n");
}
#endif

extern inline void __swapbyte (uint8_t *v1, uint8_t *v2)
{
}


extern inline void set_stack_pointer (const uint16_t s)
{
	asm __volatile__ ("\tlds %0" :: "g" (s) : "d");
}


extern inline void set_direct_page_pointer (const uint8_t dp)
{
	asm __volatile__ ("\ttfr b, dp" :: "d" (dp));
}

#endif

#endif /* _ASM_6809_H */
