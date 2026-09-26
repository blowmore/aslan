#define WIN32_LEAN_AND_MEAN

#include "dialog.hpp"
#include "resource.h"

#include <windows.h>

#include <optional>
#include <string>
#include <vector>

namespace
{
    struct Choice
    {
        std::size_t index = 0;
        std::wstring label;
    };

    struct State
    {
        std::wstring message;
        std::vector<Choice> choices;
        std::optional<std::size_t> selected;
        Settings settings;
        std::optional<std::size_t> preferred_installation;
    };

    std::wstring get_label(const Installation& installation)
    {
        return installation.version.empty() ? L"(unknown version)" : installation.version;
    }

    State* get_state(HWND hwnd)
    {
        return reinterpret_cast<State*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    }

    void populate(
        HWND combo,
        const std::vector<Choice>& choices,
        std::optional<std::size_t> preferred_installation)
    {
        for (const Choice& choice : choices)
        {
            const LRESULT item = SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(choice.label.c_str()));
            if ((item != CB_ERR) && (item != CB_ERRSPACE))
            {
                SendMessageW(combo, CB_SETITEMDATA, item, static_cast<LPARAM>(choice.index));
            }
        }

        if (!choices.empty())
        {
            std::size_t selected = choices.size() - 1;
            if (preferred_installation)
            {
                for (std::size_t index = 0; index < choices.size(); ++index)
                {
                    if (choices[index].index == *preferred_installation)
                    {
                        selected = index;
                        break;
                    }
                }
            }

            SendMessageW(combo, CB_SETCURSEL, selected, 0);
        }
    }

    void accept(HWND hwnd, State& state)
    {
        const HWND combo = GetDlgItem(hwnd, IDC_DIALOG_COMBO);
        const LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
        if (selected != CB_ERR)
        {
            const LRESULT data = SendMessageW(combo, CB_GETITEMDATA, selected, 0);
            if (data != CB_ERR)
            {
                state.selected = static_cast<std::size_t>(data);
            }
        }

        state.settings.clear_user_state =
            IsDlgButtonChecked(hwnd, IDC_DIALOG_CLEAR_STATE) == BST_CHECKED;

        EndDialog(hwnd, IDOK);
    }

    INT_PTR CALLBACK process(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
    {
        State* state = nullptr;
        switch (message)
        {
            case WM_INITDIALOG:
                state = reinterpret_cast<State*>(lparam);
                SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
                SendDlgItemMessageW(
                    hwnd,
                    IDC_DIALOG_ICON,
                    STM_SETICON,
                    reinterpret_cast<WPARAM>(LoadIconW(
                        nullptr,
                        reinterpret_cast<LPCWSTR>(IDI_WARNING))),
                    0);
                SetDlgItemTextW(hwnd, IDC_DIALOG_MESSAGE, state->message.c_str());
                CheckDlgButton(
                    hwnd,
                    IDC_DIALOG_CLEAR_STATE,
                    state->settings.clear_user_state ? BST_CHECKED : BST_UNCHECKED);
                populate(
                    GetDlgItem(hwnd, IDC_DIALOG_COMBO),
                    state->choices,
                    state->preferred_installation);
                return TRUE;
        
            case WM_COMMAND:
                state = get_state(hwnd);
                switch (LOWORD(wparam))
                {
                    case IDOK:
                        if (state)
                        {
                            accept(hwnd, *state);
                        }
                        return TRUE;
                    case IDCANCEL:
                        EndDialog(hwnd, IDCANCEL);
                        return TRUE;
                }
                break;

            case WM_CTLCOLORSTATIC:
                SetBkMode(reinterpret_cast<HDC>(wparam), TRANSPARENT);
                if (GetDlgCtrlID(reinterpret_cast<HWND>(lparam)) == IDC_DIALOG_FOOTER)
                {
                    SetBkMode(reinterpret_cast<HDC>(wparam), OPAQUE);
                    SetBkColor(reinterpret_cast<HDC>(wparam), GetSysColor(COLOR_BTNFACE));
                    return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
                }
                return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));

            case WM_CTLCOLORDLG:
                return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        }

        return FALSE;
    }
}

std::optional<DialogResult> dialog(
    const std::vector<Installation>& installations,
    const std::wstring& message,
    const Settings& settings,
    std::optional<std::size_t> preferred_installation)
{
    State state;
    state.message = message;
    state.settings = settings;
    state.preferred_installation = preferred_installation;

    for (std::size_t index = 0; (index < installations.size()); index++)
    {
        if (installations[index].executable_exists)
        {
            state.choices.push_back({index, get_label(installations[index])});
        }
    }

    if (state.choices.empty())
    {
        return std::nullopt;
    }

    const INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_DIALOG),
        nullptr,
        process,
        reinterpret_cast<LPARAM>(&state)
    );

    if (result != IDOK)
    {
        return std::nullopt;
    }

    if (!state.selected)
    {
        return std::nullopt;
    }

    return DialogResult{*state.selected, state.settings};
}
