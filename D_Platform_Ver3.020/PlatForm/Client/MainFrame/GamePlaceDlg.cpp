#include "Stdafx.h"
#include "GamePlace.h"
#include "GamePlaceDlg.h"
#include "BuildTime.h"
#include "IEAdvertise.h"
#include "ItemDlg.h"
#include "MyDiskInfo.h"


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDFEEDBACK, OnFeedBack)
	ON_BN_CLICKED(IDSERVER, OnServer)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CGamePlaceDlg, CDialog)
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_SYSCOMMAND()
	ON_WM_SETFOCUS()
	ON_WM_MOVE()
	ON_MESSAGE(IDM_HIDE_LEFT,SetLeftHideOrShow)
	ON_MESSAGE(IDM_HIDE_RIGHT,SetRightHideOrShow)
	ON_MESSAGE(WM_HOTKEY,OnHotKey)
	ON_MESSAGE(IDM_OPEN_BANK,OpenBank)
	ON_MESSAGE(IDM_OPEN_IE,OpenIE)
	ON_MESSAGE(IDM_CALL_GM, CallGM)

	ON_MESSAGE(IDM_MESSAGE_APPEAL,OpenAppeal)
	ON_MESSAGE(IDM_LOCK_ACCOUNT, OnLockAccount)	//锁定机器 zxj 2009-11-12
	ON_MESSAGE(QUIT_GAMEROOM,OnQuitGameRoom)
	ON_MESSAGE(WM_EXCHANGE_SKIN,OnExchangeSkin)
	ON_BN_CLICKED(IDC_PLAY,OnSndPlay)
	ON_BN_CLICKED(IDC_PAUSE,OnSndPause)
	ON_BN_CLICKED(IDC_VOLDOWN,OnSndVolDown)
	ON_BN_CLICKED(IDC_VOLUP,OnSndVolUp)
	ON_BN_CLICKED(IDC_SUPPORT,OnTechSupport)
	//	ON_WM_CTLCOLOR()
	ON_WM_NCPAINT()

    ON_WM_ACTIVATE()                // PengJiLin, 2011-6-23, 激活消息

	//wushuqun 2009.7.13
	//修改游戏房间中退出游戏时闪屏问题
	ON_WM_ERASEBKGND()

END_MESSAGE_MAP()

#define ID_TAB_WND					100					//属性页 ID
#define ID_TOP_WND					101					//顶部窗口
#define ID_USEINFO_WND				102					//个人信息窗口
#define ID_ACTIVE_WND				103					//活动窗口
#define ID_BOTTOM_WND				104					//底部窗口
#define ID_GAMELIST_WND				105					//游戏例表窗口
//////////////////////////////////////////////////////////////////////////
////////Kylin 20090107 添加功能 TAB 控件
//#define ID_FUNTAB_WND					106					
//////////////////////////////////////////////////////////////////////////

// PengJiLin, 2010-6-2, 是否是由第三方软件启动, TRUE = 是。(鸥朴)
BOOL g_bRunBySoftware = FALSE;
BOOL g_bAutoConnect = FALSE;        // 第一次自动登录，如果登录失败，则须手动登录
HWND g_hWndDLL = NULL;                              // DLL窗口句柄，用于通信

#define SOFTWARE_IS_OPETV   TEXT("opetv_zyl")       // 第三方为 鸥朴多媒体

#define STR_SUB_FIND_HANDLE     TEXT("handle=")     // 句柄 查找子串
#define STR_SUB_FIND_USER       TEXT("user=")       // 用户 查找子串
#define STR_SUB_FIND_PWD        TEXT("pwd=")        // MD5 密码 查找子串
#define STR_SUB_FIND_END        TEXT(",")           // 结束符

TCHAR g_chUserName[100] = {0};      // 用户名
TCHAR g_chMD5PSW[100] = {0};        // MD5 密码

// PengJiLin, 2010-6-2, 检测是否是第三方启动。(鸥朴)
void CheckIfRunBySoftware(CGamePlaceDlg* pGamePlaceDlg)
{
    // TEXT("%s opetv_zyl:handle=%d, user=%s, pwd=%s,")
    // PengJiLin, 2010-6-1, 鸥朴登录 ------------------------------------------------
    LPCTSTR lpszCmdLine=AfxGetApp()->m_lpCmdLine;

    memset(g_chUserName, 0, sizeof(g_chUserName));
    memset(g_chMD5PSW, 0, sizeof(g_chMD5PSW));

    TCHAR chData[1024] = {0};
    strcpy(chData, lpszCmdLine);
    TCHAR* ptr = chData;
    ptr = strstr(ptr, SOFTWARE_IS_OPETV);           // 鸥朴多媒体
    if(NULL != ptr)
    {
        TCHAR* ptrValue = NULL;
        ptr = strstr(ptr, STR_SUB_FIND_HANDLE);     // 句柄
        if(NULL != ptr)
        {
            ptrValue = ptr + sizeof(STR_SUB_FIND_HANDLE) - 1;
            ptr = strstr(ptr, STR_SUB_FIND_END);
            if(NULL != ptr)
            {
                *ptr = TEXT('\0');
                ++ptr;
                g_hWndDLL = (HWND)atoi(ptrValue);   // 获得 DLL 窗口句柄
                g_bRunBySoftware = TRUE;            // 由第三方启动
                g_bAutoConnect = TRUE;
            }
        }

        // 避免嵌套太多，采用此种写法
        ptr = strstr(ptr, STR_SUB_FIND_USER);       // 用户
        if(NULL != ptr)
        {
            ptrValue = ptr + sizeof(STR_SUB_FIND_USER) - 1;
            ptr = strstr(ptr, STR_SUB_FIND_END);
            if(NULL != ptr)
            {
                *ptr = TEXT('\0');
                ++ptr;
                lstrcpy(g_chUserName, ptrValue);    // 获得 用户名
            }
        }

        ptr = strstr(ptr, STR_SUB_FIND_PWD);        // MD5 密码
        if(NULL != ptr)
        {
            ptrValue = ptr + sizeof(STR_SUB_FIND_PWD) - 1;
            ptr = strstr(ptr, STR_SUB_FIND_END);
            if(NULL != ptr)
            {
                *ptr = TEXT('\0');
                ++ptr;
                lstrcpy(g_chMD5PSW, ptrValue);      // 获得 MD5 密码
            }
        }
    }
}

// add xqm 2011-3-22
//设置硬件加速
void CGamePlaceDlg::SetAcceleration(int iLevel)
{	   
    DISPLAY_DEVICE  dv;   
    int i = 0;

	ZeroMemory(&dv,sizeof(DISPLAY_DEVICE));
	dv.cb=sizeof(DISPLAY_DEVICE);
	EnumDisplayDevices(0,0,&dv,0); 		   
	while(dv.DeviceKey[i])
	{
	  dv.DeviceKey[i++]=toupper(dv.DeviceKey[i]);
	}
  
	CString strScr, strDeviceKeyName, strSub;
	strScr = dv.DeviceKey;	
	int iPos = 0, iLen = 0;
	CString sz;	
	sz = "\\SYSTEM";
	iPos = strScr.Find(sz, 0);
	iLen = strScr.GetLength();		
	iPos = iLen - iPos - 1;
	strSub = strScr.Right(iPos);		
	
	strDeviceKeyName = "Acceleration.Level";		
	HKEY hKey;   
	//打开成功则执行	
	if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSub, NULL, KEY_ALL_ACCESS, &hKey)) 
	{
		DWORD dValue = 4, dVal = 4, dValData = 0; 
		RegQueryValueEx(hKey, strDeviceKeyName, 0, &dVal,(LPBYTE)&dValData, (LPDWORD)&dValue);		
		//保存原来的硬件加速级别
		m_iLevel = dValData;	
		if (m_iLevel <= 0)
			m_iLevel = 0;
		if (m_iLevel >= 5)
			m_iLevel = 5;
	
		RegSetValueEx(hKey, strDeviceKeyName, 0, REG_DWORD, (BYTE*)&iLevel, sizeof(iLevel));
		ChangeDisplaySettings(0, 0x40);  //0x40 查MSDN没有找到什么意思，这里直接在OD中照搬。
		RegCloseKey(hKey);		
	}	   
}

// PengJiLin, 2011-7-21, 获取 CPU、硬盘 ID
int GetCPUHardInfo(CString& strCPUID, CString& strHardID);
DWORD GetHarddiskNum();
DWORD GetCPUID();
CString g_strCPUID = "";
CString g_strHardID = "";

