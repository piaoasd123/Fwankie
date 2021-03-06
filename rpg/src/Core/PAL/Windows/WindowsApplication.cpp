﻿
#include "Core.h"
#include "WindowsApplication.h"
#include "WindowsWindow.h"
#include "GenericWindowDefinition.h"

// #include "WindowsCursor.h"
// #include "GenericApplicationMessageHandler.h"
// #include "XInputInterface.h"
// #include "IInputDeviceModule.h"
// #include "IInputDevice.h"

#if WITH_EDITOR
#include "ModuleManager.h"
#include "Developer/SourceCodeAccess/Public/ISourceCodeAccessModule.h"
#endif

// Allow Windows Platform types in the entire file.
//#include "AllowWindowsPlatformTypes.h"
	#include "Ole2.h"
	#include <shlobj.h>
	#include <objbase.h>
	#include <SetupApi.h>

#pragma comment(lib, "SetupApi.lib");

// This might not be defined by Windows when maintaining backwards-compatibility to pre-Vista builds
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL                  0x020E
#endif

const IntPoint WindowsApplication::MinimizedWindowPosition(-32000,-32000);

WindowsApplication* WindowApplication = NULL;

WindowsApplication* WindowsApplication::CreateWindowsApplication( const HINSTANCE InstanceHandle, const HICON IconHandle )
{
	WindowApplication = new WindowsApplication( InstanceHandle, IconHandle );
	return WindowApplication;
}

WindowsApplication::WindowsApplication( const HINSTANCE HInstance, const HICON IconHandle )
	: GenericApplication()// MakeShareable( new FWindowsCursor() ) )
	, InstanceHandle( HInstance )
	, bUsingHighPrecisionMouseInput( false )
// 	, XInput( XInputInterface::Create( MessageHandler ) )
// 	, CVarDeferMessageProcessing( 
// 		TEXT( "Slate.DeferWindowsMessageProcessing" ),
// 		bAllowedToDeferMessageProcessing,
// 		TEXT( "Whether windows message processing is deferred until tick or if they are processed immediately" ) )
	, bAllowedToDeferMessageProcessing( true )
	, bHasLoadedInputPlugins( false )
	, bInModalSizeLoop( false )

{
	// Disable the process from being showing "ghosted" not responding messages during slow tasks
	// This is a hack.  A more permanent solution is to make our slow tasks not block the editor for so long
	// that message pumping doesn't occur (which causes these messages).
	::DisableProcessWindowsGhosting();

	// Register the Win32 class for Slate windows and assign the application instance and icon
	const bool bClassRegistered = RegisterClass( InstanceHandle, IconHandle );

	// Initialize OLE for Drag and Drop support.
	OleInitialize( NULL );

	//TODO: fix text input system
// 	TextInputMethodSystem = MakeShareable( new FWindowsTextInputMethodSystem );
// 	if(!TextInputMethodSystem->Initialize())
// 	{
// 		TextInputMethodSystem.Reset();
// 	}

	// Get initial display metrics. (display information for existing desktop, before we start changing resolutions)
	GetDisplayMetrics(InitialDisplayMetrics);
}

bool WindowsApplication::RegisterClass( const HINSTANCE HInstance, const HICON HIcon )
{
	WNDCLASS wc;
	memset( &wc, NULL, sizeof(wc) );

	wc.style = CS_DBLCLKS; // We want to receive double clicks
	wc.lpfnWndProc = AppWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = HInstance;
	wc.hIcon = HIcon;
	wc.hCursor = NULL;	// We manage the cursor ourselves
	wc.hbrBackground = NULL; // Transparent
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WindowsWindow::AppWindowClass;

	if( !::RegisterClass( &wc ) )
	{
		//ShowLastError();

		// @todo Slate: Error message should be localized!
		MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);

		return false;
	}

	return true;
}

WindowsApplication::~WindowsApplication()
{
	//TODO: fix text input system
	//TextInputMethodSystem->Terminate();

	::CoUninitialize();
	OleUninitialize();
}

std::shared_ptr< GenericWindow > WindowsApplication::MakeWindow()
{ 
	std::shared_ptr<GenericWindow> NativeParent = NULL;
// 	TSharedPtr<SWindow> ParentWindow = InSlateWindow->GetParentWindow();
// 	if (ParentWindow.IsValid())
// 	{
// 		NativeParent = ParentWindow->GetNativeWindow();
// 	}



	auto NewWindow = std::shared_ptr<GenericWindow> (WindowsWindow::Make());

// 	InSlateWindow->SetNativeWindow(NewWindow);
// 
// 	InSlateWindow->SetCachedScreenPosition(Position);
// 	InSlateWindow->SetCachedSize(Size);
// 
// 	PlatformApplication->InitializeWindow(NewWindow, Definition, NativeParent, bShowImmediately);

	return NewWindow;
}

void WindowsApplication::InitializeWindow(const std::shared_ptr < GenericWindow >& InWindow, const std::shared_ptr < GenericWindowDefinition >& InDefinition, const std::shared_ptr < GenericWindow >& InParent, const bool bShowImmediately)
{
	const std::shared_ptr < WindowsWindow > Window = std::shared_ptr < WindowsWindow >(static_cast< WindowsWindow* >(InWindow.get()));
	const std::shared_ptr < WindowsWindow > ParentWindow = std::shared_ptr < WindowsWindow >(static_cast< WindowsWindow* >(InParent.get()));

	Windows.push_back( Window );
	Window->Initialize( this, InDefinition, InstanceHandle, ParentWindow, bShowImmediately );
}

void WindowsApplication::SetMessageHandler(const std::shared_ptr < FGenericApplicationMessageHandler >& InMessageHandler)
{
	//TODO: fix set message handler. Figure out whether we still need a message handler
// 	GenericApplication::SetMessageHandler(InMessageHandler);
// 	XInput->SetMessageHandler( InMessageHandler );
// 
// 	TArray<IInputDeviceModule*> PluginImplementations = IModularFeatures::Get().GetModularFeatureImplementations<IInputDeviceModule>( IInputDeviceModule::GetModularFeatureName() );
// 	for( auto DeviceIt = ExternalInputDevices.CreateIterator(); DeviceIt; ++DeviceIt )
// 	{
// 		(*DeviceIt)->SetMessageHandler(InMessageHandler);
// 	}

}

FModifierKeysState WindowsApplication::GetModifierKeys() const
{
	const bool bIsLeftShiftDown = ( ::GetAsyncKeyState( VK_LSHIFT ) & 0x8000 ) != 0;
	const bool bIsRightShiftDown = ( ::GetAsyncKeyState( VK_RSHIFT ) & 0x8000 ) != 0;
	const bool bIsLeftControlDown = ( ::GetAsyncKeyState( VK_LCONTROL ) & 0x8000 ) != 0;
	const bool bIsRightControlDown = ( ::GetAsyncKeyState( VK_RCONTROL ) & 0x8000 ) != 0;
	const bool bIsLeftAltDown = ( ::GetAsyncKeyState( VK_LMENU ) & 0x8000 ) != 0;
	const bool bIsRightAltDown = ( ::GetAsyncKeyState( VK_RMENU ) & 0x8000 ) != 0;

	return FModifierKeysState( bIsLeftShiftDown, bIsRightShiftDown, bIsLeftControlDown, bIsRightControlDown, bIsLeftAltDown, bIsRightAltDown );
}

