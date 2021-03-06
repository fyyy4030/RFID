/*
******************************************************************
**                      uCGUIBuilder                            **
**                  Version:   4.0.0.0                          **
**                     2012 / 04                               **
**                   CpoyRight to: wyl                          **
**              Email:ucguibuilder@163.com                        **
**          This text was Created by uCGUIBuilder               **
******************************************************************/

#include <stddef.h>
#include "GUI.h"
#include "DIALOG.h"

#include "WM.h"
#include "BUTTON.h"
#include "CHECKBOX.h"
#include "DROPDOWN.h"
#include "EDIT.h"
#include "FRAMEWIN.h"
#include "LISTBOX.h"
#include "MULTIEDIT.h"
#include "RADIO.h"
#include "SLIDER.h"
#include "TEXT.h"
#include "PROGBAR.h"
#include "SCROLLBAR.h"
#include "LISTVIEW.h"
#include "BSP_ST16C554.h"
#include "BSP_Delay.h"
#include "MESSAGEBOX.h"

#define CMD     0x01
#define FLAG    0x02
#define DATA    0x03
#define DSFID   0x04
#define AFI     0x05
#define VICC    0x06
#define ICR     0x07
#define FCS     0x08

extern GUI_CONST_STORAGE GUI_FONT GUI_FontFontSong;

uint8_t xunka[20] = {0x02, 0x06, 0x00, 0x2b};
uint8_t duka[20] = {0x02, 0x0f, 0x00, 0x20};
uint8_t xieka[20] = {0x02, 0x13, 0x00, 0x21};

volatile uint8_t drbuf[5000]= {0x00};
volatile uint8_t rxflag = 0;
volatile int len = 0;
volatile int recSta = 0x01;
volatile uint8_t cmd = 0x00;
uint8_t ANS[50], LEN;

int dizhi = 0;
uint8_t Kahao[10];
uint8_t Shuju[5];

uint8_t Flag = 0x02;
int Flag1, Flag2;
int flag2 = 0;//寻卡
int flag3 = 0;//读卡
int flag4 = 0;//写卡

uint16_t crc = 0xffff;

uint16_t crc16_ccitt(uint8_t data,uint16_t crc1)
{
    uint16_t ccitt16 = 0x8408;//0x8408 为 0x1021 的逆序值,而 0x1021 为生成多项式 0x11021
    int i;
    crc1 ^= data; /*新的数据与原来的余数相加（加法就是异或操作） */
    for(i=0; i<8; i++)
    {
        if(crc1 & 0x0001) /*最低位为 1， 减去除数 */
        {
            crc1 >>= 1;
            crc1 ^= ccitt16;
        }
        else /*最低位为 0， 不需要减去除数 */
        {
            crc1 >>= 1; /*直接移位*/
        }
    }
    return crc1;
}

int Min(int a, int b)
{
    return a > b ? b : a;
}

char tran(uint16_t n)
{
    char ans;
    if(n > 9)
        ans = n % 9 +'A' - 1;
    else
        ans = n + '0' - 0;
    return ans;
}

uint8_t trannum(char c)
{
    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return c - '0';
}

