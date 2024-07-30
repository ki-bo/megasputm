/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

		.rtmodel cpu, "*"

		.section zzpage,bss
		.extern map_regs
		.extern _Zp
		.extern room_res_slot

		.section code, noreorder
		.public map_cs_main_priv
map_cs_main_priv:
		pha
		lda #0xa0
		ldx #0x20
		bra apply_map_cs

		.section code, noreorder
		.public map_cs_diskio
map_cs_diskio:
		pha
		lda #0x00
		ldx #0x21
		bra apply_map_cs

		.section code, noreorder
		.public map_cs_gfx
map_cs_gfx:
		pha
		lda #0x20
		ldx #0x21
		bra apply_map_cs

		.section code, noreorder
		.public map_cs_gfx2
map_cs_gfx2:
		pha
		lda #0xe0
		ldx #0x20
		bra apply_map_cs

		.section code, noreorder
		.public unmap_cs
unmap_cs:
		pha
		lda #0x00
		tax
		bra apply_map_cs

		.section code, noreorder
		.public unmap_ds
unmap_ds:
		ldy #0x00
		ldz #0x00
		bra apply_map_ds

		.section code, noreorder
		.public unmap_all
unmap_all:
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

		.section code, noreorder
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

		.section code, noreorder
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

		.section code, noreorder
		.public map_restore_cs
map_restore_cs:
		sta zp:map_regs
		stx zp:map_regs + 1
		ldy zp:map_regs + 2
		ldz map_regs + 3
		map
		eom
		rts

		.section code, noreorder
		.public map_restore_ds
map_restore_ds:
		lda zp:map_regs
		ldx zp:map_regs + 1
		sty zp:map_regs + 2
		stz zp:map_regs + 3
		map
		eom
		rts

		// uint8_t *map_ds_ptr(void __huge *ptr);
		.section code, noreorder
		.public map_ds_ptr
		// map_regs.ds = 0x3000 - 0x80 + (uint16_t)((uint32_t)ptr / 256);
  		// apply_map();
  		// return (uint8_t *)RES_MAPPED + (uint8_t)ptr;
map_ds_ptr:
		lda #0x80
		tax
		clc
		adc zp:_Zp+1
		stx zp:_Zp+1
		tay
		sty zp:map_regs+2
		lda zp:_Zp+2
		adc #0x2f
		taz
		stz zp:map_regs+3
		lda zp:map_regs
		ldx zp:map_regs+1
		map
		eom
		rts

		// void map_ds_resource(uint8_t res_page);
		.section code, noreorder
		.public map_ds_resource
		// // map offset: RESOURCE_MEMORY + page*256 - 0x8000
		// map_regs.ds = 0x3000 + (RESOURCE_BASE / 256) + res_page - 0x80;
		// apply_map();
map_ds_resource:
		tay
		sty zp:map_regs+2
		ldz #0x31
		stz zp:map_regs+3
		ldx zp:map_regs+1
		lda zp:map_regs
		map
		eom
		rts

		// uint8_t *map_ds_room_offset(uint16_t room_offset);
		.section code, noreorder
		.public map_ds_room_offset
map_ds_room_offset:
		// uint8_t res_slot = room_res_slot + MSB(room_offset);
		// uint8_t new_offset = LSB(room_offset);
		// map_ds_resource(res_slot);
		// return NEAR_U8_PTR(RES_MAPPED + new_offset);
		lda zp:_Zp+1
		clc
		adc room_res_slot
		tay
		sty zp:map_regs+2
		ldz #0x31
		stz zp:map_regs+3
		lda zp:map_regs
		ldx zp:map_regs+1
		map
		eom
		lda #0x80
		sta zp:_Zp+1
		rts