//初始化函数
BOOL CGamePlaceDlg::OnInitDialog()
{
	__super::OnInitDialog();
	//__super::OnInitDialog();

    // PengJiLin, 2010-6-2, 检测是否是第三方启动。(鸥朴)
    CheckIfRunBySoftware(this);

    // PengJiLin, 2011-7-21, 获取CPU、硬盘 ID
   // GetCPUHardInfo(g_strCPUID, g_strHardID);
	DWORD cpuid = GetCPUID();
	DWORD hardid = GetHarddiskNum();

	g_strCPUID.Format("%d", cpuid);
	g_strHardID.Format("%d", hardid);


	CMyDiskInfo diskInfo;
	diskInfo.GetDiskInfo();
	g_strHardID = CString(diskInfo.szSerialNumber);

	// xqm 2010-11-29		有客户需要一个不改硬件加速的版本
	//关闭硬件加速功能
	int iLevel = 3;
#ifdef HWACC
	SetAcceleration(iLevel);
#endif

	////硬件加速，修改注册表
	//char originalLevel[10] = {0};  
 //   char regDevicePath[1024] = {0};  
 //   DISPLAY_DEVICE  dv;   
 //   int i = 0;

	//ZeroMemory(&dv,sizeof(DISPLAY_DEVICE));
	//dv.cb=sizeof(DISPLAY_DEVICE);
	//EnumDisplayDevices(0,0,&dv,0); 	
	//i = 0;    
	//while(dv.DeviceKey[i])
	//{
	//  dv.DeviceKey[i++]=toupper(dv.DeviceKey[i]);
	//}
 // 
	//CString strScr, strDeviceKeyName, strSub;
	//strScr = dv.DeviceKey;	
	//int iPos = 0, iLen = 0;
	//CString sz;	
	//sz = "\\SYSTEM";
	//iPos = strScr.Find(sz, 0);
	//iLen = strScr.GetLength();		
	//iPos = iLen - iPos - 1;
	//strSub = strScr.Right(iPos);
	//
	//strDeviceKeyName = "Acceleration.Level";	
	//int level = 0;	
	//
	//HKEY hKey;   
	////打开成功则执行	
	//if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSub, NULL, KEY_ALL_ACCESS, &hKey)) 
	//{
	//	DWORD dValue = 9;
	//	RegQueryValueEx(hKey, strDeviceKeyName, 0, 0,(LPBYTE)originalLevel, (LPDWORD)&dValue);
	//	RegSetValueEx(hKey, strDeviceKeyName, 0, REG_DWORD, (BYTE*)&level, sizeof(level));
	//	ChangeDisplaySettings(0, 0x40);  //0x40 查MSDN没有找到什么意思，这里直接在OD中照搬。
	//	RegCloseKey(hKey);			
	//}

    
	CBcfFile forder( CBcfFile::GetAppPath() + "Order.bcf");
	Glb().m_nPowerOfGold = forder.GetKeyVal("VirtualMoney", "PowerOfGold", 0);


	//建立系统热键
	//m_bRegisterHot=RegisterHotKey(GetSafeHwnd(),100,0,VK_F9);
	SetWindowLong(GetSafeHwnd(),GWL_USERDATA,300L);

	//加载大厅组件的渲染方式
	CBcfFile fRander( CBcfFile::GetAppPath() + m_skinmgr.GetSkinBcfFileName());
	m_strTopWndRander = fRander.GetKeyVal(_T("TopWnd"),_T("_Rander"),"AFC");

	LoadSkin();

	//建立顶部窗口
	m_topWnd.CreateWnd(this,ID_TOP_WND);
	if (m_strTopWndRander == "DUI")
	{
		m_topWnd.ShowWindow(SW_HIDE);
		m_topDuWnd.Create(m_hWnd,NULL,WS_CLIPCHILDREN,0,0,0,0,0,NULL);
		m_topDuWnd.m_pOldTopWndClass = &m_topWnd;
		m_topDuWnd.m_pGameListWnd = &m_GameListWnd;
		m_topDuWnd.ShowWindow(SW_HIDE);
	}

CBcfFile fMsg(CBcfFile::GetAppPath()+"ClientMessage.bcf");
	//建立活动通知窗口
	m_ActInformWnd.CreateWnd(this,ID_ACTIVE_WND);
	//建立底部窗口
	//	m_BottomWnd.CreateWnd(this,ID_BOTTOM_WND);
	//游戏例表窗口
	m_GameListWnd.CreateWnd(this,ID_GAMELIST_WND);
	//建立游戏列表和房间列表
	m_RoomTabWnd.CreateTabWnd(this,ID_TAB_WND);
	//m_RoomTabWnd.ShowCopyRight(true);
	//////////////////////////////////////////////////////////////////////////
	////////Kylin 20090107 添加功能 TAB 控件
	//m_FunListTabWnd.CreateTabWnd(this,ID_FUNTAB_WND);
	//////////////////////////////////////////////////////////////////////////


	//游戏房间
	if ((m_pMainRoom=new CMainRoomEx(&m_GameListWnd,&m_ActInformWnd,&m_topWnd,&m_topDuWnd))==NULL) return FALSE;
	m_pMainRoom->Create(IDD_GAME_PLACE,&m_RoomTabWnd);//有个控件没注册
	//////////////////////////////////////////////////////////////////////////
	////////Kylin 20090107 添加功能 TAB 控件
	//m_pMainRoom->Create(IDD_GAME_PLACE,&m_FunListTabWnd);//有个控件没注册
	//m_FunListTabWnd.AddTabPage(m_pMainRoom,NULL,"游戏列表");//建立第一个选项卡		
	//m_FunListTabWnd.AddTabPage(m_pMainRoom,NULL,"充值");//建立第一个选项卡		
	//////////////////////////////////////////////////////////////////////////

	

	if (Glb().m_TabName=="")
	{
		CString str = fMsg.GetKeyVal("MainRoom","Hall","游戏大厅");
		m_RoomTabWnd.AddTabPage(m_pMainRoom,NULL,str.GetBuffer());//建立第一个选项卡		
	}
	else
	{
		char m_szTitle[32];
		memset(m_szTitle, 0,32);
		_tcscpy_s(m_szTitle, 31, Glb().m_TabName);
		m_RoomTabWnd.AddTabPage(m_pMainRoom,NULL,m_szTitle,LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)));//建立第一个选项卡
	}


	//设置图标	
	HICON hIcon=AfxGetApp()->LoadIcon(IDR_MAINFRAME);//系统图标
	SetIcon(hIcon,TRUE);
	SetWindowText(Glb().m_title);// SetWindowTitle(TEXT(Glb().m_title));


	//闪屏第1：只留下面一行，上面2行注释
	ModifyStyle( WS_CAPTION, WS_MINIMIZEBOX|WS_MAXIMIZEBOX, SWP_DRAWFRAME );

	CRect r;
	//CWnd * pDeskRect =GetDesktopWindow();
	//pDeskRect->GetClientRect(&r);
	/*AfxGetApp()->m_pMainWnd->GetWindowRect(&r);
	ScreenToClient(&r);*/
	GetWindowRect(&r);

	m_SavedScreen.x=1000;
	m_SavedScreen.y=700;
	/*m_SavedScreen.x=r.Width();
	m_SavedScreen.y=r.Height();*/

	m_bMax=true;
	m_bShowMax=true;
	CRect rect;
	::SystemParametersInfo(SPI_GETWORKAREA,NULL,&rect,NULL);
	MoveWindow(0,0,rect.Width(),rect.Height());

	m_pMainRoom->PostMessage(WM_COMMAND,IDM_CONNECT_CENTER_SERVER,0);

	//////////////////////////////////////////////////////////////////////////
	//以下是背景音乐控制

	BZSoundInitial();
	m_bgHallSoundID=BZSoundPlay(this,"music/hallbgsound.mp3",0,0);
	BZSoundSetVolume(m_bgHallSoundID,m_sndVolHall);
	m_bgRoomSoundID=BZSoundPlay(this,"music/roombgsound.mp3",0,0);
	BZSoundSetVolume(m_bgRoomSoundID,m_sndVolRoom);
	BZSoundPause(m_bgRoomSoundID);
	m_bgCurrentID=m_bgHallSoundID;
	m_btSndPlay.ShowWindow(SW_HIDE);
	if(m_bSndPause)
	{
		BZSoundPause(m_bgCurrentID);
		m_btSndPlay.ShowWindow(SW_SHOW);
		m_btSndPause.ShowWindow(SW_HIDE);
	}
	else
	{
		m_btSndPlay.ShowWindow(SW_HIDE);
		m_btSndPause.ShowWindow(SW_SHOW);
	}

	m_toolTip.Create( this,TTS_ALWAYSTIP ) ;
	
	CString strMessage;
	strMessage = fMsg.GetKeyVal("Sound","Play","播放音乐");
	m_toolTip.AddTool(GetDlgItem(IDC_PLAY),strMessage); 
	strMessage = fMsg.GetKeyVal("Sound","Stop","停止播放音乐");
	m_toolTip.AddTool(GetDlgItem(IDC_PAUSE),strMessage); 
	strMessage = fMsg.GetKeyVal("Sound","TurnDwon","调小声音");
	m_toolTip.AddTool(GetDlgItem(IDC_VOLDOWN),strMessage); 
	strMessage = fMsg.GetKeyVal("Sound","TurnUp","调大声音");
	m_toolTip.AddTool(GetDlgItem(IDC_VOLUP),strMessage);

	//m_toolTip.AddTool(GetDlgItem(IDC_BTN_MAIN1),TEXT("打开首页"));
	/*m_ToolTip.AddTool(GetDlgItem(IDC_BTN_ROOM2),TEXT("银行功能，只能在房间中才能使用"));
	m_ToolTip.AddTool(GetDlgItem(IDC_BTN_ROOM3),TEXT("呼叫网管，只能在房间中才能使用"));
	m_ToolTip.AddTool(GetDlgItem(IDC_BTN_ROOM4),TEXT("上传照片"));
	m_ToolTip.AddTool(GetDlgItem(IDC_BTN_ROOM5),TEXT("好友功能"));
	m_ToolTip.AddTool(GetDlgItem(IDC_BTN_ROOM6),TEXT("打开充值网页"));
	m_ToolTip.AddTool(GetDlgItem(IDC_BTN_ROOM7),TEXT("打开划帐网页"));*/
	//m_toolTip.Activate(true);
	//m_toolTip.SetDelayTime(TTDT_INITIAL,0);

	//m_btSndPlay.SetToolTipText("播放音乐");
	//m_btSndPause.SetToolTipText("暂停音乐");
	//m_btVolDown.SetToolTipText("减小音量");
	//m_btVolUp.SetToolTipText("增大音量");


	//m_bSndPause=false;
	//m_sndBtnCx=m_sndBtnCy=16;
	//////////////////////////////////////////////////////////////////////////

	ShowWindow(SW_HIDE);

	return FALSE;
}

