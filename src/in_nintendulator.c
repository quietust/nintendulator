/*
Nintendulator - A Win32 NES emulator written in C.
Designed for maximum emulation accuracy.
Copyright (c) 2002  Quietust

Based on NinthStar, a portable Win32 NES Emulator written in C++
Copyright (C) 2000  David de Regt

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

For a copy of the GNU General Public License, go to:
http://www.gnu.org/copyleft/gpl.html#SEC1
*/

/*
** Example Winamp .RAW input plug-in
** Copyright (c) 1998, Justin Frankel/Nullsoft Inc.
*/

#include <windows.h>
#include <mmreg.h>
#include <msacm.h>
#include <math.h>

#include "in_nintendulator.h"
#include "MapperInterface.h"

#include "CPU.h"
#include "APU.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF WM_USER+2

In_Module mod;					// the output module (declared near the bottom of this file)
char lastfn[MAX_PATH];				// currently playing file (used for getting info on the current file)
int file_length;				// file length, in bytes
int decode_pos_ms;				// current decoding position, in milliseconds
int paused;					// are we paused?
char sample_buffer[576*NCH*(BPS/8)*2];		// sample buffer

HANDLE input_file=INVALID_HANDLE_VALUE;		// input file handle

BOOL killPlayThread = FALSE;			// the kill switch for the play thread
HANDLE thread_handle = INVALID_HANDLE_VALUE;	// the handle to the play thread
DWORD WINAPI PlayThread(void *b);	// the decode thread procedure

void config(HWND hwndParent)
{
	MessageBox(hwndParent,"No configuration is yet available","Configuration",MB_OK);
}
void about(HWND hwndParent)
{
	MessageBox(hwndParent,"Nintendulator NSF Player, by Quietust","About Nintendulator NSF player",MB_OK);
}

void	init (void)
{
	APU_Init();
	APU_Create();
	MapperInterface_Init();
}

void	quit (void)
{
	APU_Release();
	MapperInterface_Release();
}

int isourfile(char *fn) { return 0; }	// used for detecting URL streams.. unused here. strncmp(fn,"http://",7) to detect HTTP streams, etc

struct tNES NES;

unsigned char PRG_ROM[MAX_PRGROM_MASK+1][0x1000];
unsigned char PRG_RAM[MAX_PRGRAM_MASK+1][0x1000];

void	NES_Reset (void)
{
	int i;
	for (i = 0x0; i < 0x10; i++)
	{
		CPU.ReadHandler[i] = CPU_ReadPRG;
		CPU.WriteHandler[i] = CPU_WritePRG;
		CPU.Readable[i] = FALSE;
		CPU.Writable[i] = FALSE;
	}
	CPU.ReadHandler[0] = CPU_ReadRAM;	CPU.WriteHandler[0] = CPU_WriteRAM;
	CPU.ReadHandler[1] = CPU_ReadRAM;	CPU.WriteHandler[1] = CPU_WriteRAM;
	CPU.ReadHandler[4] = CPU_Read4k;	CPU.WriteHandler[4] = CPU_Write4k;

	CPU_PowerOn();
	if (MI->Reset)
		MI->Reset(RESET_HARD);
	APU_Reset();
	CPU_Reset();
}

int play(char *fn) 
{
	int maxlatency;
	int thread_id;
	unsigned char Header[128];	// NSF header bytes
	DWORD numBytesRead;	// so ReadFile() won't crash under Windows 9x

	input_file = CreateFile(fn,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if (input_file == INVALID_HANDLE_VALUE) // error opening file
		return 1;

	file_length = GetFileSize(input_file,NULL) - 128;

	strcpy(lastfn,fn);
	paused=0;
	decode_pos_ms=0;

	maxlatency = mod.outMod->Open(SAMPLERATE,NCH,BPS, -1,-1);
	if (maxlatency < 0) // error opening device
	{
		CloseHandle(input_file);
		input_file=INVALID_HANDLE_VALUE;
		return 1;
	}
	// dividing by 1000 for the first parameter of setinfo makes it
	// display 'H'... for hundred.. i.e. 14H Kbps.
	mod.SetInfo((SAMPLERATE*BPS*NCH)/1000,SAMPLERATE/1000,NCH,1);

	// initialize vis stuff
	mod.SAVSAInit(maxlatency,SAMPLERATE);
	mod.VSASetInfo(SAMPLERATE,NCH);

	mod.outMod->SetVolume(-666); // set the output plug-ins default volume

	ReadFile(input_file,Header,128,&numBytesRead,NULL);
	if (memcmp(Header,"NESM\x1a\x01",6))
		return 1;

	RI.Filename = lastfn;
	RI.ROMType = ROM_NSF;
	RI.NSF_DataSize = file_length;
	RI.NSF_NumSongs = Header[0x06];
	RI.NSF_SoundChips = Header[0x7B];
	RI.NSF_NTSCPAL = Header[0x7A];
	RI.NSF_NTSCSpeed = Header[0x6E] | (Header[0x6F] << 8);
	RI.NSF_PALSpeed = Header[0x78] | (Header[0x79] << 8);
	memcpy(RI.NSF_InitBanks,&Header[0x70],8);
	RI.NSF_InitSong = Header[0x07];
	RI.NSF_InitAddr = Header[0x0A] | (Header[0x0B] << 8);
	RI.NSF_PlayAddr = Header[0x0C] | (Header[0x0D] << 8);
	RI.NSF_Title = (char *)malloc(32);
	RI.NSF_Artist = (char *)malloc(32);
	RI.NSF_Copyright = (char *)malloc(32);
	memcpy(RI.NSF_Title,&Header[0x0E],32);
	memcpy(RI.NSF_Artist,&Header[0x2E],32);
	memcpy(RI.NSF_Copyright,&Header[0x4E],32);
	if (memcmp(RI.NSF_InitBanks,"\0\0\0\0\0\0\0\0",8))
		ReadFile(input_file,&PRG_ROM[0][0] + ((Header[0x8] | (Header[0x9] << 8)) & 0x0FFF),file_length,&numBytesRead,NULL);
	else
	{
		memcpy(RI.NSF_InitBanks,"\x00\x01\x02\x03\x04\x05\x06\x07",8);
		ReadFile(input_file,&PRG_ROM[0][0] + ((Header[0x8] | (Header[0x9] << 8)) & 0x7FFF),file_length,&numBytesRead,NULL);
	}

	if ((RI.NSF_NTSCSpeed == 16666) || (RI.NSF_NTSCSpeed == 16667))
		RI.NSF_NTSCSpeed = 16639;	// adjust NSF playback speed to match actual NTSC NES framerate
	if (RI.NSF_PALSpeed == 20000)
		RI.NSF_PALSpeed = 19997;	// same for PAL NSFs (though we don't really support those right now)

	NES.PRGMask = MAX_PRGROM_MASK;

	if (!MapperInterface_LoadMapper(&RI))
	{
		RI.ROMType = 0;
		return 1;	// couldn't load mapper!
	}
	if (MI->Load)
		MI->Load();

	NES_Reset();	// NSF loaded successfully, reset the NES
			// and start it running
	killPlayThread = FALSE;
	thread_handle = (HANDLE) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) PlayThread,NULL,0,&thread_id);
	return 0;
}

