/*
 Name:		ProtonPackCommon.h
 Created:	1/15/2023 3:38:37 PM
 Author:	oliwr
 Editor:	http://www.visualmicro.com
*/
#pragma once
#ifndef _ProtonPackCommon_h
#define _ProtonPackCommon_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

const char* tracks[] = { "MUSIC/THEME.WAV", "MUSIC/CLEANIN.WAV" ,"MUSIC/SAVIN.WAV" ,"MUSIC/MAGICCUT.WAV", "MUSIC/RGBTHEME.WAV", "MUSIC/TITLE.WAV", "MUSIC/ONOUROWN.WAV" };
int trackCount = 7;

enum State { OFF, WANDON, BOOTING, IDLE, POWERDOWN, FIRING_HEAT, FIRING_NOHEAT, FIRING_OHWARNING, OVERHEAT, FIRING_STOP, MUSIC_MODE };



#endif

