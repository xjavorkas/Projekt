
// ApplicationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Application.h"
#include "ApplicationDlg.h"
#include "afxdialogex.h"
#include <utility>
#include <tuple>
#include <vector>
#include "Utils.h"
#include <omp.h>

namespace {
	//void LoadAndCalc(CString fileName, Gdiplus::Bitmap* pBitmap, std::vector<int>&Red)
	void LoadAndCalc(CString fileName, Gdiplus::Bitmap* pBitmap, std::vector<int>&Red, std::vector<int>&Green, std::vector<int>&Blue)
	{	
		Red.clear();
		Green.clear();
		Blue.clear();
		Red.assign(256, 0);
		Green.assign(256, 0);
		Blue.assign(256, 0);
		pBitmap = Gdiplus::Bitmap::FromFile(fileName);

		Gdiplus::Rect ret;
		Gdiplus::BitmapData bmpData;

		pBitmap->LockBits(&ret, Gdiplus::ImageLockModeRead, Gdiplus::PixelFormat PixelFormat32bppRGB, &bmpData);
		
		
		for (int y = 0; y < ret.Height; y++) {
			 uint32_t* pLine = (uint32_t*)((uint8_t*)bmpData.Scan0 + bmpData.Stride*y);
				for (int x = 0; x < ret.Width; x++) {
					int r = ((*pLine) >> 16)&0xff;
					int g = ((*pLine) >> 8)&0xff;
					int b = ((*pLine))&0xff;
					Red[r]++;
					Green[g]++;
					Blue[b]++;
					pLine++;
				}
			}

	


	/*	for (int x = 0; x < pBitmap->GetWidth(); x++)
		{
			for (int y = 0; y < pBitmap->GetHeight(); y++)
			{
				Gdiplus::Color color;
				pBitmap->GetPixel(x, y, &color);
				Red[color.GetRed()]++;
				Green[color.GetGreen()]++;
				Blue[color.GetBlue()]++;
			}
		} */
	}
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifndef MIN_SIZE
#define MIN_SIZE 300
#endif

void CStaticImage::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	GetParent()->SendMessage(CApplicationDlg::WM_DRAW_IMAGE, (WPARAM)lpDrawItemStruct);
}

void CStaticHistogram::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	GetParent()->SendMessage(CApplicationDlg::WM_DRAW_HISTOGRAM, (WPARAM)lpDrawItemStruct);
}




// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	void DoDataExchange(CDataExchange* pDX) override    // DDX/DDV support
	{
		CDialogEx::DoDataExchange(pDX);
	}

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


namespace
{
	typedef BOOL(WINAPI *LPFN_GLPI)(
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
		PDWORD);


	// Helper function to count set bits in the processor mask.
	DWORD CountSetBits(ULONG_PTR bitMask)
	{
		DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
		DWORD bitSetCount = 0;
		ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
		DWORD i;

		for (i = 0; i <= LSHIFT; ++i)
		{
			bitSetCount += ((bitMask & bitTest) ? 1 : 0);
			bitTest /= 2;
		}

		return bitSetCount;
	}

