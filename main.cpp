#include "widget.h"

#include <QApplication>
#include <windows.h>
#include "Xfsapi.h"
#include "zf_log.h"
#include "Xfspin.h"
#define USER_INTERACTION_TIMEOUT 70000
#define ALL_PIN_KEY  WFS_PIN_FK_0 | WFS_PIN_FK_1 | WFS_PIN_FK_2 | WFS_PIN_FK_3 | WFS_PIN_FK_4 | WFS_PIN_FK_5 | WFS_PIN_FK_6 | WFS_PIN_FK_7 | WFS_PIN_FK_8 | WFS_PIN_FK_9
#define ALL_FDK WFS_PIN_FK_FDK01 | WFS_PIN_FK_FDK02 | WFS_PIN_FK_FDK03 | WFS_PIN_FK_FDK04 | WFS_PIN_FK_FDK05 | WFS_PIN_FK_FDK06  | WFS_PIN_FK_FDK07 | WFS_PIN_FK_FDK08
#define WOSA_CMD_TIMEOUT 35000
static HAPP hApp;
HSERVICE hService;
HWND hwnd;  //window handler, you need set qwidget::winId to hwnd


int initWosaStartUp();
int wfsSyncOpen(const char *logicalName, int timeout);
int wfsSyncRegister();
HRESULT wfsSyncExecute(int CMD, int timeout, LPVOID readData, WFSRESULT& wfsResult);
void handleGetData(const MSG &msg);
void checkPinKeyValue(const WFSPINKEY &pinKey, std::string pinString);
QString destCard;
int wfsSyncDeregister();
void handleExeEvent(const MSG &msg);
    QString line;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();


     hwnd = (HWND)w.winId(); // your main window from qt
    initWosaStartUp();

    const char *logicalName= "Encryptor";//logical name of device
    int timeout =USER_INTERACTION_TIMEOUT;

    if( wfsSyncOpen(logicalName,timeout) ==0){

     HRESULT r ;
     WFSRESULT wfsResult;
     r =wfsSyncRegister();
     bool pass =false;
     if(r == WFS_SUCCESS){
              WFSRESULT wfsResult;

         if(!pass){

         ulong activePin = ALL_PIN_KEY | WFS_PIN_FK_CANCEL | WFS_PIN_FK_ENTER |WFS_PIN_FK_00 | WFS_PIN_FK_000 | WFS_PIN_FK_BACKSPACE;;
//         ulong activePin = ALL_PIN_KEY | WFS_PIN_FK_CANCEL | WFS_PIN_FK_ENTER |WFS_PIN_FK_00 | WFS_PIN_FK_000 | WFS_PIN_FK_CLEAR;
         ulong terminatePin = WFS_PIN_FK_CANCEL | WFS_PIN_FK_ENTER;

         WFSPINGETDATA PinGetData;
         memset(&PinGetData,0,sizeof(WFSPINGETDATA));
         PinGetData.bAutoEnd = false;
         PinGetData.usMaxLen = 0;
         PinGetData.ulActiveFDKs =0x0;
         PinGetData.ulActiveKeys =activePin;
         PinGetData.ulTerminateFDKs =0x0;
         PinGetData.ulTerminateKeys =terminatePin;
         r=wfsSyncExecute(WFS_CMD_PIN_GET_DATA,timeout,&PinGetData,wfsResult);

        }else {
             ulong activeFdk= 0x0;
             ulong terminateFdk= 0x0;
             ulong activePin= ALL_PIN_KEY | WFS_PIN_FK_CANCEL | WFS_PIN_FK_CLEAR;// | WFS_PIN_FK_ENTER;;
//             ulong activePin= ALL_PIN_KEY | WFS_PIN_FK_CANCEL | WFS_PIN_FK_BACKSPACE;// | WFS_PIN_FK_ENTER;;
             ulong terminatePin= WFS_PIN_FK_CANCEL | WFS_PIN_FK_CLEAR;// | WFS_PIN_FK_ENTER;

             WFSPINGETPIN getPin;
             memset(&getPin,0,sizeof(WFSPINGETPIN));
             getPin.usMinLen=4;
             getPin.usMaxLen=4;
             getPin.bAutoEnd=true;
             getPin.cEcho='*';
             getPin.ulActiveFDKs =activeFdk;
             getPin.ulActiveKeys =activePin;
             getPin.ulTerminateFDKs =terminateFdk;
             getPin.ulTerminateKeys =terminatePin;
             r=wfsSyncExecute(WFS_CMD_PIN_GET_PIN,WOSA_CMD_TIMEOUT,&getPin,wfsResult);

         }

        while(1){  // make a decision how to handle mes here. by timer by event or inside loop... . its releavant to your app design
            MSG msg;
            int receiveMsg = GetMessage(&msg, NULL, 0, 0);//get msg from wosa xfs event trigger
            if (receiveMsg != 0) {
                switch (msg.message) {
                case WFS_EXECUTE_COMPLETE:
                    ZF_LOGI("WFS_EXECUTE_COMPLETE");
                    break;
                case WFS_EXECUTE_EVENT:
                    ZF_LOGI("WFS_EXECUTE_EVENT");
                    if(pass)
                       handleExeEvent(msg);
                    else
                        handleGetData(msg);
                    break;
                }
            } else {
                ZF_LOGI("did not GetMessage, ret=%d", receiveMsg);
                break;
            }
        }
     }

    }else {
        ZF_LOGI("failed to open pin pad port");
    }





    return a.exec();
}