void jieXi(int mode)
{
    int i, j;
    uint8_t ret;
    rxflag = 0;
    for(i = 0; i < 4000; i++)
    {
        if(drbuf[i] == cmd) rxflag = 1;
        if(rxflag)
        {
            ret = drbuf[i];
            switch(recSta)
            {
            case CMD:
                if(ret != cmd)
                {
                    rxflag = 0;
                    break;
                }
                Flag1 = 0;
                Flag2 = 0;
                LEN = 0;
                ANS[LEN++] = ret;
                crc = 0xffff;
                recSta = FLAG;
                break;
            case FLAG:
                ANS[LEN++] = ret;
                crc = crc16_ccitt(ret, crc);
                recSta = DATA;
                break;
            case DATA:
                ANS[LEN++] = ret;
                crc = crc16_ccitt(ret, crc);
                if(LEN == 11)
                {
                    if(ANS[2] & 0x01)
                        recSta = DSFID;
                    else
                    {
                        if(ANS[2] & 0x02)
                            recSta = AFI;
                        else
                        {
                            if(ANS[2] & 0x04)
                                recSta = VICC;
                            else
                            {
                                if(ANS[2] & 0x08)
                                    recSta = ICR;
                                else
                                    recSta = FCS;
                            }
                        }
                    }
                }
                break;
            case DSFID:
                ANS[LEN++] = ret;
                crc = crc16_ccitt(ret, crc);
                if(ANS[2] & 0x02)
                    recSta = AFI;
                else
                {
                    if(ANS[2] & 0x04)
                        recSta = VICC;
                    else
                    {
                        if(ANS[2] & 0x08)
                            recSta = ICR;
                        else
                            recSta = FCS;
                    }
                }
                break;
            case AFI:
                ANS[LEN++] = ret;
                crc = crc16_ccitt(ret, crc);
                if(ANS[2] & 0x04)
                    recSta = VICC;
                else
                {
                    if(ANS[2] & 0x08)
                        recSta = ICR;
                    else
                        recSta = FCS;
                }
                break;
            case VICC:
                ANS[LEN++] = ret;
                crc = crc16_ccitt(ret, crc);
                Flag1++;
                if(Flag1 == 2)
                {
                    if(ANS[2] & 0x08)
                        recSta = ICR;
                    else
                        recSta = FCS;
                }
                break;
            case ICR:
                ANS[LEN++] = ret;
                crc = crc16_ccitt(ret, crc);
                recSta = FCS;
                break;
            case FCS:
                ANS[LEN++] = ret;
                Flag2++;
                if(Flag2 == 1)
                    recSta = FCS;
                else if(Flag2 == 2)
                {
                    recSta = CMD;
                    for(j = 0; j < 5000; j++) drbuf[j] = 0;
                    len = 0;
                    if(mode == 1) flag2 = 1;
                    if(mode == 2) flag3 = 1;
                }
                break;
            }
        }
        if(mode == 1 && flag2 == 1) break;
        if(mode == 2 && flag3 == 1) break;
    }
}

void Xunka()
{
    uint8_t i;
    int fcs = 0;
    for(i = 0; i < 4; i++)//选卡
    {
        if(i == 2)
        {
            BSP_ST16C554_CS1A_Write(Flag);
            fcs += Flag;
            BSP_Delay_ms(10);
            continue;
        }
        BSP_ST16C554_CS1A_Write(xunka[i]);
        if(i) fcs += xunka[i];
        if(i == 3) cmd = xunka[i];
        BSP_Delay_ms(10);
    }
    fcs %= 256;
    BSP_ST16C554_CS1A_Write(fcs);
    BSP_Delay_ms(10);
    BSP_ST16C554_CS1A_Write(0x03);
    BSP_Delay_ms(30);
    jieXi(1);
    if(flag2 != 1) flag2 = 2;
    rxflag = 0;
}

void Duka()
{
    uint8_t i;
    int fcs = 0;
    for(i = 0; i < 4; i++)
    {
        if(i == 2)
        {
            BSP_ST16C554_CS1A_Write(Flag);
            fcs += Flag;
            BSP_Delay_ms(10);
            continue;
        }
        BSP_ST16C554_CS1A_Write(duka[i]);
        if(i) fcs += duka[i];
        if(i == 3) cmd = duka[i];
        BSP_Delay_ms(10);
    }
    for(i = 0; i < 8; i++)
    {
        BSP_ST16C554_CS1A_Write(Kahao[i]);
        fcs += Kahao[i];
        BSP_Delay_ms(10);
    }
    BSP_ST16C554_CS1A_Write(dizhi);
    BSP_Delay_ms(10);
    fcs += dizhi;
    fcs %= 256;
    BSP_ST16C554_CS1A_Write(fcs);
    BSP_Delay_ms(10);
    BSP_ST16C554_CS1A_Write(0x03);
    BSP_Delay_ms(30);
    jieXi(2);
    if(flag3 != 1) flag3 = 2;
    rxflag = 0;
}

