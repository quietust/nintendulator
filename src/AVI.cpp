/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) QMT Productions
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include <vfw.h>
#include "NES.h"
#include "APU.h"
#include "PPU.h"
#include "GFX.h"
#include "AVI.h"
#include <commdlg.h>

#pragma comment(lib, "vfw32.lib")

class AVIHandle
{
protected:
	PAVIFILE pfile;		// AVI file handle
	WAVEFORMATEX wfx;	// Desired audio format
	int period;		// Desired framerate divisor (dividend is 1,000,000,000)
	PAVISTREAM audStreamU;	// Uncompressed audio stream handle. Compression does not seem to work.
	PAVISTREAM vidStreamU;	// Uncompressed video stream handle
	PAVISTREAM vidStreamC;	// Compressed video stream handle

	unsigned long nframe;	// Next frame number
	unsigned long nsamp;	// Next sample number

	bool iserr;		// Error state - if true, then all functions will abort
	bool WineHack;		// special hack for Wine's broken implementation of AVIStreamWrite

	bool CreateAudioStream (void)
	{
		if (audStreamU)
			return true;

		HRESULT hr;
		AVISTREAMINFO ahdr;
		ZeroMemory(&ahdr, sizeof(ahdr));
		ahdr.fccType = streamtypeAUDIO;
		ahdr.dwScale = wfx.nBlockAlign;
		ahdr.dwRate = wfx.nSamplesPerSec * wfx.nBlockAlign;
		ahdr.dwQuality = (DWORD)-1;
		ahdr.dwSampleSize = wfx.nBlockAlign;
		_stprintf(ahdr.szName, _T("Audio"));

		hr = AVIFileCreateStream(pfile, &audStreamU, &ahdr);
		if (hr)
		{
			iserr = true;
			Error(hr, _T("CreateAudioStream::AVIFileCreateStream"));
			return false;
		}

		hr = AVIStreamSetFormat(audStreamU, 0, &wfx, sizeof(WAVEFORMATEX) + wfx.cbSize);
		if (hr)
		{
			iserr = true;
			Error(hr, _T("CreateAudioStream::AVIStreamSetFormat"));
			return false;
		}
		return true;
	}

	bool CreateVideoStream (DIBSECTION *dibs)
	{
		if (vidStreamU)
			return true;

		HRESULT hr;
		AVISTREAMINFO strhdr;
		ZeroMemory(&strhdr, sizeof(strhdr));
		strhdr.fccType = streamtypeVIDEO;
		strhdr.fccHandler = 0; 
		strhdr.dwScale = period;
		strhdr.dwRate = 1000000000;
		strhdr.dwQuality = (DWORD)-1;
		strhdr.dwSuggestedBufferSize = dibs->dsBmih.biSizeImage;
		_stprintf(strhdr.szName, _T("Video"));

		SetRect(&strhdr.rcFrame, 0, 0, dibs->dsBmih.biWidth, dibs->dsBmih.biHeight);
		hr = AVIFileCreateStream(pfile, &vidStreamU, &strhdr);
		if (hr)
		{
			iserr = true;
			Error(hr, _T("CreateVideoStream::AVIFileCreateStream"));
			return false;
		}

		hr = AVIStreamSetFormat(vidStreamU, 0, &dibs->dsBmih, dibs->dsBmih.biSize + dibs->dsBmih.biClrUsed * sizeof(RGBQUAD));
		if (hr)
		{
			iserr = true;
			Error(hr, _T("CreateVideoStream::AVIStreamSetFormat"));
			return false;
		}
		return true;
	}

