/* in_nintendulator - NSF player plugin for Winamp, based on Nintendulator
 * Copyright (C) QMT Productions
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $URL$
 * $Id$
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

extern In_Module mod;				// the output module (declared near the bottom of this file)
TCHAR lastfn[MAX_PATH];				// currently playing file (used for getting info on the current file)
int file_length;				// file length, in bytes
int decode_pos_ms;				// current decoding position, in milliseconds
int paused;					// are we paused?
char sample_buffer[576*NCH*(BPS/8)*2];		// sample buffer

HANDLE input_file=INVALID_HANDLE_VALUE;		// input file handle

BOOL killPlayThread = FALSE;			// the kill switch for the play thread
HANDLE thread_handle = INVALID_HANDLE_VALUE;	// the handle to the play thread
DWORD WINAPI PlayThread(void *b);	// the decode thread procedure

namespace NES
{
int PRGSizeROM, PRGSizeRAM;
int PRGMaskROM, PRGMaskRAM;

BOOL ROMLoaded;

unsigned char PRG_ROM[MAX_PRGROM_SIZE][0x1000];
unsigned char PRG_RAM[MAX_PRGRAM_SIZE][0x1000];

// Generates a bit mask sufficient to fit the specified value
DWORD	getMask (unsigned int maxval)
{
	DWORD result = 0;
	while (maxval > 0)
	{
		result = (result << 1) | 1;
		maxval >>= 1;
	}
	return result;
}

Region CurRegion = REGION_NTSC;	// hardcoded to NTSC for now

void	CloseFile (void)
{
	if (ROMLoaded)
	{
		MapperInterface::UnloadMapper();
		ROMLoaded = FALSE;
	}

	if (RI.ROMType)
	{
		if (RI.ROMType == ROM_NSF)
		{
			delete[] RI.NSF_Title;
			delete[] RI.NSF_Artist;
			delete[] RI.NSF_Copyright;
		}
		ZeroMemory(&RI, sizeof(RI));
	}
}

void	Reset (void)
{
	int i;
	for (i = 0x0; i < 0x10; i++)
	{
		EI.SetCPUReadHandler(i, CPU::ReadPRG);
		EI.SetCPUWriteHandler(i, CPU::WritePRG);
		EI.SetPRG_OB4(i);
	}
	EI.SetCPUReadHandler(0, CPU::ReadRAM);	EI.SetCPUWriteHandler(0, CPU::WriteRAM);
	EI.SetCPUReadHandler(1, CPU::ReadRAM);	EI.SetCPUWriteHandler(1, CPU::WriteRAM);
	EI.SetCPUReadHandler(4, APU::IntRead);	EI.SetCPUWriteHandler(4, APU::IntWrite);

	CPU::PowerOn();
	APU::PowerOn();
	if (MI->Reset)
		MI->Reset(RESET_HARD);
	APU::Reset();
	CPU::Reset();
}
} // namespace NES

void config(HWND hwndParent)
{
	MessageBox(hwndParent, _T("No configuration is yet available"), _T("Configuration"), MB_OK);
}
void about(HWND hwndParent)
{
	MessageBox(hwndParent, _T("Nintendulator NSF Player, by Quietust"), _T("About Nintendulator NSF player"), MB_OK);
}

void	init (void)
{
	NES::ROMLoaded = FALSE;
	MapperInterface::Init();
	APU::Init();
	APU::Start();
	APU::SetRegion();
}

void	quit (void)
{
	if (NES::ROMLoaded)
		NES::CloseFile();
	APU::Destroy();
	MapperInterface::Destroy();
}

int isourfile(const TCHAR *fn) { return 0; }	// used for detecting URL streams.. unused here. strncmp(fn,"http://",7) to detect HTTP streams, etc

int play(const TCHAR *fn) 
{
	int maxlatency;
	int thread_id;
	unsigned char Header[128];	// NSF header bytes
	DWORD numBytesRead;	// so ReadFile() won't crash under Windows 9x
	int LoadAddr;

	if (NES::ROMLoaded)
	{
		// this should never happen
		NES::CloseFile();
		MessageBox(mod.hMainWindow, _T("Already playing NSF?"), _T("in_nintendulator"), MB_OK | MB_ICONERROR);
	}

	input_file = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (input_file == INVALID_HANDLE_VALUE) // error opening file
		return 1;

	file_length = GetFileSize(input_file, NULL) - 128;
	// make sure the NSF isn't so large that it overflows the PRG ROM buffer
	if (file_length > MAX_PRGROM_SIZE * 0x1000)
		return 1;

	_tcscpy(lastfn, fn);
	paused=0;
	decode_pos_ms=0;

	maxlatency = mod.outMod->Open(SAMPLERATE, NCH, BPS, -1, -1);
	if (maxlatency < 0) // error opening device
	{
		CloseHandle(input_file);
		input_file=INVALID_HANDLE_VALUE;
		return 1;
	}
	// dividing by 1000 for the first parameter of setinfo makes it
	// display 'H'... for hundred.. i.e. 14H Kbps.
	mod.SetInfo((SAMPLERATE*BPS*NCH)/1000, SAMPLERATE/1000, NCH, 1);

	// initialize vis stuff
	mod.SAVSAInit(maxlatency, SAMPLERATE);
	mod.VSASetInfo(SAMPLERATE, NCH);

	mod.outMod->SetVolume(-666); // set the output plug-ins default volume

	ReadFile(input_file, Header, 128, &numBytesRead, NULL);
	if (memcmp(Header, "NESM\x1a\x01", 6))
		return 1;

	RI.Filename = lastfn;
	RI.ROMType = ROM_NSF;
	RI.NSF_DataSize = file_length;
	RI.NSF_NumSongs = Header[0x06];
	RI.NSF_SoundChips = Header[0x7B];
	RI.NSF_NTSCPAL = Header[0x7A];
	RI.NSF_NTSCSpeed = Header[0x6E] | (Header[0x6F] << 8);
	RI.NSF_PALSpeed = Header[0x78] | (Header[0x79] << 8);
	memcpy(RI.NSF_InitBanks, &Header[0x70], 8);
	RI.NSF_InitSong = Header[0x07];
	LoadAddr = Header[0x08] | (Header[0x09] << 8);
	RI.NSF_InitAddr = Header[0x0A] | (Header[0x0B] << 8);
	RI.NSF_PlayAddr = Header[0x0C] | (Header[0x0D] << 8);
	RI.NSF_Title = new char[32];
	RI.NSF_Artist = new char[32];
	RI.NSF_Copyright = new char[32];
	memcpy(RI.NSF_Title, &Header[0x0E], 32);
	RI.NSF_Title[31] = 0;
	memcpy(RI.NSF_Artist, &Header[0x2E], 32);
	RI.NSF_Artist[31] = 0;
	memcpy(RI.NSF_Copyright, &Header[0x4E], 32);
	RI.NSF_Copyright[31] = 0;
	if (memcmp(RI.NSF_InitBanks, "\0\0\0\0\0\0\0\0", 8))
	{
		ReadFile(input_file, &NES::PRG_ROM[0][0] + (LoadAddr & 0x0FFF), file_length, &numBytesRead, NULL);
		NES::PRGSizeROM = file_length + (LoadAddr & 0xFFF);
	}
	else
	{
		memcpy(RI.NSF_InitBanks, "\x00\x01\x02\x03\x04\x05\x06\x07", 8);
		ReadFile(input_file, &NES::PRG_ROM[0][0] + (LoadAddr & 0x7FFF), file_length, &numBytesRead, NULL);
		NES::PRGSizeROM = file_length + (LoadAddr & 0x7FFF);
	}
	NES::PRGSizeROM = (NES::PRGSizeROM / 0x1000) + ((NES::PRGSizeROM % 0x1000) ? 1 : 0);
	NES::PRGSizeRAM = 0x2;	// 8KB at $6000-$7FFF

	if ((RI.NSF_NTSCSpeed == 16666) || (RI.NSF_NTSCSpeed == 16667))
		RI.NSF_NTSCSpeed = 16639;	// adjust NSF playback speed to match actual NTSC NES framerate
	if (RI.NSF_PALSpeed == 20000)
		RI.NSF_PALSpeed = 19997;	// same for PAL NSFs (though we don't really support those right now)

	NES::PRGMaskROM = NES::getMask(NES::PRGSizeROM - 1) & MAX_PRGROM_MASK;
	NES::PRGMaskRAM = NES::getMask(NES::PRGSizeRAM - 1) & MAX_PRGRAM_MASK;

	if (!MapperInterface::LoadMapper(&RI))
	{
		NES::CloseFile();
		return 1;	// couldn't load mapper!
	}
	NES::ROMLoaded = TRUE;

	NES::Reset();	// NSF loaded successfully, reset the NES
			// and start it running
	killPlayThread = FALSE;
	thread_handle = (HANDLE) CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PlayThread, NULL, 0, (LPDWORD)&thread_id);
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
		if (WaitForSingleObject(thread_handle, INFINITE) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow, _T("error asking thread to die!"), _T("error killing decode thread"), 0);
			TerminateThread(thread_handle, 0);
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

	NES::CloseFile();
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

int infoDlg(const TCHAR *fn, HWND hwnd)
{
	if (RI.ROMType)
		MI->Config(CFG_WINDOW, TRUE);
	return 0;
}

void getfileinfo(const TCHAR *filename, TCHAR *title, int *length_in_ms)
{
	if (!filename || !*filename)  // currently playing file
	{
		if (length_in_ms) *length_in_ms=getlength();
		if (title) 
		{
			TCHAR *p=lastfn+_tcslen(lastfn);
			while (*p != '\\' && p >= lastfn) p--;
			_tcscpy(title, ++p);
		}
	}
	else // some other file
	{
		if (length_in_ms) 
			*length_in_ms = -1000;
		if (title) 
		{
			const TCHAR *p=filename+_tcslen(filename);
			while (*p != _T('\\') && p >= filename) p--;
			_tcscpy(title, ++p);
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
	APU::sample_ok = FALSE;
	while (!killPlayThread)
	{
		while (!APU::sample_ok)
			CPU::ExecOp();
		APU::sample_ok = FALSE;
		((short *)&sample_buffer)[bufpos] = APU::sample_pos;
		bufpos++;
		if (bufpos == 576)
		{
			bufpos = 0;
			while (mod.outMod->CanWrite() < ((576*NCH*(BPS/8))<<(mod.dsp_isactive()?1:0)))
				Sleep(10);

			l=576*NCH*(BPS/8);
			mod.SAAddPCMData((char *)sample_buffer, NCH, BPS, decode_pos_ms);
			mod.VSAAddPCMData((char *)sample_buffer, NCH, BPS, decode_pos_ms);
			decode_pos_ms+=(576*1000)/SAMPLERATE;
			if (mod.dsp_isactive())
				l = mod.dsp_dosamples((short *)sample_buffer, l/NCH/(BPS/8), BPS, NCH, SAMPLERATE)*(NCH*(BPS/8));
			mod.outMod->Write(sample_buffer, l);
		}
	}
	ExitThread(0);
// warning C4702: unreachable code
//	return 0;
}

In_Module mod = 
{
	IN_VER,
	"Nintendulator NSF Player v0.985",
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

extern "C" __declspec(dllexport) In_Module *winampGetInModule2()
{
	return &mod;
}