/*******************************************************************************************************/

//构造函数
CAboutDlg::CAboutDlg() : CDialog(IDD_ABOUTBOX)
{
}

BOOL CAboutDlg::OnInitDialog()
{
	__super::OnInitDialog();

	Init();
	return TRUE;
}

void CAboutDlg::Init()
{
	m_bk.SetLoadInfo(AfxGetInstanceHandle(),IDB_BITMAP9,TRUE);
	MoveWindow(0,0,m_bk.GetWidth(),m_bk.GetHeight(),0);
	CGameImageHelper help(&m_bk);
	HRGN hRgn;
	hRgn = AFCBmpToRgn(help,m_bk.GetPixel(0,0),RGB(1,1,1));
	if(hRgn)
		SetWindowRgn(hRgn,TRUE);

	CString s=CBcfFile::GetAppPath ();/////本地路径
	CString strSkin = m_skinmgr.GetSkinBcfFileName();
	CBcfFile f( s + strSkin);
	TCHAR path[MAX_PATH];
	CString skinfolder;
	skinfolder=f.GetKeyVal(m_skinmgr.GetKeyVal(strSkin),"skinfolder",m_skinmgr.GetSkinPath());
	
	CGameImage					m_bt;
	wsprintf(path,"%slogon\\log_exit_bt.bmp",skinfolder);
	m_bt.SetLoadInfo(path,CGameImageLink::m_bAutoLock);
	m_Bt1.LoadButtonBitmap(path,false);
	HRGN hRgn2;
	hRgn2=AFCBmpToRgn(m_bt,RGB(255,0,255),RGB(1,1,1));
	if(hRgn2)
		m_Bt1.SetWindowRgn(hRgn2,true);
	m_Bt1.MoveWindow(m_bk.GetWidth()-m_bt.GetWidth()/4-5,0,m_bt.GetWidth()/4,m_bk.GetHeight(),0);

}
//DDX/DDV 支持
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK,m_Bt1);	
}

void CAboutDlg::OnPaint()
{
	CPaintDC dc(this); 
	CGameImageHelper	help(&m_bk);
	help.BitBlt(dc.GetSafeHdc(),0,0);
 
	CFont Font, *OldFont;
	Font.CreateFont(12,6,0,0,0,0,0,0,134,3,2,1,2,TEXT("宋体"));// (12,9,0,0,0,0,0,0,134,3,2,1,2,TEXT("宋体"));
	OldFont=dc.SelectObject(&Font);


	dc.SetBkMode(TRANSPARENT);   
	dc.TextOut(115,130, "面对面棋牌2.0正式版（2.0.1.23）");
	dc.TextOut(145,160, "正易龙公司 版权所有");
	dc.TextOut(95,190, "Copyright(C)2008-2011 kfgame.com, Inc.");
	dc.TextOut(145,220, "All rights reserved.");
	Font.DeleteObject();
}

void CAboutDlg::OnFeedBack()
{
	ShellExecute(NULL,   TEXT("open"),   TEXT("http://bbs.kfgame.com/showforum-10.aspx"),   NULL,   NULL,   SW_SHOWNORMAL);
}

void CAboutDlg::OnServer()
{
	//ShellExecute(NULL,   TEXT("open"),   TEXT("http://www.google.com.hk"),   NULL,   NULL,   SW_SHOWNORMAL);
	CItemDlg itemDlg;
	itemDlg.DoModal();
	itemDlg.CenterWindow();
}

/*******************************************************************************************************/

//构造函数
CGamePlaceDlg::CGamePlaceDlg() : CDialog(IDD_MAIN_DIALOG)
{
	m_pMainRoom=NULL;
	//m_bRegisterHot=FALSE;
	m_bgHallSoundID=0;
	m_bgRoomSoundID=0;
	m_bgCurrentID=0;
	//	hIcon=AfxGetApp()->LoadIcon(IDI_CHECKON); 

	//add xqm 2011-3-22
	m_iLevel = 0; //	保存玩家原来的硬件加速设置
	m_Rgn.CreateRectRgn(0,0,0,0);
}

CGamePlaceDlg::~CGamePlaceDlg()
{
	m_Rgn.DeleteObject();
}

//DDX/DDV 支持
void CGamePlaceDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PLAY,m_btSndPlay);
	DDX_Control(pDX, IDC_PAUSE,m_btSndPause);
	DDX_Control(pDX, IDC_VOLDOWN,m_btVolDown);
	DDX_Control(pDX, IDC_VOLUP,m_btVolUp);
#ifndef DEV_NOFREE
	//DDX_Control(pDX, IDC_SUPPORT,m_btTechSupport);
#else
	if (GetDlgItem(IDC_SUPPORT) != NULL)
		GetDlgItem(IDC_SUPPORT)->ShowWindow(SW_HIDE);//隐藏按钮
#endif
}


//系统菜单
void CGamePlaceDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID==IDM_ABOUTBOX)
	{
		//CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else 
		__super::OnSysCommand(nID, lParam);
}
void CGamePlaceDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	if(pRect->right-pRect->left<1024)
		pRect->right=pRect->left+1024;
	if(pRect->bottom-pRect->top<768)
		pRect->bottom=pRect->top+768;
	__super::OnSizing(fwSide, pRect);
}

//移动窗口
bool CGamePlaceDlg::SafeMoveWindow(UINT uID, int x, int y, int iWidth, int iHeight)
{
	CWnd * pControlWnd=GetDlgItem(uID);
	if ((pControlWnd!=NULL)&&(pControlWnd->GetSafeHwnd())) 
	{
		if ((iWidth<=0)||(iHeight<=0)) iWidth=0,iHeight=0;
		pControlWnd->MoveWindow(x,y,iWidth,iHeight);
		return true;
	}
	return false;
}

//移动窗口
bool CGamePlaceDlg::SafeMoveWindow(CWnd * pControlWnd, int x, int y, int iWidth, int iHeight)
{
	if ((pControlWnd!=NULL)&&(pControlWnd->GetSafeHwnd())) 
	{
		if ((iWidth<=0)||(iHeight<=0)) iWidth=0,iHeight=0;
		pControlWnd->MoveWindow(x,y,iWidth,iHeight);
		return true;
	}
	return false;
}

//位置变化 
void CGamePlaceDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if(!m_topWnd.m_hWnd)
		return;
	CRect rect;
	CRgn rgn;
	GetClientRect(&rect);
	m_Rgn.SetRectRgn(&rect);
	int iLeftW,iLeftH,iTopW,iTopH;
	//顶部
	SafeMoveWindow(&m_topWnd,0,0,cx,86);//顶部高度73
	if (m_topDuWnd.GetHWND())
	{
		::MoveWindow(m_topDuWnd.GetHWND(),0,0,cx,86,true);
	}

	rgn.CreateRectRgn(0,0,cx,86);
	rgn.CombineRgn(&rgn, &m_Rgn, RGN_AND);
	m_Rgn.CombineRgn(&m_Rgn, &rgn, RGN_XOR);
	//m_topWnd.AutoSize(0,0);
	//例表宽度
	m_topWnd.GetClientRect(&rect);
	iTopW = rect.Width();
	iTopH = rect.Height();
	//游戏例表
	SafeMoveWindow(&m_GameListWnd,m_lc.GetWidth(),iTopH,LEFT_WND_WIDTH,cy-iTopH-m_bc.GetHeight());


	m_GameListWnd.GetClientRect(&rect);
	iLeftW = rect.Width();
	iLeftH = rect.Height();

	//属性页
	SafeMoveWindow(&m_RoomTabWnd,
		m_lc.GetWidth()+iLeftW,
		iTopH,
		cx-iLeftW-m_rc.GetWidth()-m_lc.GetWidth(),
		cy-iTopH-m_bc.GetHeight());

	//活动广告

	#ifdef DEV_NOFREE
		SafeMoveWindow(&m_ActInformWnd, m_lc.GetWidth() + 10, cy-m_bc.GetHeight(), cx-m_lc.GetWidth() - 300, 30);
	#else
		SafeMoveWindow(&m_ActInformWnd, m_lc.GetWidth() + 10, cy-m_bc.GetHeight(), cx-m_lc.GetWidth() - 20, 30);
	#endif // DEV_NOFREE
	
	rgn.SetRectRgn(m_lc.GetWidth()+iLeftW, cy-m_bc.GetHeight()-30, cx-m_rc.GetWidth(), cy-m_bc.GetHeight());
	m_Rgn.CombineRgn(&m_Rgn, &rgn, RGN_XOR);

	SafeMoveWindow(&m_btSndPlay,sndPlayX>0?sndPlayX:cx+sndPlayX,sndPlayY>0?sndPlayY:cy+sndPlayY,m_sndBtnCx,m_sndBtnCy);
	SafeMoveWindow(&m_btSndPause,sndPauseX>0?sndPauseX:cx+sndPauseX,sndPauseY>0?sndPauseY:cy+sndPauseY,m_sndBtnCx,m_sndBtnCy);
	SafeMoveWindow(&m_btVolDown,sndVolDownX>0?sndVolDownX:cx+sndVolDownX,sndVolDownY>0?sndVolDownY:cy+sndVolDownY,m_sndBtnCx,m_sndBtnCy);
	SafeMoveWindow(&m_btVolUp,sndVolUpX>0?sndVolUpX:cx+sndVolUpX,sndVolUpY>0?sndVolUpY:cy+sndVolUpY,m_sndBtnCx,m_sndBtnCy);

