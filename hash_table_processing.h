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
 * File:   hash_table_processing.h
 * 
 * Content: Declarations of global functions in hash_table_processing.c .
 *
 * Created on November 30, 2016, 4:32 AM
 */

#ifndef HASH_TABLE_PROCESSING_H
#define HASH_TABLE_PROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif

struct Elem *add_elem(char *pKey, struct Elem **ppTable, tableindex_t tablesize, 
        tableindex_t seed, struct Parameters *pParam);
struct Elem *find_elem(char *key, struct Elem **table, tableindex_t tablesize,
             tableindex_t seed);

#ifdef __cplusplus
}
#endif

#endif /* HASH_TABLE_PROCESSING_H */

