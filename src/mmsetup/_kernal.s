;=============================================================================
;KERNAL Interface module
;=============================================================================
;
; KERNAL standard pattern module.
;
; Copyright (c) 2025 Daniel England.
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
;
;=============================================================================


  .section code
      .public kernal_get_last_error
      .public kernal_close_all
      .public kernal_set_banks
      .public kernal_set_banks_long
      .public kernal_set_logical_file
      .public kernal_set_name
      .public kernal_open
      .public kernal_set_logical_output
      .public kernal_write_byte
      .public kernal_close_logical_file
      .public kernal_reset_channels

      .public kernal_error


kernal_get_last_error:
  lda kernal_error
  rts



kernal_close_all:         
; .A = device
    jsr 0xff50

    lda #0x00
    sta kernal_error
    rts


kernal_set_banks:
;   .A = memory, .X = file
    jsr 0xff6b
    bcc noerror$
    sta kernal_error
    lda #1
    rts
noerror$:
    lda #0x00
    sta kernal_error
    rts

kernal_set_banks_long:
;   uint32_t memory, uint32_t fileName

//  unimplemented

    lda #0xff
    sta kernal_error

    lda #1   
    rts

kernal_set_logical_file:
; .A = logical, .X = device, .Y = secondary

    jsr 0xffba
    lda #0x00
    sta kernal_error
    rts

kernal_set_name:
;   .X = <fileName, .Y = >filename, .A len

    jsr 0xffbd 
    lda #0
    sta kernal_error
    rts

kernal_open:
    jsr 0xffc0
    bcc noerror$
    sta kernal_error
    lda #1
    rts
noerror$:
    lda #0x00
    sta kernal_error
    rts

kernal_set_logical_output:
; .X = logical
    
    jsr 0xffc9
    bcc noerror$
    sta kernal_error
    lda #1
    rts
noerror$:
    lda #0x00
    sta kernal_error
    rts

kernal_write_byte:
; .A = data

    jsr 0xffd2
    bcc noerror$
    sta kernal_error
    lda #0x01
    rts
noerror$:
    lda #0x00
    sta kernal_error
    rts


kernal_close_logical_file:
; .A = logical

    clc
    jsr 0xffc3
    bcc noerror$
    sta kernal_error
    lda #1
    rts
noerror$:
    lda #0x00
    sta kernal_error
    rts


kernal_reset_channels:
    jsr 0xffcc
    rts


  .section data, data


kernal_error:
  .byte 0x00
