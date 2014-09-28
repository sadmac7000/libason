/**
 * Copyright © 2013, 2014 Red Hat, Casey Dahlin <casey.dahlin@gmail.com>
 *
 * This file is part of libason.
 *
 * libason is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libason is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libason. If not, see <http://www.gnu.org/licenses/>.
 **/

#if defined(ASON_NAMESPACE_H) && defined(ASON_READ_H)

#ifndef ASON__NS_READ_H
#define ASON__NS_READ_H

#ifdef __cplusplus
extern "C" {
#endif

ason_t *ason_ns_read(ason_ns_t *ns, const char *text, ...);
ason_t *ason_ns_readn(ason_ns_t *ns, const char *text, size_t length, ...);

#ifdef __cplusplus
}
#endif

#endif /* ASON__NS_READ_H */
#endif /* ASON_NAMESPACE_H && ASON_READ_H */
