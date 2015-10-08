//==================================================================================================
///  @file AboutDialog.h
///
///  Definition of the class AboutDialog.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
//==================================================================================================

#ifndef _RYFT_ABOUTDIALOG_H_
#define _RYFT_ABOUTDIALOG_H_

#include "DSNConfiguration.h"
#include "BaseDialog.h"

namespace Ryft
{
    /// @brief This class encapsulates the functionality of the DSN configuration dialog.
    class AboutDialog : public BaseDialog
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        AboutDialog() {}

        /// @brief Pops up the dialog. 
        /// 
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_configSettings    The settings modified by the dialog.
        /// @param in_onlyRequired      Flag indicating only required fields should be enabled.
        ///
        /// @return Returns true if the user OKed the dialog; false, if they cancelled it.
        static bool Show(
            HWND in_parentWindow, 
            DSNConfiguration& in_configSettings);

    // Private =====================================================================================
    private:
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

        /// @brief Action taken when users click on the cancel button.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void DoCancelAction(HWND in_dialogHandle);

        /// @brief Action taken when users click on the ok button.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void DoOkAction(HWND in_dialogHandle);

        /// @brief Initialize all of the components of the dialog.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_lParam            Specifies additional message-specific information.
        static void Initialize(HWND in_dialogHandle, LPARAM in_lParam);
    };
}
#endif
