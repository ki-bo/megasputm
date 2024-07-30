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

#pragma once

typedef enum {
    ERR_FILE_NOT_FOUND = 1,
    ERR_SECTOR_DATA_CORRUPT = 2,
    ERR_SECTOR_NOT_FOUND = 3,
    ERR_INVALID_DISK_LOCATION = 4,
    ERR_RESOURCE_NOT_FOUND = 5,
    ERR_RESOURCE_TOO_LARGE = 6,
    ERR_OUT_OF_RESOURCE_MEMORY = 7,
    ERR_OUT_OF_SCRIPT_SLOTS = 8,
    ERR_UNKNOWN_OPCODE = 9,
    ERR_VARIDX_OUT_OF_RANGE = 10,
    ERR_UNKNOWN_RESOURCE_OPERATION = 11,
    ERROR_LOCKING_RESOURCE_NOT_IN_MEMORY = 12,
    ERROR_UNLOCKING_RESOURCE_NOT_IN_MEMORY = 13,
    ERR_OUT_OF_HEAP_MEMORY = 14,
    ERR_TOO_MANY_INVENTORY_OBJECTS = 15,
    ERR_SCRIPT_RECURSION = 16,
    ERR_UNKNOWN_VERB = 17,
    ERR_CMD_STACK_OVERFLOW = 18,
    ERR_NOT_IMPLEMENTED = 19,
    ERR_EXEC_PARALLEL_SCRIPT = 20,
    ERR_TOO_MANY_LOCAL_ACTORS = 21,
    ERR_COST_BUFSIZE_EXCEEDED = 22,
    ERR_CHRCOUNT_EXCEEDED = 23,
    ERR_UNKNOWN_CURSOR_IMAGE = 24,
    ERR_INCONSISTENT_BAM = 25,
    ERR_DISK_FULL = 26,
    ERR_CHAINING_ROOM_SCRIPT = 27,
    ERR_OBJECT_SCRIPT_STILL_RUNNING_AFTER_FIRST_CYCLE = 28,
    ERR_FILE_READ_BEYOND_EOF = 29,
    ERR_TOO_MANY_LOCKED_RESOURCES = 30,
    ERR_TOO_MANY_ACTOR_PALETTES = 31,
    ERR_READ_TRACK_FAILED = 32,
} error_code_t;
