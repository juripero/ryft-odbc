//==================================================================================================
///  @file R1MainDialog.h
///
///  Definition of the class R1MainDialog.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#ifndef _RYFTONE_R1MAINDIALOG_H_
#define _RYFTONE_R1MAINDIALOG_H_

#include "R1DSNConfiguration.h"
#include "R1BaseDialog.h"
#include "R1TestResultDialog.h"

namespace RyftOne
{
    struct DialogSettings
    {
        // The settings modified by the dialog.
        R1DSNConfiguration& m_configSettings;

        // Flag indicating only required fields should be enabled.
        bool m_onlyRequired;

        /// @brief Constructor.
        /// 
        /// @param in_configSettings    The settings modified by the dialog.
        /// @param in_onlyRequired      Flag indicating only required fields should be enabled.
        ///
        /// @return Returns true if the user OKed the dialog; false, if they cancelled it.
        DialogSettings(R1DSNConfiguration& in_configSettings, bool in_onlyRequired) :
            m_configSettings(in_configSettings),
            m_onlyRequired(in_onlyRequired)
        {
            ; // Do nothing.
        }

    // Private =====================================================================================
    private:
        /// @brief Disabled assignment operator.
        ///
        /// @param in_object        The object to copy.
        ///
        /// @return The reference to the copied object.
        DialogSettings& operator=(const DialogSettings& in_object);
    };

    /// @brief This class encapsulates the functionality of the DSN configuration dialog.
    class R1MainDialog : public R1BaseDialog
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        R1MainDialog() {}

        /// @brief Pops up the dialog. 
        /// 
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_configSettings    The settings modified by the dialog.
        /// @param in_onlyRequired      Flag indicating only required fields should be enabled.
        ///
        /// @return Returns true if the user OKed the dialog; false, if they cancelled it.
        static bool Show(
            HWND in_parentWindow, 
            R1DSNConfiguration& in_configSettings,
            bool in_onlyRequired = false);

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

        /// @brief Action taken when users click on the cancel button.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void DoCancelAction(HWND in_dialogHandle);

        /// @brief Show the Logging dialog.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void DoLoggingDialogAction(HWND in_dialogHandle);

        /// @brief Action taken when users click on the ok button.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void DoOkAction(HWND in_dialogHandle);

        /// @brief Test the connection represented by the dialog.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        static void DoTestAction(HWND in_dialogHandle);

        /// @brief Initialize all of the components of the dialog.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_lParam            Specifies additional message-specific information.
        static void Initialize(HWND in_dialogHandle, LPARAM in_lParam);

        /// @brief Initializes all of the static text of the dialog.
        ///
        /// @param in_dialogHandle      The window handle of the dialog.
        /// @param in_isConnDialog      Flag indicating if this should load as a connection dialog.
        static void InitializeText(HWND in_dialogHandle, bool in_isConnDialog);

        /// @brief Save the dialog settings into the configuration.
        ///
        /// @param in_dialogHandle      The handle to the dialog that created in_dialog.
        /// @param in_savePassword      Flag to indicate the password should be saved.
        static void SaveSettings(HWND in_dialogHandle, bool in_savePassword);
    };
}

#endif
