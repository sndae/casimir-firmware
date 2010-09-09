/*
   CASIMIR - Firmware
   Copyright (C) 2009,2010:
         Daniel Roggen, droggen@gmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef __LOG_H
#define __LOG_H

#include "fs.h"


unsigned char log_start(FS_STATE *fs_state,unsigned long deviceid,unsigned long time_absolute_offset,unsigned char *lognumber);
unsigned char log_stop(FS_STATE *fs_state);
unsigned short log_logpacket(FS_STATE *fs_state);

unsigned char log_dologging(FS_STATE *fs_state,unsigned long deviceid,unsigned long time_absolute_offset);


#endif
