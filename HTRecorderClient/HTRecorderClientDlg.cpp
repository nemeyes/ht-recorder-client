
// HTRecorderClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "HTRecorderClient.h"
#include "HTRecorderClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
	EnableActiveAccessibility();
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CHTRecorderClientDlg dialog



CHTRecorderClientDlg::CHTRecorderClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHTRecorderClientDlg::IDD, pParent)
{
	EnableActiveAccessibility();
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CHTRecorderClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_SITE, _sites);
	DDX_Control(pDX, IDC_BUTTON_SETTUP, _btn_setup);
}

BEGIN_MESSAGE_MAP(CHTRecorderClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CHTRecorderClientDlg message handlers

BOOL CHTRecorderClientDlg::OnInitDialog()
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


	HTREEITEM root;
	root = _sites.InsertItem(L"부산교통공사1호선", 0, 1, TVI_ROOT, TVI_LAST);

	HMODULE self = GetModuleHandleW(NULL);
	WCHAR szModuleName[MAX_PATH] = { 0 };
	WCHAR szModulePath[FILENAME_MAX] = { 0 };
	WCHAR *pszModuleName = szModulePath;
	pszModuleName += GetModuleFileName(self, pszModuleName, (sizeof(szModulePath) / sizeof(*szModulePath)) - (pszModuleName - szModulePath));
	if (pszModuleName != szModulePath)
	{
		WCHAR *slash = wcsrchr(szModulePath, '\\');
		if (slash != NULL)
		{
			pszModuleName = slash + 1;
			_wcsset(pszModuleName, 0);
		}
		else
		{
			_wcsset(szModulePath, 0);
		}
	}

	CString setup_png_path = szModulePath;
	setup_png_path += _T("images\\setup.png");
	_btn_setup.LoadStdImage((LPWSTR)(LPCWSTR)setup_png_path);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CHTRecorderClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CHTRecorderClientDlg::OnPaint()
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
HCURSOR CHTRecorderClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CHTRecorderClientDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	
}


BOOL CHTRecorderClientDlg::OnEraseBkgnd(CDC* pDC)
{
	//// TODO: Add your message handler code here and/or call default
	//CRect rect;

	//GetClientRect(&rect);

	//CBrush myBrush(RGB(0, 0, 0));    // dialog background color <- 요기 바꾸면 됨.




	//CBrush *pOld = pDC->SelectObject(&myBrush);

	//BOOL bRes = pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);

	//pDC->SelectObject(pOld);    // restore old brush

	//return bRes;
	return CDialogEx::OnEraseBkgnd(pDC);
}