	DWORD CountMaxThreads()
	{
		LPFN_GLPI glpi;
		BOOL done = FALSE;
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
		DWORD returnLength = 0;
		DWORD logicalProcessorCount = 0;
		DWORD numaNodeCount = 0;
		DWORD processorCoreCount = 0;
		DWORD processorL1CacheCount = 0;
		DWORD processorL2CacheCount = 0;
		DWORD processorL3CacheCount = 0;
		DWORD processorPackageCount = 0;
		DWORD byteOffset = 0;
		PCACHE_DESCRIPTOR Cache;

		glpi = (LPFN_GLPI)GetProcAddress(
			GetModuleHandle(TEXT("kernel32")),
			"GetLogicalProcessorInformation");
		if (NULL == glpi)
		{
			TRACE(TEXT("\nGetLogicalProcessorInformation is not supported.\n"));
			return (1);
		}

		while (!done)
		{
			DWORD rc = glpi(buffer, &returnLength);

			if (FALSE == rc)
			{
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					if (buffer)
						free(buffer);

					buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
						returnLength);

					if (NULL == buffer)
					{
						TRACE(TEXT("\nError: Allocation failure\n"));
						return (2);
					}
				}
				else
				{
					TRACE(TEXT("\nError %d\n"), GetLastError());
					return (3);
				}
			}
			else
			{
				done = TRUE;
			}
		}

		ptr = buffer;

		while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
		{
			switch (ptr->Relationship)
			{
			case RelationNumaNode:
				// Non-NUMA systems report a single record of this type.
				numaNodeCount++;
				break;

			case RelationProcessorCore:
				processorCoreCount++;

				// A hyperthreaded core supplies more than one logical processor.
				logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
				break;

			case RelationCache:
				// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
				Cache = &ptr->Cache;
				if (Cache->Level == 1)
				{
					processorL1CacheCount++;
				}
				else if (Cache->Level == 2)
				{
					processorL2CacheCount++;
				}
				else if (Cache->Level == 3)
				{
					processorL3CacheCount++;
				}
				break;

			case RelationProcessorPackage:
				// Logical processors share a physical package.
				processorPackageCount++;
				break;

			default:
				TRACE(TEXT("\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n"));
				break;
			}
			byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
			ptr++;
		}

		TRACE(TEXT("\nGetLogicalProcessorInformation results:\n"));
		TRACE(TEXT("Number of NUMA nodes: %d\n"), numaNodeCount);
		TRACE(TEXT("Number of physical processor packages: %d\n"), processorPackageCount);
		TRACE(TEXT("Number of processor cores: %d\n"), processorCoreCount);
		TRACE(TEXT("Number of logical processors: %d\n"), logicalProcessorCount);
		TRACE(TEXT("Number of processor L1/L2/L3 caches: %d/%d/%d\n"), processorL1CacheCount, processorL2CacheCount, processorL3CacheCount);

		free(buffer);
		TRACE(_T("OPENMP - %i/%i\n"), omp_get_num_procs(), omp_get_max_threads());
		return logicalProcessorCount;
	}
}

// CApplicationDlg dialog


CApplicationDlg::CApplicationDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_APPLICATION_DIALOG, pParent)
	, m_pBitmap(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_nMaxThreads = CountMaxThreads();
}

void CApplicationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE_LIST, m_ctrlFileList);
	DDX_Control(pDX, IDC_IMAGE, m_ctrlImage);
	DDX_Control(pDX, IDC_HISTOGRAM, m_ctrlHistogram);
}

BEGIN_MESSAGE_MAP(CApplicationDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, OnUpdateFileClose)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_MESSAGE(WM_DRAW_IMAGE, OnDrawImage)
	ON_MESSAGE(WM_DRAW_HISTOGRAM, OnDrawHistogram)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_FILE_LIST, OnLvnItemchangedFileList)
	ON_COMMAND(ID_LOG_OPEN, OnLogOpen)
	ON_UPDATE_COMMAND_UI(ID_LOG_OPEN, OnUpdateLogOpen)
	ON_COMMAND(ID_LOG_CLEAR, OnLogClear)
	ON_UPDATE_COMMAND_UI(ID_LOG_CLEAR, OnUpdateLogClear)
	ON_WM_DESTROY()
	ON_COMMAND(ID_HSTGRM_RED, &CApplicationDlg::OnHstgrmRed)
	ON_UPDATE_COMMAND_UI(ID_HSTGRM_RED, &CApplicationDlg::OnUpdateHstgrmRed)
END_MESSAGE_MAP()


void CApplicationDlg::OnDestroy()
{
	m_ctrlLog.DestroyWindow();
	Default();

	if (m_pBitmap != nullptr)
	{
		delete m_pBitmap;
		m_pBitmap = nullptr;
	}
}

