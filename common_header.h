/*
 * Copyright (C) 2016 Zhuge Chen, Risto Vaarandi and Mauno Pihelgas
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* 
 * File:   common_header.h
 * 
 * Content: For the convenience of usage, the common header files resides here. 
 * Generally, every source file should include common_header.h at the very 
 * beginning.
 *
 * Created on November 29, 2016, 7:24 PM
 */

#ifndef COMMON_HEADER_H
#define COMMON_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>    /* for malloc(), atoi/f() and rand() */

#include "macro.h"
#include "struct.h"

#ifdef __cplusplus
}
#endif

#endif /* COMMON_HEADER_H */

