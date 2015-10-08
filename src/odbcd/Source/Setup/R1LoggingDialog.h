//==================================================================================================
///  @file R1LoggingDialog.h
///
///  Definition of the class R1LoggingDialog.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#ifndef _RYFTONE_R1LOGGINGDIALOG_H_
#define _RYFTONE_R1LOGGINGDIALOG_H_

#include "R1BaseDialog.h"
#include "R1DSNConfiguration.h"

namespace RyftOne
{
    /// @brief This class encapsulates the functionality of the logging dialog.
    class R1LoggingDialog : public R1BaseDialog
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        R1LoggingDialog() {}

        /// @brief Pops up the dialog. 
        /// 
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_configSettings    The settings modified by the dialog.
        ///
        /// @return Returns true if the user OKed the dialog; false, if they cancelled it.
        static bool Show(HWND in_parentWindow, R1DSNConfiguration& in_configSettings);

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

        /// @brief Enable or disable the buttons based on the contents of the edit controls.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void CheckEnableButtons(HWND in_dialogHandle);

        /// @brief Enable or disable the logging controls based on the contents of the edit controls.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void CheckEnableLoggingOptions(HWND in_dialogHandle);

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

        /// @brief Initializes all of the static text of the dialog.
        ///
        /// @param in_dialogHandle      The window handle of the dialog.
        static void InitializeText(HWND in_dialogHandle);
    };
}

#endif
