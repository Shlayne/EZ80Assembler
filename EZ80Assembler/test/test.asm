#include "ti84pce.inc"

.equ gfx_LCDWidth        320                                ;; 320
.equ gfx_LCDHeight       240                                ;; 240
.equ gfx_LCDSize         gfx_LCDWidth * gfx_LCDHeight       ;; 76800
.equ gfx_pBuffer1        $D40000                            ;; 320 * 240 bytes ; $D40000-$D52BFF
.equ gfx_pBuffer2        gfx_pBuffer1 + gfx_LCDSize         ;; 320 * 240 bytes ; $D52C00-$D657FF
.equ gfx_pVRamEnd        $D65800                            ;; 0 bytes ; $D65800 ; just the address of the byte immediately after vRam
.equ gfx_PaletteSize     256 * 2                            ;; 512
.equ gfx_pPalette        $E30200                            ;; 256 * 2 bytes; $E30200-$E303FF ; memory mapped to ports $4200-$43FF
.equ gfx_pPaletteEnd     gfx_pPalette + gfx_PaletteSize     ;; 0 bytes ; $E30400 ; just the address of the byte immediately after mpLcdPalette

; .org now only changes the address offset for labels (might pad bytes, idk, todo check that)
.org UserMem - 2
.db tExtTok, tAsm84CeCmp
	jp Main

#include "test_include.asm"

#define some_evaluated_integral 1
#if some_evaluated_integral
	#macro add_de_hl $macro_arg_1 $macro_arg_2 $macro_arg_n
		ex de, hl
		add hl, de
		ex de, hl
	#endmacro
#elif 0
	#assert 0, " this is a mses " ; can't happen
#else
	#macro add_de_hl $macro_arg_1 $macro_arg_2 $macro_arg_n
	#endmacro
#endif

Main:
	ld hl, String
	ld de, (ix + 9 + 12) ; example of a real instruction
	ld bc, name_space.StringEnd - name_space.String
	ldir
	ret
	lea ix, ix + 17
	pea iy - 4

#namespace name_space
	String: .db "test; thing"
	StringEnd:
#endnamespace