void Xieka()
{
    uint8_t i;
    int fcs = 0;
    for(i = 0; i < 4; i++)
    {
        if(i == 2)
        {
            BSP_ST16C554_CS1A_Write(Flag);
            fcs += Flag;
            BSP_Delay_ms(10);
            continue;
        }
        BSP_ST16C554_CS1A_Write(xieka[i]);
        if(i) fcs += xieka[i];
        if(i == 3) cmd = xieka[i];
        BSP_Delay_ms(10);
    }
    for(i = 0; i < 8; i++)
    {
        BSP_ST16C554_CS1A_Write(Kahao[i]);
        fcs += Kahao[i];
        BSP_Delay_ms(10);
    }
    BSP_ST16C554_CS1A_Write(dizhi);
    BSP_Delay_ms(10);
    fcs += dizhi;
    for(i = 0; i < 4; i++)
    {
        BSP_ST16C554_CS1A_Write(Shuju[i]);
        fcs += Shuju[i];
        BSP_Delay_ms(10);
    }
    BSP_ST16C554_CS1A_Write(fcs);
    BSP_Delay_ms(10);
    BSP_ST16C554_CS1A_Write(0x03);
    BSP_Delay_ms(30);
    rxflag = 0;
}

static WM_HWIN _CreateMessageBox(const char * sMessage, const char * sCaption, int Flags, const GUI_FONT * pFont)
{
    WM_HWIN hWin;
    WM_HWIN hItem;
    GUI_RECT Rect;
    hWin = MESSAGEBOX_Create(sMessage, sCaption, Flags);
//
// Change font of message box window
//
    FRAMEWIN_SetFont(hWin, pFont);
//
// Adjust size
//
    WM_GetWindowRectEx(hWin, &Rect);
    WM_SetWindowPos(hWin, Rect.x0 - 15,
                    Rect.y0 - 40,
                    Rect.x1 - Rect.x0 + 1 + 30,
                    Rect.y1 - Rect.y0 + 1 + 30);
//
// Change font of button widget
//
    hItem = WM_GetDialogItem(hWin, GUI_ID_OK);
    BUTTON_SetFont(hItem, pFont);
//
// Adjust size of button widget
//
    WM_GetWindowRectEx(hItem, &Rect);
    WM_SetWindowPos(hItem, Rect.x0,
                    Rect.y0 + 10,
                    Rect.x1 - Rect.x0 + 1 + 30,
                    Rect.y1 - Rect.y0 + 1 + 5);
//
// Change font of text widget
//
    hItem = WM_GetDialogItem(hWin, GUI_ID_TEXT0);
    TEXT_SetFont(hItem, pFont);
//
// Adjust size text widget
//
    WM_GetWindowRectEx(hItem, &Rect);
    WM_SetWindowPos(hItem, Rect.x0,
                    Rect.y0,
                    Rect.x1 - Rect.x0 + 1 + 30,
                    Rect.y1 - Rect.y0 + 1 + 12);
    return hWin;
}


