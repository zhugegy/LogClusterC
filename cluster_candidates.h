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
 * File:   cluster_candidates.h
 * 
 * Content: Declarations of global functions in cluster_candidates.c .
 *
 * Created on November 30, 2016, 6:11 AM
 */

#ifndef CLUSTER_CANDIDATES_H
#define CLUSTER_CANDIDATES_H

#ifdef __cplusplus
extern "C" {
#endif

void step_2_create_cluster_candidate_sketch(struct Parameters *pParam);
void step_2_find_cluster_candidates(struct Parameters *pParam);
void debug_1_print_cluster_candidates(struct Parameters *pParam);

#ifdef __cplusplus
}
#endif

#endif /* CLUSTER_CANDIDATES_H */