#ifndef DEV_NOFREE
	//SafeMoveWindow(&m_btTechSupport,cx-305,cy-20,302,18);
#endif

	int x1 = m_lc.GetWidth();
	int y1 = iTopH;
	int x2 = cx - m_rc.GetWidth();
	int y2 = cy-m_bc.GetHeight();
	rgn.SetRectRgn(x1, y1, x2, y2);
	rgn.CombineRgn(&rgn, &m_Rgn, RGN_AND);
	m_Rgn.CombineRgn(&m_Rgn, &rgn, RGN_XOR);
	rgn.SetRectRgn(0, cy-m_bc.GetHeight(), cx, cy);
	m_Rgn.CombineRgn(&m_Rgn, &rgn, RGN_OR);
	rgn.DeleteObject();

// 	POINT m_arPoints[6];
// 	CRect winRect;
// 	GetWindowRect(&winRect);
// 	m_arPoints[0].x = winRect.left;
// 	m_arPoints[0].y = winRect.top;
// 	m_arPoints[1].x = winRect.right;
// 	m_arPoints[1].y = winRect.top;
// 	m_arPoints[2].x = winRect.right;
// 	m_arPoints[2].y = winRect.bottom-5;
// 	m_arPoints[3].x = winRect.right-5;
// 	m_arPoints[3].y = winRect.bottom;
// 	m_arPoints[4].x = winRect.left+5;
// 	m_arPoints[4].y = winRect.bottom;
// 	m_arPoints[5].x = winRect.left;
// 	m_arPoints[5].y = winRect.bottom-5;
// 
// 	HRGN hRgn = CreatePolygonRgn(m_arPoints, 6, WINDING);
// 	if (hRgn!=NULL)
// 	{
// 		SetWindowRgn(hRgn,TRUE);
// 		DeleteObject(hRgn);
// 	}
}

//得到焦点
void CGamePlaceDlg::OnSetFocus(CWnd * pOldWnd)
{
	__super::OnSetFocus(pOldWnd);
	if (m_RoomTabWnd.GetSafeHwnd()) 
		m_RoomTabWnd.SetFocus();
}

// PengJiLin, 2011-5-30, 激活
void CGamePlaceDlg::OnActivate(UINT uState, CWnd* pOldWnd, BOOL bMinisted)
{
    if(m_pMainRoom->GetSafeHwnd() && WA_INACTIVE != uState)
    {
        m_pMainRoom->SetActiveToMainRoom(uState);
    }
}

/// 
/// 定时器响应消息
/// @param[in]		UINT_PTR nIDEvent		定时器ID
void CGamePlaceDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_GP_SHOW_WINDOW)
	{
		KillTimer(IDT_GP_SHOW_WINDOW);
		ShowWindow(SW_SHOW);
	}
}

//取消函数
void CGamePlaceDlg::OnCancel()
{
	if (m_pMainRoom->CloseAllGameRoom()==false) return;

	
	CString s=CBcfFile::GetAppPath ();/////本地路径
	CBcfFile f( s + "bzgame.bcf");
	bool isUseIEAd = f.GetKeyVal("IEAdvertise", "UseIEAd", 0);

	// xqm 2011-3-22		有客户需要一个不改硬件加速的版本
	//关闭硬件加速功能	
#ifdef HWACC
	SetAcceleration(m_iLevel);
#endif
	
	//判断退出大厅是否弹出IE广告 add by lxl 2010-11-24
	if(isUseIEAd)
	{
		AfxGetApp()->m_pMainWnd->ShowWindow(SW_HIDE);
		
		m_topWnd.m_toplogo.DestroyWindow();
		if (m_strTopWndRander == "DUI")
		{
			m_topDuWnd.ShowWindow(SW_HIDE);
		}
		ShowWindow(SW_HIDE);
		CIEAdvertise dlgIEAd;
		dlgIEAd.DoModal();
	}
	exit(0);
	__super::OnCancel();
}

///< 退出房间GameRoom时弹出对话框
///< wParam 退出房间类型
///< return LRESULT
LRESULT CGamePlaceDlg::OnQuitGameRoom(WPARAM wParam, LPARAM lParam)
{
	CString str;
	CBcfFile fMsg(CBcfFile::GetAppPath()+"ClientMessage.bcf");
	switch (wParam)
	{
	case 1:
		str.Format(fMsg.GetKeyVal("GameRoom","VIPRoomSetPWLeft","由于玩家 ID%ld 修改了密码，你被踢出房间，请谅解。可以联系该玩家索要密码。"),(long)lParam);
		break;

	case 2:
		/*TCHAR szNum[32];
		GetNumString(szNum, (UINT)lParam, Glb().m_nPowerOfGold,Glb().m_bUseSpace, Glb().m_strSpaceChar);
		str.Format(TEXT("此游戏室最少需要有%s的金币,您的金币不够!"),szNum);*/
		return 0;
		break;
	
	case 3:
		str = fMsg.GetKeyVal("GameRoom","ConnectionDown","对不起，您与服务器之间的连接中断了！");
		break;

	case 4:
		str = fMsg.GetKeyVal("GameRoom","KickOutCallAdmin","您被踢出游戏房间,如有疑问,请与管理员联系!");
		break;
    case 5:     // PengJiLin, 2010-8-26, 房主踢玩家离开房间
        str = fMsg.GetKeyVal("GameRoom","OwnerTOneLeftRoomResultMsg","对不起，您已经被房主踢出游戏房间!");
        break;
	case 6:
		{
			CString szOrgStr = fMsg.GetKeyVal("GameRoom","KickOutTimeOut","您在%ld分钟内没有进行游戏操作，被系统请出房间！");
			str.Format(szOrgStr, (long)lParam);
		}
		break;

	default :
		return 1;
	}
	
	DUIMessageBox(m_hWnd,MB_ICONINFORMATION|MB_OK,"系统提示",false,str.GetBuffer());

    // PengJiLin, 2011-7-27, 与服务器断开连接后回到登陆界面
    //if(3 == wParam)
    //{
   //     GetMainRoom()->PostMessage(WM_COMMAND,MAKELONG(IDC_BTN_MAIN5,BN_CLICKED));
   // }

	return 0;
}