/*
************************************************************************************************
* 键盘对话框
************************************************************************************************
*/
/*********************************************************************
*
* Defines
*
**********************************************************************
*/
#define ID_WINDOW_04 (GUI_ID_USER + 0x53)
// USER START (Optionally insert additional static data)
// USER END
/*********************************************************************
*
* _aDialogCreate
*/
static const GUI_WIDGET_CREATE_INFO _aDialogCreatePageKeyboard[] = {
    { WINDOW_CreateIndirect,    "Window",            ID_WINDOW_04,                 260, 305, 530, 165, 0, 0x0, 0},
    { BUTTON_CreateIndirect,    "4",                 ID_BUTTON_4,                  2, 58, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "7",                 ID_BUTTON_7,                  2, 104, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "8",                 ID_BUTTON_8,                  93, 104, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "9",                 ID_BUTTON_9,                  189, 104, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "0",                 ID_BUTTON_0,                  274, 104, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "5",                 ID_BUTTON_5,                  93, 58, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "6",                 ID_BUTTON_6,                  189, 58, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "Backspace",         ID_BUTTON_Backspace,          363, 104, 160, 40, 0, 0},
    { BUTTON_CreateIndirect,    "D",                 ID_BUTTON_D,                  274, 58, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "E",                 ID_BUTTON_E,                  363, 58, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "F",                 ID_BUTTON_F,                  453, 58, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "1",                 ID_BUTTON_1,                  2, 3, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "2",                 ID_BUTTON_2,                  93, 3, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "3",                 ID_BUTTON_3,                  189, 3, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "A",                 ID_BUTTON_A,                  274, 3, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "B",                 ID_BUTTON_B,                  363, 3, 70, 40, 0, 0},
    { BUTTON_CreateIndirect,    "C",                 ID_BUTTON_C,                  453, 3, 70, 40, 0, 0},
};
/*********************************************************************
*
* Static code
*
**********************************************************************
*/
// USER START (Optionally insert additional static code)
// USER END
/*********************************************************************
*
* _cbDialog
*/
static void _cbDialogPageKeyboard(WM_MESSAGE * pMsg) {
    WM_HWIN hItem;
    int NCode;
    int Id;
    int Pressed = 0;
    hItem = pMsg->hWin;
// USER START (Optionally insert additional variables)
// USER END
    switch (pMsg->MsgId) {
    case WM_INIT_DIALOG:
        WINDOW_SetBkColor(hItem, 0x578b2e);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_4), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_4), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_7), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_7), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_8), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_8), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_9), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_9), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_0), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_0), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_5), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_5), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_6), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_6), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_Backspace), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_Backspace), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_D), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_D), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_E), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_E), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_F), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_F), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_1), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_1), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_2), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_2), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_3), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_3), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_A), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_A), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_B), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_B), 0);

        BUTTON_SetFont(WM_GetDialogItem(hItem,ID_BUTTON_C), &GUI_FontFontSong);
        BUTTON_SetFocussable(WM_GetDialogItem(hItem,ID_BUTTON_C), 0);

// USER START (Optionally insert additional code for further widget initialization)
// USER END
        break;
    case WM_NOTIFY_PARENT:
        Id = WM_GetId(pMsg->hWinSrc);
        NCode = pMsg->Data.v;
        switch(NCode) {
        case WM_NOTIFICATION_CLICKED:
            Pressed = 1;
        case WM_NOTIFICATION_RELEASED:
            if ((Id >= GUI_ID_USER + 10 && Id <= GUI_ID_USER + 18) || (Id >= GUI_ID_USER + 0 && Id <= GUI_ID_USER + 7)) {
                int Key;
                if (Id != GUI_ID_USER + 7) {
                    char acBuffer[10];
                    BUTTON_GetText(pMsg->hWinSrc, acBuffer, sizeof(acBuffer)); /* Get the text of the button */
                    Key = acBuffer[0];
                } else {
                    Key = GUI_KEY_BACKSPACE; /* Get the text from the array */
                }
                GUI_SendKeyMsg(Key, Pressed); /* Send a key message to the focussed window */
            }
            break;
// USER START (Optionally insert additional code for further Ids)
// USER END
        }
        break;
    default:
        WM_DefaultProc(pMsg);
        break;
    }
}
/*********************************************************************
*
* CreateWindowPage3
*/
WM_HWIN CreateWindowPageKeyboard(void) {
    WM_HWIN hWin;
    hWin = GUI_CreateDialogBox(_aDialogCreatePageKeyboard, GUI_COUNTOF(_aDialogCreatePageKeyboard), _cbDialogPageKeyboard,
                               WM_HBKWIN, 0, 0);
    return hWin;
}

/*********************************************************************
*
*       static data
*
**********************************************************************
*/
/*********************************************************************
*
*       Dialog resource
*
* This table conatins the info required to create the dialog.
* It has been created by ucGUIbuilder.
*/