void WindowsApplication::SetCapture(const std::shared_ptr< GenericWindow >& InWindow)
{
	if ( InWindow )
	{
		::SetCapture( (HWND)InWindow->GetOSWindowHandle() );
	}
	else
	{
		::SetCapture( NULL );
	}
}

void* WindowsApplication::GetCapture( void ) const
{
	return ::GetCapture();
}

void WindowsApplication::SetHighPrecisionMouseMode(const bool Enable, const std::shared_ptr< GenericWindow >& InWindow)
{
	HWND hwnd = NULL;
	DWORD flags = RIDEV_REMOVE;
	bUsingHighPrecisionMouseInput = Enable;

	if ( Enable )
	{
		flags = 0;

		if ( InWindow )
		{
			hwnd = (HWND)InWindow->GetOSWindowHandle(); 
		}
	}

	// NOTE: Currently has to be created every time due to conflicts with Direct8 Input used by the wx unrealed
	RAWINPUTDEVICE RawInputDevice;

	//The HID standard for mouse
	const uint16 StandardMouse = 0x02;

	RawInputDevice.usUsagePage = 0x01; 
	RawInputDevice.usUsage = StandardMouse;
	RawInputDevice.dwFlags = flags;

	// Process input for just the window that requested it.  NOTE: If we pass NULL here events are routed to the window with keyboard focus
	// which is not always known at the HWND level with Slate
	RawInputDevice.hwndTarget = hwnd; 

	// Register the raw input device
	::RegisterRawInputDevices( &RawInputDevice, 1, sizeof( RAWINPUTDEVICE ) );
}

// bool FWindowsApplication::TryCalculatePopupWindowPosition( const FPlatformRect& InAnchor, const Vector2D& InSize, const EPopUpOrientation::Type Orientation, /*OUT*/ FVector2D* const CalculatedPopUpPosition ) const
// {
// #if(_WIN32_WINNT >= 0x0601) 
// 	POINT AnchorPoint = { FMath::TruncToInt(InAnchor.Left), FMath::TruncToInt(InAnchor.Top) };
// 	SIZE WindowSize = { FMath::TruncToInt(InSize.X), FMath::TruncToInt(InSize.Y) };
// 
// 	RECT WindowPos;
// 	::CalculatePopupWindowPosition( &AnchorPoint, &WindowSize, TPM_CENTERALIGN | TPM_VCENTERALIGN, NULL, &WindowPos );
// 
// 	CalculatedPopUpPosition->Set( WindowPos.left, WindowPos.right );
// 	return true;
// #else
// 	return false;
// #endif
// }

FPlatformRect WindowsApplication::GetWorkArea( const FPlatformRect& CurrentWindow ) const
{
	RECT WindowsWindowDim;
	WindowsWindowDim.left = CurrentWindow.Left;
	WindowsWindowDim.top = CurrentWindow.Top;
	WindowsWindowDim.right = CurrentWindow.Right;
	WindowsWindowDim.bottom = CurrentWindow.Bottom;

	// ... figure out the best monitor for that window.
	HMONITOR hBestMonitor = MonitorFromRect( &WindowsWindowDim, MONITOR_DEFAULTTONEAREST );

	// Get information about that monitor...
	MONITORINFO MonitorInfo;
	MonitorInfo.cbSize = sizeof(MonitorInfo);
	GetMonitorInfoW( hBestMonitor, &MonitorInfo);

	// ... so that we can figure out the work area (are not covered by taskbar)
	MonitorInfo.rcWork;

	FPlatformRect WorkArea;
	WorkArea.Left = MonitorInfo.rcWork.left;
	WorkArea.Top = MonitorInfo.rcWork.top;
	WorkArea.Right = MonitorInfo.rcWork.right;
	WorkArea.Bottom = MonitorInfo.rcWork.bottom;

	return WorkArea;
}

/**
 * Extracts EDID data from the given registry key and reads out native display with and height
 * @param hDevRegKey - Registry key where EDID is stored
 * @param OutWidth - Reference to output variable for monitor native width
 * @param OutHeight - Reference to output variable for monitor native height
 * @returns 'true' if data was extracted successfully, 'false' otherwise
 **/
static bool GetMonitorSizeFromEDID(const HKEY hDevRegKey, int32& OutWidth, int32& OutHeight)
{	
	static const uint32 NameSize = 512;
	static TCHAR ValueName[NameSize];

	DWORD Type;
	DWORD ActualValueNameLength = NameSize;

	BYTE EDIDData[1024];
	DWORD EDIDSize = sizeof(EDIDData);

	for (LONG i = 0, RetValue = ERROR_SUCCESS; RetValue != ERROR_NO_MORE_ITEMS; ++i)
	{
		RetValue = RegEnumValue ( hDevRegKey, 
			i, 
			&ValueName[0],
			&ActualValueNameLength, NULL, &Type,
			EDIDData,
			&EDIDSize);

		if (RetValue != ERROR_SUCCESS || (_tcscmp(ValueName, TEXT("EDID")) != 0))
		{
			continue;
		}

		// EDID data format documented here:
		// http://en.wikipedia.org/wiki/EDID

		int DetailTimingDescriptorStartIndex = 54;
		OutWidth = ((EDIDData[DetailTimingDescriptorStartIndex+4] >> 4) << 8) | EDIDData[DetailTimingDescriptorStartIndex+2];
		OutHeight = ((EDIDData[DetailTimingDescriptorStartIndex+7] >> 4) << 8) | EDIDData[DetailTimingDescriptorStartIndex+5];

		return true; // valid EDID found
	}

	return false; // EDID not found
}

/**
 * Locate registry record for the given display device ID and extract native size information
 * @param TargetDevID - Name of taret device
 * @praam OutWidth - Reference to output variable for monitor native width
 * @praam OutHeight - Reference to output variable for monitor native height
 * @returns TRUE if data was extracted successfully, FALSE otherwise
 **/
