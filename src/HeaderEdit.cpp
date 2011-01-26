/* Nintendulator - Win32 NES emulator written in C++
 * Copyright (C) 2002-2011 QMT Productions
 *
 * $URL$
 * $Id$
 */

#include "stdafx.h"
#include "Nintendulator.h"
#include "resource.h"
#include "HeaderEdit.h"

namespace HeaderEdit
{
INT_PTR CALLBACK	InesHeader (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TCHAR *filename;
	static char header[16];
	int i;
	FILE *rom;
	TCHAR name[MAX_PATH];
	switch (message)
	{
	case WM_INITDIALOG:
		filename = (TCHAR *)lParam;
		rom = _tfopen(filename, _T("rb"));
		if (!rom)
		{
			MessageBox(hMainWnd, _T("Unable to open ROM!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			EndDialog(hDlg, 0);
			break;
		}
		fread(header, 16, 1, rom);
		fclose(rom);
		if (memcmp(header, "NES\x1A", 4))
		{
			MessageBox(hMainWnd, _T("Selected file is not an iNES ROM image!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			EndDialog(hDlg, 0);
			break;
		}
		if ((header[7] & 0x0C) == 0x08)
		{
			MessageBox(hMainWnd, _T("Selected ROM contains iNES 2.0 information,  which is not yet supported. Please use a stand-alone editor."), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			EndDialog(hDlg, 0);
			break;
		}
		if ((header[7] & 0x0C) == 0x04)
		{
			MessageBox(hMainWnd, _T("Selected ROM appears to have been corrupted by \"DiskDude!\" - cleaning..."), _T("Nintendulator"), MB_OK | MB_ICONWARNING);
			for (i = 7; i < 16; i++)
				header[i] = 0;
		}
		else if (((header[8] || header[9] || header[10] || header[11] || header[12] || header[13] || header[14] || header[15])) && 
			(MessageBox(hDlg, _T("Unrecognized data has been detected in the reserved region of this ROM's header! Do you wish to clean it?"), _T("Nintendulator"), MB_YESNO | MB_ICONQUESTION) == IDYES))
		{
			for (i = 7; i < 16; i++)
				header[i] = 0;
		}

		i = (int)_tcslen(filename)-1;
		while ((i >= 0) && (filename[i] != _T('\\')))
			i--;
		_tcscpy(name, filename+i+1);
		name[_tcslen(name)-4] = 0;
		SetDlgItemText(hDlg, IDC_INES_NAME, name);
		SetDlgItemInt(hDlg, IDC_INES_PRG, header[4], FALSE);
		SetDlgItemInt(hDlg, IDC_INES_CHR, header[5], FALSE);
		SetDlgItemInt(hDlg, IDC_INES_MAP, ((header[6] >> 4) & 0xF) | (header[7] & 0xF0), FALSE);
		CheckDlgButton(hDlg, IDC_INES_BATT, (header[6] & 0x02) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_INES_TRAIN, (header[6] & 0x04) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_INES_4SCR, (header[6] & 0x08) ? BST_CHECKED : BST_UNCHECKED);
		if (header[6] & 0x01)
			CheckRadioButton(hDlg, IDC_INES_HORIZ, IDC_INES_VERT, IDC_INES_VERT);
		else	CheckRadioButton(hDlg, IDC_INES_HORIZ, IDC_INES_VERT, IDC_INES_HORIZ);
		CheckDlgButton(hDlg, IDC_INES_VS, (header[7] & 0x01) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_INES_PC10, (header[7] & 0x02) ? BST_CHECKED : BST_UNCHECKED);
		if (IsDlgButtonChecked(hDlg, IDC_INES_4SCR))
		{
			EnableWindow(GetDlgItem(hDlg, IDC_INES_HORIZ), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_INES_VERT), FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg, IDC_INES_HORIZ), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_INES_VERT), TRUE);
		}
		if (IsDlgButtonChecked(hDlg, IDC_INES_VS))
			EnableWindow(GetDlgItem(hDlg, IDC_INES_PC10), FALSE);
		else if (IsDlgButtonChecked(hDlg, IDC_INES_PC10))
			EnableWindow(GetDlgItem(hDlg, IDC_INES_VS), FALSE);
		else
		{
			EnableWindow(GetDlgItem(hDlg, IDC_INES_VS), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_INES_PC10), TRUE);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_INES_PRG:
			i = GetDlgItemInt(hDlg, IDC_INES_PRG, NULL, FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg, IDC_INES_PRG, i = 0, FALSE);
			header[4] = (char)(i & 0xFF);
			return TRUE;
		case IDC_INES_CHR:
			i = GetDlgItemInt(hDlg, IDC_INES_CHR, NULL, FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg, IDC_INES_CHR, i = 0, FALSE);
			header[5] = (char)(i & 0xFF);
			return TRUE;
		case IDC_INES_MAP:
			i = GetDlgItemInt(hDlg, IDC_INES_MAP, NULL, FALSE);
			if ((i < 0) || (i > 255))
				SetDlgItemInt(hDlg, IDC_INES_MAP, i = 0, FALSE);
			header[6] = (char)((header[6] & 0x0F) | ((i & 0x0F) << 4));
			header[7] = (char)((header[7] & 0x0F) | (i & 0xF0));
			return TRUE;
		case IDC_INES_BATT:
			if (IsDlgButtonChecked(hDlg, IDC_INES_BATT))
				header[6] |= 0x02;
			else	header[6] &= ~0x02;
			return TRUE;
		case IDC_INES_TRAIN:
			if (IsDlgButtonChecked(hDlg, IDC_INES_TRAIN))
			{
				MessageBox(hDlg, _T("Trained ROMs are not supported in Nintendulator!"), _T("Nintendulator"), MB_OK | MB_ICONWARNING);
				header[6] |= 0x04;
			}
			else	header[6] &= ~0x04;
			return TRUE;
		case IDC_INES_4SCR:
			if (IsDlgButtonChecked(hDlg, IDC_INES_4SCR))
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_HORIZ), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_INES_VERT), FALSE);
				header[6] |= 0x08;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_HORIZ), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_INES_VERT), TRUE);
				header[6] &= ~0x08;
			}
			return TRUE;
		case IDC_INES_HORIZ:
			if (IsDlgButtonChecked(hDlg, IDC_INES_HORIZ))
				header[6] &= ~0x01;
			return TRUE;
		case IDC_INES_VERT:
			if (IsDlgButtonChecked(hDlg, IDC_INES_VERT))
				header[6] |= 0x01;
			return TRUE;
		case IDC_INES_VS:
			if (IsDlgButtonChecked(hDlg, IDC_INES_VS))
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_PC10), FALSE);
				header[7] |= 0x01;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_PC10), TRUE);
				header[7] &= ~0x01;
			}
			return TRUE;
		case IDC_INES_PC10:
			if (IsDlgButtonChecked(hDlg, IDC_INES_PC10))
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_VS), FALSE);
				header[7] |= 0x02;
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_INES_VS), TRUE);
				header[7] &= ~0x02;
			}
			return TRUE;
		case IDOK:
			rom = _tfopen(filename, _T("r+b"));
			if (rom)
			{
				fwrite(header, 16, 1, rom);
				fclose(rom);
			}
			else	MessageBox(hMainWnd, _T("Failed to open ROM!"), _T("Nintendulator"), MB_OK | MB_ICONERROR);
			// fall through
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
	}
	return FALSE;
}

void	Open (TCHAR *filename)
{
	DialogBoxParam(hInst, (LPCTSTR)IDD_INESHEADER, hMainWnd, InesHeader, (LPARAM)filename);
}
}