static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
    { WINDOW_CreateIndirect,    NULL,                0,                    0, 0, 800, 480, 0, 0, 0},
    { TEXT_CreateIndirect,      "标签内存显示区",    ID_TEXT_0,            8, 18, 120, 30, 0, 0},
    { TEXT_CreateIndirect,      "地址",              ID_TEXT_1,            540, 120, 50, 30, 0, 0},
    { TEXT_CreateIndirect,      "RFID原理机",      ID_TEXT_2,            620, 18, 217, 30, 0, 0},
    { BUTTON_CreateIndirect,    "写卡",          		 ID_BUTTON_xieka,      670, 250, 110, 40, 0, 0},
    { BUTTON_CreateIndirect,    "寻卡",              ID_BUTTON_xunka,      330, 250, 180, 40, 0, 0},
    { BUTTON_CreateIndirect,    "读卡",          		 ID_BUTTON_duka,       540, 250, 110, 40, 0, 0},
    { TEXT_CreateIndirect,      "数据",              ID_TEXT_3,            540, 200, 50, 30, 0, 0},
    { DROPDOWN_CreateIndirect,   NULL,               ID_DROPDOWN_0,        600, 120, 180, 30, 0, 0},
    { EDIT_CreateIndirect,       NULL,               ID_EDIT_3,            600, 200, 180, 30, 0, 180},
    { RADIO_CreateIndirect,      NULL,               ID_RADIO_0,           330, 128, 150, 70, 0, 0x2a02, 0},
    { LISTVIEW_CreateIndirect,  "Listview",             GUI_ID_LISTVIEW0,           8, 55, 230, 420, 0, 0x0, 0 },
    { EDIT_CreateIndirect,       NULL,               ID_EDIT_2,            330, 200, 180, 30, 0, 180},
    { TEXT_CreateIndirect,      "卡号：",              ID_TEXT_4,            270, 200, 50, 30, 0, 0},
    { TEXT_CreateIndirect,      "编码模式",              ID_TEXT_5,            260, 140, 80, 30, 0, 0},
};

/*****************************************************************
**      FunctionName:void InitDialog(WM_MESSAGE * pMsg)
**      Function: to initialize the Dialog items
**
**      call this function in _cbCallback --> WM_INIT_DIALOG
*****************************************************************/

void InitDialog(WM_MESSAGE * pMsg)
{
    uint8_t i;
    SCROLLBAR_Handle hScroll;
    char temp[5];
    WM_HWIN hWin = pMsg->hWin;

    GUI_UC_SetEncodeUTF8();
    //
    //WINDOW
    //
    WINDOW_SetBkColor(hWin,0x578b2e);
    //
    //ID_TEXT_0
    //
    TEXT_SetBkColor(WM_GetDialogItem(hWin,ID_TEXT_0),0x578b2e);
    TEXT_SetTextColor(WM_GetDialogItem(hWin,ID_TEXT_0),0xffffff);
    TEXT_SetTextAlign(WM_GetDialogItem(hWin,ID_TEXT_0),GUI_TA_VCENTER|GUI_TA_CENTER);
    TEXT_SetFont(WM_GetDialogItem(hWin,ID_TEXT_0),&GUI_FontFontSong);

    //
    //ID_TEXT_1
    //
    TEXT_SetBkColor(WM_GetDialogItem(hWin,ID_TEXT_1),0x578b2e);
    TEXT_SetTextAlign(WM_GetDialogItem(hWin,ID_TEXT_1),GUI_TA_VCENTER|GUI_TA_CENTER);
    TEXT_SetFont(WM_GetDialogItem(hWin,ID_TEXT_1),&GUI_FontFontSong);

    //
    //ID_TEXT_2
    //
    TEXT_SetBkColor(WM_GetDialogItem(hWin,ID_TEXT_2),0x578b2e);
    TEXT_SetTextColor(WM_GetDialogItem(hWin,ID_TEXT_2),0x8b0000);
    TEXT_SetTextAlign(WM_GetDialogItem(hWin,ID_TEXT_2),GUI_TA_VCENTER|GUI_TA_CENTER);
    TEXT_SetFont(WM_GetDialogItem(hWin,ID_TEXT_2),&GUI_FontFontSong);
    //
    //ID_TEXT_3
    //
    TEXT_SetTextAlign(WM_GetDialogItem(hWin,ID_TEXT_3),GUI_TA_VCENTER|GUI_TA_CENTER);
    TEXT_SetFont(WM_GetDialogItem(hWin,ID_TEXT_3),&GUI_FontFontSong);

    TEXT_SetTextAlign(WM_GetDialogItem(hWin,ID_TEXT_4),GUI_TA_VCENTER|GUI_TA_CENTER);
    TEXT_SetFont(WM_GetDialogItem(hWin,ID_TEXT_4),&GUI_FontFontSong);

    TEXT_SetTextAlign(WM_GetDialogItem(hWin,ID_TEXT_5),GUI_TA_VCENTER|GUI_TA_CENTER);
    TEXT_SetFont(WM_GetDialogItem(hWin,ID_TEXT_5),&GUI_FontFontSong);
//
    //
    //ID_DROPDOWN_0
    //
//        DROPDOWN_SetAutoScroll(WM_GetDialogItem(hWin,ID_DROPDOWN_0), 1);
//				DROPDOWN_SetScrollbarWidth(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), 16);
    DROPDOWN_SetTextAlign(WM_GetDialogItem(hWin,ID_DROPDOWN_0), GUI_TA_HCENTER | GUI_TA_VCENTER);
    DROPDOWN_SetTextHeight(WM_GetDialogItem(hWin,ID_DROPDOWN_0), 34);
    DROPDOWN_SetListHeight(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), 120);
    DROPDOWN_SetFont(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), GUI_FONT_16B_ASCII);
    DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "00");
    DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "01");
    DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "02");
    DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "03");
    DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "04");
    DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "05");
    DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "06");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "07");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "08");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "09");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "0A");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "0B");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "0C");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "0D");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "0E");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "0F");