inline bool GetSizeForDevID(const std::tstring& TargetDevID, int32& Width, int32& Height)
{
	static const GUID ClassMonitorGuid = {0x4d36e96e, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};

	HDEVINFO DevInfo = SetupDiGetClassDevsEx(
		&ClassMonitorGuid, //class GUID
		NULL,
		NULL,
		DIGCF_PRESENT,
		NULL,
		NULL,
		NULL);

	if (NULL == DevInfo)
	{
		return false;
	}

	bool bRes = false;

	for (ULONG MonitorIndex = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++MonitorIndex)
	{ 
		SP_DEVINFO_DATA DevInfoData;
		ZeroMemory(&DevInfoData, sizeof(DevInfoData));
		DevInfoData.cbSize = sizeof(DevInfoData);

		if (SetupDiEnumDeviceInfo(DevInfo, MonitorIndex, &DevInfoData) == TRUE)
		{
			HKEY hDevRegKey = SetupDiOpenDevRegKey(DevInfo, &DevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

			if(!hDevRegKey || (hDevRegKey == INVALID_HANDLE_VALUE))
			{
				continue;
			}

			bRes = GetMonitorSizeFromEDID(hDevRegKey, Width, Height);

			RegCloseKey(hDevRegKey);
		}
	}
	
	if (SetupDiDestroyDeviceInfoList(DevInfo) == FALSE)
	{
		bRes = false;
	}

	return bRes;
}

/**
 * Extract hardware information about connect monitors
 * @param OutMonitorInfo - Reference to an array for holding records about each detected monitor
 **/
void GetMonitorInfo(std::vector<FMonitorInfo>& OutMonitorInfo)
{
	DISPLAY_DEVICE DisplayDevice;
	DisplayDevice.cb = sizeof(DisplayDevice);
	DWORD DeviceIndex = 0; // device index

	FMonitorInfo* PrimaryDevice = NULL;
	OutMonitorInfo.reserve(2); // Reserve two slots, as that will be the most common maximum

	std::tstring DeviceID;
	while (EnumDisplayDevices(0, DeviceIndex, &DisplayDevice, 0))
	{
		if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) > 0)
		{
			DISPLAY_DEVICE Monitor;
			ZeroMemory(&Monitor, sizeof(Monitor));
			Monitor.cb = sizeof(Monitor);
			DWORD MonitorIndex = 0;

			while (EnumDisplayDevices(DisplayDevice.DeviceName, MonitorIndex, &Monitor, 0))
			{
				if (Monitor.StateFlags & DISPLAY_DEVICE_ACTIVE &&
					!(Monitor.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
				{
					FMonitorInfo Info;

					Info.ID = Monitor.DeviceID;
					Info.Name = Info.ID;//.Mid (8, Info.ID.Find (TEXT("\\"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 9) - 8);

					if (GetSizeForDevID(Info.Name, Info.NativeWidth, Info.NativeHeight))
					{
						Info.ID = Monitor.DeviceID;
						Info.bIsPrimary = (DisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) > 0;
						OutMonitorInfo.push_back(Info);

						if (PrimaryDevice == NULL && Info.bIsPrimary)
						{
							PrimaryDevice = OutMonitorInfo.end()._Ptr;
						}
					}
				}
				MonitorIndex++;

				ZeroMemory(&Monitor, sizeof(Monitor));
				Monitor.cb = sizeof(Monitor);
			}
		}

		ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
		DisplayDevice.cb = sizeof(DisplayDevice);
		DeviceIndex++;
	}
}

void WindowsApplication::GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const
{
	// Total screen size of the primary monitor
	OutDisplayMetrics.PrimaryDisplayWidth = ::GetSystemMetrics( SM_CXSCREEN );
	OutDisplayMetrics.PrimaryDisplayHeight = ::GetSystemMetrics( SM_CYSCREEN );

	// Get the screen rect of the primary monitor, excluding taskbar etc.
	RECT WorkAreaRect;
	WorkAreaRect.top = WorkAreaRect.bottom = WorkAreaRect.left = WorkAreaRect.right = 0;
	SystemParametersInfo( SPI_GETWORKAREA, 0, &WorkAreaRect, 0 );
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Left = WorkAreaRect.left;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Top = WorkAreaRect.top;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right = WorkAreaRect.right;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom = WorkAreaRect.bottom;

	// Virtual desktop area
	OutDisplayMetrics.VirtualDisplayRect.Left = ::GetSystemMetrics( SM_XVIRTUALSCREEN );
	OutDisplayMetrics.VirtualDisplayRect.Top = ::GetSystemMetrics( SM_YVIRTUALSCREEN );
	OutDisplayMetrics.VirtualDisplayRect.Right = OutDisplayMetrics.VirtualDisplayRect.Left + ::GetSystemMetrics( SM_CXVIRTUALSCREEN );
	OutDisplayMetrics.VirtualDisplayRect.Bottom = OutDisplayMetrics.VirtualDisplayRect.Top + ::GetSystemMetrics( SM_CYVIRTUALSCREEN );

	// Get connected monitor information
	GetMonitorInfo(OutDisplayMetrics.MonitorInfo);
}

void WindowsApplication::GetInitialDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const
{
	OutDisplayMetrics = InitialDisplayMetrics;
}

void WindowsApplication::DestroyApplication()
{

}

static std::shared_ptr< WindowsWindow > FindWindowByHWND(const std::vector< std::shared_ptr< WindowsWindow > >& WindowsToSearch, HWND HandleToFind)
{
	for (int32 WindowIndex=0; WindowIndex < WindowsToSearch.size(); ++WindowIndex)
	{
		std::shared_ptr < WindowsWindow > Window = WindowsToSearch[WindowIndex];
		if ( Window->GetHWnd() == HandleToFind )
		{
			return Window;
		}
	}

	return std::shared_ptr < WindowsWindow >(NULL);
}


/** All WIN32 messages sent to our app go here; this method simply passes them on */
LRESULT CALLBACK WindowsApplication::AppWndProc(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam)
{
	//TODO: is there a need to check for multi threading?
	//ensure( IsInGameThread() );

	return WindowApplication->ProcessMessage( hwnd, msg, wParam, lParam );
}

int32 WindowsApplication::ProcessMessage( HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam )
{
	std::shared_ptr < WindowsWindow > CurrentNativeEventWindowPtr = FindWindowByHWND(Windows, hwnd);

	if( Windows.size() && CurrentNativeEventWindowPtr )
	{
		std::shared_ptr< WindowsWindow > CurrentNativeEventWindow = CurrentNativeEventWindowPtr;

		static const std::map <uint32, std::tstring> WindowsMessageStrings = []()
		{
			std::map<uint32, std::tstring> Result;
#define ADD_WINDOWS_MESSAGE_STRING(WMCode) Result[WMCode] = TEXT(#WMCode);
			ADD_WINDOWS_MESSAGE_STRING(WM_INPUTLANGCHANGEREQUEST);
			ADD_WINDOWS_MESSAGE_STRING(WM_INPUTLANGCHANGE);
			ADD_WINDOWS_MESSAGE_STRING(WM_IME_SETCONTEXT);
			ADD_WINDOWS_MESSAGE_STRING(WM_IME_NOTIFY);
			ADD_WINDOWS_MESSAGE_STRING(WM_IME_REQUEST);
			ADD_WINDOWS_MESSAGE_STRING(WM_IME_STARTCOMPOSITION);
			ADD_WINDOWS_MESSAGE_STRING(WM_IME_COMPOSITION);
			ADD_WINDOWS_MESSAGE_STRING(WM_IME_ENDCOMPOSITION);
			ADD_WINDOWS_MESSAGE_STRING(WM_IME_CHAR);
#undef ADD_WINDOWS_MESSAGE_STRING
			return Result;
		}();

		static const std::map<uint32, std::tstring> IMNStrings = []()
		{
			std::map<uint32, std::tstring> Result;
#define ADD_IMN_STRING(IMNCode) Result[IMNCode] = TEXT(#IMNCode);
			ADD_IMN_STRING(IMN_CLOSESTATUSWINDOW);
			ADD_IMN_STRING(IMN_OPENSTATUSWINDOW);
			ADD_IMN_STRING(IMN_CHANGECANDIDATE);
			ADD_IMN_STRING(IMN_CLOSECANDIDATE);
			ADD_IMN_STRING(IMN_OPENCANDIDATE);
			ADD_IMN_STRING(IMN_SETCONVERSIONMODE);
			ADD_IMN_STRING(IMN_SETSENTENCEMODE);
			ADD_IMN_STRING(IMN_SETOPENSTATUS);
			ADD_IMN_STRING(IMN_SETCANDIDATEPOS);
			ADD_IMN_STRING(IMN_SETCOMPOSITIONFONT);
			ADD_IMN_STRING(IMN_SETCOMPOSITIONWINDOW);
			ADD_IMN_STRING(IMN_SETSTATUSWINDOWPOS);
			ADD_IMN_STRING(IMN_GUIDELINE);
			ADD_IMN_STRING(IMN_PRIVATE);
#undef ADD_IMN_STRING
			return Result;
		}();

		static const std::map<uint32, std::tstring> IMRStrings = []()
		{
			std::map<uint32, std::tstring> Result;
#define ADD_IMR_STRING(IMRCode) Result[IMRCode] = TEXT(#IMRCode);
	ADD_IMR_STRING(IMR_CANDIDATEWINDOW);
	ADD_IMR_STRING(IMR_COMPOSITIONFONT);
	ADD_IMR_STRING(IMR_COMPOSITIONWINDOW);
	ADD_IMR_STRING(IMR_CONFIRMRECONVERTSTRING);
	ADD_IMR_STRING(IMR_DOCUMENTFEED);
	ADD_IMR_STRING(IMR_QUERYCHARPOSITION);
	ADD_IMR_STRING(IMR_RECONVERTSTRING);
#undef ADD_IMR_STRING
			return Result;
		}();

		switch(msg)
		{
		case WM_INPUTLANGCHANGEREQUEST:
		case WM_INPUTLANGCHANGE:
		case WM_IME_SETCONTEXT:
		case WM_IME_STARTCOMPOSITION:
		case WM_IME_COMPOSITION:
		case WM_IME_ENDCOMPOSITION:
		case WM_IME_CHAR:
			//UE_LOG(LogWindowsDesktop, Verbose, TEXT("%s"), *(WindowsMessageStrings[msg]));
			DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			return 0;
		case WM_IME_NOTIFY:
			//UE_LOG(LogWindowsDesktop, Verbose, TEXT("WM_IME_NOTIFY - %s"), IMNStrings.Find(wParam) ? *(IMNStrings[wParam]) : nullptr);
			DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			return 0;
		case WM_IME_REQUEST:
			//UE_LOG(LogWindowsDesktop, Verbose, TEXT("WM_IME_REQUEST - %s"), IMRStrings.Find(wParam) ? *(IMRStrings[wParam]) : nullptr);
			DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			return 0;
			// Character
		case WM_CHAR:
			DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			return 0;
		case WM_SYSCHAR:
			{
				if( ( HIWORD(lParam) & 0x2000 ) != 0 && wParam == VK_SPACE )
				{
					// Do not handle Alt+Space so that it passes through and opens the window system menu
					break;
				}
				else
				{
					return 0;
				}
			}
			
			break;

		case WM_SYSKEYDOWN:
			{
				// Alt-F4 or Alt+Space was pressed. 
				// Allow alt+f4 to close the window and alt+space to open the window menu
				if( wParam != VK_F4 && wParam != VK_SPACE)
				{
					DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
				}
			}
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_SETCURSOR:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
				// Handled
				return 0;
			}
			break;

		// Mouse Movement
		case WM_INPUT:
			{
				uint32 Size = 0;
				std::vector<uint8> RawData;

				::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &Size, sizeof(RAWINPUTHEADER));
				RawData.reserve( Size );

				RAWINPUT* Raw = NULL;
				if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, RawData.data(), &Size, sizeof(RAWINPUTHEADER)) == Size )
				{
					Raw = (RAWINPUT*)RawData.data();
				}

				if (Raw->header.dwType == RIM_TYPEMOUSE) 
				{
					const bool IsAbsoluteInput = (Raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE;
					if( IsAbsoluteInput )
					{
						// Since the raw input is coming in as absolute it is likely the user is using a tablet
						// or perhaps is interacting through a virtual desktop
						DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, 0, 0, MOUSE_MOVE_ABSOLUTE );
						return 1;
					}

					// Since raw input is coming in as relative it is likely a traditional mouse device
					const int xPosRelative = Raw->data.mouse.lLastX;
					const int yPosRelative = Raw->data.mouse.lLastY;

					DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, xPosRelative, yPosRelative, MOUSE_MOVE_RELATIVE );
					return 1;
				} 
				
			}
			break;


		case WM_NCCALCSIZE:
			{
				// Let windows absorb this message if using the standard border
				if ( wParam && !CurrentNativeEventWindow->GetDefinition().HasOSWindowBorder )
				{
					return 0;
				}
			}
			break;

		case WM_SHOWWINDOW:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			}
			break;

		case WM_SIZE:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );

				return 0;
			}
			break;

		case WM_SIZING:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, 0, 0 );
			}
			break;
		case WM_ENTERSIZEMOVE:
			{
				bInModalSizeLoop = true;
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, 0, 0 );
			}
			break;
		case WM_EXITSIZEMOVE:
			{
				bInModalSizeLoop = false;
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam, 0, 0 );
			}
			break;


		case WM_MOVE:
			{
				// client area position
				const int32 NewX = (int)(short)(LOWORD(lParam));
				const int32 NewY = (int)(short)(HIWORD(lParam));
				IntPoint NewPosition(NewX,NewY);

				// Only cache the screen position if its not minimized
				if ( WindowsApplication::MinimizedWindowPosition != NewPosition )
				{
					//TODO: fix message handler reference here
					//MessageHandler->OnMovedWindow( CurrentNativeEventWindow, NewX, NewY );

					return 0;
				}
			}
			break;

		case WM_NCHITTEST:
			{
 //TODO: fix only if window border is needed.
				// Only needed if not using the os window border as this is determined automatically
// 				if( !CurrentNativeEventWindow->GetDefinition().HasOSWindowBorder )
// 				{
// 					RECT rcWindow;
// 					GetWindowRect(hwnd, &rcWindow);
// 
// 					const int32 LocalMouseX = (int)(short)(LOWORD(lParam)) - rcWindow.left;
// 					const int32 LocalMouseY = (int)(short)(HIWORD(lParam)) - rcWindow.top;
// 					if ( CurrentNativeEventWindow->IsRegularWindow() )
// 					{
// 						EWindowZone::Type Zone;
// 					
// 						if( MessageHandler->ShouldProcessUserInputMessages( CurrentNativeEventWindowPtr ) )
// 						{
// 							// Assumes this is not allowed to leave Slate or touch rendering
// 							Zone = MessageHandler->GetWindowZoneForPoint( CurrentNativeEventWindow, LocalMouseX, LocalMouseY );
// 						}
// 						else
// 						{
// 							// Default to client area so that we are able to see the feedback effect when attempting to click on a non-modal window when a modal window is active
// 							// Any other window zones could have side effects and NotInWindow prevents the feedback effect.
// 							Zone = EWindowZone::ClientArea;
// 						}
// 
// 						static const LRESULT Results[] = {HTNOWHERE, HTTOPLEFT, HTTOP, HTTOPRIGHT, HTLEFT, HTCLIENT,
// 							HTRIGHT, HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT,
// 							HTCAPTION, HTMINBUTTON, HTMAXBUTTON, HTCLOSE, HTSYSMENU};
// 
// 						return Results[Zone];
// 					}
// 				}
			}
			break;

			// Window focus and activation
		case WM_ACTIVATE:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			}
			break;

		case WM_ACTIVATEAPP:
			{
				// When window activation changes we are not in a modal size loop
				bInModalSizeLoop = false;
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
			}
			break;

		case WM_PAINT:
			{
				if( bInModalSizeLoop)//TODO: do we need this?// && IsInGameThread() )
				{
					//Fix message handler
					//MessageHandler->OnOSPaint(CurrentNativeEventWindowPtr.ToSharedRef() );
				}
			}
			break;

		case WM_ERASEBKGND:
			{
				// Intercept background erasing to eliminate flicker.
				// Return non-zero to indicate that we'll handle the erasing ourselves
				return 1;
			}
			break;

		case WM_NCACTIVATE:
			{
				if( !CurrentNativeEventWindow->GetDefinition().HasOSWindowBorder )
				{
					// Unless using the OS window border, intercept calls to prevent non-client area drawing a border upon activation or deactivation
					// Return true to ensure standard activation happens
					return true;
				}
			}
			break;

		case WM_NCPAINT:
			{
				if( !CurrentNativeEventWindow->GetDefinition().HasOSWindowBorder )
				{
					// Unless using the OS window border, intercept calls to draw the non-client area - we do this ourselves
					return 0;
				}
			}
			break;

		case WM_DESTROY:
			{
				//TODO: fix window removal. URGENT!!
				//Windows.Remove( CurrentNativeEventWindow );
				return 0;
			}
			break;

		case WM_CLOSE:
			{
				DeferMessage( CurrentNativeEventWindowPtr, hwnd, msg, wParam, lParam );
				return 0;
			}
			break;

		case WM_SYSCOMMAND:
			{
				switch( wParam & 0xfff0 )
				{
				case SC_RESTORE:
					// Checks to see if the window is minimized.
					if( IsIconic(hwnd) )
					{
						// This is required for restoring a minimized fullscreen window
						::ShowWindow(hwnd,SW_RESTORE);
						return 0;
					}
					else
					{
						//TODO: fix message handler
// 						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::Restore))
// 						{
// 							return 1;
// 						}
					}
					break;
				case SC_MAXIMIZE:
					{
// 						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::Maximize))
// 						{
// 							return 1;
// 						}
					}
					break;
				default:
// 					if( !( MessageHandler->ShouldProcessUserInputMessages( CurrentNativeEventWindow ) && IsInputMessage( msg ) ) )
// 					{
// 						return 0;
// 					}
					break;
				}
			}
			break;
			
		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
			{
				switch( wParam )
				{
				case HTMINBUTTON:
					{
// 						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::ClickedNonClientArea))
// 						{
// 							return 1;
// 						}
					}
					break;
				case HTMAXBUTTON:
					{
// 						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::ClickedNonClientArea))
// 						{
// 							return 1;
// 						}
					}
					break;
				case HTCLOSE:
					{
// 						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::ClickedNonClientArea))
// 						{
// 							return 1;
// 						}
					}
					break;
				case HTCAPTION:
					{
// 						if(!MessageHandler->OnWindowAction( CurrentNativeEventWindow, EWindowAction::ClickedNonClientArea))
// 						{
// 							return 1;
// 						}
					}
					break;
				}
			}
			break;

		case WM_GETDLGCODE:
			{
				// Slate wants all keys and messages.
				return DLGC_WANTALLKEYS;
			}
			break;
		
		case WM_CREATE:
			{
				return 0;
			}
			break;

		case WM_DEVICECHANGE:
			{
				//TODO: fix for xinput
				//XInput->SetNeedsControllerStateUpdate(); 
			}
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void WindowsApplication::CheckForShiftUpEvents(const int32 KeyCode)
{
	// Since VK_SHIFT doesn't get an up message if the other shift key is held we need to poll for it
	//TODO: Fix VK_SHIFT up
	// 	if (PressedModifierKeys.Contains(KeyCode) && ((::GetKeyState(KeyCode) & 0x8000) == 0) )
// 	{
// 		PressedModifierKeys.Remove(KeyCode);
// 		MessageHandler->OnKeyUp( KeyCode, 0, false );
// 	}
}

