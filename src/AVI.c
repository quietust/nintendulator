/* Nintendulator - Win32 NES emulator written in C
 * Copyright (C) 2002-2009 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "MapperInterface.h"
#include <vfw.h>
#include "APU.h"
#include "PPU.h"
#include "GFX.h"
#include "AVI.h"
#include <commdlg.h>

// AVI utilities -- for creating avi files
// (c) 2002 Lucian Wischik. No restrictions on use.
// http://www.wischik.com/lu/programmer/avi_utils.html

// This is the internal structure represented by the HAVI handle:
typedef struct
{
	PAVIFILE pfile;			// created by CreateAvi
	WAVEFORMATEX wfx;		// as given to CreateAvi (.nChanels=0 if none was given). Used when audio stream is first created.
	int period;			// specified in CreateAvi, used when the video stream is first created
	PAVISTREAM audStream;		// audio stream, initialised when audio stream is first created
	PAVISTREAM vidStream;		// video stream, uncompressed
	PAVISTREAM vidStreamComp;	// video stream, compressed
	unsigned long nframe, nsamp;	// which frame will be added next, which sample will be added next
	BOOL iserr;			// if true, then no function will do anything
} TAviUtil;

HAVI CreateAvi (const TCHAR *filename, int frameperiod, const WAVEFORMATEX *wfx)
{
	PAVIFILE pfile;
	HRESULT hr;
	TAviUtil *au;

	AVIFileInit();
	hr = AVIFileOpen(&pfile, filename, OF_WRITE | OF_CREATE, NULL);
	if (hr)
	{
		AVIFileExit();
		return NULL;
	}
	au = (TAviUtil *)malloc(sizeof(TAviUtil));
	au->pfile = pfile;
	if (wfx)
		CopyMemory(&au->wfx, wfx, sizeof(WAVEFORMATEX));
	else	ZeroMemory(&au->wfx, sizeof(WAVEFORMATEX));
	au->period = frameperiod;
	au->audStream = NULL;
	au->vidStream = NULL;
	au->vidStreamComp = NULL;
	au->nframe = 0;
	au->nsamp = 0;
	au->iserr = FALSE;
	return (HAVI)au;
}

HRESULT CloseAvi (HAVI avi)
{
	TAviUtil *au;
	if (!avi)
		return AVIERR_BADHANDLE;
	au = (TAviUtil *)avi;
	if (au->audStream)
		AVIStreamRelease(au->audStream);
	if (au->vidStreamComp)
		AVIStreamRelease(au->vidStreamComp);
	if (au->vidStream)
		AVIStreamRelease(au->vidStream);
	if (au->pfile)
		AVIFileRelease(au->pfile);
	AVIFileExit();
	free(au);
	return S_OK;
}

HRESULT SetAviVideoCompression (HAVI avi, HBITMAP hbm, AVICOMPRESSOPTIONS *opts, BOOL ShowDialog, HWND hparent)
{
	DIBSECTION dibs;
	int sbm;
	TAviUtil *au;
	HRESULT hr;

	if (!avi)
		return AVIERR_BADHANDLE;
	if (!hbm)
		return AVIERR_BADPARAM;
	sbm = GetObject(hbm, sizeof(dibs), &dibs);
	if (sbm != sizeof(DIBSECTION))
		return AVIERR_BADPARAM;
	au = (TAviUtil*)avi;
	if (au->iserr)
		return AVIERR_ERROR;
	// has compression already been configured?
	if (au->vidStreamComp)
		return AVIERR_COMPRESSOR;

	if (!au->vidStream)	// create the stream, if it wasn't there before
	{
		AVISTREAMINFO strhdr;
		ZeroMemory(&strhdr,sizeof(strhdr));
		strhdr.fccType = streamtypeVIDEO;
		strhdr.fccHandler = 0; 
		strhdr.dwScale = au->period;
		strhdr.dwRate = 1000000000;
		strhdr.dwSuggestedBufferSize = dibs.dsBmih.biSizeImage;
		SetRect(&strhdr.rcFrame, 0, 0, dibs.dsBmih.biWidth, dibs.dsBmih.biHeight);
		hr = AVIFileCreateStream(au->pfile, &au->vidStream, &strhdr);
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
	}

	if (!au->vidStreamComp)	// set the compression, prompting dialog if necessary
	{
		AVICOMPRESSOPTIONS myopts;
		AVICOMPRESSOPTIONS *aopts[1];
		DIBSECTION dibs;

		ZeroMemory(&myopts, sizeof(myopts));
		if (opts)
			aopts[0] = opts;
		else	aopts[0] = &myopts;
		if (ShowDialog)
		{
			BOOL res = (BOOL)AVISaveOptions(hparent, 0, 1, &au->vidStream, aopts);
			if (!res)
			{
				AVISaveOptionsFree(1, aopts);
				au->iserr = TRUE;
				return AVIERR_USERABORT;
			}
		}
		hr = AVIMakeCompressedStream(&au->vidStreamComp, au->vidStream, aopts[0], NULL);
		AVISaveOptionsFree(1, aopts);
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
		GetObject(hbm, sizeof(dibs), &dibs);
		hr = AVIStreamSetFormat(au->vidStreamComp, 0, &dibs.dsBmih, dibs.dsBmih.biSize + dibs.dsBmih.biClrUsed * sizeof(RGBQUAD));
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
	}
	return AVIERR_OK;
}

/*
HRESULT SetAviAudioCompression (HAVI avi, AVICOMPRESSOPTIONS *opts, BOOL ShowDialog, HWND hparent)
{
	DIBSECTION dibs;
	int sbm;
	TAviUtil *au;
	HRESULT hr;

	if (!avi)
		return AVIERR_BADHANDLE;
	if (!hbm)
		return AVIERR_BADPARAM;
	sbm = GetObject(hbm, sizeof(dibs), &dibs);
	if (sbm != sizeof(DIBSECTION))
		return AVIERR_BADPARAM;
	au = (TAviUtil*)avi;
	if (au->iserr)
		return AVIERR_ERROR;
	// has compression already been configured?
	if (au->vidStreamComp)
		return AVIERR_COMPRESSOR;

	if (!au->vidStream)	// create the stream, if it wasn't there before
	{
		AVISTREAMINFO strhdr;
		ZeroMemory(&strhdr,sizeof(strhdr));
		strhdr.fccType = streamtypeVIDEO;
		strhdr.fccHandler = 0; 
		strhdr.dwScale = au->period;
		strhdr.dwRate = 1000000000;
		strhdr.dwSuggestedBufferSize = dibs.dsBmih.biSizeImage;
		SetRect(&strhdr.rcFrame, 0, 0, dibs.dsBmih.biWidth, dibs.dsBmih.biHeight);
		hr = AVIFileCreateStream(au->pfile, &au->vidStream, &strhdr);
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
	}

	if (!au->vidStreamComp)	// set the compression, prompting dialog if necessary
	{
		AVICOMPRESSOPTIONS myopts;
		AVICOMPRESSOPTIONS *aopts[1];
		DIBSECTION dibs;

		ZeroMemory(&myopts, sizeof(myopts));
		if (opts)
			aopts[0] = opts;
		else	aopts[0] = &myopts;
		if (ShowDialog)
		{
			BOOL res = (BOOL)AVISaveOptions(hparent, 0, 1, &au->vidStream, aopts);
			if (!res)
			{
				AVISaveOptionsFree(1, aopts);
				au->iserr = TRUE;
				return AVIERR_USERABORT;
			}
		}
		hr = AVIMakeCompressedStream(&au->vidStreamComp, au->vidStream, aopts[0], NULL);
		AVISaveOptionsFree(1, aopts);
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
		GetObject(hbm, sizeof(dibs), &dibs);
		hr = AVIStreamSetFormat(au->vidStreamComp, 0, &dibs.dsBmih, dibs.dsBmih.biSize + dibs.dsBmih.biClrUsed * sizeof(RGBQUAD));
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
	}
	return AVIERR_OK;
}
*/