//				DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "10");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "11");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "12");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "13");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "14");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "15");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "16");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "17");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "18");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "19");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "1A");
//        DROPDOWN_AddString(WM_GetDialogItem(pMsg->hWin, ID_DROPDOWN_0), "1B");

    //
    //ID_EDIT_2
    //
    EDIT_SetTextAlign(WM_GetDialogItem(hWin,ID_EDIT_2),GUI_TA_VCENTER|GUI_TA_LEFT);
    EDIT_SetFont(WM_GetDialogItem(hWin, ID_EDIT_2), &GUI_Font16B_ASCII);
    EDIT_SetFocussable(WM_GetDialogItem(hWin, ID_EDIT_2), 0);
    EDIT_SetText(WM_GetDialogItem(hWin,ID_EDIT_2),"");

    //
    //ID_EDIT_3
    //
    EDIT_SetTextAlign(WM_GetDialogItem(hWin,ID_EDIT_3),GUI_TA_VCENTER|GUI_TA_LEFT);
    EDIT_SetFont(WM_GetDialogItem(hWin, ID_EDIT_3), &GUI_Font16B_ASCII);
    EDIT_SetMaxLen(WM_GetDialogItem(hWin, ID_EDIT_3), 8);
    EDIT_SetText(WM_GetDialogItem(hWin,ID_EDIT_3),"");
    //
    //BUTTON
    //

    BUTTON_SetFont(WM_GetDialogItem(hWin,ID_BUTTON_xieka),&GUI_FontFontSong);

    BUTTON_SetFont(WM_GetDialogItem(hWin,ID_BUTTON_xunka),&GUI_FontFontSong);

    BUTTON_SetFont(WM_GetDialogItem(hWin,ID_BUTTON_duka),&GUI_FontFontSong);

    RADIO_SetFont(WM_GetDialogItem(pMsg->hWin, ID_RADIO_0), &GUI_Font16B_ASCII);
    RADIO_SetText(WM_GetDialogItem(pMsg->hWin, ID_RADIO_0), "4 take 1", 0);
    RADIO_SetText(WM_GetDialogItem(pMsg->hWin, ID_RADIO_0), "256 take 1", 1);

    HEADER_Handle hHeader;

    WM_HWIN hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_LISTVIEW0);
    hHeader = LISTVIEW_GetHeader(WM_GetDialogItem(pMsg->hWin, GUI_ID_LISTVIEW0));
    HEADER_SetFont(hHeader, &GUI_Font16B_ASCII);
    LISTVIEW_SetFont(WM_GetDialogItem(hWin,GUI_ID_LISTVIEW0), &GUI_Font16B_ASCII);
    LISTVIEW_SetHeaderHeight(hItem, 40);
    LISTVIEW_SetRowHeight(hItem, 40);
    LISTVIEW_AddColumn(hItem, 40, "", GUI_TA_HCENTER | GUI_TA_VCENTER);
    LISTVIEW_AddColumn(hItem, 40, "00", GUI_TA_HCENTER | GUI_TA_VCENTER);
    LISTVIEW_AddColumn(hItem, 40, "01", GUI_TA_HCENTER | GUI_TA_VCENTER);
    LISTVIEW_AddColumn(hItem, 40, "02", GUI_TA_HCENTER | GUI_TA_VCENTER);
    LISTVIEW_AddColumn(hItem, 40, "03", GUI_TA_HCENTER | GUI_TA_VCENTER);
    for(i = 0; i < 28; i++)
        LISTVIEW_AddRow(hItem, NULL);
    for(i = 0; i < 28; i++)
    {
        temp[0] = tran(i/16);
        temp[1] = tran(i%16);
        temp[2] = '\0';
        LISTVIEW_SetItemText(WM_GetDialogItem(hWin,GUI_ID_LISTVIEW0), 0, i, temp);
        LISTVIEW_SetItemText(WM_GetDialogItem(hWin,GUI_ID_LISTVIEW0), 1, i, "00");
        LISTVIEW_SetItemText(WM_GetDialogItem(hWin,GUI_ID_LISTVIEW0), 2, i, "00");
        LISTVIEW_SetItemText(WM_GetDialogItem(hWin,GUI_ID_LISTVIEW0), 3, i, "00");
        LISTVIEW_SetItemText(WM_GetDialogItem(hWin,GUI_ID_LISTVIEW0), 4, i, "00");
    }
    LISTVIEW_SetGridVis(hItem, 1);
    //LISTVIEW_SetAutoScrollV(hItem, 1);
    hScroll = SCROLLBAR_CreateAttached(hItem, GUI_ID_VSCROLL);
    SCROLLBAR_SetWidth(hScroll, 15);

}

