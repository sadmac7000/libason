/**
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

#ifndef STRINGFUNC_H
#define STRINGFUNC_H

#ifdef __cplusplus
extern "C" {
#endif

char *string_to_utf8(const char *in);
char *string_from_utf8(const char *in);
char *string_escape(const char *in);
char *string_unescape(const char *in);

#ifdef __cplusplus
}
#endif

#endif /* STRINGFUNC_H */