int initWosaStartUp()
{
    int returnCode=0;
    HRESULT r;
    char MinMinor = 0
    , MinMajor = 1
    , MaxMinor = 2
    , MaxMajor = 2;
    DWORD Ver = (DWORD)MaxMajor | (DWORD)MaxMinor << 8 | (DWORD)MinMajor << 16 | (DWORD)MinMinor << 24; //66050
    WFSVERSION wfsVersion;// = { 3, 1, 3, "", "" };

    if ((r = WFSStartUp(Ver, &wfsVersion)) == WFS_SUCCESS) {
        ZF_LOGI("startup done.lets create app handle, result: %ld", r);
    } else {
        ZF_LOGE("startup failed. result: %ld", r);
        return -1;
    }
    if (WFSCreateAppHandle(&hApp) != WFS_SUCCESS) {
        ZF_LOGI("StartUp FAILED, result: %ld", r);
        return -2;
    } else {
        ZF_LOGI("WFSCreateAppHandle is WFS_SUCCESS, result: %ld", r);
        ZF_LOGI("Description---->%s", wfsVersion.szDescription);
    }
    return returnCode;
}


int wfsSyncOpen(const char *logicalName, int timeout)
{
    ZF_LOGI("wfsSyncOpen()");
    HRESULT r;
    LPSTR lpszAppID = (LPSTR)"Mousavi";
    DWORD dwTraceLevel = 0;
    char MinMinor = 0
    , MinMajor = 1
    , MaxMinor = 0
    , MaxMajor = 3;

    DWORD dwSrvcVersionsRequired = (DWORD)MaxMajor | (DWORD)MaxMinor << 8 |
                                   (DWORD)MinMajor << 16 | (DWORD)MinMinor << 24; // 01000103;
    WFSVERSION lpSrvcVersion;
    WFSVERSION lpSPIVersion;
    if ((r = WFSOpen((LPSTR)logicalName,hApp, lpszAppID, dwTraceLevel, timeout,
                     dwSrvcVersionsRequired, &lpSrvcVersion, &lpSPIVersion, &hService)) != WFS_SUCCESS) {
        return r;
    }
    return r;
}


int wfsSyncRegister()
{
    ZF_LOGI("wfsSyncRegister()");
    HRESULT r;
    DWORD dwEventClass = EXECUTE_EVENTS | USER_EVENTS | SYSTEM_EVENTS | SERVICE_EVENTS;
    r = WFSRegister(hService, dwEventClass, hwnd);
    if (r == WFS_SUCCESS) {
        ZF_LOGI("WFSRegister complete successfully, result: %ld", r);
    }else {
        ZF_LOGE("WFSRegister could not done, result: %ld", r);
    }
    return r;
}

HRESULT wfsSyncExecute(int CMD, int timeout, LPVOID readData, WFSRESULT& wfsResult)
{

    HRESULT r;
    LPWFSRESULT lpwfsResult=NULL;
    Sleep(50);
    r = WFSExecute(hService, CMD, readData, timeout, &lpwfsResult);

    if (r == WFS_SUCCESS) {
        ZF_LOGI("wfsSyncExecute execute successfully, result: %ld", r);
        wfsResult = *lpwfsResult;
    }else {
        ZF_LOGE("wfsSyncExecute could not execute, result: %ld", r);
    }
    WFSFreeResult(lpwfsResult);
    return r;
}