	bool CreateVideoStreamComp (DIBSECTION *dibs, AVICOMPRESSOPTIONS *opts)
	{
		if (vidStreamC)
			return true;

		HRESULT hr;
		hr = AVIMakeCompressedStream(&vidStreamC, vidStreamU, opts, NULL);
		if (hr)
		{
			iserr = true;
			Error(hr, _T("CreateVideoStreamComp::AVIMakeCompressedStream"));
			return false;
		}

		hr = AVIStreamSetFormat(vidStreamC, 0, &dibs->dsBmih, dibs->dsBmih.biSize + dibs->dsBmih.biClrUsed * sizeof(RGBQUAD));
		if (hr)
		{
			AVIStreamRelease(vidStreamC);
			vidStreamC = NULL;
			Error(hr, _T("CreateVideoStreamComp::AVIStreamSetFormat"));
		}
		return true;
	}

public:
	AVIHandle (const TCHAR *filename, int frameperiod)
	{
		HRESULT hr;
		AVIFileInit();
		hr = AVIFileOpen(&pfile, filename, OF_WRITE | OF_CREATE, NULL);
		if (hr)
		{
			AVIFileExit();
			Error(hr, _T("AVIFileOpen"));
			throw hr;
		}

		ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = 1;
		wfx.nSamplesPerSec = 44100;
		wfx.nAvgBytesPerSec = 88200;
		wfx.nBlockAlign = 2;
		wfx.wBitsPerSample = 16;
		wfx.cbSize = 0;

		period = frameperiod;
		audStreamU = NULL;
		vidStreamU = NULL;
		vidStreamC = NULL;

		nframe = 0;
		nsamp = 0;

		iserr = false;
	}
	~AVIHandle ()
	{
		if (audStreamU)
			AVIStreamRelease(audStreamU);
		if (vidStreamC)
			AVIStreamRelease(vidStreamC);
		if (vidStreamU)
			AVIStreamRelease(vidStreamU);
		AVIFileRelease(pfile);
		AVIFileExit();
	}

	bool SetCompression (HBITMAP hbm)
	{
		if (iserr)
			return false;

		if (!hbm)
		{
			Error(AVIERR_BADPARAM, _T("SetCompression::hbm"));
			return false;
		}

		DIBSECTION dibs;
		if (GetObject(hbm, sizeof(dibs), &dibs) != sizeof(DIBSECTION))
		{
			Error(AVIERR_BADPARAM, _T("SetCompression::dibs"));
			return false;
		}

		// has compression already been configured?
		if (vidStreamC)
			return true;

		// Ensure uncompressed video and audio streams are created
		if (!CreateVideoStream(&dibs))
			return false;
		if (!CreateAudioStream())
			return false;

		AVICOMPRESSOPTIONS opts;
		ZeroMemory(&opts, sizeof(opts));
		opts.fccType = streamtypeVIDEO;

		LPAVICOMPRESSOPTIONS aopts[1] = { &opts };
		PAVISTREAM ppavi[1] = { vidStreamU };

		INT_PTR res = AVISaveOptions(hMainWnd, 0, 1, ppavi, aopts);
		if (res != TRUE)
		{
			AVISaveOptionsFree(1, aopts);
			return false;
		}

		if (!CreateVideoStreamComp(&dibs, aopts[0]))
			return false;

		AVISaveOptionsFree(1, aopts);
		return true;
	}