/*********************************************************************
*
*       Dialog callback routine
*/
static void _cbDialog(WM_MESSAGE * pMsg)
{
    uint8_t i, cnt;
    char temp[20], buf[40], tp[5];
    int NCode, Id;
    WM_HWIN hWin = pMsg->hWin;
    switch (pMsg->MsgId)
    {
    case WM_INIT_DIALOG:
        InitDialog(pMsg);
        break;
    case WM_NOTIFY_PARENT:
        Id = WM_GetId(pMsg->hWinSrc);
        NCode = pMsg->Data.v;
        switch (Id)
        {
        case ID_DROPDOWN_0:
            switch(NCode)
            {
            case WM_NOTIFICATION_CLICKED:

                break;
            case WM_NOTIFICATION_RELEASED:
                break;
            case WM_NOTIFICATION_SEL_CHANGED:
                dizhi = DROPDOWN_GetSel(WM_GetDialogItem(hWin, ID_DROPDOWN_0));
                break;
            }
            break;
        case ID_RADIO_0:
            switch(NCode)
            {
            case WM_NOTIFICATION_CLICKED:
                break;
            case WM_NOTIFICATION_RELEASED:
                break;
            case WM_NOTIFICATION_MOVED_OUT:
                break;
            case WM_NOTIFICATION_VALUE_CHANGED:
                if(RADIO_GetValue(WM_GetDialogItem(hWin, ID_RADIO_0)))
                    Flag = 0x06;
                else
                    Flag = 0x02;
                break;
            }
            break;
        case ID_BUTTON_xunka:
            switch(NCode)
            {
            case WM_NOTIFICATION_CLICKED:
                Xunka();
                if(flag2 == 1)
                {
                    cnt = 0;
                    for(i = 3; i < 11; i++)
                    {
                        temp[cnt++] = tran(ANS[i]/16);
                        temp[cnt++] = tran(ANS[i]%16);
                    }
                    temp[cnt] = '\0';
                    EDIT_SetTextAlign(WM_GetDialogItem(hWin,ID_EDIT_2), GUI_TA_VCENTER | GUI_TA_LEFT);
                    EDIT_SetFont(WM_GetDialogItem(hWin, ID_EDIT_2), &GUI_Font16B_ASCII);
                    EDIT_SetText(WM_GetDialogItem(hWin,ID_EDIT_2),temp);
                }
                break;
            case WM_NOTIFICATION_RELEASED:
                break;
            case WM_NOTIFICATION_MOVED_OUT:
                break;
            }
            break;
        case ID_BUTTON_duka:
            switch(NCode)
            {
            case WM_NOTIFICATION_CLICKED:
                EDIT_GetText(WM_GetDialogItem(hWin,GUI_ID_USER+36), buf, sizeof(buf));
                for(i = 0; i < 8; i++)
                    Kahao[i] = trannum(buf[i*2]) * 16 + trannum(buf[i*2+1]);
                Duka();
                if(flag3 == 1)
                {
                    cnt = 0;
                    for(i = 2; i < 7; i++)
                    {
                        temp[cnt++] = tran(ANS[i]/16);
                        temp[cnt++] = tran(ANS[i]%16);
                    }
                    temp[cnt] = '\0';
                    EDIT_SetTextAlign(WM_GetDialogItem(hWin,ID_EDIT_3), GUI_TA_VCENTER | GUI_TA_LEFT);
                    EDIT_SetFont(WM_GetDialogItem(hWin, ID_EDIT_3), &GUI_Font16B_ASCII);
                    for(i = 0; i < 4; i++)
                    {
                        tp[0] = temp[i*2];
                        tp[1] = temp[i*2+1];
                        tp[2] = '\0';
                        LISTVIEW_SetItemText(WM_GetDialogItem(hWin,GUI_ID_LISTVIEW0), i+1, dizhi, tp);
                    }
                    EDIT_SetText(WM_GetDialogItem(hWin,ID_EDIT_3),temp);
                }
                break;
            case WM_NOTIFICATION_RELEASED:
                break;
            case WM_NOTIFICATION_MOVED_OUT:
                break;
            }
            break;
        case ID_BUTTON_xieka:
            switch(NCode)
            {
            case WM_NOTIFICATION_CLICKED:
                EDIT_GetText(WM_GetDialogItem(hWin, GUI_ID_USER+36), buf, sizeof(buf));
                for(i = 0; i < 8; i++)
                    Kahao[i] = trannum(buf[i*2]) * 16 + trannum(buf[i*2+1]);
                EDIT_GetText(WM_GetDialogItem(hWin, ID_EDIT_3), buf, sizeof(buf));
                for(i = 0; i < 4; i++)
                    Shuju[i] = trannum(buf[i*2]) * 16 + trannum(buf[i*2+1]);
                Xieka();
                break;
            case WM_NOTIFICATION_RELEASED:
                break;
            case WM_NOTIFICATION_MOVED_OUT:
                break;
            }
            break;
        }
        break;
    default:
        WM_DefaultProc(pMsg);
    }
}


