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

#include "Nintendulator.h"
#include <vfw.h>
#include "APU.h"
#include "GFX.h"
#include "AVI.h"
#include <commdlg.h>

// First, we'll define the WAV file format.
#include <pshpack1.h>
typedef struct
{
	char id[4];	      //="fmt "
	unsigned long size; //=16
	short wFormatTag;   //=WAVE_FORMAT_PCM=1
	unsigned short wChannels;	  //=1 or 2 for mono or stereo
	unsigned long dwSamplesPerSec;	//=11025 or 22050 or 44100
	unsigned long dwAvgBytesPerSec; //=wBlockAlign * dwSamplesPerSec
	unsigned short wBlockAlign;	  //=wChannels * (wBitsPerSample==8?1:2)
	unsigned short wBitsPerSample;	//=8 or 16, for bits per sample
} FmtChunk;

typedef struct
{
	char id[4];		 //="data"
	unsigned long size;	 //=datsize, size of the following array
	unsigned char data[1]; //=the raw data goes here
} DataChunk;

typedef struct
{
	char id[4];	      //="RIFF"
	unsigned long size; //=datsize+8+16+4
	char type[4];	    //="WAVE"
	FmtChunk fmt;
	DataChunk dat;
} WavChunk;
#include <poppack.h>

// This is the internal structure represented by the HAVI handle:
typedef struct
{
	IAVIFile *pfile;    // created by CreateAvi
	WAVEFORMATEX wfx;   // as given to CreateAvi (.nChanels=0 if none was given). Used when audio stream is first created.
	int period;	      // specified in CreateAvi, used when the video stream is first created
	IAVIStream *as;     // audio stream, initialised when audio stream is first created
	IAVIStream *ps, *psCompressed;	// video stream, when first created
	unsigned long nframe, nsamp;	  // which frame will be added next, which sample will be added next
	BOOL iserr;	      // if true, then no function will do anything
} TAviUtil;


HAVI CreateAvi(const char *fn, int frameperiod, const WAVEFORMATEX *wfx)
{
	IAVIFile *pfile;
	HRESULT hr;
	TAviUtil *au;
	AVIFileInit();
	hr = AVIFileOpen(&pfile, fn, OF_WRITE|OF_CREATE, NULL);
	if (hr!=AVIERR_OK) {AVIFileExit(); return NULL;}
	au = (TAviUtil *)malloc(sizeof(TAviUtil));
	au->pfile = pfile;
	if (wfx==NULL) ZeroMemory(&au->wfx,sizeof(WAVEFORMATEX)); else CopyMemory(&au->wfx,wfx,sizeof(WAVEFORMATEX));
	au->period = frameperiod;
	au->as=0; au->ps=0; au->psCompressed=0;
	au->nframe=0; au->nsamp=0;
	au->iserr=FALSE;
	return (HAVI)au;
}

HRESULT CloseAvi(HAVI avi)
{
	TAviUtil *au;
	if (avi==NULL) return AVIERR_BADHANDLE;
	au = (TAviUtil*)avi;
	if (au->as!=0) AVIStreamRelease(au->as); au->as=0;
	if (au->psCompressed!=0) AVIStreamRelease(au->psCompressed); au->psCompressed=0;
	if (au->ps!=0) AVIStreamRelease(au->ps); au->ps=0;
	if (au->pfile!=0) AVIFileRelease(au->pfile); au->pfile=0;
	AVIFileExit();
	free(au);
	return S_OK;
}