BOOL CGamePlaceDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message==ID_REST_MAX)
	{
		m_bMax=false;
		m_bShowMax=false;
 		CRect rect;
 		::SystemParametersInfo(SPI_GETWORKAREA,NULL,&rect,NULL);
// 		MoveWindow(0,0,1024,768);
// 		CenterWindow();

		ShowWindow(SW_RESTORE);

		int iSrcW = GetSystemMetrics(SM_CXSCREEN);
		int iSrcH = GetSystemMetrics(SM_CYSCREEN);
		int iWndW = 1024;
		int iWndH = 768;
		::MoveWindow(m_hWnd,(iSrcW - iWndW)/2,(iSrcH - iWndH)/2,iWndW,iWndH,true);

		
		m_topWnd.m_BtRestore.ShowWindow(SW_HIDE);
		m_topWnd.m_BtMax.ShowWindow(SW_SHOW);

		if (m_topDuWnd.GetHWND())
		{
			::GetWindowRect(m_hWnd,&rect);
			::MoveWindow(m_topDuWnd.GetHWND(),rect.left,rect.top,rect.right - rect.left,87,true);
			m_topDuWnd.FixCtrlBtnPosition(ID_REST_MAX,m_bMax);
		}

		return TRUE;
	}
	if(pMsg->message==WM_SYSCOMMAND)
	{
		if(pMsg->wParam==SC_CLOSE)
		{
			if (m_pMainRoom->CloseAllGameRoom()==false) 
				return TRUE;
		}
		if(pMsg->wParam==SC_MAXIMIZE)
		{
			m_bMax=true;
			m_bShowMax=true;
			CRect rect;
			::SystemParametersInfo(SPI_GETWORKAREA,NULL,&rect,NULL);
			MoveWindow(0,0,rect.Width(),rect.Height());
			m_topWnd.m_BtRestore.ShowWindow(SW_SHOW);
			m_topWnd.m_BtMax.ShowWindow(SW_HIDE);

			if (m_topDuWnd.GetHWND())
			{
				::GetWindowRect(m_hWnd,&rect);
				::MoveWindow(m_topDuWnd.GetHWND(),rect.left,rect.top,rect.right - rect.left,87,true);
				m_topDuWnd.FixCtrlBtnPosition(SC_MAXIMIZE,m_bMax);
			}

			return TRUE;
		}
		if(pMsg->wParam==SC_RESTORE)
		{
			//m_bMax=false;
			m_bShowMax=false;
			//ShowWindow(SW_RESTORE);
			//MoveWindow(0,0,m_SavedScreen.x,m_SavedScreen.y);
			//CRect rect;
			//::SystemParametersInfo(SPI_GETWORKAREA,NULL,&rect,NULL);
			//MoveWindow(0,0,rect.Width(),rect.Height());
			//CenterWindow();
			ShowWindow(SW_RESTORE);
			m_topWnd.m_BtRestore.ShowWindow(SW_HIDE);
			m_topWnd.m_BtMax.ShowWindow(SW_SHOW);

			if (m_topDuWnd.GetHWND())
			{
				CRect rect;
				::GetWindowRect(m_hWnd,&rect);
				::MoveWindow(m_topDuWnd.GetHWND(),rect.left,rect.top,rect.right - rect.left,87,true);
				m_topDuWnd.FixCtrlBtnPosition(SC_RESTORE,m_bMax);
			}

			return TRUE;
		}
	}
	//if(pMsg->message==WM_NCLBUTTONDOWN && pMsg->wParam==HTCAPTION && m_bMax)
	//	return TRUE;

	if ((pMsg->message==WM_KEYDOWN)&&(pMsg->wParam==VK_ESCAPE))
	{
		return TRUE;
	}
	if ((pMsg->message==WM_KEYDOWN)&&(pMsg->wParam==VK_F1))
	{
		return TRUE;
	}
	if(pMsg->message==WM_USER+0x10000)
	{
		m_GameListWnd.ReloadLogo();
		m_GameListWnd.Invalidate(FALSE);
		return TRUE;
	}

	///// 排队机窗口显示时，不能切换房间
	//if (WM_LBUTTONDOWN == pMsg->message || WM_LBUTTONDBLCLK == pMsg->message)
	//{
	//	if (NULL != m_pMainRoom)
	//	{
	//		CGameRoomEx* pGameRoom = (CGameRoomEx*)m_pMainRoom->m_RoomInfo[0].pGameRoomWnd;
	//		if (NULL != pGameRoom && NULL != pGameRoom->m_pQueueInRoomDlg)
	//		{
	//			//OutputDebugString("排队机窗口指针创建了");
	//			if (NULL != pGameRoom->m_pQueueInRoomDlg->GetSafeHwnd() 
	//				&& pGameRoom->m_pQueueInRoomDlg->IsWindowVisible())
	//			{
	//				//OutputDebugString("排队机窗口显示了");
	//				return TRUE;
	//			}
	//		}
	//	}
	//}

	//wushuqun 2009.6.10
	//最大化后不能拖动
	if((WM_NCLBUTTONDOWN == pMsg->message)
	&& (HTCAPTION == pMsg->wParam)
	&& (m_hWnd == pMsg->hwnd) && m_bMax)  // 是自己窗口消息
	{
		return TRUE;
	}

	if(pMsg->message==WM_USER+100)
	{
		int uIndex=pMsg->wParam;
		static bool bInGame=false;
		if(uIndex==0)
		{
			m_bgCurrentID=m_bgHallSoundID;
			if(!bInGame)
			{
				BZSoundPause(m_bgRoomSoundID);
				if(!m_bSndPause)
					BZSoundContinue(m_bgHallSoundID);
			}
		}
		if(uIndex==1)
		{
			m_bgCurrentID=m_bgRoomSoundID;
			if(!bInGame)
			{
				BZSoundPause(m_bgHallSoundID);
				if(!m_bSndPause)
					BZSoundContinue(m_bgRoomSoundID);
			}
		}

		if(uIndex==2)
		{//进入了游戏
			bInGame=true;
			BZSoundPause(m_bgHallSoundID);
			BZSoundPause(m_bgRoomSoundID);
			m_btSndPause.EnableWindow(FALSE);
			m_btSndPlay.EnableWindow(FALSE);
		}
		if(uIndex==3)
		{//退出了游戏
			bInGame=false;
			if(!m_bSndPause)
				BZSoundContinue(m_bgCurrentID);
			m_btSndPause.EnableWindow(TRUE);
			m_btSndPlay.EnableWindow(TRUE);
		}
	}

	if(m_toolTip.m_hWnd!=NULL)
		m_toolTip.RelayEvent(pMsg);

	if (this->GetSafeHwnd()==NULL)
	{
		return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}

BOOL CGamePlaceDlg::ShowWindow(int nCmdShow)
{
	CRect rcWnd;
	GetWindowRect(&rcWnd);
	if (nCmdShow == SW_HIDE)
	{
		MoveWindow(-rcWnd.Width(),-rcWnd.Height(),rcWnd.Width(),rcWnd.Height(),true);
	}
	else
	{
		MoveWindow(0,0,rcWnd.Width(),rcWnd.Height(),true);
	}

// 	m_RoomTabWnd.ShowAllWindow(nCmdShow);
// 
// 	m_GameListWnd.ShowWindow(nCmdShow);
// 	m_ActInformWnd.ShowWindow(nCmdShow);
// 	if (m_pMainRoom)
// 	{
// 		m_pMainRoom->ShowWindow(nCmdShow);
// 	}

	if (m_strTopWndRander == "DUI")
	{
		m_topDuWnd.ShowWindow(nCmdShow);
	}
	else if (m_strTopWndRander == "AFC")
	{
		m_topWnd.ShowWindow(nCmdShow);
	}

	return __super::ShowWindow(nCmdShow);
}

//热建函数
LRESULT CGamePlaceDlg::OnHotKey(WPARAM wparam, LPARAM lparam)
{
	if (AfxGetMainWnd()->IsWindowVisible()) 
	{
		AfxGetMainWnd()->ShowWindow(SW_HIDE);
		m_RoomTabWnd.ShowAllWindow(SW_HIDE);
	}
	else 
	{
		AfxGetMainWnd()->ShowWindow(SW_SHOW);
		m_RoomTabWnd.ShowAllWindow(SW_SHOW);
	}

	return 0;
}


//换肤
void CGamePlaceDlg::LoadSkin(void)
{
	CString s=CBcfFile::GetAppPath ();/////本地路径
	CString strSkin = m_skinmgr.GetSkinBcfFileName();
	CBcfFile f( s + strSkin);
	CString key=TEXT("PlaceDlg");
	TCHAR path[MAX_PATH];
	CString skinfolder;
	skinfolder=f.GetKeyVal(m_skinmgr.GetKeyVal(strSkin),"skinfolder",m_skinmgr.GetSkinPath());
	//玩家例表方框
	wsprintf(path,"%splace\\place_lc.bmp",skinfolder);
	m_lc.SetLoadInfo(path,CGameImageLink::m_bAutoLock);
	wsprintf(path,"%splace\\place_rc.bmp",skinfolder);
	m_rc.SetLoadInfo(path,CGameImageLink::m_bAutoLock);
	wsprintf(path,"%splace\\place_bc.bmp",skinfolder);
	m_bc.SetLoadInfo(path,CGameImageLink::m_bAutoLock);
	wsprintf(path,"%splace\\place_tc.bmp",skinfolder);
	m_tc.SetLoadInfo(path,CGameImageLink::m_bAutoLock);

	//玩家例表四角
	wsprintf(path,"%splace\\place_lt.bmp",skinfolder);
	m_lt.SetLoadInfo(path,CGameImageLink::m_bAutoLock);
	wsprintf(path,"%splace\\place_rt.bmp",skinfolder);
	m_rt.SetLoadInfo(path,CGameImageLink::m_bAutoLock);
	wsprintf(path,"%splace\\place_lb.bmp",skinfolder);
	m_lb.SetLoadInfo(path,CGameImageLink::m_bAutoLock);
	wsprintf(path,"%splace\\place_rb.bmp",skinfolder);
	m_rb.SetLoadInfo(path,CGameImageLink::m_bAutoLock);
	wsprintf(path,"%splace\\place_rbext.bmp",skinfolder);
	m_rbext.SetLoadInfo(path,CGameImageLink::m_bAutoLock);

	wsprintf(path,"%splace\\SndPlay.bmp",skinfolder);
	m_btSndPlay.LoadButtonBitmap(path,false);

	CGameImage sndImg;
	sndImg.SetLoadInfo(path,true);
	m_sndBtnCx=sndImg.GetWidth()/4;
	m_sndBtnCy=sndImg.GetHeight();

	wsprintf(path,"%splace\\SndPause.bmp",skinfolder);
	m_btSndPause.LoadButtonBitmap(path,false);

	wsprintf(path,"%splace\\SndVolUp.bmp",skinfolder);
	m_btVolUp.LoadButtonBitmap(path,false);

	wsprintf(path,"%splace\\SndVolDown.bmp",skinfolder);
	m_btVolDown.LoadButtonBitmap(path,false);

	//wsprintf(path,"%splace\\TechSupport.bmp",skinfolder);
	//m_btTechSupport.LoadButtonBitmap(path,false);
#ifndef DEV_NOFREE
	//m_btTechSupport.LoadButtonBitmap(AfxGetInstanceHandle(),IDB_BITMAP8,false);// 技术支持 加载资源里的图片使其不可替换 add by lxl 2011-1-23

	//简单加密一下add by wys 20110304
	//char *strsz = "啡土构球婴坝牱齐刈峭寅显诀济构型疼定然"; 
	//CString strDest;
	//EnCode_Key(strDest,strsz,-3);
	//m_btTechSupport.SetWindowTextA(strDest);
	//m_btTechSupport.ShowWindow(SW_HIDE);

#endif

	sndPlayX=f.GetKeyVal(key,"SndPlayX",100);
	sndPlayY=f.GetKeyVal(key,"SndPlayY",-200);

	sndPauseX=f.GetKeyVal(key,"SndPauseX",100);
	sndPauseY=f.GetKeyVal(key,"SndPauseY",-200);

	sndVolDownX=f.GetKeyVal(key,"SndVolDownX",100);
	sndVolDownY=f.GetKeyVal(key,"SndVolDownY",-200);

	sndVolUpX=f.GetKeyVal(key,"SndVolUpX",100);
	sndVolUpY=f.GetKeyVal(key,"SndVolUpY",-200);

	techSupportX=f.GetKeyVal(key,"TechSupportX",100);
	techSupportY=f.GetKeyVal(key,"TechSupportY",-200);


	m_bSndPause=f.GetKeyVal(key,"PauseBgSound",0);

	m_sndVolHall=f.GetKeyVal(key,"HallSoundVolume",200);
	m_sndVolRoom=f.GetKeyVal(key,"HallSoundVolume",200);

}

