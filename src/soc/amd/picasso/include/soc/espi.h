/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc.
 * (Written by Hugh Edward Richard <hugh.e.dick@silverbackltd.com> for AMD Inc.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef AMD_PICASSO_ESPI_H
#define AMD_PICASSO_ESPI_H

int espi_enable_resources(const struct resource *resource_linked_list);
void espi_enable_children_resources(struct device *espi);

#endif /* AMD_PICASSO_ESPI_H */
