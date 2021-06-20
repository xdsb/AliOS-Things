/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_EXTMOD_VFS_POSIX_H
#define MICROPY_INCLUDED_EXTMOD_VFS_POSIX_H

#include "py/lexer.h"
#include "py/obj.h"

extern const mp_obj_type_t mp_type_vfs_posix;
extern const mp_obj_type_t mp_type_vfs_posix_fileio;
extern const mp_obj_type_t mp_type_vfs_posix_textio;

mp_obj_t mp_vfs_posix_file_open(const mp_obj_type_t *type, mp_obj_t file_in, mp_obj_t mode_in);

MP_DECLARE_CONST_FUN_OBJ_3(vfs_posix_mount_obj);
MP_DECLARE_CONST_FUN_OBJ_1(vfs_posix_umount_obj);
MP_DECLARE_CONST_FUN_OBJ_3(vfs_posix_open_obj);
MP_DECLARE_CONST_FUN_OBJ_2(vfs_posix_chdir_obj);
MP_DECLARE_CONST_FUN_OBJ_1(vfs_posix_getcwd_obj);
MP_DECLARE_CONST_FUN_OBJ_2(vfs_posix_ilistdir_obj);
MP_DECLARE_CONST_FUN_OBJ_2(vfs_posix_mkdir_obj);
MP_DECLARE_CONST_FUN_OBJ_2(vfs_posix_remove_obj);
MP_DECLARE_CONST_FUN_OBJ_3(vfs_posix_rename_obj);
MP_DECLARE_CONST_FUN_OBJ_2(vfs_posix_rmdir_obj);
MP_DECLARE_CONST_FUN_OBJ_2(vfs_posix_stat_obj);
MP_DECLARE_CONST_FUN_OBJ_2(vfs_posix_statvfs_obj);


#endif // MICROPY_INCLUDED_EXTMOD_VFS_POSIX_H