void CGamePlaceDlg::DrawViewFrame(CDC * pDC, int iWidth, int iHeight)
{
	//玩家例表
	//上下
	//	CGameImageHelper helptc(&m_tc);
	CGameImageHelper helpbc(&m_bc);

	if(helpbc.GetWidth() > 0)
	{
		for(int i = 0; i < iWidth; i+=helpbc.GetWidth())
			helpbc.BitBlt(pDC->GetSafeHdc(),i,iHeight-helpbc.GetHeight());
	}

	CRect tr;
	m_topWnd.GetWindowRect(&tr);
	//人物头像左右
	CGameImageHelper helplc(&m_lc);
	CGameImageHelper helprc(&m_rc);
	if(helplc.GetWidth() > 0)
	{
		for(int i = tr.Height(); i < iHeight; i+=helplc.GetHeight())
		{
			helplc.BitBlt(pDC->GetSafeHdc(),0,i);
			helprc.BitBlt(pDC->GetSafeHdc(),iWidth-helprc.GetWidth(),i);
		}
	}
	//边角
	//边界down
	CGameImageHelper helplb(&m_lb);
	CGameImageHelper helprb(&m_rb);
	CGameImageHelper helprbext(&m_rbext);
	helplb.BitBlt(pDC->GetSafeHdc(),0,iHeight - helplb.GetHeight());	
	helprb.BitBlt(pDC->GetSafeHdc(),iWidth - helprb.GetWidth(),iHeight - helprb.GetHeight());	
	helprbext.BitBlt(pDC->GetSafeHdc(),iWidth - helprbext.GetWidth(),iHeight - helprbext.GetHeight());	

	CFont Font,*OldFont;
	Font.CreateFont(12,6,0,0,0,0,0,0,134,3,2,1,2,"Verdana");// (12,9,0,0,0,0,0,0,134,3,2,1,2,TEXT("宋体"));
	CString s=CBcfFile::GetAppPath ();/////本地路径
	CBcfFile f( s + m_skinmgr.GetSkinBcfFileName());
	CString key=TEXT("PlaceDlg");
	int r,g,b;

	r=f.GetKeyVal(key,"versionr",255);
	g=f.GetKeyVal(key,"versiong",255);
	b=f.GetKeyVal(key,"versionb",0);

	pDC->SetTextColor(RGB(r,g,b));
	pDC->SetBkMode(TRANSPARENT);

	OldFont=pDC->SelectObject(&Font);
	CString sver;
	//*sver.Format("版本:%d.%d.%d Build:%s",
	//	VER_MAIN,VER_MIDDLE,VER_RESVERSE,VER_BUILDTIME);*/
#ifndef DEV_NOFREE
	char *strsz = "啡土构球婴坝牱齐刈峭寅显诀济构型疼定然"; 
	CString strDest;
	EnCode_Key(strDest,strsz,-3);	
	int nLen = strDest.GetLength();
	nLen *= 6;
	pDC->TextOut(iWidth-10-nLen,iHeight-18,strDest);
	Font.DeleteObject();
#endif

	if (m_strTopWndRander == "DUI")
	{
		m_topWnd.ShowWindow(SW_HIDE);
	}
}


LRESULT CGamePlaceDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	//switch(message)
	//{
	//case WM_DISPLAYCHANGE:
	//	this->ShowWindow(SW_NORMAL);
	//	MoveWindow(0,0, GetSystemMetrics(SM_CXFULLSCREEN),GetSystemMetrics(SM_CYFULLSCREEN)+GetSystemMetrics(SM_CYCAPTION));
	//	this->ShowWindow(SW_SHOWMINIMIZED);
	//	return true;
	//}
	return CDialog::WindowProc(message, wParam, lParam);
}

void CGamePlaceDlg::SetDisplays(void)
{
	TCHAR sz[100];
	DEVMODE devmode;
	bool set=false;
	bool needset=false;
	int color[6]={32,32,16,16,32,16};
	int frequency[6]={85,75,85,75,60,60};
	devmode.dmSize=sizeof(devmode);
	if(!::EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode))
	{
		//printf("%d",GetLastError());	
	}
	else if(devmode.dmPelsWidth<1024||devmode.dmPelsHeight<768)
	{
		needset=true;
		for(int j=0;j<6&&!set;j++)
		{
			for(int i=0;;i++)
			{
				if(!::EnumDisplaySettings(NULL,i,&devmode))
				{
					//printf("%d",GetLastError());	
					break;
				}
				if(devmode.dmPelsWidth>=1024&&devmode.dmPelsHeight>=768)
				{
					if(devmode.dmDisplayFrequency>=frequency[j]&&devmode.dmBitsPerPel>=color[j])
					{
						//设置分辨率
						if(::ChangeDisplaySettings(&devmode,CDS_TEST|CDS_FULLSCREEN)==DISP_CHANGE_SUCCESSFUL)
						{
							if(::ChangeDisplaySettings(&devmode,CDS_FULLSCREEN)==DISP_CHANGE_SUCCESSFUL)
							{
								set=true;
								break;
							}
						}
					}
				}
			}
		}
	}
	if(needset&&!set)
	{
		CBcfFile fMsg(CBcfFile::GetAppPath()+"ClientMessage.bcf");
//		wsprintf(sz,"您的分辨率在游戏中不适合,请您更改分辨率到1024*768以上,包括1024*768,以更好地游戏!");
		 DUIMessageBox(m_hWnd,MB_ICONINFORMATION|MB_OK,fMsg.GetKeyVal("MainRoom","Config","设置"),false,
			 fMsg.GetKeyVal("MainRoom","ScreenConfig","您的分辨率在游戏中不适合,请您更改分辨率到1024*768以上,包括1024*768,以更好地游戏!"));

	}
}

//设置显示模式
LRESULT  CGamePlaceDlg::SetLeftHideOrShow(WPARAM wparam,LPARAM lparam)
{
	CRect rect;
	m_GameListWnd.GetClientRect(&rect);
	int list_cx = rect.Width();
	int list_cy = rect.Height();

	m_RoomTabWnd.GetClientRect(&rect);
	int tab_cx = rect.Width();
	int tab_cy = rect.Height();

	m_ActInformWnd.GetClientRect(&rect);
	int act_cx = rect.Width();
	int act_cy = rect.Height();
	if(list_cx >= LEFT_WND_WIDTH &&!(int)wparam)//最小化后不再处理
	{
		return 0;
	}
	/// 如果当前激活页面不是大厅，则不响应点击事件 
	if (m_RoomTabWnd.GetCurrentActivePage()>0 && wparam>0) 
	{
		RECT rt;
		m_GameListWnd.GetClientRect(&rt);
		if (rt.right - rt.left<10)
		{
			m_GameListWnd.ShowHideList(true);
			return 0;
		}

	}
	else
	{
		m_GameListWnd.ShowHideList(false);
	}
	if((int)wparam)
	{
		//m_GameListWnd.SetSmall();
		//游戏例表
		SafeMoveWindow(&m_GameListWnd,m_lc.GetWidth(),LEFT_WND_TOP,6,list_cy);
		//属性页
		SafeMoveWindow(&m_RoomTabWnd,m_lc.GetWidth()+6,LEFT_WND_TOP,tab_cx - 6 + LEFT_WND_WIDTH,tab_cy);
		//活动广告
		//SafeMoveWindow(&m_ActInformWnd,m_lc.GetWidth()+6,LEFT_WND_TOP + list_cy,act_cx -6 + LEFT_WND_WIDTH,act_cy);
	}
	else
	{
		//m_GameListWnd.SetNormal();
		//游戏例表
		SafeMoveWindow(&m_GameListWnd,m_lc.GetWidth(),LEFT_WND_TOP,LEFT_WND_WIDTH,list_cy);
		//属性页
		SafeMoveWindow(&m_RoomTabWnd,m_lc.GetWidth()+LEFT_WND_WIDTH,LEFT_WND_TOP,tab_cx + 6 -LEFT_WND_WIDTH ,tab_cy);
		//活动广告
		//SafeMoveWindow(&m_ActInformWnd,m_lc.GetWidth() + LEFT_WND_WIDTH,LEFT_WND_TOP + list_cy,act_cx + 6 - LEFT_WND_WIDTH,act_cy);
	}

	return 0;
}

