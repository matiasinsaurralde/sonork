#if !defined(SRKBICHO_DEFS_H)
#define SRKBICHO_DEFS_H

/*
	Sonork Messaging System

	Portions Copyright (C) 2001 Sonork SRL:

	This program is free software; you can redistribute it and/or modify
	it under the terms of the Sonork Source Code License (SSCL).

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	SSCL for more details.

	You should have received a copy of the SSCL	along with this program;
	if not, write to sscl@sonork.com.

	You may NOT use this source code before reading and accepting the
	Sonork Source Code License (SSCL).
*/

class TSonorkBicho; 
enum SONORK_SEQUENCE
{
  SONORK_SEQUENCE_IDLE
, SONORK_SEQUENCE_WAKEUP
, SONORK_SEQUENCE_YAWN
, SONORK_SEQUENCE_SLEEP
, SONORK_SEQUENCE_LOOK_LR
, SONORK_SEQUENCE_LOOK_L
, SONORK_SEQUENCE_LOOK_R
, SONORK_SEQUENCE_LOOK_UPDN
, SONORK_SEQUENCE_ZOOM
, SONORK_SEQUENCE_READ
, SONORK_SEQUENCE_SEARCHING
, SONORK_SEQUENCE_WORK
, SONORK_SEQUENCE_OPEN_WINDOW
, SONORK_SEQUENCE_CLOSE_WINDOW
, SONORK_SEQUENCE_EMAIL
, SONORK_SEQUENCE_ERROR
, SONORK_SEQUENCE_CALL
, SONORK_SEQUENCE_SEARCH_START
, SONORK_SEQUENCE_SEARCH_END
, SONORK_SEQUENCE_RADAR
, SONORK_SEQUENCE_LOOK_DN
, SONORK_SEQUENCE_SURPRISED_LEFT
, SONORK_SEQUENCE_SURPRISED_RIGHT
, SONORK_SEQUENCES
, SONORK_SEQUENCE_NULL
};
#endif