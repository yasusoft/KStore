//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TMyThread : public TThread
{
private:
        AnsiString LogText;
        int PPos1, PMax1, PPos2, PMax2;

        AnsiString Port;
        HANDLE hCom;
        int usb;
        int BaudRate, FrameSize;
        AnsiString File;

        TStringList *LogDebug;
        void __fastcall LogDebugAdd(AnsiString s, unsigned char buf[], int len);

        void __fastcall SyncLogAdd();
        void __fastcall LogAdd(AnsiString text);
        void __fastcall SyncProgress1();
        void __fastcall Progress1(int pos, int max = -1);
        void __fastcall SyncProgress2();
        void __fastcall Progress2(int pos, int max = -1);

        bool __fastcall SetState(HANDLE h, AnsiString state);
        bool __fastcall WriteBuf(HANDLE h, unsigned char buf[], int len);
        bool __fastcall ReadBuf(HANDLE h, unsigned char buf[], int len);
        bool __fastcall Wait(HANDLE h, unsigned char data[], int len);
        //unsigned char __fastcall Command(HANDLE h, unsigned short cmd,
        //        unsigned char data[], int len, unsigned char cont = 0x03);
        unsigned char __fastcall Send(HANDLE h, unsigned char data[], int len, unsigned char cont = 0x03);
        int __fastcall Receive(HANDLE h, unsigned char buf[]);

public:
        bool Debug;
        void __fastcall Execute();

        unsigned int TimeOut;
        unsigned int WaitCmd;

        AnsiString OpenDir, SaveDir;

        __fastcall TMyThread(bool CreateSuspended, AnsiString port, int usb = 0);
        __fastcall ~TMyThread();
};
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE 管理のコンポーネント
        TPanel *PanelTop;
        TComboBox *ComboPort;
        TComboBox *ComboUSB;
        TButton *ButtonOpen;
        TEdit *EditOpenDir;
        TEdit *EditSaveDir;
        TLabel *LabelPort;
        TCheckBox *CheckOpen;
        TCheckBox *CheckSave;
        TButton *ButtonBrowseOpen;
        TButton *ButtonBrowseSave;
        TRichEdit *Log;
        TProgressBar *ProgressBar2;
        TProgressBar *ProgressBar1;
        TLabel *LabelProgress;
        TButton *ButtonClear;
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
        void __fastcall FormDestroy(TObject *Sender);
        void __fastcall PanelTopResize(TObject *Sender);
        void __fastcall LogChange(TObject *Sender);
        void __fastcall ButtonOpenClick(TObject *Sender);
        void __fastcall ButtonClearClick(TObject *Sender);
        void __fastcall ButtonBrowseOpenClick(TObject *Sender);
        void __fastcall ButtonBrowseSaveClick(TObject *Sender);
private:	// ユーザー宣言
        AnsiString IniFile;
        bool Debug;
        int wcmd;
        TMyThread *MyThread;
        void __fastcall ThreadTerminate(TObject *Sender);
public:		// ユーザー宣言
        __fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