//设置显示模式
LRESULT  CGamePlaceDlg::SetRightHideOrShow(WPARAM wparam,LPARAM lparam)
{
	return 0;
}

//打开银行
LRESULT CGamePlaceDlg::OpenBank(WPARAM wparam,LPARAM lparam)
{
	::PostMessage(m_RoomTabWnd.GetSafeHwnd(),IDM_OPEN_BANK,wparam,lparam);
	return 0;
}

LRESULT CGamePlaceDlg::CallGM(WPARAM wparam,LPARAM lparam)
{
	::PostMessage(m_RoomTabWnd.GetSafeHwnd(), IDM_CALL_GM, wparam,lparam);
	return 0;
}

//打开IE
LRESULT CGamePlaceDlg::OpenIE(WPARAM wparam,LPARAM lparam)
{

	//收到大厅顶部的IE消息
	if(wparam==7)//呼叫网管，只能在游戏当中
		::PostMessage(m_RoomTabWnd.GetSafeHwnd(),IDM_OPEN_BANK,1,1);
	else
		::PostMessage(m_RoomTabWnd.GetSafeHwnd(),IDM_OPEN_IE,wparam,lparam);
	return 0;
}

LRESULT CGamePlaceDlg::OpenAppeal(WPARAM wparam,LPARAM lparam)
{
	//投诉
	::PostMessage(m_RoomTabWnd.GetSafeHwnd(),IDM_MESSAGE_APPEAL,wparam,lparam);
	return 0;
}

//锁定机器 zxj 2009-11-12
LRESULT CGamePlaceDlg::OnLockAccount(WPARAM wParam, LPARAM lParam)
{
	::PostMessage(m_RoomTabWnd.GetSafeHwnd(), IDM_LOCK_ACCOUNT, wParam, lParam);
	return 0;
}

//版本核对
BOOL CGamePlaceDlg::CheckVersion()
{
	CString sn ="20081231";////截止日期

	long in=atol(sn.GetBuffer (sn.GetLength ()));
	if(in<=0)
		return FALSE;

	int y1=atoi(sn.Mid (0,4)),
		m1   =atoi(sn.Mid (4,2)),
		d1=atoi(sn.Mid (6,2));

	CTime t1(y1,m1,d1,0,0,0);
	time_t ct;  
	time( &ct ) ;

	CTime t2(ct);

	CTimeSpan tsp;
	tsp=t2-t1;//// 当前日期  - 截止日期

	LONGLONG dd=tsp.GetDays ();

	if(t2 > t1)//dd > 0)
	{
		return FALSE;
	}
	return TRUE;
}

void CGamePlaceDlg::OnPaint()
{
	CPaintDC realdc(this);
	CRect ClientRect, clipRect;
	GetClientRect(&ClientRect);
	int iWidth = ClientRect.Width();
	int iHeight = ClientRect.Height();
	GetClipBox( realdc.GetSafeHdc(), &clipRect );
	realdc.SelectClipRgn(&m_Rgn, RGN_AND);
	CRgn rgn;
	rgn.CreateRectRgnIndirect(&clipRect);
	if (m_Rgn.CombineRgn(&m_Rgn, &rgn, RGN_AND)==NULLREGION)
	{
		rgn.DeleteObject();
		return;
	}

	/// 创建内存缓冲DC
	CBitmap BufBmp;
	CDC dc;
	CRect rc;
	GetClientRect(&rc);
	BufBmp.CreateCompatibleBitmap(&realdc, rc.Width(), rc.Height());
	dc.CreateCompatibleDC(&realdc);
	CBitmap * pOldBmp=dc.SelectObject(&BufBmp);

	DrawViewFrame(&dc,iWidth,iHeight);
	//绘画屏幕
	realdc.BitBlt(
		0,
		0,
		iWidth, 
		iHeight,
		&dc,
		0,
		0,
		SRCCOPY);
	dc.SelectObject(pOldBmp);
	BufBmp.DeleteObject();
	dc.DeleteDC();
	rgn.DeleteObject();
}

void CGamePlaceDlg::OnMove(int x,int y)
{
	__super::OnMove(x, y);
	if(m_pMainRoom != NULL)
	{
		int uActivePage = m_RoomTabWnd.GetCurrentActivePage();
		if(uActivePage == 0)
		{
			return;
		}
		uActivePage = (uActivePage == MAX_PAGE) ? 1 : uActivePage;
		CGameRoomEx* p = (CGameRoomEx*)(m_pMainRoom->m_RoomInfo[uActivePage - 1].pGameRoomWnd);
		if(p != NULL)
		{
			p->OnParentMove(x, y);
		}
	}
	
}

void CGamePlaceDlg::OnNcPaint()
{
	CPaintDC realdc(this);
	CRect ClientRect, clipRect, wndRect;
	GetClipBox( realdc.GetSafeHdc(), &clipRect );
	GetClientRect(&ClientRect);
	CRgn rgn1, rgn2;
	CGameImageHelper helpbc(&m_bc);
	CGameImageHelper helprc(&m_rc);
	CGameImageHelper helplc(&m_lc);
	rgn1.CreateRectRgn(ClientRect.left, ClientRect.bottom-helpbc.GetHeight(), ClientRect.right, ClientRect.bottom);
	rgn2.CreateRectRgn(ClientRect.left, ClientRect.top+86, ClientRect.left+helplc.GetWidth(), ClientRect.bottom);
	rgn1.CombineRgn(&rgn1, &rgn2, RGN_OR);
	rgn2.SetRectRgn(ClientRect.right-helprc.GetWidth(), ClientRect.top+86, ClientRect.right, ClientRect.bottom);
	rgn1.CombineRgn(&rgn1, &rgn2, RGN_OR);
	int cx = ClientRect.left;
	int cy = ClientRect.top;
	CRect rcBtn;
	/// 播放按钮位置
	rgn2.SetRectRgn(
		sndPlayX>0?sndPlayX:cx+sndPlayX,
		sndPlayY>0?sndPlayY:cy+sndPlayY,
		m_sndBtnCx + sndPlayX>0?sndPlayX:cx+sndPlayX,
		m_sndBtnCy + sndPlayY>0?sndPlayY:cy+sndPlayY);
	rgn2.CombineRgn(&rgn1, &rgn2, RGN_AND);
	rgn1.CombineRgn(&rgn1, &rgn2, RGN_XOR);
	/// 声音小
	rgn2.SetRectRgn(sndVolDownX>0?sndVolDownX:cx+sndVolDownX,
		sndVolDownY>0?sndVolDownY:cy+sndVolDownY,
		sndVolDownX>0?sndVolDownX:cx+sndVolDownX+m_sndBtnCx,
		sndVolDownY>0?sndVolDownY:cy+sndVolDownY+m_sndBtnCy);
	rgn2.CombineRgn(&rgn1, &rgn2, RGN_AND);
	rgn1.CombineRgn(&rgn1, &rgn2, RGN_XOR);
	/// 声音大
	rgn2.SetRectRgn(sndVolUpX>0?sndVolUpX:cx+sndVolUpX,
		sndVolUpY>0?sndVolUpY:cy+sndVolUpY,
		sndVolUpX>0?sndVolUpX:cx+sndVolUpX+m_sndBtnCx,
		sndVolUpY>0?sndVolUpY:cy+sndVolUpY+m_sndBtnCy);
	rgn2.CombineRgn(&rgn1, &rgn2, RGN_AND);
	rgn1.CombineRgn(&rgn1, &rgn2, RGN_XOR);

	if (!rgn1.RectInRegion(&clipRect))
	{
		rgn1.DeleteObject();
		rgn2.DeleteObject();
		return;
	}

	realdc.SelectClipRgn(&rgn1, RGN_AND);
	int iWidth = ClientRect.Width();
	int iHeight = ClientRect.Height();

	/// 创建内存缓冲DC
	CBitmap BufBmp;
	CDC dc;
	CRect rc;
	GetWindowRect(&rc);
	BufBmp.CreateCompatibleBitmap(&realdc, rc.Width(), rc.Height());
	dc.CreateCompatibleDC(&realdc);
	CBitmap * pOldBmp=dc.SelectObject(&BufBmp);

	DrawViewFrame(&dc,iWidth,iHeight);
	//绘画屏幕
	realdc.BitBlt(
		0,
		0,
		iWidth, 
		iHeight,
		&dc,
		0,
		0,
		SRCCOPY);
	rgn2.DeleteObject();
	rgn1.DeleteObject();
	dc.SelectObject(pOldBmp);
	BufBmp.DeleteObject();
	dc.DeleteDC();

}