HRESULT AddAviFrame (HAVI avi, HBITMAP hbm)
{
	DIBSECTION dibs;
	TAviUtil *au;
	int sbm;
	HRESULT hr;

	if (!avi)
		return AVIERR_BADHANDLE;
	if (!hbm)
		return AVIERR_BADPARAM;
	sbm = GetObject(hbm, sizeof(dibs), &dibs);
	if (sbm != sizeof(DIBSECTION))
		return AVIERR_BADPARAM;
	au = (TAviUtil*)avi;
	if (au->iserr)
		return AVIERR_ERROR;

	if (!au->vidStream)	// create the stream, if it wasn't there before
	{
		AVISTREAMINFO strhdr;
		ZeroMemory(&strhdr,sizeof(strhdr));
		strhdr.fccType = streamtypeVIDEO;
		strhdr.fccHandler = 0; 
		strhdr.dwScale = au->period;
		strhdr.dwRate = 1000000000;
		strhdr.dwSuggestedBufferSize = dibs.dsBmih.biSizeImage;
		SetRect(&strhdr.rcFrame, 0, 0, dibs.dsBmih.biWidth, dibs.dsBmih.biHeight);
		hr = AVIFileCreateStream(au->pfile, &au->vidStream, &strhdr);
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
	}
	if (!au->vidStreamComp)	// create an empty compression, if the user hasn't set any
	{
		AVICOMPRESSOPTIONS opts;
		ZeroMemory(&opts,sizeof(opts));
		opts.fccHandler = mmioFOURCC('D','I','B',' '); 
		hr = AVIMakeCompressedStream(&au->vidStreamComp, au->vidStream, &opts, NULL);
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
		hr = AVIStreamSetFormat(au->vidStreamComp, 0, &dibs.dsBmih, dibs.dsBmih.biSize + dibs.dsBmih.biClrUsed * sizeof(RGBQUAD));
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
	}

	// Now we can add the frame
	hr = AVIStreamWrite(au->vidStreamComp, au->nframe, 1, dibs.dsBm.bmBits, dibs.dsBmih.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	if (hr)
	{
		au->iserr = TRUE;
		return hr;
	}
	au->nframe++;
	return S_OK;
}

HRESULT AddAviAudio (HAVI avi, void *dat, unsigned long numbytes)
{
	TAviUtil *au;
	unsigned long numsamps;
	HRESULT hr;

	if (!avi)
		return AVIERR_BADHANDLE;
	if ((!dat) || (!numbytes))
		return AVIERR_BADPARAM;
	au = (TAviUtil*)avi;
	if (au->iserr)
		return AVIERR_ERROR;
	if (!au->wfx.nChannels)
		return AVIERR_BADFORMAT;

	// make sure it's a whole number of samples
	numsamps = numbytes * 8 / au->wfx.wBitsPerSample;
	if ((numsamps * au->wfx.wBitsPerSample / 8) != numbytes)
		return AVIERR_BADPARAM;

	if (!au->audStream)	// create the stream if necessary
	{
		AVISTREAMINFO ahdr;
		ZeroMemory(&ahdr, sizeof(ahdr));
		ahdr.fccType = streamtypeAUDIO;
		ahdr.dwScale = au->wfx.nBlockAlign;
		ahdr.dwRate = au->wfx.nSamplesPerSec * au->wfx.nBlockAlign; 
		ahdr.dwSampleSize = au->wfx.nBlockAlign;
		ahdr.dwQuality = (DWORD)-1;
		hr = AVIFileCreateStream(au->pfile, &au->audStream, &ahdr);
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
		hr = AVIStreamSetFormat(au->audStream, 0, &au->wfx, sizeof(WAVEFORMATEX));
		if (hr)
		{
			au->iserr = TRUE;
			return hr;
		}
	}

	// now we can write the data
	hr = AVIStreamWrite(au->audStream, au->nsamp, numsamps, dat, numbytes, 0, NULL, NULL);
	if (hr)
	{
		au->iserr = TRUE;
		return hr;
	}
	au->nsamp += numsamps;
	return S_OK;
}

unsigned int FormatAviMessage (HRESULT code, TCHAR *buf, unsigned int len)
{
	unsigned int mlen, n;
	const TCHAR *msg = _T("Unknown AVI result code");
	switch (code)
	{
	case S_OK:			msg = _T("Success");									break;
	case AVIERR_BADFORMAT:		msg = _T("AVIERR_BADFORMAT: corrupt file or unrecognized format");			break;
	case AVIERR_MEMORY:		msg = _T("AVIERR_MEMORY: insufficient memory");						break;
	case AVIERR_FILEREAD:		msg = _T("AVIERR_FILEREAD: disk error while reading file");				break;
	case AVIERR_FILEOPEN:		msg = _T("AVIERR_FILEOPEN: disk error while opening file");				break;
	case REGDB_E_CLASSNOTREG:	msg = _T("REGDB_E_CLASSNOTREG: file type not recognised");				break;
	case AVIERR_READONLY:		msg = _T("AVIERR_READONLY: file is read-only");						break;
	case AVIERR_NOCOMPRESSOR:	msg = _T("AVIERR_NOCOMPRESSOR: a suitable compressor could not be found");		break;
	case AVIERR_UNSUPPORTED:	msg = _T("AVIERR_UNSUPPORTED: compression is not supported for this type of data");	break;
	case AVIERR_INTERNAL:		msg = _T("AVIERR_INTERNAL: internal error");						break;
	case AVIERR_BADFLAGS:		msg = _T("AVIERR_BADFLAGS");								break;
	case AVIERR_BADPARAM:		msg = _T("AVIERR_BADPARAM");								break;
	case AVIERR_BADSIZE:		msg = _T("AVIERR_BADSIZE");								break;
	case AVIERR_BADHANDLE:		msg = _T("AVIERR_BADHANDLE");								break;
	case AVIERR_FILEWRITE:		msg = _T("AVIERR_FILEWRITE: disk error while writing file");				break;
	case AVIERR_COMPRESSOR:		msg = _T("AVIERR_COMPRESSOR");								break;
	case AVIERR_NODATA:		msg = _T("AVIERR_READONLY");								break;
	case AVIERR_BUFFERTOOSMALL:	msg = _T("AVIERR_BUFFERTOOSMALL");							break;
	case AVIERR_CANTCOMPRESS:	msg = _T("AVIERR_CANTCOMPRESS");							break;
	case AVIERR_USERABORT:		msg = _T("AVIERR_USERABORT");								break;
	case AVIERR_ERROR:		msg = _T("AVIERR_ERROR");								break;
	}
	mlen = (unsigned int)_tcslen(msg);
	if (!buf || !len)
		return mlen;
	n = mlen;
	if (n + 1 > len)
		n = len - 1;
	_tcsncpy(buf, msg, n);
	buf[n] = 0;
	return mlen;
}

HAVI aviout = NULL;
void	AVI_End (void)
{
	if (!aviout)
	{
		MessageBox(hMainWnd,_T("No AVI capture is currently in progress!"),_T("Nintendulator"),MB_OK);
		return;
	}
	CloseAvi(aviout);
	aviout = NULL;
	EnableMenuItem(hMenu,ID_MISC_STARTAVICAPTURE,MF_ENABLED);
	EnableMenuItem(hMenu,ID_MISC_STOPAVICAPTURE,MF_GRAYED);
}

void	AVI_Start (void)
{
	WAVEFORMATEX wfx;
	HBITMAP hbm;
	BITMAPINFOHEADER bmih;
	long *pBits;
	HRESULT hr;
	AVICOMPRESSOPTIONS opts;

	TCHAR FileName[MAX_PATH] = {0};
	OPENFILENAME ofn;

	if (aviout)
	{
		MessageBox(hMainWnd,_T("An AVI capture is already in progress!"),_T("Nintendulator"),MB_OK);
		return;
	}

	ZeroMemory(&ofn,sizeof(ofn));
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
		return;

	_tcscpy(Path_AVI,FileName);
	Path_AVI[ofn.nFileOffset-1] = 0;

	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 44100;
	wfx.nAvgBytesPerSec = 88200;
	wfx.nBlockAlign = 2;
	wfx.wBitsPerSample = 16;
	wfx.cbSize = 0;

	if (PPU.IsPAL)
		aviout = CreateAvi(FileName,19997209,&wfx);
	else	aviout = CreateAvi(FileName,16639263,&wfx);

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
	
	hbm = CreateDIBSection(NULL,(BITMAPINFO *)&bmih,DIB_RGB_COLORS,&pBits,NULL,0);

	ZeroMemory(&opts,sizeof(opts));
	hr = SetAviVideoCompression(aviout,hbm,&opts,TRUE,hMainWnd);
	if (hr)
	{
		TCHAR msg[256];
		FormatAviMessage(hr,msg,256);
		MessageBox(hMainWnd,msg,_T("Nintendulator"),MB_OK);
		AVI_End();
		return;
	}
	DeleteObject(hbm);
	EnableMenuItem(hMenu,ID_MISC_STARTAVICAPTURE,MF_GRAYED);
	EnableMenuItem(hMenu,ID_MISC_STOPAVICAPTURE,MF_ENABLED);
}

void	AVI_AddVideo (void)
{
	HBITMAP hbm;
	unsigned long *pBits;
	BITMAPINFOHEADER bmih;
	HRESULT hr;

	if (!aviout)
	{
		MessageBox(hMainWnd,_T("Error! AVI frame capture attempted while not recording!"),_T("Nintendulator"),MB_OK);
		return;
	}
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
	
	hbm = CreateDIBSection(NULL,(BITMAPINFO *)&bmih,DIB_RGB_COLORS,&pBits,NULL,0);

	{
		register unsigned short *src = DrawArray;
		int x, y;
		for (y = 0; y < 240; y++)
		{
			register unsigned long *dst = pBits + 256*(239-y);
			for (x = 0; x < 256; x++)
				dst[x] = GFX.Palette32[src[x]];
			src += x;
		}
	}
	hr = AddAviFrame(aviout,hbm);
	if (hr)
	{
		TCHAR msg[256];
		FormatAviMessage(hr,msg,256);
		MessageBox(hMainWnd,msg,_T("Nintendulator"),MB_OK);
	}
	DeleteObject(hbm);
}

void	AVI_AddAudio (void)
{
	HRESULT hr;
	if (!aviout)
	{
		MessageBox(hMainWnd,_T("Error! AVI audio capture attempted while not recording!"),_T("Nintendulator"),MB_OK);
		return;
	}
	hr = AddAviAudio(aviout,APU.buffer,735*2);
	if (hr)
	{
		TCHAR msg[256];
		FormatAviMessage(hr,msg,256);
		MessageBox(hMainWnd,msg,_T("Nintendulator"),MB_OK);
	}
}