void handleGetData(const MSG &msg)
{
    ZF_LOGI("handleExeEvent()");
    LPWFSRESULT wfsResult = (LPWFSRESULT)msg.lParam;
    ZF_LOGI("wfsResult->u.dwEventID %d",(int)wfsResult->u.dwEventID);
    if(wfsResult->lpBuffer) {
        switch (wfsResult->u.dwEventID) {

        case WFS_EXEE_PIN_KEY:
            ZF_LOGI("WFS_EXEE_PIN_KEY");
            WFSPINKEY PinKey;
            memcpy(&PinKey, wfsResult->lpBuffer, sizeof(WFSPINKEY));
            std::string enteredPINString = "";
            ZF_LOGI("PinKey.ulDigit= %d",PinKey.ulDigit);
            switch (PinKey.ulDigit) {

            case WFS_PIN_FK_0:
                enteredPINString = '0';
                break;
            case WFS_PIN_FK_1:
                enteredPINString = '1';
                break;
            case WFS_PIN_FK_2:
                enteredPINString = '2';
                break;
            case WFS_PIN_FK_3:
                enteredPINString = '3';
                break;
            case WFS_PIN_FK_4:
                enteredPINString = '4';
                break;
            case WFS_PIN_FK_5:
                enteredPINString = '5';
                break;
            case WFS_PIN_FK_6:
                enteredPINString = '6';
                break;
            case WFS_PIN_FK_7:
                enteredPINString = '7';
                break;
            case WFS_PIN_FK_8:
                enteredPINString = '8';
                break;
            case WFS_PIN_FK_9:
                enteredPINString = '9';
                break;
            case WFS_PIN_FK_00:
                enteredPINString = "00";
                break;
            case WFS_PIN_FK_000:
                enteredPINString = "000";
                break;
            case WFS_PIN_FK_BACKSPACE:
                ZF_LOGI("WFS_PIN_FK_BACKSPACE");
                //clear data
                break;
            case WFS_PIN_FK_CLEAR:
                ZF_LOGI("WFS_PIN_FK_CLEAR");
                //clear data
                break;
            case WFS_PIN_FK_CANCEL:
                wfsSyncDeregister();
                break;
            case WFS_PIN_FK_ENTER:
                ZF_LOGI("WFS_PIN_FK_ENTER");
                break;
            }
            checkPinKeyValue(PinKey,enteredPINString);
        }
    } else {
        ZF_LOGW("wfsResult->lpBuffer is NULL!");
    }
}


//get pass sync
void handleExeEvent(const MSG &msg)
{
    ZF_LOGI("#############handleExeEvent###############");
    LPWFSRESULT wfsResult = (LPWFSRESULT)msg.lParam;
    ZF_LOGI("wfsResult->u.dwEventID %d",wfsResult->u.dwEventID);
    switch (wfsResult->u.dwEventID) {
    case WFS_EXEE_PIN_KEY:
        ZF_LOGI("##################WFS_EXEE_PIN_KEY#####################");
        WFSPINKEY PinKey;
        memcpy(&PinKey, wfsResult->lpBuffer, sizeof(WFSPINKEY));
//        std::string enteredPINString = "*";
        switch (PinKey.ulDigit) {
            case 0://encrypted value
                ZF_LOGI("0");
                line.append("*");
            case WFS_PIN_FK_0:
                ZF_LOGI("WFS_PIN_FK_0");
                line.append("*");
            break;
        case WFS_PIN_FK_BACKSPACE:
            ZF_LOGI("WFS_PIN_FK_BACKSPACE");
            if(line.size()>0) {
                line.chop(1);
            }
            break;
        case WFS_PIN_FK_CLEAR:
            ZF_LOGI("WFS_PIN_FK_CLEAR");
            if(line.size()>0) {
                line.clear();
            }
            break;
        case WFS_PIN_FK_CANCEL:
            ZF_LOGI("WFS_PIN_FK_CANCEL");
            break;
        case WFS_PIN_FK_ENTER:
            ZF_LOGI("WFS_PIN_FK_ENTER");
            break;
        default:
            ZF_LOGI("default case: %d", PinKey.ulDigit);
            break;
        }

    }//end of switch (wfsResult->u.dwEventID) {
}

void checkPinKeyValue(const WFSPINKEY &pinKey, std::string pinString)
{
    //get value of parameters
        if (pinKey.ulDigit == WFS_PIN_FK_0  || pinKey.ulDigit == WFS_PIN_FK_1 ||
            pinKey.ulDigit == WFS_PIN_FK_2  || pinKey.ulDigit == WFS_PIN_FK_3 ||
            pinKey.ulDigit == WFS_PIN_FK_4  || pinKey.ulDigit == WFS_PIN_FK_5 ||
            pinKey.ulDigit == WFS_PIN_FK_6  || pinKey.ulDigit == WFS_PIN_FK_7 ||
            pinKey.ulDigit == WFS_PIN_FK_8  || pinKey.ulDigit == WFS_PIN_FK_9 ) {
            destCard.append(QString::fromStdString(pinString));
        }
}


int wfsSyncDeregister()
{
    ZF_LOGI("wfsSyncDeregister()");
    HRESULT r;
    DWORD dwEventClass = EXECUTE_EVENTS | USER_EVENTS | SYSTEM_EVENTS | SERVICE_EVENTS;
    r = WFSDeregister(hService, dwEventClass, hwnd);
    if (r != WFS_SUCCESS) {
        ZF_LOGE("WFSDeregister could not done, result: %ld", r);
    }else {
        ZF_LOGI("WFSDeregister complete successfully, result: %ld", r);
    }
    return r;
}