LRESULT CApplicationDlg::OnDrawHistogram(WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT lpDI = (LPDRAWITEMSTRUCT)wParam;

	CDC * pDC = CDC::FromHandle(lpDI->hDC);

	pDC->FillSolidRect(&(lpDI->rcItem), RGB(255, 255, 255));

	if (m_vHistRed.size() == 0)
	{
		return S_OK;
	}

	if (m_vHistGreen.size() == 0)
	{
		return S_OK;
	}

	if (m_vHistBlue.size() == 0)
	{
		return S_OK;
	}

	int left = lpDI->rcItem.left;
	int right = lpDI->rcItem.right;
	int top = lpDI->rcItem.top;
	int bottom = lpDI->rcItem.bottom;
	int width = right - left;
	int height = bottom - top;

	double ScalX = double(width) / 256.;

	int m_max = 0;

	for (int x = 0; x < (int)m_vHistRed.size(); x++)
	{
		if (m_vHistRed[x] > m_max) {
			m_max = m_vHistRed[x];
		}
	}

	double ScalY = double(height) / double(log(m_max));

	for (int x = 0; x < (int)m_vHistRed.size(); x++)
	{
		double dHeight = (double)(log(m_vHistRed[x])) * (double)ScalY;
		CRect Rectangle(ScalX*x, bottom - dHeight, ScalX*x + 1, bottom);
		pDC->FillSolidRect(Rectangle, RGB(255, 0, 0));
	}


	for (int x = 0; x < (int)m_vHistGreen.size(); x++)
	{
		if (m_vHistGreen[x] > m_max) {
			m_max = m_vHistGreen[x];
		}
	}

	for (int x = 0; x < (int)m_vHistGreen.size(); x++)
	{
		double dHeight = (double)(log(m_vHistGreen[x])) * (double)ScalY;
		CRect Rectangle(ScalX*x, bottom - dHeight, ScalX*x + 1, bottom);
		pDC->FillSolidRect(Rectangle, RGB(0, 255, 0));
	}


	for (int x = 0; x < (int)m_vHistBlue.size(); x++)
	{
		if (m_vHistBlue[x] > m_max) {
			m_max = m_vHistBlue[x];
		}
	}

	for (int x = 0; x < (int)m_vHistBlue.size(); x++)
	{
		double dHeight = (double)(log(m_vHistBlue[x])) * (double)ScalY;
		CRect Rectangle(ScalX*x, bottom - dHeight, ScalX*x + 1, bottom);
		pDC->FillSolidRect(Rectangle, RGB(0, 0, 255));
	}

	CBrush brBlack(RGB(0, 0, 0));
	pDC->FrameRect(&(lpDI->rcItem), &brBlack);

	return S_OK;
}

LRESULT CApplicationDlg::OnDrawImage(WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT lpDI = (LPDRAWITEMSTRUCT)wParam;

	CDC * pDC = CDC::FromHandle(lpDI->hDC);

	if (m_pBitmap == nullptr)
	{
		pDC->FillSolidRect(&(lpDI->rcItem), RGB(255, 255, 255));
	}
	else
	{
		// Fit bitmap into client area
		double dWtoH = (double)m_pBitmap->GetWidth() / (double)m_pBitmap->GetHeight();

		CRect rct(lpDI->rcItem);
		rct.DeflateRect(1, 1, 1, 1);

		UINT nHeight = rct.Height();
		UINT nWidth = (UINT)(dWtoH * nHeight);

		if (nWidth > (UINT)rct.Width())
		{
			nWidth = rct.Width();
			nHeight = (UINT)(nWidth / dWtoH);
			_ASSERTE(nHeight <= (UINT)rct.Height());
		}

		if (nHeight < (UINT)rct.Height())
		{
			UINT nBanner = (rct.Height() - nHeight) / 2;
			pDC->FillSolidRect(rct.left, rct.top, rct.Width(), nBanner, RGB(255, 255, 255));
			pDC->FillSolidRect(rct.left, rct.bottom - nBanner - 2, rct.Width(), nBanner + 2, RGB(255, 255, 255));
		}

		if (nWidth < (UINT)rct.Width())
		{
			UINT nBanner = (rct.Width() - nWidth) / 2;
			pDC->FillSolidRect(rct.left, rct.top, nBanner, rct.Height(), RGB(255, 255, 255));
			pDC->FillSolidRect(rct.right - nBanner - 2, rct.top, nBanner + 2, rct.Height(), RGB(255, 255, 255));
		}

		Gdiplus::Graphics gr(lpDI->hDC);
		Gdiplus::Rect destRect(rct.left + (rct.Width() - nWidth) / 2, rct.top + (rct.Height() - nHeight) / 2, nWidth, nHeight);
		gr.DrawImage(m_pBitmap, destRect);
	}

	CBrush brBlack(RGB(0, 0, 0));
	pDC->FrameRect(&(lpDI->rcItem), &brBlack);

	return S_OK;
}