	bool AddVideo (HBITMAP hbm)
	{
		DIBSECTION dibs;
		HRESULT hr;

		if (iserr)
			return false;

		if (!hbm)
			return false;
		if (GetObject(hbm, sizeof(dibs), &dibs) != sizeof(DIBSECTION))
			return false;

		// Make sure the video stream has been set up
		if (!vidStreamC && !vidStreamU)
			return false;

		// And add the frame
		hr = AVIStreamWrite(vidStreamC ? vidStreamC : vidStreamU, nframe, 1, dibs.dsBm.bmBits, dibs.dsBmih.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
		if (hr)
		{
			iserr = true;
			Error(hr, _T("AddVideo::AVIStreamWrite"));
			return false;
		}
		nframe++;
		return true;
	}

	bool AddAudio (short *data, unsigned long numbytes)
	{
		HRESULT hr;

		if (iserr)
			return false;

		if ((!data) || (!numbytes))
			return false;

		// make sure it's a whole number of samples
		unsigned long numsamps = numbytes * 8 / wfx.wBitsPerSample;
		if ((numsamps * wfx.wBitsPerSample / 8) != numbytes)
			return false;

		// Make sure the audio stream has been set up
		if (!audStreamU)
			return false;

		// And write the data
		hr = AVIStreamWrite(audStreamU, nsamp, numsamps, data, numbytes, 0, NULL, NULL);

		// Wine doesn't handle the "lStart" parameter correctly for audio,
		// so the second frame will always fail if we do it correctly.
		// Ref: https://bugs.winehq.org/show_bug.cgi?id=49074
		if ((hr) && (!WineHack) && (nsamp == numsamps))
		{
			// It expects it to increment by 1
			// rather than by the number of samples
			nsamp = 1;
			WineHack = true;
			hr = AVIStreamWrite(audStreamU, nsamp, numsamps, data, numbytes, 0, NULL, NULL);
		}
		if (hr)
		{
			iserr = true;
			Error(hr, _T("AddAudio::AVIStreamWrite"));
			return false;
		}
		if (WineHack)
			nsamp++;
		else	nsamp += numsamps;
		return true;
	}

	static void Error (HRESULT code, TCHAR *location)
	{
		const TCHAR *msg;
		switch (code)
		{
		case AVIERR_BADFORMAT:		msg = _T("AVIERR_BADFORMAT");		break;
		case AVIERR_MEMORY:		msg = _T("AVIERR_MEMORY");		break;
		case AVIERR_FILEREAD:		msg = _T("AVIERR_FILEREAD");		break;
		case AVIERR_FILEOPEN:		msg = _T("AVIERR_FILEOPEN");		break;
		case REGDB_E_CLASSNOTREG:	msg = _T("REGDB_E_CLASSNOTREG");	break;
		case AVIERR_READONLY:		msg = _T("AVIERR_READONLY");		break;
		case AVIERR_NOCOMPRESSOR:	msg = _T("AVIERR_NOCOMPRESSOR");	break;
		case AVIERR_UNSUPPORTED:	msg = _T("AVIERR_UNSUPPORTED");		break;
		case AVIERR_INTERNAL:		msg = _T("AVIERR_INTERNAL");		break;
		case AVIERR_BADFLAGS:		msg = _T("AVIERR_BADFLAGS");		break;
		case AVIERR_BADPARAM:		msg = _T("AVIERR_BADPARAM");		break;
		case AVIERR_BADSIZE:		msg = _T("AVIERR_BADSIZE");		break;
		case AVIERR_BADHANDLE:		msg = _T("AVIERR_BADHANDLE");		break;
		case AVIERR_FILEWRITE:		msg = _T("AVIERR_FILEWRITE");		break;
		case AVIERR_COMPRESSOR:		msg = _T("AVIERR_COMPRESSOR");		break;
		case AVIERR_NODATA:		msg = _T("AVIERR_READONLY");		break;
		case AVIERR_BUFFERTOOSMALL:	msg = _T("AVIERR_BUFFERTOOSMALL");	break;
		case AVIERR_CANTCOMPRESS:	msg = _T("AVIERR_CANTCOMPRESS");	break;
		case AVIERR_USERABORT:		msg = _T("AVIERR_USERABORT");		break;
		case AVIERR_ERROR:		msg = _T("AVIERR_ERROR");		break;
		default:			msg = _T("UNKNOWN");
		}

		TCHAR buf[256];	// long enough for all of the above errors
		_stprintf(buf, _T("AVI error %s (%08x) at %s"), msg, code, location);
		MessageBox(hMainWnd, buf, _T("Nintendulator"), MB_OK);
	}
};

namespace AVI
{
	AVIHandle *handle;
	unsigned long *videoBuffer;
	HBITMAP hbm;

BOOL	IsActive (void)
{
	return (handle != NULL);
}

void	Init (void)
{
	handle = NULL;
	videoBuffer = NULL;
	hbm = NULL;
}

void	End (void)
{
	if (!IsActive())
	{
		MessageBox(hMainWnd, _T("No AVI capture is currently in progress!"), _T("Nintendulator"), MB_OK);
		return;
	}
	BOOL running = NES::Running;
	NES::Stop();

	delete handle; handle = NULL;
	if (hbm)
		DeleteObject(hbm);
	hbm = NULL;
	videoBuffer = NULL;

	EnableMenuItem(hMenu, ID_MISC_STARTAVICAPTURE, MF_ENABLED);
	EnableMenuItem(hMenu, ID_MISC_STOPAVICAPTURE, MF_GRAYED);
	GFX::ForceNoSkip(FALSE);

	if (running)
		NES::Start(FALSE);
}

void	Start (void)
{
	TCHAR FileName[MAX_PATH] = {0};

	if (IsActive())
	{
		MessageBox(hMainWnd, _T("An AVI capture is already in progress!"), _T("Nintendulator"), MB_OK);
		return;
	}

	BOOL running = NES::Running;
	NES::Stop();

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hMainWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFilter = _T("AVI File (*.AVI)\0") _T("*.AVI\0") _T("\0");
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = Path_AVI;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = _T("AVI");
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	if (!GetSaveFileName(&ofn))
	{
		if (running)
			NES::Start(FALSE);
		return;
	}

	_tcscpy(Path_AVI, FileName);
	Path_AVI[ofn.nFileOffset - 1] = 0;

	try
	{
		if (NES::CurRegion == NES::REGION_NTSC)
			handle = new AVIHandle(FileName, 16639263);	// 60.098816 fps
		else	handle = new AVIHandle(FileName, 19997209);	// 50.006978 fps
	}
	catch (HRESULT)
	{
		// Error will be reported from the constructor,
		// so there's nothing to do here but resume emulation
		if (running)
			NES::Start(FALSE);
		return;
	}

	BITMAPINFOHEADER bmih;
	ZeroMemory(&bmih, sizeof(BITMAPINFOHEADER));
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = 256;
	bmih.biHeight = 240;
	bmih.biPlanes = 1;
	bmih.biBitCount = 32;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
	hbm = CreateDIBSection(NULL, (BITMAPINFO *)&bmih, DIB_RGB_COLORS, (void **)&videoBuffer, NULL, 0);

	if (!handle->SetCompression(hbm))
	{
		MessageBox(hMainWnd, _T("Failed to configure AVI compression!"), _T("Nintendulator"), MB_OK);
		End();
		if (running)
			NES::Start(FALSE);
		return;
	}
	GFX::ForceNoSkip(TRUE);
	EnableMenuItem(hMenu, ID_MISC_STARTAVICAPTURE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MISC_STOPAVICAPTURE, MF_ENABLED);
	if (running)
		NES::Start(FALSE);
}

void	AddVideo (void)
{
	if (!handle)
	{
		MessageBox(hMainWnd, _T("Error! AVI frame capture attempted while not recording!"), _T("Nintendulator"), MB_OK);
		return;
	}
	register unsigned short *src = PPU::DrawArray;
	register unsigned long *dst = videoBuffer + 256 * 240;
	for (int y = 0; y < 240; y++)
	{
		dst -= 256;
		for (int x = 0; x < 256; x++)
			dst[x] = GFX::Palette32[src[x]];
		src += 256;
	}
	if (!handle->AddVideo(hbm))
	{
		MessageBox(hMainWnd, _T("Failed to write video to AVI!"), _T("Nintendulator"), MB_OK);
		NES::DoStop = STOPMODE_NOW;
	}
}

void	AddAudio (void)
{
	if (!handle)
	{
		MessageBox(hMainWnd, _T("Error! AVI audio capture attempted while not recording!"), _T("Nintendulator"), MB_OK);
		return;
	}
	if (!handle->AddAudio(APU::buffer, APU::LockSize))
	{
		MessageBox(hMainWnd, _T("Failed to write audio to AVI!"), _T("Nintendulator"), MB_OK);
		NES::DoStop = STOPMODE_NOW;
	}
}
} // namespace AVI
