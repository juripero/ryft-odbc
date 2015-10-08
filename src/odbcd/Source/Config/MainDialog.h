// =================================================================================================
///  @file MainDialog.h
///
///  Definition of the Class MainDialog.
///
///  Copyright (C) 2009-2011 Simba Technologies Incorporated
// =================================================================================================

#ifndef _SIMBA_R1TRALIGHT_CONFIG_MAINDIALOG_H_
#define _SIMBA_R1TRALIGHT_CONFIG_MAINDIALOG_H_

#include "Simba.h"

namespace Simba
{
namespace RyftOne
{
    class ConfigSettings;
}
}

namespace Simba
{
namespace RyftOne
{
    /// @brief This class implements the main dialog for the DSN configuration.
    class MainDialog
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        MainDialog();

        /// @brief This function is passed to the Win32 dialog functions as a callback, and will 
        /// call out to the appropriate Do*Action functions.
        ///
        /// @param hwndDlg              The window handle of the dialog.
        /// @param message              Used to control the window action.
        /// @param wParam               Specifies the type of action the user performed.
        /// @param lParam               Specifies additional message-specific information.
        static INT_PTR ActionProc(
            HWND hwndDlg,
            UINT message,
            WPARAM wParam,
            LPARAM lParam);

        /// @brief Allow the dialog to clean up any resources that it needs to.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void Dispose(HWND in_dialogHandle);

        /// @brief Display the dialog box on the screen
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_moduleHandle      The handle of the module.
        /// @param in_configSettings    The settings modified by the dialog.
		/// @param edit_DSN				True if the DSN name should be editable
        ///
        /// @return Returns true if the user OKed the dialog; false, if they cancelled it.
        static bool Show(
            HWND in_parentWindow, 
            simba_handle in_moduleHandle,
            ConfigSettings& in_configSettings,
            bool edit_DSN);
        
    // Private =====================================================================================
    private:
        /// @brief Initialize all of the components of the dialog.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_configSettings    The settings for this dialog.
        static void Initialize(HWND in_dialogHandle, ConfigSettings* in_configSettings);

        /// @brief Action taken when users click on the cancel button.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void DoCancelAction(HWND in_dialogHandle);

        /// @brief Action taken when users click on the ok button.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_configSettings    The settings for this dialog.
        static void DoOkAction(HWND in_dialogHandle, ConfigSettings* in_configSettings);

        /// @brief Action taken when users click on the brows button.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void DoBrowseAction(HWND in_dialogHandle);

        /// @brief Enable or disable the OK button based on the contents of the edit controls.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void CheckEnableOK(HWND in_dialogHandle);

        /// @brief Centers the dialog in the screen.
        ///
        /// @param in_dialogHandle      The handle to the dialog to be centered.
        static void CenterDialog(HWND in_dialogHandle);

        /// @brief Fetches the trimmed text from an Edit component.
        ///
        /// @param in_component         The component identifier to get the text from.
        /// @param in_dialogHandle      The handle to the dialog.
        ///
        /// @return The text from the specified component.
        static simba_wstring GetEditText(simba_int32 in_component, HWND in_dialogHandle);

        /// @brief Trim a string of the indicated characters.
        ///
        /// @param in_string                The string to trim.
        /// @param in_what                  The characters to trim from the beginning and end of 
        ///                                 the string.
        ///
        /// @return A new trimmed string.
        static std::wstring Trim(const std::wstring& in_string, const std::wstring& in_what);
    };
}
}

#endif
