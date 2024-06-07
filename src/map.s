		.rtmodel cpu, "*"

		.section zzpage,bss
		.extern map_regs
save_a:		.space 1

		.section code
		.public map_cs_main_priv2
map_cs_main_priv2:
		pha
		lda #0xa0
		ldx #0x20
		bra apply_map2

		.public map_cs_gfx2
map_cs_gfx2:
		pha
		lda #0x20
		ldx #0x21
		bra apply_map2

		.public unmap_cs2
unmap_cs2:
		pha
		lda #0x00
		ldx #0x00
		bra apply_map2

apply_map2:
		sta zp:map_regs
		stx zp:map_regs + 1
		ldy zp:map_regs + 2
		ldz map_regs + 3
		map
		eom
		pla
		rts

		.public map_auto_restore_cs
map_auto_restore_cs:
		plx
		stx zp:map_regs + 1
		sta zp:save_a
		pla
		sta zp:map_regs
		ldy zp:map_regs + 2
		ldz map_regs + 3
		map
		eom
		lda zp:save_a
		rts

		.public map_restore_cs
map_restore_cs:
		sta zp:map_regs
		stx zp:map_regs + 1
		ldy zp:map_regs + 2
		ldz map_regs + 3
		map
		eom
		rts
