/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_wifi.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app_wifi.h"
#include "definitions.h"
#include "string.h"
#include "wdrv_pic32mzw_ps.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_WIFI_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_WIFI_DATA app_wifiData;



// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/

void WIFI_Config(void)
{
    static SYS_WIFI_CONFIG wifiSrvcConfig ;
    // Get existing config
    SYS_WIFI_CtrlMsg(sysObj.syswifi, SYS_WIFI_GETWIFICONFIG, &wifiSrvcConfig, sizeof(SYS_WIFI_CONFIG)) ;
    // Save new config

    // Set mode as STA
    wifiSrvcConfig.mode = SYS_WIFI_STA ;
    // Enable saving wifi configuration
    wifiSrvcConfig.saveConfig = true ;
    // Set country code
    strcpy((char *)wifiSrvcConfig.countryCode, (const char *)"GEN") ;
    // Set the auth type to SYS_WIFI_WPA2
    wifiSrvcConfig.staConfig.authType = SYS_WIFI_WPAWPA2MIXED ;
    // Enable all the channels(0)
    wifiSrvcConfig.staConfig.channel = 0 ;
    // Device doesn't wait for user request
    wifiSrvcConfig.staConfig.autoConnect = 1;
    // Set SSID
    memcpy(wifiSrvcConfig.staConfig.ssid, WIFI_DEV_SSID, strlen(WIFI_DEV_SSID));
    // Set PSK
    memcpy(wifiSrvcConfig.staConfig.psk, WIFI_DEV_PSK, strlen(WIFI_DEV_PSK));
    // sysObj.syswifi return from SYS_WIFI_Initialize()
    if (SYS_WIFI_OBJ_INVALID != SYS_WIFI_CtrlMsg (sysObj.syswifi, SYS_WIFI_CONNECT, &wifiSrvcConfig, sizeof(SYS_WIFI_CONFIG)))
    {
        SYS_CONSOLE_PRINT("Wi-Fi Configuration done.") ;
    }
    else
    {
        SYS_CONSOLE_PRINT("Wi-Fi Configuration failed.") ;
    }
}

// Load default config
void WIFI_LoadConfig(void)
{
    // Get existing config
    SYS_WIFI_CtrlMsg(sysObj.syswifi, SYS_WIFI_GETWIFICONFIG, &app_wifiData.wifiConfig, sizeof(SYS_WIFI_CONFIG)) ;
}

// Set new config
void WIFI_SetNewConfig(void)
{
    // Set mode as STA
    app_wifiData.wifiConfig.mode = SYS_WIFI_STA ;
    // Enable saving wifi configuration
    app_wifiData.wifiConfig.saveConfig = true ;
    // Set country code
    strcpy((char *)app_wifiData.wifiConfig.countryCode, (const char *)"GEN") ;
    // Enable all the channels(0)
    app_wifiData.wifiConfig.staConfig.channel = 0 ;
    // Device doesn't wait for user request
    app_wifiData.wifiConfig.staConfig.autoConnect = 1;

    // SSID, AUTHTYPE, PSK have been set by APP_BLE
/*    
    // Set SSID
    memcpy(app_wifiData.wifiConfig.staConfig.ssid, WIFI_DEV_SSID, strlen(WIFI_DEV_SSID)) ;
    // Set the auth type to SYS_WIFI_WPA2
    app_wifiData.wifiConfig.staConfig.authType = SYS_WIFI_WPAWPA2MIXED ;
    // Set PSK
    memcpy(app_wifiData.wifiConfig.staConfig.psk, WIFI_DEV_PSK, strlen(WIFI_DEV_PSK));
*/    
    // sysObj.syswifi return from SYS_WIFI_Initialize()
    if (SYS_WIFI_OBJ_INVALID != SYS_WIFI_CtrlMsg (sysObj.syswifi, SYS_WIFI_CONNECT, &app_wifiData.wifiConfig, sizeof(SYS_WIFI_CONFIG)))
    {
        SYS_CONSOLE_PRINT("Wi-Fi Configuration done.") ;
    }
    else
    {
        SYS_CONSOLE_PRINT("Wi-Fi Configuration failed.") ;
    }    
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_WIFI_Initialize ( void )

  Remarks:
    See prototype in app_wifi.h.
 */

void APP_WIFI_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    app_wifiData.state = APP_WIFI_STATE_INIT;
}

/******************************************************************************
  Function:
    void APP_WIFI_Tasks ( void )

  Remarks:
    See prototype in app_wifi.h.
 */

void APP_WIFI_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( app_wifiData.state )
    {
        /* Application's initial state. */
        case APP_WIFI_STATE_INIT:
        {
            if (SYS_WIFI_GetStatus(sysObj.syswifi) == SYS_WIFI_STATUS_TCPIP_READY)
            {
                WIFI_LoadConfig() ;
                app_wifiData.state = APP_WIFI_STATE_WAIT_PROVISIONING ;
            }
            break;
        }
        case APP_WIFI_STATE_WAIT_PROVISIONING:
        {
            if (app_wifiData.newWiFiConfig)
            {
                app_wifiData.newWiFiConfig = false ;
                WIFI_SetNewConfig() ;
            }
            break;
        }
        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