void pause() { paused=1; mod.outMod->Pause(1); }
void unpause() { paused=0; mod.outMod->Pause(0); }
int ispaused() { return paused; }

void stop() {
	if (ispaused())
		unpause();
	if (thread_handle != INVALID_HANDLE_VALUE)
	{
		killPlayThread = TRUE;
		if (WaitForSingleObject(thread_handle,INFINITE) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow,"error asking thread to die!\n","error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}
	if (input_file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(input_file);
		input_file=INVALID_HANDLE_VALUE;
	}

	mod.outMod->Close();

	mod.SAVSADeInit();

	MapperInterface_UnloadMapper();
	free(RI.NSF_Title);
	free(RI.NSF_Artist);
	free(RI.NSF_Copyright);
	RI.ROMType = 0;
}

int getlength() { 
	return -1;	// infinite length
}

int getoutputtime() { 
	return decode_pos_ms+(mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime()); 
}

void setoutputtime(int time_in_ms) { 
}

void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }

int infoDlg(char *fn, HWND hwnd)
{
	if (RI.ROMType)
		MI->Config(CFG_WINDOW,TRUE);
	return 0;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
	if (!filename || !*filename)  // currently playing file
	{
		if (length_in_ms) *length_in_ms=getlength();
		if (title) 
		{
			char *p=lastfn+strlen(lastfn);
			while (*p != '\\' && p >= lastfn) p--;
			strcpy(title,++p);
		}
	}
	else // some other file
	{
		if (length_in_ms) 
		{
			HANDLE hFile;
			*length_in_ms=-1000;
			hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
				OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				*length_in_ms = (GetFileSize(hFile,NULL)*10)/(SAMPLERATE/100*NCH*(BPS/8));
			}
			CloseHandle(hFile);
		}
		if (title) 
		{
			char *p=filename+strlen(filename);
			while (*p != '\\' && p >= filename) p--;
			strcpy(title,++p);
		}
	}
}

void eq_set(int on, char data[10], int preamp) 
{ 
}


DWORD	WINAPI	PlayThread (void *param)
{
	int bufpos = 0;
	int l;
	sample_ok = FALSE;
	while (!killPlayThread)
	{
		while (!sample_ok)
			CPU_ExecOp();
		sample_ok = FALSE;
		((short *)&sample_buffer)[bufpos] = sample_pos;
		bufpos++;
		if (bufpos == 576)
		{
			bufpos = 0;
			while (mod.outMod->CanWrite() < ((576*NCH*(BPS/8))<<(mod.dsp_isactive()?1:0)))
				Sleep(10);

			l=576*NCH*(BPS/8);
			mod.SAAddPCMData((char *)sample_buffer,NCH,BPS,decode_pos_ms);
			mod.VSAAddPCMData((char *)sample_buffer,NCH,BPS,decode_pos_ms);
			decode_pos_ms+=(576*1000)/SAMPLERATE;
			if (mod.dsp_isactive())
				l = mod.dsp_dosamples((short *)sample_buffer,l/NCH/(BPS/8),BPS,NCH,SAMPLERATE)*(NCH*(BPS/8));
			mod.outMod->Write(sample_buffer,l);
		}
	}
	ExitThread(0);
	return 0;
}

In_Module mod = 
{
	IN_VER,
	"Nintendulator NSF Player v0.950",
	0,	// hMainWindow
	0,	// hDllInstance
	"NSF\0Nintendo Sound File (*.NSF)\0"
	,
	0,	// is_seekable
	1,	// uses output

	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	
	getlength,
	getoutputtime,
	setoutputtime,
	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0,	// vis stuff
	0,0,			// dsp
	eq_set,
	NULL,			// setinfo
	0			// out_mod
};

__declspec(dllexport) In_Module *winampGetInModule2()
{
	return &mod;
}