HRESULT SetAviVideoCompression(HAVI avi, HBITMAP hbm, AVICOMPRESSOPTIONS *opts, BOOL ShowDialog, HWND hparent)
{
	DIBSECTION dibs;
	int sbm;
	TAviUtil *au;
	HRESULT hr;

	if (avi==NULL) return AVIERR_BADHANDLE;
	if (hbm==NULL) return AVIERR_BADPARAM;
	sbm = GetObject(hbm,sizeof(dibs),&dibs);
	if (sbm!=sizeof(DIBSECTION)) return AVIERR_BADPARAM;
	au = (TAviUtil*)avi;
	if (au->iserr) return AVIERR_ERROR;
	if (au->psCompressed!=0) return AVIERR_COMPRESSOR;
	//
	if (au->ps==0) // create the stream, if it wasn't there before
	{
		AVISTREAMINFO strhdr; ZeroMemory(&strhdr,sizeof(strhdr));
		strhdr.fccType = streamtypeVIDEO;// stream type
		strhdr.fccHandler = 0; 
		strhdr.dwScale = au->period;
		strhdr.dwRate = 1000000000;
		strhdr.dwSuggestedBufferSize  = dibs.dsBmih.biSizeImage;
		SetRect(&strhdr.rcFrame, 0, 0, dibs.dsBmih.biWidth, dibs.dsBmih.biHeight);
		hr=AVIFileCreateStream(au->pfile, &au->ps, &strhdr);
		if (hr!=AVIERR_OK) {au->iserr=TRUE; return hr;}
	}
	//
	if (au->psCompressed==0) // set the compression, prompting dialog if necessary
	{
		AVICOMPRESSOPTIONS myopts;
		AVICOMPRESSOPTIONS *aopts[1];
		DIBSECTION dibs;

		ZeroMemory(&myopts,sizeof(myopts));
		if (opts!=NULL) aopts[0]=opts; else aopts[0]=&myopts;
		if (ShowDialog)
		{
			BOOL res = (BOOL)AVISaveOptions(hparent,0,1,&au->ps,aopts);
			if (!res) {AVISaveOptionsFree(1,aopts); au->iserr=TRUE; return AVIERR_USERABORT;}
		}
		hr = AVIMakeCompressedStream(&au->psCompressed, au->ps, aopts[0], NULL);
		AVISaveOptionsFree(1,aopts);
		if (hr != AVIERR_OK) {au->iserr=TRUE; return hr;}
		GetObject(hbm,sizeof(dibs),&dibs);
		hr = AVIStreamSetFormat(au->psCompressed, 0, &dibs.dsBmih, dibs.dsBmih.biSize+dibs.dsBmih.biClrUsed*sizeof(RGBQUAD));
		if (hr!=AVIERR_OK) {au->iserr=TRUE; return hr;}
	}
	//
	return AVIERR_OK;
}