int32 WindowsApplication::ProcessDeferredMessage( const FDeferredWindowsMessage& DeferredMessage )
{
	if ( Windows.size() && !DeferredMessage.NativeWindow.expired() )
	{
		HWND hwnd = DeferredMessage.hWND;
		uint32 msg = DeferredMessage.Message;
		WPARAM wParam = DeferredMessage.wParam;
		LPARAM lParam = DeferredMessage.lParam;

		std::shared_ptr< WindowsWindow > CurrentNativeEventWindowPtr = DeferredMessage.NativeWindow.lock();

		// This effectively disables a window without actually disabling it natively with the OS.
		// This allows us to continue receiving messages for it.
		//TODO: fix for message handler
// 		if ( !MessageHandler->ShouldProcessUserInputMessages( CurrentNativeEventWindowPtr ) && IsInputMessage( msg ) )
// 		{
// 			return 0;	// consume input messages
// 		}

		switch(msg)
		{
		case WM_INPUTLANGCHANGEREQUEST:
		case WM_INPUTLANGCHANGE:
		case WM_IME_SETCONTEXT:
		case WM_IME_NOTIFY:
		case WM_IME_REQUEST:
		case WM_IME_STARTCOMPOSITION:
		case WM_IME_COMPOSITION:
		case WM_IME_ENDCOMPOSITION:
		case WM_IME_CHAR:
			{
				//TODO: fix text input method system
// 				if(TextInputMethodSystem.IsValid())
// 				{
// 					TextInputMethodSystem->ProcessMessage(hwnd, msg, wParam, lParam);
// 				}
				return 0;
			}
			break;
			// Character
		case WM_CHAR:
			{
				// Character code is stored in WPARAM
				const TCHAR Character = wParam;

				// LPARAM bit 30 will be ZERO for new presses, or ONE if this is a repeat
				const bool bIsRepeat = ( lParam & 0x40000000 ) != 0;

				//TODO: fix message handler
				//MessageHandler->OnKeyChar( Character, bIsRepeat );

				// Note: always return 0 to handle the message.  Win32 beeps if WM_CHAR is not handled...
				return 0;
			}
			break;


			// Key down
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			{
				// Character code is stored in WPARAM
				const int32 Win32Key = wParam;

				// The actual key to use.  Some keys will be translated into other keys. 
				// I.E VK_CONTROL will be translated to either VK_LCONTROL or VK_RCONTROL as these
				// keys are never sent on their own
				int32 ActualKey = Win32Key;

				// LPARAM bit 30 will be ZERO for new presses, or ONE if this is a repeat
				bool bIsRepeat = ( lParam & 0x40000000 ) != 0;

				switch( Win32Key )
				{
				case VK_MENU:
					// Differentiate between left and right alt
					if( (lParam & 0x1000000) == 0 )
					{
						ActualKey = VK_LMENU;
						//TODO: fix macros for checking modifier keys
// 						if ( (bIsRepeat = PressedModifierKeys.Contains( VK_LMENU )) == false)
// 						{
// 							PressedModifierKeys.Add( VK_LMENU );
// 						}
					}
					else
					{
						ActualKey = VK_RMENU;
// 						if ( (bIsRepeat = PressedModifierKeys.Contains( VK_RMENU )) == false)
// 						{
// 							PressedModifierKeys.Add( VK_RMENU );
// 						}
					}
					break;
				case VK_CONTROL:
					// Differentiate between left and right control
					if( (lParam & 0x1000000) == 0 )
					{
						ActualKey = VK_LCONTROL;
// 						if ( (bIsRepeat = PressedModifierKeys.Contains( VK_LCONTROL )) == false)
// 						{
// 							PressedModifierKeys.Add( VK_LCONTROL );
// 						}
					}
					else
					{
						ActualKey = VK_RCONTROL;
// 						if ( (bIsRepeat = PressedModifierKeys.Contains( VK_RCONTROL )) == false)
// 						{
// 							PressedModifierKeys.Add( VK_RCONTROL );
// 						}
					}

					break;
				case VK_SHIFT:
					// Differentiate between left and right shift
					ActualKey = MapVirtualKey( (lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
// 					if ( (bIsRepeat = PressedModifierKeys.Contains( ActualKey )) == false)
// 					{
// 						PressedModifierKeys.Add( ActualKey );
// 					}
					break;
				default:
					// No translation needed
					break;
				}

				// Get the character code from the virtual key pressed.  If 0, no translation from virtual key to character exists
				uint32 CharCode = ::MapVirtualKey( Win32Key, MAPVK_VK_TO_CHAR );

				//TODO: fix message handler and value of result
				const bool Result = 0; //MessageHandler->OnKeyDown( ActualKey, CharCode, bIsRepeat );

				// Always return 0 to handle the message or else windows will beep
				if( Result || msg != WM_SYSKEYDOWN )
				{
					// Handled
					return 0;
				}
			}
			break;


			// Key up
		case WM_SYSKEYUP:
		case WM_KEYUP:
			{
				// Character code is stored in WPARAM
				int32 Win32Key = wParam;

				// The actual key to use.  Some keys will be translated into other keys. 
				// I.E VK_CONTROL will be translated to either VK_LCONTROL or VK_RCONTROL as these
				// keys are never sent on their own
				int32 ActualKey = Win32Key;

				bool bModifierKeyReleased = false;
				switch( Win32Key )
				{
				case VK_MENU:
					// Differentiate between left and right alt
					if( (lParam & 0x1000000) == 0 )
					{
						ActualKey = VK_LMENU;
					}
					else
					{
						ActualKey = VK_RMENU;
					}
					//TODO: fix remove macros 
					//PressedModifierKeys.Remove( ActualKey );
					break;
				case VK_CONTROL:
					// Differentiate between left and right control
					if( (lParam & 0x1000000) == 0 )
					{
						ActualKey = VK_LCONTROL;
					}
					else
					{
						ActualKey = VK_RCONTROL;
					}
					//PressedModifierKeys.Remove( ActualKey );
					break;
				case VK_SHIFT:
					// Differentiate between left and right shift
					ActualKey = MapVirtualKey( (lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
					//PressedModifierKeys.Remove( ActualKey );
					break;
				default:
					// No translation needed
					break;
				}

				// Get the character code from the virtual key pressed.  If 0, no translation from virtual key to character exists
				uint32 CharCode = ::MapVirtualKey( Win32Key, MAPVK_VK_TO_CHAR );

				// Key up events are never repeats
				const bool bIsRepeat = false;

				//TODO: fix result value
				const bool Result = 0; // MessageHandler->OnKeyUp(ActualKey, CharCode, bIsRepeat);

				// Note that we allow system keys to pass through to DefWndProc here, so that core features
				// like Alt+F4 to close a window work.
				if( Result || msg != WM_SYSKEYUP )
				{
					// Handled
					return 0;
				}
			}
			break;

			// Mouse Button Down
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
			{
				if( msg == WM_LBUTTONDOWN )
				{
					//TODO: fix message handler
					//MessageHandler->OnMouseDown( CurrentNativeEventWindowPtr, EMouseButtons::Left );
				}
				else
				{
					//MessageHandler->OnMouseDoubleClick( CurrentNativeEventWindowPtr, EMouseButtons::Left );
				}
				return 0;
			}
			break;

		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
			{
				if( msg == WM_MBUTTONDOWN )
				{
					//MessageHandler->OnMouseDown( CurrentNativeEventWindowPtr, EMouseButtons::Middle );
				}
				else
				{
					//MessageHandler->OnMouseDoubleClick( CurrentNativeEventWindowPtr, EMouseButtons::Middle );
				}
				return 0;
			}
			break;

		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			{
				if( msg == WM_RBUTTONDOWN )
				{
					//MessageHandler->OnMouseDown( CurrentNativeEventWindowPtr, EMouseButtons::Right );
				}
				else
				{
					//MessageHandler->OnMouseDoubleClick( CurrentNativeEventWindowPtr, EMouseButtons::Right );
				}
				return 0;
			}
			break;

		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
			{
				//EMouseButtons::Type MouseButton = ( HIWORD(wParam) & XBUTTON1 ) ? EMouseButtons::Thumb01  : EMouseButtons::Thumb02;

				BOOL Result = false;
				if( msg == WM_XBUTTONDOWN )
				{
					//Result = MessageHandler->OnMouseDown( CurrentNativeEventWindowPtr, MouseButton );
				}
				else
				{
					//Result = MessageHandler->OnMouseDoubleClick( CurrentNativeEventWindowPtr, MouseButton );
				}

				return Result ? 0 : 1;
			}
			break;

			// Mouse Button Up
		case WM_LBUTTONUP:
			{
				//return MessageHandler->OnMouseUp( EMouseButtons::Left ) ? 0 : 1;
			}
			break;

		case WM_MBUTTONUP:
			{
				//return MessageHandler->OnMouseUp( EMouseButtons::Middle ) ? 0 : 1;
			}
			break;

		case WM_RBUTTONUP:
			{
				//return MessageHandler->OnMouseUp( EMouseButtons::Right ) ? 0 : 1;
			}
			break;

		case WM_XBUTTONUP:
			{
				//EMouseButtons::Type MouseButton = ( HIWORD(wParam) & XBUTTON1 ) ? EMouseButtons::Thumb01  : EMouseButtons::Thumb02;
				//return MessageHandler->OnMouseUp( MouseButton ) ? 0 : 1;
			}
			break;

		// Mouse Movement
		case WM_INPUT:
			{
				if( DeferredMessage.RawInputFlags == MOUSE_MOVE_RELATIVE )
				{
					//MessageHandler->OnRawMouseMove( DeferredMessage.X, DeferredMessage.Y );
				}
				else
				{
					// Absolute coordinates given through raw input are simulated using MouseMove to get relative coordinates
					//MessageHandler->OnMouseMove();
				}

				return 0;
			}
			break;

		// Mouse Movement
		case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
			{
				BOOL Result = false;
				if( !bUsingHighPrecisionMouseInput )
				{
					Result = 0;// MessageHandler->OnMouseMove();
				}

				return Result ? 0 : 1;
			}
			break;
			// Mouse Wheel
		case WM_MOUSEWHEEL:
			{
				const float SpinFactor = 1 / 120.0f;
				const SHORT WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

				const BOOL Result = 0;// MessageHandler->OnMouseWheel(static_cast<float>(WheelDelta)* SpinFactor);
				return Result ? 0 : 1;
			}
			break;

			// Mouse Cursor
		case WM_SETCURSOR:
			{
				return 0;// MessageHandler->OnCursorSet() ? 1 : 0;
			}
			break;

			// Window focus and activation
		case WM_ACTIVATE:
			{
				//TODO: fix windows activation
				//EWindowActivation::Type ActivationType;

				if (LOWORD(wParam) & WA_ACTIVE)
				{
					//ActivationType = EWindowActivation::Activate;
				}
				else if (LOWORD(wParam) & WA_CLICKACTIVE)
				{
					//ActivationType = EWindowActivation::ActivateByMouse;
				}
				else
				{
					//ActivationType = EWindowActivation::Deactivate;
				}

				if ( CurrentNativeEventWindowPtr )
				{
					BOOL Result = false;
					Result = 0; //MessageHandler->OnWindowActivationChanged( CurrentNativeEventWindowPtr.ToSharedRef(), ActivationType );
					return Result ? 0 : 1;
				}

				return 1;
			}
			break;

		case WM_ACTIVATEAPP:
			//MessageHandler->OnApplicationActivationChanged( !!wParam );
			break;

	
		case WM_NCACTIVATE:
			{
				if( CurrentNativeEventWindowPtr && !CurrentNativeEventWindowPtr->GetDefinition().HasOSWindowBorder )
				{
					// Unless using the OS window border, intercept calls to prevent non-client area drawing a border upon activation or deactivation
					// Return true to ensure standard activation happens
					return true;
				}
			}
			break;

		case WM_NCPAINT:
			{
				if( CurrentNativeEventWindowPtr && !CurrentNativeEventWindowPtr->GetDefinition().HasOSWindowBorder )
				{
					// Unless using the OS window border, intercept calls to draw the non-client area - we do this ourselves
					return 0;
				}
			}
			break;

		case WM_CLOSE:
			{
				if ( CurrentNativeEventWindowPtr )
				{
					// Called when the OS close button is pressed
					//MessageHandler->OnWindowClose( CurrentNativeEventWindowPtr.ToSharedRef() );
				}
				return 0;
			}
			break;

		case WM_SHOWWINDOW:
			{
				if( CurrentNativeEventWindowPtr )
				{
					switch(lParam)
					{
					case SW_PARENTCLOSING:
						CurrentNativeEventWindowPtr->OnParentWindowMinimized();
						break;
					case SW_PARENTOPENING:
						CurrentNativeEventWindowPtr->OnParentWindowRestored();
						break;
					default:
						break;
					}
				}
			}
			break;

		case WM_SIZE:
			{
				if( CurrentNativeEventWindowPtr )
				{
					// @todo Fullscreen - Perform deferred resize
					// Note WM_SIZE provides the client dimension which is not equal to the window dimension if there is a windows border 
					const int32 NewWidth = (int)(short)(LOWORD(lParam));
					const int32 NewHeight = (int)(short)(HIWORD(lParam));

					const GenericWindowDefinition& Definition = CurrentNativeEventWindowPtr->GetDefinition();
					if ( Definition.IsRegularWindow && !Definition.HasOSWindowBorder )
					{
						CurrentNativeEventWindowPtr->AdjustWindowRegion(NewWidth, NewHeight);
					}

					const bool bWasMinimized = (wParam == SIZE_MINIMIZED);

					const bool Result = 0; //MessageHandler->OnSizeChanged( CurrentNativeEventWindowPtr.ToSharedRef(), NewWidth, NewHeight, bWasMinimized );
				}
			}
			break;
		case WM_SIZING:
			{
				if( CurrentNativeEventWindowPtr )
				{
					//MessageHandler->OnResizingWindow( CurrentNativeEventWindowPtr.ToSharedRef() );
				}
			}
			break;
		case WM_ENTERSIZEMOVE:
			{
				if( CurrentNativeEventWindowPtr )
				{
					//MessageHandler->BeginReshapingWindow( CurrentNativeEventWindowPtr.ToSharedRef() );
				}
			}
			break;
		case WM_EXITSIZEMOVE:
			{
				if( CurrentNativeEventWindowPtr )
				{
					//MessageHandler->FinishedReshapingWindow( CurrentNativeEventWindowPtr.ToSharedRef() );
				}
			}
			break;
		}
	}

	return 0;
}

void WindowsApplication::ProcessDeferredDragDropOperation(const FDeferredWindowsDragDropOperation& Op)
{
	// Since we deferred the drag/drop event, we could not specify the correct cursor effect in time. Now we will just throw away the value.
	DWORD DummyCursorEffect = 0;

	switch (Op.OperationType)
	{
		case EWindowsDragDropOperationType::DragEnter:
			OnOLEDragEnter(Op.HWnd, Op.OLEData, Op.KeyState, Op.CursorPosition, &DummyCursorEffect);
			break;
		case EWindowsDragDropOperationType::DragOver:
			OnOLEDragOver(Op.HWnd, Op.KeyState, Op.CursorPosition, &DummyCursorEffect);
			break;
		case EWindowsDragDropOperationType::DragLeave:
			OnOLEDragOut(Op.HWnd);
			break;
		case EWindowsDragDropOperationType::Drop:
			OnOLEDrop(Op.HWnd, Op.OLEData, Op.KeyState, Op.CursorPosition, &DummyCursorEffect);
			break;
		default:
			//TODO: fix ensure
			//ensureMsgf(0, TEXT("Unhandled deferred drag/drop operation type: %d"), Op.OperationType);
			break;
	}
}

bool WindowsApplication::IsInputMessage( uint32 msg )
{
	switch(msg)
	{
	// Keyboard input notification messages...
	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_SYSCOMMAND:
	// Mouse input notification messages...
	case WM_MOUSEHWHEEL:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHOVER:
	case WM_MOUSELEAVE:
	case WM_MOUSEMOVE:
	case WM_NCMOUSEHOVER:
	case WM_NCMOUSELEAVE:
	case WM_NCMOUSEMOVE:
	case WM_NCMBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCXBUTTONDBLCLK:
	case WM_NCXBUTTONDOWN:
	case WM_NCXBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_XBUTTONDBLCLK:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	// Raw input notification messages...
	case WM_INPUT:
	case WM_INPUT_DEVICE_CHANGE:
		return true;
	}
	return false;
}

void WindowsApplication::DeferMessage(std::shared_ptr <WindowsWindow>& NativeWindow, HWND InHWnd, uint32 InMessage, WPARAM InWParam, LPARAM InLParam, int32 MouseX, int32 MouseY, uint32 RawInputFlags)
{
	if( true ) //TODO: FIX defer message conditions //GPumpingMessagesOutsideOfMainLoop && bAllowedToDeferMessageProcessing )
	{
		DeferredMessages.push_back( FDeferredWindowsMessage( NativeWindow, InHWnd, InMessage, InWParam, InLParam, MouseX, MouseY, RawInputFlags ) );
	}
	else
	{
		// When not deferring messages, process them immediately
		ProcessDeferredMessage( FDeferredWindowsMessage( NativeWindow, InHWnd, InMessage, InWParam, InLParam, MouseX, MouseY, RawInputFlags ) );
	}
}

void WindowsApplication::PumpMessages( const float TimeDelta )
{
	MSG Message;

	// standard Windows message handling
	while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
	{ 
		TranslateMessage(&Message);
		DispatchMessage(&Message); 
	}
}

void WindowsApplication::ProcessDeferredEvents( const float TimeDelta )
{
	// Process windows messages
	{
		// This function can be reentered when entering a modal tick loop.
		// We need to make a copy of the events that need to be processed or we may end up processing the same messages twice 
		std::vector<FDeferredWindowsMessage> EventsToProcess( DeferredMessages );

		DeferredMessages.clear();
		for( int32 MessageIndex = 0; MessageIndex < EventsToProcess.size(); ++MessageIndex )
		{
			const FDeferredWindowsMessage& DeferredMessage = EventsToProcess[MessageIndex];
			ProcessDeferredMessage( DeferredMessage );
		}

		CheckForShiftUpEvents(VK_LSHIFT);
		CheckForShiftUpEvents(VK_RSHIFT);
	}

	// Process drag/drop operations
	{
		std::vector<FDeferredWindowsDragDropOperation> DragDropOperationsToProcess(DeferredDragDropOperations);

		DeferredDragDropOperations.clear();
		for (int32 OperationIndex = 0; OperationIndex < DragDropOperationsToProcess.size(); ++OperationIndex)
		{
			const FDeferredWindowsDragDropOperation& DeferredDragDropOperation = DragDropOperationsToProcess[OperationIndex];
			ProcessDeferredDragDropOperation(DeferredDragDropOperation);
		}
	}
}

void WindowsApplication::PollGameDeviceState( const float TimeDelta )
{
	//TODO: FIX pull game device state
	// initialize any externally-implemented input devices (we delay load initialize the array so any plugins have had time to load)
// 	if (!bHasLoadedInputPlugins)
// 	{
// 		std::vector<IInputDeviceModule*> PluginImplementations = IModularFeatures::Get().GetModularFeatureImplementations<IInputDeviceModule>( IInputDeviceModule::GetModularFeatureName() );
// 		for( auto InputPluginIt = PluginImplementations.CreateIterator(); InputPluginIt; ++InputPluginIt )
// 		{
// 			TSharedPtr<IInputDevice> Device = (*InputPluginIt)->CreateInputDevice(MessageHandler);
// 			if ( Device.IsValid() )
// 			{
// 				ExternalInputDevices.Add(Device);
// 			}
// 		}
// 
// 		bHasLoadedInputPlugins = true;
// 	}
// 
// 	// Poll game device states and send new events
// 	XInput->SendControllerEvents();
// 
// 	// Poll externally-implemented devices
// 	for( auto DeviceIt = ExternalInputDevices.CreateIterator(); DeviceIt; ++DeviceIt )
// 	{
// 		(*DeviceIt)->Tick( TimeDelta );
// 		(*DeviceIt)->SendControllerEvents();
// 	}
}

// TODO: Fix force feedback system
// void FWindowsApplication::SetChannelValue (int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value)
// {
// 	// send vibration to externally-implemented devices
// 	for( auto DeviceIt = ExternalInputDevices.CreateIterator(); DeviceIt; ++DeviceIt )
// 	{
// 		(*DeviceIt)->SetChannelValue(ControllerId, ChannelType, Value);
// 	}
// }
// 
// void FWindowsApplication::SetChannelValues (int32 ControllerId, const FForceFeedbackValues &Values)
// {
// 	// send vibration to externally-implemented devices
// 	for( auto DeviceIt = ExternalInputDevices.CreateIterator(); DeviceIt; ++DeviceIt )
// 	{
// 		(*DeviceIt)->SetChannelValues(ControllerId, Values);
// 	}
// }

void WindowsApplication::DeferDragDropOperation(const FDeferredWindowsDragDropOperation& DeferredDragDropOperation)
{
	DeferredDragDropOperations.push_back(DeferredDragDropOperation);
}

HRESULT WindowsApplication::OnOLEDragEnter( const HWND HWnd, const DragDropOLEData& OLEData, DWORD KeyState, POINTL CursorPosition, DWORD *CursorEffect)
{
	const std::shared_ptr < WindowsWindow > Window = FindWindowByHWND(Windows, HWnd);

	if ( !Window )
	{
		return 0;
	}

	switch (OLEData.Type)
	{
		case DragDropOLEData::Text:
			//TODO: fix drag and drop
			//*CursorEffect = MessageHandler->OnDragEnterText(Window.ToSharedRef(), OLEData.OperationText);
			break;
		case DragDropOLEData::Files:
			//*CursorEffect = MessageHandler->OnDragEnterFiles(Window.ToSharedRef(), OLEData.OperationFilenames);
			break;
		case DragDropOLEData::None:
		default:
			break;
	}

	return 0;
}

HRESULT WindowsApplication::OnOLEDragOver( const HWND HWnd, DWORD KeyState, POINTL CursorPosition, DWORD *CursorEffect)
{
	const std::shared_ptr < WindowsWindow > Window = FindWindowByHWND(Windows, HWnd);

	if ( Window )
	{
		//*CursorEffect = MessageHandler->OnDragOver( Window.ToSharedRef() );
	}

	return 0;
}

HRESULT WindowsApplication::OnOLEDragOut( const HWND HWnd )
{
	const std::shared_ptr < WindowsWindow > Window = FindWindowByHWND(Windows, HWnd);

	if ( Window )
	{
		// User dragged out of a Slate window. We must tell Slate it is no longer in drag and drop mode.
		// Note that this also gets triggered when the user hits ESC to cancel a drag and drop.
		//MessageHandler->OnDragLeave( Window.ToSharedRef() );
	}

	return 0;
}

HRESULT WindowsApplication::OnOLEDrop( const HWND HWnd, const DragDropOLEData& OLEData, DWORD KeyState, POINTL CursorPosition, DWORD *CursorEffect)
{
	const std::shared_ptr < WindowsWindow > Window = FindWindowByHWND(Windows, HWnd);

	if ( Window )
	{
		//*CursorEffect = MessageHandler->OnDragDrop( Window.ToSharedRef() );
	}

	return 0;
}
