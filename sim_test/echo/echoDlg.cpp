
// echoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "echo.h"
#include "echoDlg.h"
#include "afxdialogex.h"

#include "echo_h264_common.h"
#include "utf8to.h"

#include <vector>
#include <string>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
//#include "vld.h"
#endif


// CAboutDlg dialog used for App About
#define SIM_PORT 16008

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
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CechoDlg dialog



CechoDlg::CechoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CechoDlg::IDD, pParent)
	, m_strDev(_T(""))
	, m_strIP(_T("180.150.184.115"))
	, m_iPort(SIM_PORT)
	, m_iUser(1000)
	, m_strState(_T(""))
	, m_strInfo(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_viewing = FALSE;

	m_viRecorder = NULL;
	m_viPlayer = NULL;

	m_recording = FALSE;
	m_playing = FALSE;
}

void CechoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CBX_DEVICE, m_cbxDevice);
	DDX_Control(pDX, IDC_SRC_VIDEO, m_srcVideo);
	DDX_CBString(pDX, IDC_CBX_DEVICE, m_strDev);
	DDX_Control(pDX, IDC_BTNVIEW, m_btnView);
	DDX_Control(pDX, IDC_DST_VIDEO, m_dstVideo);
	DDX_Text(pDX, IDC_EDIP, m_strIP);
	DDX_Text(pDX, IDC_EDPORT, m_iPort);
	DDV_MinMaxInt(pDX, m_iPort, 0, 65535);
	DDX_Text(pDX, IDC_EDUSER, m_iUser);
	DDX_Text(pDX, IDC_STATE, m_strState);
	DDX_Control(pDX, IDC_BTNCONNECT, m_btnEcho);
	DDX_Text(pDX, IDC_EDINFO, m_strInfo);
}

