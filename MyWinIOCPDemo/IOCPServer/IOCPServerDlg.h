﻿
// IOCPServerDlg.h: 头文件
//

#pragma once

class CMyIOCPServer;

// CIOCPServerDlg 对话框
class CIOCPServerDlg : public CDialogEx
{
// 构造
public:
	CIOCPServerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IOCPSERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtRunSrv();
	afx_msg void OnBnClickedBtSend();

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	CMyIOCPServer* m_pIOCPServer;
};