void CGamePlaceDlg::OnSndPlay()
{
	m_bSndPause=false;
	BZSoundContinue(m_bgCurrentID);
	m_btSndPlay.ShowWindow(SW_HIDE);
	m_btSndPause.ShowWindow(SW_SHOW);

	CString s=CBcfFile::GetAppPath ();/////本地路径
	//CBcfFile f( s + "skin0.bcf");
	CString key=TEXT("PlaceDlg");

	//f.SetKeyValString(key,"PauseBgSound","0");

	DWORD dwCfgFile = ::cfgOpenFile(s + m_skinmgr.GetSkinBcfFileName());

	if(dwCfgFile < 0x10) //文件打开失败
	{
		return;
	}

	//背景音乐开关配置
	::cfgSetValue(dwCfgFile,key,"PauseBgSound","0");

	::cfgClose(dwCfgFile);

	return;

}

void CGamePlaceDlg::OnSndPause()
{
	m_bSndPause=true;
	BZSoundPause(m_bgCurrentID);
	m_btSndPlay.ShowWindow(SW_SHOW);
	m_btSndPause.ShowWindow(SW_HIDE);

	CString s=CBcfFile::GetAppPath ();/////本地路径
	//CBcfFile f( s + "skin0.bcf");
	CString key=TEXT("PlaceDlg");
	//f.SetKeyValString(key,"PauseBgSound","1");

	DWORD dwCfgFile = ::cfgOpenFile(s + m_skinmgr.GetSkinBcfFileName());

	if(dwCfgFile < 0x10) //文件打开失败
	{
		return;
	}

	//背景音乐开关配置
	::cfgSetValue(dwCfgFile,key,"PauseBgSound","1");

	::cfgClose(dwCfgFile);

	return;

}
void CGamePlaceDlg::OnSndVolDown()
{
	int nVol=BZSoundGetVolume(m_bgCurrentID);
	BZSoundSetVolume(m_bgCurrentID,nVol-50);

	CString sini;
	sini.Format("%d",(nVol-50)>1?nVol-50:1);
	CString s=CBcfFile::GetAppPath ();/////本地路径
	CBcfFile f( s + m_skinmgr.GetSkinBcfFileName());
	CString key=TEXT("PlaceDlg");
	if(m_bgCurrentID==m_bgHallSoundID)
		f.SetKeyValString(key,"HallSoundVolume",sini);
	else
		f.SetKeyValString(key,"RoomSoundVolume",sini);

}
void CGamePlaceDlg::OnSndVolUp()
{
	int nVol=BZSoundGetVolume(m_bgCurrentID);
	BZSoundSetVolume(m_bgCurrentID,nVol+50);

	CString sini;
	sini.Format("%d",(nVol+50)<1000?nVol+50:1000);
	CString s=CBcfFile::GetAppPath ();/////本地路径
	CBcfFile f( s + m_skinmgr.GetSkinBcfFileName());
	CString key=TEXT("PlaceDlg");
	if(m_bgCurrentID==m_bgHallSoundID)
		f.SetKeyValString(key,"HallSoundVolume",sini);
	else
		f.SetKeyValString(key,"RoomSoundVolume",sini);
}


void CGamePlaceDlg::OnTechSupport()
{
	//CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}
void CGamePlaceDlg::EnCode_Key(CString &strdest, char *strsource, char key)
{
	if(NULL == strsource)
	{
		return;		 
	}
	char strTarget[200];
	memset(strTarget,0,sizeof(strTarget));

	int i = 0;
	while(strsource[i]!='\0')
	{
		strTarget[i] +=(strsource[i]- key);
		i++;
	}
	strdest.Format("%s",strTarget);		
}

// PengJiLin, 2011-7-20, 获取 CPU、硬盘 ID
#include <Wbemidl.h>
#include <comdef.h>
#pragma comment(lib, "wbemuuid.lib")
int GetCPUHardInfo(CString& strCPUID, CString& strHardID)
{
    strCPUID = "No Found CPU ID";
    strHardID = "No Found Hard ID";

    HRESULT hres;
    hres =  CoInitializeEx(0, COINIT_MULTITHREADED );
    if (FAILED(hres))
    {
        return 1;
    }

    hres =  CoInitializeSecurity(
                            NULL, 
                            -1,                          // COM authentication
                            NULL,                        // Authentication services
                            NULL,                        // Reserved
                            RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
                            RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
                            NULL,                        // Authentication info
                            EOAC_NONE,                   // Additional capabilities 
                            NULL                         // Reserved
                            );
    if (FAILED(hres))
    {
        CoUninitialize();
        return 1;
    }

    // 以上不是必须的，若已有“::COMInit();”，则要跳过
    // Obtain the initial locator to WMI 
    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(
                        CLSID_WbemLocator,             
                        0, 
                        CLSCTX_INPROC_SERVER, 
                        IID_IWbemLocator, (LPVOID *) &pLoc);
    if (FAILED(hres))
    {
        CoUninitialize();
        return 1;
    }

    // Connect to WMI through the IWbemLocator::ConnectServer method
    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(
                            _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace //
                            NULL,                    // User name. NULL = current user
                            NULL,                    // User password. NULL = current
                            0,                       // Locale. NULL indicates current
                            NULL,                    // Security flags.
                            0,                       // Authority (e.g. Kerberos)
                            0,                       // Context object 
                            &pSvc                    // pointer to IWbemServices proxy
                            );
    if (FAILED(hres))
    {
        pLoc->Release();     
        CoUninitialize();
        return 1;
    }

    // Set security levels on the proxy
    hres = CoSetProxyBlanket(
                            pSvc,                        // Indicates the proxy to set
                            RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
                            RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
                            NULL,                        // Server principal name 
                            RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
                            RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
                            NULL,                        // client identity
                            EOAC_NONE                    // proxy capabilities 
                            );
    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return 1; 
    }

    // Use the IWbemServices pointer to make requests of WMI ----
    IEnumWbemClassObject* pEnumerator = NULL;
    
    // 计算CPUID
    hres = pSvc->ExecQuery(
                            bstr_t("WQL"), 
                            bstr_t("SELECT * FROM Win32_Processor"),//Win32_OperatingSystem
                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
                            NULL,
                            &pEnumerator);
    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;
    }

    CString strProcessID = "";
    char chInfo[20+1] = {0};

    // Get the data from the query 
    IWbemClassObject *pclsObj;
    ULONG uReturn = 0;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,  &pclsObj, &uReturn);
        if(0 == uReturn)
        {
            break;
        }
        VARIANT vtProp;
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);
        strProcessID=_com_util::ConvertBSTRToString(vtProp.bstrVal);
    }

    memcpy(chInfo, strProcessID.GetBuffer(), 20);
    chInfo[20] = '\0';
    strCPUID = chInfo;

    // 计算硬盘系列号
    hres = pSvc->ExecQuery(
                            bstr_t("WQL"), 
                            bstr_t("SELECT * FROM Win32_DiskDrive"),
                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
                            NULL,
                            &pEnumerator);
    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1; 
    }

    CString strDisk = "";
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if(0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;
        VariantInit(&vtProp);
        hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
        strDisk=_com_util::ConvertBSTRToString(vtProp.bstrVal);
    }

    memset(chInfo, 0, sizeof(chInfo));
    memcpy(chInfo, strDisk.GetBuffer(), 20);
    chInfo[20] = '\0';
    strHardID = chInfo;

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    pclsObj->Release();
    CoUninitialize();

    return 0;
}

//换肤
LRESULT CGamePlaceDlg::OnExchangeSkin(WPARAM wpara,LPARAM lpara)
{
	LoadSkin();
	OnNcPaint();
	Invalidate();
	return LRESULT(0);
}

DWORD GetHarddiskNum()  
{  
	char cVolume[256];                                         //   
	char cFileSysName[256];   
	DWORD dwSerialNum;                                          //硬盘序列号   
	DWORD dwFileNameLength;   
	DWORD dwFileSysFlag;   

	::GetVolumeInformation("C://", cVolume, 256, &dwSerialNum, &dwFileNameLength,   
		&dwFileSysFlag, cFileSysName, 256);  

	return dwSerialNum;  
}  


DWORD GetCPUID()  
{         
	DWORD   dwId1, dwId2, dwId3, dwId4;     
	char   szCompany[13];     
	PCHAR   pCompany = szCompany;  

	//DWORD dwCPU;   

	szCompany[12]=0;     
	_asm     
	{     
		pushfd     
			pushad     
			//取得CPU的ID号     
			mov   eax,1   //功能号     
			_emit   0x0f     
			_emit   0xa2     
			mov   dwId1,eax     
			mov   dwId2,ebx     
			mov   dwId3,ecx     
			mov   dwId4,edx     

			//取得CPU的制造公司名称  
			mov   edi,pCompany   //功能号     
			mov   eax,0     
			_emit   0x0f     
			_emit   0xa2     
			mov   eax,ebx     
			stosd     
			mov   eax,edx     
			stosd     
			mov   eax,ecx     
			stosd     
			popad     
			popfd     
	}     

	DWORD dwResult = 0;  
	DWORD dwTemp1 = dwId1 << 12;  
	DWORD dwTemp2 = dwId2 << 8 ;  
	DWORD dwTemp3 = dwId3 << 4;  

	dwResult = dwTemp1 + dwTemp2 + dwTemp3 + dwId4;  

	return dwResult;  
}