BEGIN_MESSAGE_MAP(CechoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()

	ON_MESSAGE(WM_CONNECT_SUCC, OnConnectSucc)
	ON_MESSAGE(WM_CONNECT_FAILED, OnConnectFailed)
	ON_MESSAGE(WM_TIMEOUT, OnTimeout)
	ON_MESSAGE(WM_DISCONNECTED, OnDisconnected)

	ON_MESSAGE(WM_NET_INTERRUPT, OnNetInterrupt)
	ON_MESSAGE(WM_NET_RECOVER, OnNetRecover)
	ON_MESSAGE(WM_CHANGE_BITRATE, OnChangeBitrate)

	ON_MESSAGE(WM_START_PLAY, OnStartPlay)
	ON_MESSAGE(WM_STOP_PLAY, OnStopPlay)
	ON_MESSAGE(WM_STATE_INFO, OnStateInfo)

	ON_BN_CLICKED(IDC_BTNVIEW, &CechoDlg::OnBnClickedBtnview)
	ON_BN_CLICKED(IDC_BTNCONNECT, &CechoDlg::OnBnClickedBtnconnect)
END_MESSAGE_MAP()


// CechoDlg message handlers

BOOL CechoDlg::OnInitDialog()
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

	struct WSAData wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
	
	srand((uint32_t)time(NULL));

	::CoInitialize(NULL);

	m_srcVideo.SetWindowPos(NULL, 0, 0, PIC_WIDTH_640, PIC_HEIGHT_480, SWP_NOMOVE);
	m_dstVideo.SetWindowPos(NULL, 0, 0, PIC_WIDTH_640, PIC_HEIGHT_480, SWP_NOMOVE);

	InitVideoDevices();

	m_frame = new SimFramework(GetSafeHwnd());
	m_frame->init(SIM_PORT, MIN_VIDEO_BITARE, START_VIDEO_BITRATE, MAX_VIDEO_BITRAE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CechoDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CechoDlg::OnPaint()
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
HCURSOR CechoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CechoDlg::CloseAll()
{
	if (m_connected){
		if (m_recording){
			m_frame->stop_recorder();
			if (m_viRecorder != NULL){
				m_viRecorder->close();
				delete m_viRecorder;
				m_viRecorder = NULL;
			}
			m_recording = FALSE;
		}

		m_frame->disconnect();

		m_btnView.EnableWindow(TRUE);
		m_btnEcho.SetWindowText(_T("start echo"));
		m_connected = FALSE;
	}
}

void CechoDlg::OnDestroy()
{
	CloseAll();

	if (m_playing){
		m_frame->stop_player();
		if (m_viPlayer != NULL){
			m_viPlayer->close();
			delete m_viPlayer;
			m_viPlayer = NULL;
		}
		m_playing = FALSE;
	}

	if (m_viewing){
		m_viewing = FALSE;

		m_view.stop();
		if (m_viRecorder != NULL){
			m_viRecorder->close();
			delete m_viRecorder;
			m_viRecorder = NULL;
		}

		if (m_viPlayer != NULL){
			m_viPlayer->close();
			delete m_viPlayer;
			m_viPlayer = NULL;
		}

		m_btnView.SetWindowText(_T("start view"));
	}

	if (m_frame != NULL){
		m_frame->destroy();
		delete m_frame;
		m_frame = NULL;
	}

	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	WSACleanup();
}

void CechoDlg::InitVideoDevices()
{
	vector<wstring> cameras;
	int count = get_camera_input_devices(cameras);
	int cur_count = m_cbxDevice.GetCount();
	for (int i = 0; i < cur_count; ++i)
		m_cbxDevice.DeleteString(0);

	for (std::wstring& name : cameras)
		m_cbxDevice.AddString(name.c_str());

	if (cur_count > 0)
		m_cbxDevice.SetCurSel(cur_count - 1);
	else
		m_cbxDevice.SetCurSel(0);
}



void CechoDlg::OnBnClickedBtnview()
{
	UpdateData(TRUE);
	// TODO: Add your control notification handler code here
	if (!m_viewing){
		m_viewing = TRUE;

		/*创建一个录制设备，包括视频编解码*/
		TCHAR* device;
		device = m_strDev.GetBuffer(m_strDev.GetLength());
		m_viRecorder = new CFVideoRecorder(device);

		video_info_t info;
		info.pix_format = RGB24;
		info.rate = 24;
		info.width = PIC_WIDTH_640;
		info.height = PIC_HEIGHT_480;
		info.codec_width = PIC_WIDTH_640;
		info.codec_height = PIC_HEIGHT_480;
		m_viRecorder->set_video_info(info);

		RECT display_rect;
		m_srcVideo.GetClientRect(&display_rect);

		m_viRecorder->set_view_hwnd(m_srcVideo.GetSafeHwnd(), display_rect);
		m_strDev.ReleaseBuffer();
		m_viRecorder->open();

		/*创建一个播放设备*/
		m_dstVideo.GetClientRect(&display_rect);
		m_viPlayer = new CFVideoPlayer(m_dstVideo.GetSafeHwnd(), display_rect);
		m_viPlayer->open();

		/*打开预览线程*/
		m_btnView.SetWindowText(_T("stop view"));
		m_view.set_video_devices(m_viRecorder, m_viPlayer);
		m_view.start();
	}
	else{
		m_viewing = FALSE;

		m_view.stop();
		if (m_viRecorder != NULL){
			m_viRecorder->close();
			delete m_viRecorder;
			m_viRecorder = NULL;
		}

		if (m_viPlayer != NULL){
			m_viPlayer->close();
			delete m_viPlayer;
			m_viPlayer = NULL;
		}

		m_btnView.SetWindowText(_T("start view"));
	}
}


void CechoDlg::OnBnClickedBtnconnect()
{
	UpdateData(TRUE);

	if (!m_connected){
		TCHAR* wip;
		wip = m_strIP.GetBuffer(m_strIP.GetLength());
		std::string ip = helper::app2asci(wip);

		if (m_frame->connect(m_iUser, ip.c_str(), m_iPort) == 0){
			m_btnView.EnableWindow(FALSE);
			m_btnEcho.SetWindowText(_T("stop echo"));
			m_connected = TRUE;
		}
		else{
			m_strInfo += _T("connect P2P network failed!\r\n");
			UpdateData(FALSE);
		}

		m_strIP.ReleaseBuffer();
	}
	else{
		CloseAll();
	}
}

LRESULT CechoDlg::OnConnectSucc(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	m_strInfo += _T("connect server ");
	m_strInfo += m_strIP;

	char tmp[100];
	sprintf(tmp, ":%d success\r\n", m_iPort);
	m_strInfo += tmp;

	UpdateData(FALSE);

	/*创建一个录制设备，包括视频编解码*/
	TCHAR* device;
	device = m_strDev.GetBuffer(m_strDev.GetLength());
	m_viRecorder = new CFVideoRecorder(device);

	video_info_t info;
	info.pix_format = RGB24;
	info.rate = 24;
	info.width = PIC_WIDTH_640;
	info.height = PIC_HEIGHT_480;
	info.codec_width = PIC_WIDTH_640;
	info.codec_height = PIC_HEIGHT_480;
	m_viRecorder->set_video_info(info);

	RECT display_rect;
	m_srcVideo.GetClientRect(&display_rect);

	m_viRecorder->set_view_hwnd(m_srcVideo.GetSafeHwnd(), display_rect);
	m_strDev.ReleaseBuffer();
	m_viRecorder->open();

	/*启动录制线程*/
	m_frame->start_recorder(m_viRecorder);
	m_recording = TRUE;

	return 0L;
}

LRESULT CechoDlg::OnConnectFailed(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	m_strInfo += _T("connect receiver ");
	m_strInfo += m_strIP;

	char tmp[100];
	sprintf(tmp, ":%d failed\r\n", m_iPort);
	m_strInfo += tmp;

	UpdateData(FALSE);

	CloseAll();

	return 0L;
}

LRESULT CechoDlg::OnTimeout(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	m_strInfo += _T("net timeout, receiver ");
	m_strInfo += m_strIP;

	char tmp[100];
	sprintf(tmp, ":%d\r\n", m_iPort);
	m_strInfo += tmp; 

	UpdateData(FALSE);

	CloseAll();

	return 0L;
}

LRESULT CechoDlg::OnDisconnected(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	m_strInfo += _T("net disconnected, receiver ");
	m_strInfo += m_strIP;

	char tmp[100];
	sprintf(tmp, ":%d\r\n", m_iPort);
	m_strInfo += tmp;

	UpdateData(FALSE);

	return 0L;
}

LRESULT CechoDlg::OnStartPlay(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	char data[128];
	uint32_t uid = (uint32_t)wparam;
	sprintf(data, "start play, player = %u\r\n", uid);

	m_strInfo += data;
	UpdateData(FALSE);

	/*创建一个播放设备*/
	RECT display_rect;
	m_dstVideo.GetClientRect(&display_rect);
	m_viPlayer = new CFVideoPlayer(m_dstVideo.GetSafeHwnd(), display_rect);
	m_viPlayer->open();

	m_frame->start_player(uid, m_viPlayer);
	m_playing = TRUE;

	return 0L;
}

LRESULT CechoDlg::OnStopPlay(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	char data[128];
	uint32_t uid = (uint32_t)wparam;
	sprintf(data, "stop play, player = %u\r\n", uid);

	m_strInfo += data;
	UpdateData(FALSE);
	
	if (m_playing){
		m_frame->stop_player();
		if (m_viPlayer != NULL){
			m_viPlayer->close();
			delete m_viPlayer;
			m_viPlayer = NULL;
		}
		m_playing = FALSE;
	}

	return 0L;
}

LRESULT CechoDlg::OnChangeBitrate(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	//进行发送端带宽调整
	if (m_viRecorder != NULL){
		uint32_t bitrate = (uint32_t)wparam;
		bitrate = max(bitrate, MIN_VIDEO_BITARE / 1000);
		bitrate = min(bitrate, MAX_VIDEO_BITRAE / 1000);
		m_viRecorder->on_change_bitrate(bitrate);
	}
	return 0L;
}

LRESULT CechoDlg::OnNetInterrupt(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	m_strInfo += _T("net interrupt\r\n");
	UpdateData(FALSE);

	if (m_viRecorder != NULL)
		m_viRecorder->disable_encode();

	return 0L;
}

LRESULT CechoDlg::OnNetRecover(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);

	m_strInfo += _T("net recover\r\n");
	UpdateData(FALSE);

	if (m_viRecorder != NULL)
		m_viRecorder->enable_encode();

	return 0L;
}

LRESULT CechoDlg::OnStateInfo(WPARAM wparam, LPARAM lparam)
{
	UpdateData(TRUE);
	char* info = (char*)wparam;

	if (info == NULL)
		return 0L;


	m_strState = info;
	UpdateData(FALSE);

	free(info);

	return 0L;
}
