
// IOCPServerDlg.cpp: 实现文件
//

#include "framework.h"
#include "IOCPServer.h"
#include "IOCPServerDlg.h"
#include "afxdialogex.h"
#include "CMyIOCPServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CIOCPServerDlg 对话框



CIOCPServerDlg::CIOCPServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IOCPSERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pIOCPServer = nullptr;
}

void CIOCPServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CIOCPServerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BT_RUN_SRV, &CIOCPServerDlg::OnBnClickedBtRunSrv)
	ON_BN_CLICKED(IDC_BT_SEND, &CIOCPServerDlg::OnBnClickedBtSend)
END_MESSAGE_MAP()


// CIOCPServerDlg 消息处理程序

BOOL CIOCPServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CIOCPServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CIOCPServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CIOCPServerDlg::OnBnClickedBtRunSrv()
{
	// TODO: 在此添加控件通知处理程序代码
	if(m_pIOCPServer == nullptr)
	{
		m_pIOCPServer = new CMyIOCPServer();
		m_pIOCPServer->SetWnd(this);
		m_pIOCPServer->StartServer(9999);
	}
	
}


void CIOCPServerDlg::OnBnClickedBtSend()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strText;
	GetDlgItemText(IDC_EDIT_SEND, strText);
	if (m_pIOCPServer && !strText.IsEmpty())
	{
		m_pIOCPServer->PostSend(strText.GetString(), strText.GetLength() * sizeof(TCHAR));
		SetDlgItemText(IDC_EDIT_SEND, "");
	}
}


LRESULT CIOCPServerDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_SHOW_RECV_MSG) {
		CEdit* pEditShow = (CEdit*)GetDlgItem(IDC_EDIT2);
		pEditShow->SetSel(-1, -1);
		CString strMsg((TCHAR*)lParam);
		pEditShow->ReplaceSel(_T("client msg :") + strMsg  + _T("\r\n"));
		return TRUE;
	}
	return CDialog::WindowProc(message, wParam, lParam);
}