void CApplicationDlg::OnSizing(UINT nSide, LPRECT lpRect)
{
	if ((lpRect->right - lpRect->left) < MIN_SIZE)
	{
		switch (nSide)
		{
		case WMSZ_LEFT:
		case WMSZ_BOTTOMLEFT:
		case WMSZ_TOPLEFT:
			lpRect->left = lpRect->right - MIN_SIZE;
		default:
			lpRect->right = lpRect->left + MIN_SIZE;
			break;
		}
	}

	if ((lpRect->bottom - lpRect->top) < MIN_SIZE)
	{
		switch (nSide)
		{
		case WMSZ_TOP:
		case WMSZ_TOPRIGHT:
		case WMSZ_TOPLEFT:
			lpRect->top = lpRect->bottom - MIN_SIZE;
		default:
			lpRect->bottom = lpRect->top + MIN_SIZE;
			break;
		}
	}

	__super::OnSizing(nSide, lpRect);
}

void CApplicationDlg::OnSize(UINT nType, int cx, int cy)
{
	Default();

	if (!::IsWindow(m_ctrlFileList.m_hWnd) || !::IsWindow(m_ctrlImage.m_hWnd))
		return;

	m_ctrlFileList.SetWindowPos(nullptr, -1, -1, m_ptFileList.x, cy - m_ptFileList.y, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
	m_ctrlFileList.Invalidate();


	m_ctrlImage.SetWindowPos(nullptr, -1, -1, cx - m_ptImage.x, cy - m_ptImage.y, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
	m_ctrlImage.Invalidate();

	CRect rct;
	GetClientRect(&rct);

	m_ctrlHistogram.SetWindowPos(nullptr, rct.left + m_ptHistogram.x, rct.bottom - m_ptHistogram.y, -1, -1, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
	m_ctrlHistogram.Invalidate();
}

void CApplicationDlg::OnClose()
{
	EndDialog(0);
}

BOOL CApplicationDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

									// TODO: Add extra initialization here
	CRect rct;
	m_ctrlFileList.GetClientRect(&rct);
	m_ctrlFileList.InsertColumn(0, _T("Filename"), 0, rct.Width());

	CRect rctClient;
	GetClientRect(&rctClient);
	m_ctrlFileList.GetWindowRect(&rct);
	m_ptFileList.y = rctClient.Height() - rct.Height();
	m_ptFileList.x = rct.Width();

	m_ctrlImage.GetWindowRect(&rct);
	m_ptImage.x = rctClient.Width() - rct.Width();
	m_ptImage.y = rctClient.Height() - rct.Height();

	m_ctrlHistogram.GetWindowRect(&rct);
	ScreenToClient(&rct);
	m_ptHistogram.x = rct.left - rctClient.left;
	m_ptHistogram.y = rctClient.bottom - rct.top;

	m_ctrlLog.Create(IDD_LOG_DIALOG, this);
	m_ctrlLog.ShowWindow(SW_HIDE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CApplicationDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CApplicationDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CApplicationDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CApplicationDlg::OnFileOpen()
{
	CFileDialog dlg(true, nullptr, nullptr
		, OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST
		, _T("Bitmap Files (*.bmp)|*.bmp|JPEG Files (*.jpg;*.jpeg)|*.jpg;*.jpeg|PNG Files (*.png)|*.png||")
		, this);
	CString cs;
	const int maxFiles = 100;
	const int buffSize = maxFiles * (MAX_PATH + 1) + 1;

	dlg.GetOFN().lpstrFile = cs.GetBuffer(buffSize);
	dlg.GetOFN().nMaxFile = buffSize;

	if (dlg.DoModal() == IDOK)
	{
		m_ctrlFileList.DeleteAllItems();

		if (m_pBitmap != nullptr)
		{
			delete m_pBitmap;
			m_pBitmap = nullptr;
		}

		m_ctrlImage.Invalidate();
		m_ctrlHistogram.Invalidate();

		cs.ReleaseBuffer();

		std::vector<CString> names;

		std::tie(m_csDirectory, names) = Utils::ParseFiles(cs);

		for (int i = 0; i < (int)names.size(); ++i)
		{
			m_ctrlFileList.InsertItem(i, names[i]);
		}
	}
	else
	{
		cs.ReleaseBuffer();
	}

}


void CApplicationDlg::OnUpdateFileOpen(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}


void CApplicationDlg::OnFileClose()
{
	m_ctrlFileList.DeleteAllItems();
}


void CApplicationDlg::OnUpdateFileClose(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_ctrlFileList.GetItemCount() > 0);
}

LRESULT CApplicationDlg::OnKickIdle(WPARAM wParam, LPARAM lParam)
{
	CMenu* pMainMenu = GetMenu();
	CCmdUI cmdUI;
	for (UINT n = 0; n < (UINT)pMainMenu->GetMenuItemCount(); ++n)
	{
		CMenu* pSubMenu = pMainMenu->GetSubMenu(n);
		cmdUI.m_nIndexMax = pSubMenu->GetMenuItemCount();
		for (UINT i = 0; i < cmdUI.m_nIndexMax; ++i)
		{
			cmdUI.m_nIndex = i;
			cmdUI.m_nID = pSubMenu->GetMenuItemID(i);
			cmdUI.m_pMenu = pSubMenu;
			cmdUI.DoUpdate(this, FALSE);
		}
	}
	return TRUE;
}

void CApplicationDlg::OnLvnItemchangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (m_pBitmap != nullptr)
	{
		delete m_pBitmap;
		m_pBitmap = nullptr;
	}

	CString csFileName;
	POSITION pos = m_ctrlFileList.GetFirstSelectedItemPosition();
	if (pos)
		csFileName = m_csDirectory + m_ctrlFileList.GetItemText(m_ctrlFileList.GetNextSelectedItem(pos), 0);

	//LoadAndCalc(csFileName, m_pBitmap, m_vHistRed)

	/*if (csFileName.IsEmpty())
	{
		return;
	}*/

	
	LoadAndCalc(csFileName, m_pBitmap, m_vHistRed, m_vHistGreen, m_vHistBlue);


	m_ctrlImage.Invalidate();

	m_ctrlHistogram.Invalidate();

	*pResult = 0;
}


void CApplicationDlg::OnLogOpen()
{
	m_ctrlLog.ShowWindow(SW_SHOW);
}


void CApplicationDlg::OnUpdateLogOpen(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(::IsWindow(m_ctrlLog.m_hWnd) && !m_ctrlLog.IsWindowVisible());
}


void CApplicationDlg::OnLogClear()
{
	m_ctrlLog.SendMessage(CLogDlg::WM_TEXT);
}


void CApplicationDlg::OnUpdateLogClear(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(::IsWindow(m_ctrlLog.m_hWnd) && m_ctrlLog.IsWindowVisible());
}


void CApplicationDlg::OnHstgrmRed()
{

	// TODO: Add your command handler code here
}


void CApplicationDlg::OnUpdateHstgrmRed(CCmdUI *pCmdUI)
{
	// TODO: update histogramu, zaskrtnutie aleo zakazanie

}