HRESULT AddAviFrame(HAVI avi, HBITMAP hbm)
{
	DIBSECTION dibs;
	TAviUtil *au;
	int sbm;
	HRESULT hr;

	if (avi==NULL) return AVIERR_BADHANDLE;
	if (hbm==NULL) return AVIERR_BADPARAM;
	sbm = GetObject(hbm,sizeof(dibs),&dibs);
	if (sbm!=sizeof(DIBSECTION)) return AVIERR_BADPARAM;
	au = (TAviUtil*)avi;
	if (au->iserr) return AVIERR_ERROR;
	//
	if (au->ps==0) // create the stream, if it wasn't there before
	{
		AVISTREAMINFO strhdr; ZeroMemory(&strhdr,sizeof(strhdr));
		strhdr.fccType = streamtypeVIDEO;// stream type
		strhdr.fccHandler = 0; 
		strhdr.dwScale = au->period;
		strhdr.dwRate = 1000000000;
		strhdr.dwSuggestedBufferSize  = dibs.dsBmih.biSizeImage;
		SetRect(&strhdr.rcFrame, 0, 0, dibs.dsBmih.biWidth, dibs.dsBmih.biHeight);
		hr=AVIFileCreateStream(au->pfile, &au->ps, &strhdr);
		if (hr!=AVIERR_OK) {au->iserr=TRUE; return hr;}
	}
	//
	// create an empty compression, if the user hasn't set any
	if (au->psCompressed==0)
	{
		AVICOMPRESSOPTIONS opts; ZeroMemory(&opts,sizeof(opts));
		opts.fccHandler=mmioFOURCC('D','I','B',' '); 
		hr = AVIMakeCompressedStream(&au->psCompressed, au->ps, &opts, NULL);
		if (hr != AVIERR_OK) {au->iserr=TRUE; return hr;}
		hr = AVIStreamSetFormat(au->psCompressed, 0, &dibs.dsBmih, dibs.dsBmih.biSize+dibs.dsBmih.biClrUsed*sizeof(RGBQUAD));
		if (hr!=AVIERR_OK) {au->iserr=TRUE; return hr;}
	}
	//
	//Now we can add the frame
	hr = AVIStreamWrite(au->psCompressed, au->nframe, 1, dibs.dsBm.bmBits, dibs.dsBmih.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	if (hr!=AVIERR_OK) {au->iserr=TRUE; return hr;}
	au->nframe++; return S_OK;
}



HRESULT AddAviAudio(HAVI avi, void *dat, unsigned long numbytes)
{
	TAviUtil *au;
	unsigned long numsamps;
	HRESULT hr;

	if (avi==NULL) return AVIERR_BADHANDLE;
	if (dat==NULL || numbytes==0) return AVIERR_BADPARAM;
	au = (TAviUtil*)avi;
	if (au->iserr) return AVIERR_ERROR;
	if (au->wfx.nChannels==0) return AVIERR_BADFORMAT;
	numsamps = numbytes*8 / au->wfx.wBitsPerSample;
	if ((numsamps*au->wfx.wBitsPerSample/8)!=numbytes) return AVIERR_BADPARAM;
	//
	if (au->as==0) // create the stream if necessary
	{
		AVISTREAMINFO ahdr; ZeroMemory(&ahdr,sizeof(ahdr));
		ahdr.fccType=streamtypeAUDIO;
		ahdr.dwScale=au->wfx.nBlockAlign;
		ahdr.dwRate=au->wfx.nSamplesPerSec*au->wfx.nBlockAlign; 
		ahdr.dwSampleSize=au->wfx.nBlockAlign;
		ahdr.dwQuality=(DWORD)-1;
		hr = AVIFileCreateStream(au->pfile, &au->as, &ahdr);
		if (hr!=AVIERR_OK) {au->iserr=TRUE; return hr;}
		hr = AVIStreamSetFormat(au->as,0,&au->wfx,sizeof(WAVEFORMATEX));
		if (hr!=AVIERR_OK) {au->iserr=TRUE; return hr;}
	}
	//
	// now we can write the data
	hr = AVIStreamWrite(au->as,au->nsamp,numsamps,dat,numbytes,0,NULL,NULL);
	if (hr!=AVIERR_OK) {au->iserr=TRUE; return hr;}
	au->nsamp+=numsamps; return S_OK;
}

unsigned int FormatAviMessage(HRESULT code, char *buf,unsigned int len)
{
	unsigned int mlen, n;
	const char *msg="unknown avi result code";
	switch (code)
	{
	case S_OK: msg="Success"; break;
	case AVIERR_BADFORMAT: msg="AVIERR_BADFORMAT: corrupt file or unrecognized format"; break;
	case AVIERR_MEMORY: msg="AVIERR_MEMORY: insufficient memory"; break;
	case AVIERR_FILEREAD: msg="AVIERR_FILEREAD: disk error while reading file"; break;
	case AVIERR_FILEOPEN: msg="AVIERR_FILEOPEN: disk error while opening file"; break;
	case REGDB_E_CLASSNOTREG: msg="REGDB_E_CLASSNOTREG: file type not recognised"; break;
	case AVIERR_READONLY: msg="AVIERR_READONLY: file is read-only"; break;
	case AVIERR_NOCOMPRESSOR: msg="AVIERR_NOCOMPRESSOR: a suitable compressor could not be found"; break;
	case AVIERR_UNSUPPORTED: msg="AVIERR_UNSUPPORTED: compression is not supported for this type of data"; break;
	case AVIERR_INTERNAL: msg="AVIERR_INTERNAL: internal error"; break;
	case AVIERR_BADFLAGS: msg="AVIERR_BADFLAGS"; break;
	case AVIERR_BADPARAM: msg="AVIERR_BADPARAM"; break;
	case AVIERR_BADSIZE: msg="AVIERR_BADSIZE"; break;
	case AVIERR_BADHANDLE: msg="AVIERR_BADHANDLE"; break;
	case AVIERR_FILEWRITE: msg="AVIERR_FILEWRITE: disk error while writing file"; break;
	case AVIERR_COMPRESSOR: msg="AVIERR_COMPRESSOR"; break;
	case AVIERR_NODATA: msg="AVIERR_READONLY"; break;
	case AVIERR_BUFFERTOOSMALL: msg="AVIERR_BUFFERTOOSMALL"; break;
	case AVIERR_CANTCOMPRESS: msg="AVIERR_CANTCOMPRESS"; break;
	case AVIERR_USERABORT: msg="AVIERR_USERABORT"; break;
	case AVIERR_ERROR: msg="AVIERR_ERROR"; break;
	}
	mlen=(unsigned int)strlen(msg);
	if (buf==0 || len==0) return mlen;
	n=mlen; if (n+1>len) n=len-1;
	strncpy(buf,msg,n); buf[n]=0;
	return mlen;
}

HAVI aviout = NULL;
void AVI_End ()
{
	if (!aviout)
	{
		MessageBox(mWnd,"No AVI capture is currently in progress!","Nintendulator",MB_OK);
		return;
	}
	CloseAvi(aviout);
	aviout = NULL;
	EnableMenuItem(GetMenu(mWnd),ID_MISC_STARTAVICAPTURE,MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_STOPAVICAPTURE,MF_BYCOMMAND | MF_GRAYED);
}
void AVI_Start ()
{
	WAVEFORMATEX wfx;
	HBITMAP hbm;
	BITMAPINFOHEADER bmih;
	long *pBits;
	HRESULT hr;
	AVICOMPRESSOPTIONS opts;

	char FileName[256] = {0};
	OPENFILENAME ofn;

	if (aviout)
	{
		MessageBox(mWnd,"An AVI capture is already in progress!","Nintendulator",MB_OK);
		return;
	}

	if (GFX.Depth != 32)
	{
		MessageBox(mWnd,"AVI capture currently requires 32-bit color!","Nintendulator",MB_OK);
		return;
	}

	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = mWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFilter = "AVI File (*.AVI)\0" "*.AVI\0" "\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = 256;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = "";
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = NULL;
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	if (!GetSaveFileName(&ofn))
		return;

	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 44100;
	wfx.nAvgBytesPerSec = 88200;
	wfx.nBlockAlign = 2;
	wfx.wBitsPerSample = 16;
	wfx.cbSize = 0;

	aviout = CreateAvi(FileName,16639261,&wfx);

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
	if (hr = SetAviVideoCompression(aviout,hbm,&opts,TRUE,mWnd))
	{
		char msg[256];
		FormatAviMessage(hr,msg,256);
		MessageBox(mWnd,msg,"Nintendulator",MB_OK);
		AVI_End();
		return;
	}
	DeleteObject(hbm);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_STARTAVICAPTURE,MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(GetMenu(mWnd),ID_MISC_STOPAVICAPTURE,MF_BYCOMMAND | MF_ENABLED);
}

void AVI_AddVideo ()
{
	HBITMAP hbm;
	long *pBits;
	BITMAPINFOHEADER bmih;
	HRESULT hr;
	int i;

	if (!aviout)
	{
		MessageBox(mWnd,"Error! AVI frame capture attempted while not recording!","Nintendulator",MB_OK);
		return;
	}
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = 256;
	bmih.biHeight = -240;
	bmih.biPlanes = 1;
	bmih.biBitCount = 32;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
	
	hbm = CreateDIBSection(NULL,(BITMAPINFO *)&bmih,DIB_RGB_COLORS,&pBits,NULL,0);

	for (i = 0; i < 240; i++)
		memcpy((unsigned char *)pBits + GFX.Pitch*i,(unsigned char *)GFX.DrawArray + GFX.Pitch*(239 - i),GFX.Pitch);
	if (hr = AddAviFrame(aviout,hbm))
	{
		char msg[256];
		FormatAviMessage(hr,msg,256);
		MessageBox(mWnd,msg,"Nintendulator",MB_OK);
	}
	DeleteObject(hbm);
}

void AVI_AddAudio ()
{
	HRESULT hr;
	if (!aviout)
	{
		MessageBox(mWnd,"Error! AVI audio capture attempted while not recording!","Nintendulator",MB_OK);
		return;
	}
	if (hr = AddAviAudio(aviout,APU.buffer,735*2))
	{
		char msg[256];
		FormatAviMessage(hr,msg,256);
		MessageBox(mWnd,msg,"Nintendulator",MB_OK);
	}
}