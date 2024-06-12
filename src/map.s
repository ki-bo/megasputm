		.rtmodel cpu, "*"

		.section zzpage,bss
		.extern map_regs

		.section code
		.public map_cs_main_priv
map_cs_main_priv:
		pha
		lda #0xa0
		ldx #0x20
		bra apply_map_cs

		.public map_cs_diskio2
map_cs_diskio2:
		pha
		lda #0x00
		ldx #0x21
		bra apply_map_cs

		.public map_cs_gfx2
map_cs_gfx2:
		pha
		lda #0x20
		ldx #0x21
		bra apply_map_cs

		.public unmap_cs2
unmap_cs2:
		pha
		lda #0x00
		tax
		bra apply_map_cs

		.public unmap_ds2
unmap_ds2:
		ldy #0x00
		ldz #0x00
		bra apply_map_ds

		.public unmap_all2
unmap_all2:
		pha
		lda #0x00
		tax
		tay
		taz
		bra apply_map_cs



apply_map_cs:
		sta zp:map_regs
		stx zp:map_regs + 1
		ldy zp:map_regs + 2
		ldz map_regs + 3
		map
		eom
		pla
		rts

apply_map_ds:
		pha
		lda zp:map_regs
		ldx zp:map_regs + 1
		sty zp:map_regs + 2
		stz map_regs + 3
		map
		eom
		pla
		rts

		.public map_auto_restore_cs
map_auto_restore_cs:
		plx
		stx zp:map_regs + 1
		ply
		pha
		tya
		sta zp:map_regs
		ldy zp:map_regs + 2
		ldz map_regs + 3
		map
		eom
		pla
		rts

		.public map_auto_restore_ds
map_auto_restore_ds:
		plz
		stz zp:map_regs + 3
		ply
		sty zp:map_regs + 2
		pha
		lda zp:map_regs
		ldx zp:map_regs + 1
		map
		eom
		pla
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

		.public map_restore_ds
map_restore_ds:
		lda zp:map_regs
		ldx zp:map_regs + 1
		sty zp:map_regs + 2
		stz zp:map_regs + 3
		map
		eom
		rts