WM_HWIN CreateFramewin0(void)
{
    WM_HWIN hWin;
    hWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), _cbDialog, WM_HBKWIN, 0, 0);
    return hWin;
}

/*********************************************************************
*
*       MainTask
*
**********************************************************************
*/
void MainTask(void)
{
    GUI_Init();
    WM_SetDesktopColor(GUI_WHITE);      /* Automacally update desktop window */
    WM_SetCreateFlags(WM_CF_MEMDEV);  /* Use memory devices on all windows to avoid flicker */
    CreateFramewin0();
    CreateWindowPageKeyboard();
    while(1)
    {
        //寻卡
        if(flag2 == 1)
        {
            GUI_ExecCreatedDialog(_CreateMessageBox("OK!", "", 0, &GUI_Font16B_ASCII));
            flag2 = 0;
        }
        else if(flag2 == 2)
        {
            GUI_ExecCreatedDialog(_CreateMessageBox("failed!", "", 0, &GUI_Font16B_ASCII));
            flag2 = 0;
        }

        //读卡
        if(flag3 == 1)
        {
            GUI_ExecCreatedDialog(_CreateMessageBox("OK!", "", 0, &GUI_Font16B_ASCII));
            flag3 = 0;
        }
        else if(flag3 == 2)
        {
            GUI_ExecCreatedDialog(_CreateMessageBox("failed!", "", 0, &GUI_Font16B_ASCII));
            flag3 = 0;
        }

        BSP_Delay_ms(100);
        GUI_Delay(1);
    }
}

