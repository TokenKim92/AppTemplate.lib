#include "Resource.h"
#include "WindowDialog.h"
#include <typeinfo>

extern ApplicationCore *gp_appCore;

LRESULT CALLBACK WindowDialog::WindowProcedure(HWND ah_window, UINT a_messageID, WPARAM a_wordParam, LPARAM a_longParam)
{
    if (a_messageID == WM_NCCREATE) {
        // recover the "this" pointer which was passed as a parameter to CreateWindow(Ex) and
        // put the value in a safe place for future use
        SetWindowLongPtr(
            ah_window,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(
                reinterpret_cast<LPCREATESTRUCT>(a_longParam)->lpCreateParams
            )
        );

        return DefWindowProc(ah_window, a_messageID, a_wordParam, a_longParam);
    }
    
    // recover the "this" pointer from where our WM_NCCREATE handler stashed it.
    WindowDialog *p_dialog = reinterpret_cast<WindowDialog *>(GetWindowLongPtr(ah_window, GWLP_USERDATA));
    if (p_dialog) {
        // find message handler of the message ID
        auto handler = p_dialog->GetMessageHandler(a_messageID);
        if (handler) {
            (p_dialog->*handler)(a_wordParam, a_longParam);

            return 1;
        }
    }

    // process messages not handled by the developer
    return DefWindowProc(ah_window, a_messageID, a_wordParam, a_longParam);
}

WindowDialog::WindowDialog(const wchar_t *const ap_windowClass, const wchar_t *const ap_title)
{
    size_t length = wcslen(ap_windowClass) + 1;
    mp_windowClass = new wchar_t[length];
    wcscpy_s(mp_windowClass, length, ap_windowClass);

    if (ap_title) {
        length = wcslen(ap_title) + 1;
        mp_title = new wchar_t[length];
        wcscpy_s(mp_title, length, ap_title);
    }
    else {
        mp_title = new wchar_t[1];
        mp_title[0] = '\0';
    }

    m_showType = SW_SHOWNORMAL;

    mh_window = nullptr;
    m_messageMap[WM_DESTROY] = &WindowDialog::DestroyHandler;
    m_messageMap[WM_PAINT] = &WindowDialog::PaintHandler;

    mp_direct2d = nullptr;
}

WindowDialog::~WindowDialog()
{
    delete[] mp_windowClass;
    delete[] mp_title;

    if (mp_direct2d) {
        delete mp_direct2d;
    }
}

// window class registration
void WindowDialog::RegistWindowClass()
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProcedure;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = gp_appCore->GetHandleInstance();
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // icon of application
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = mp_windowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);
}

// Functions that handle messages issued to the application
int WindowDialog::Run()
{
    MSG message;
    while (GetMessage(&message, nullptr, 0, 0)) { // until occuring the WM_QUIT message
        TranslateMessage(&message); // check whether additional messages are generated when keyboard messages occur
        DispatchMessage(&message);  // handle the occured message
    }
    // when exiting the while statement, the WM_QUIT message is included in msg.
    // therefore, the wParam value of the WM_QUIT message is returned. 
    // The wParam value is the argument value (0) used by the PostQuitMessage function.
    return static_cast<int>(message.wParam);
}

int WindowDialog::Create()
{
    RegistWindowClass();
    if (!InitInstance()) {
        return 0;
    }

    return Run();
}

// find the message handler for a given message ID.
MessageHandler WindowDialog::GetMessageHandler(unsigned int a_messageID)
{
    auto messageEntry = m_messageMap.find(a_messageID);
    if (messageEntry == m_messageMap.end()) {
        return nullptr;
    }

    return messageEntry->second;
}

void WindowDialog::AddMessageHandler(unsigned int a_messageID, MessageHandler a_handler)
{
    m_messageMap[a_messageID] = a_handler;
}

void WindowDialog::RemoveMessageHandler(unsigned int a_messageID)
{
    m_messageMap.erase(a_messageID);
}

// create and initialize a main window
bool WindowDialog::InitInstance(int a_x, int a_y, int a_width, int a_height)
{
    HWND h_window = ::CreateWindowW(
        mp_windowClass, mp_title, WS_OVERLAPPEDWINDOW,
        a_x, a_y, a_width, a_height, 
        nullptr, nullptr, gp_appCore->GetHandleInstance(),
        this
    );

    if (h_window) {
        mh_window = h_window;

        mp_direct2d = new Direct2DEx(mh_window);
        if (S_OK == mp_direct2d->Create()) {
            OnInitDialog();

            ::ShowWindow(h_window, m_showType);
            ::UpdateWindow(h_window);

            return true;
        }

        delete mp_direct2d;
    }

    return false;
}

// to handle the WM_DESTROY message that occurs when a window is destroyed
msg_handler int WindowDialog::DestroyHandler(WPARAM a_wordParam, LPARAM a_longParam)
{
    OnDestroy();
    PostQuitMessage(0);

    return S_OK;
}

// to handle the WM_PAINT message that occurs when a window is created
msg_handler int WindowDialog::PaintHandler(WPARAM a_wordParam, LPARAM a_longParam)
{
    mp_direct2d->BeginDraw();
    OnPaint();
    mp_direct2d->EndDraw();

    return S_OK;
}

void WindowDialog::OnInitDialog()
{
    
}

void WindowDialog::OnDestroy()
{

}

void WindowDialog::OnPaint()
{

}