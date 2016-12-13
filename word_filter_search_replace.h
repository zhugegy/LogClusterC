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
 * File:   word_filter_search_replace.h
 * 
 * Content: Declarations of global functions in word_filter_search_replace.c .
 *
 * Created on November 30, 2016, 4:14 AM
 */

#ifndef WORD_FILTER_SEARCH_REPLACE_H
#define WORD_FILTER_SEARCH_REPLACE_H

#ifdef __cplusplus
extern "C" {
#endif

int is_word_filtered(char *pStr, struct Parameters *pParam);
char *word_search_replace(char *pOriginStr, struct Parameters *pParam);

#ifdef __cplusplus
}
#endif

#endif /* WORD_FILTER_SEARCH_REPLACE_H */

