
/* Copyright 2019 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

extern Kind kindWindowBase;

struct WindowBase {
    Kind kind = nullptr;

    // data that can be set before calling Create()

    // either a custom class that we registered or
    // a win32 control class. Assumed static so not freed
    const WCHAR* winClass = nullptr;

    HWND parent = nullptr;
    RECT initialPos = {};
    DWORD dwStyle = 0;
    DWORD dwExStyle = 0;
    HFONT hfont = nullptr; // TODO: this should be abstract Font description
    int menuId = 0;

    // called at start of windows proc to allow intercepting messages
    MsgFilter msgFilter;

    COLORREF textColor = ColorUnset;
    COLORREF backgroundColor = ColorUnset;
    HBRUSH backgroundColorBrush = nullptr;

    str::Str text;

    HWND hwnd = nullptr;
    UINT_PTR subclassId = 0;
    UINT_PTR subclassParentId = 0;

    WindowBase() = default;
    WindowBase(HWND p);
    virtual ~WindowBase();

    virtual bool Create();
    virtual SIZE GetIdealSize();

    virtual LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, bool& didHandle);
    virtual LRESULT WndProcParent(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, bool& didHandle);

    void Subclass();
    void SubclassParent();
    void Unsubclass();

    void SetIsEnabled(bool);
    bool IsEnabled();

    void SetIsVisible(bool);
    bool IsVisible();

    void SetFocus();
    void SetFont(HFONT f);
    void SetText(const WCHAR* s);
    void SetText(std::string_view);
    void SetPos(RECT* r);
    void SetBounds(const RECT& r);
    void SetTextColor(COLORREF);
    void SetBackgroundColor(COLORREF);
    void SetColors(COLORREF bg, COLORREF txt);
    void SetRtl(bool);
};

extern Kind kindWindow;

struct Window;

struct WindowCloseArgs {
    Window* window = nullptr;
    bool cancel = false;
};

typedef std::function<void(WindowCloseArgs*)> OnClose;

// a top-level window. Must set winClass before
// calling Create()
struct Window : public WindowBase {
    // those tweak WNDCLASSEX for RegisterClass() class
    HICON hIcon = nullptr;
    HICON hIconSm = nullptr;
    LPCWSTR lpszMenuName = nullptr;

    WmCommandCb onCommand = nullptr;
    OnSize onSize = nullptr;
    OnClose onClose = nullptr;

    Window();
    ~Window() override;

    bool Create() override;

    void SetTitle(std::string_view);
};

struct WindowBaseLayout : public ILayout {
    WindowBase* wb = nullptr;

    WindowBaseLayout(WindowBase*, Kind);
    virtual ~WindowBaseLayout();

    Size Layout(const Constraints bc) override;
    Length MinIntrinsicHeight(Length) override;
    Length MinIntrinsicWidth(Length) override;
    void SetBounds(const Rect bounds) override;
};

void HwndSetText(HWND hwnd, std::string_view s);
UINT_PTR NextSubclassId();
int RunMessageLoop(HACCEL accelTable);
