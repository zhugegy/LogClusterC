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
 * File:   frequent_words.h
 * 
 * Content: Declarations of global functions in frequent_words.c .
 *
 * Created on November 30, 2016, 3:13 AM
 */

#ifndef FREQUENT_WORDS_H
#define FREQUENT_WORDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "macro.h"    //optional. not necessary.
  
void step_1_create_word_sketch(struct Parameters *pParam);
wordnumber_t step_1_create_vocabulary(struct Parameters *pParam);
wordnumber_t step_1_find_frequent_words(struct Parameters *pParam, 
        wordnumber_t sum);
void debug_1_print_frequent_words(struct Parameters *pParam);

#ifdef __cplusplus
}
#endif

#endif /* FREQUENT_WORDS_H */

