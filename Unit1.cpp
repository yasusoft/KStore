//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
#include <IniFiles.hpp>
#include <FileCtrl.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TMyThread::TMyThread(bool CreateSuspended, AnsiString port, int usb)
        : TThread(CreateSuspended)
{
  Port = port;
  hCom = INVALID_HANDLE_VALUE;
  this->usb = usb;

  BaudRate = 600;
  FrameSize = 4096;

  TimeOut = 2500;
  WaitCmd = 500;

  Debug = true;
  LogDebug = new TStringList;
}
//---------------------------------------------------------------------------
__fastcall TMyThread::~TMyThread()
{
  if (hCom != INVALID_HANDLE_VALUE)
    CloseHandle(hCom);

  if (Debug)
  {
    try {
      LogDebug->SaveToFile(ChangeFileExt(Application->ExeName, ".log"));
    } catch (...) { }
  }
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::LogDebugAdd(AnsiString s, unsigned char buf[], int len)
{
  if (Debug)
  {
    if (len > 0)
    {
      AnsiString text = IntToStr(GetTickCount()) + "(" + s + "," + IntToStr(len) + ")\t:";
      for (int i = 0; i < len; i ++)
        text += " 0x" + IntToHex(buf[i], 2);
      LogDebug->Add(text);
    }
    else
      LogDebug->Add(IntToStr(GetTickCount()) + "(M)\t: " + s);
  }
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncLogAdd()
{
  Form1->Log->Lines->Add(LogText);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::LogAdd(AnsiString text)
{
  LogText = text;
  Synchronize(SyncLogAdd);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncProgress1()
{
  Form1->ProgressBar1->Max = PMax1;
  Form1->ProgressBar1->Position = PPos1;
  Form1->LabelProgress->Caption = IntToStr(PPos1) + "/" + IntToStr(PMax1);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::Progress1(int pos, int max)
{
  if (max != -1)
    PMax1 = max;
  if (pos >= 0)
    PPos1 = pos;
  else
    PPos1 = PMax1 + pos;
  Synchronize(SyncProgress1);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncProgress2()
{
  Form1->ProgressBar2->Max = PMax2;
  Form1->ProgressBar2->Position = PPos2;
  //Form1->LabelProgress->Caption = IntToStr(PPos2) + "/" + IntToStr(PMax2);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::Progress2(int pos, int max)
{
  if (max != -1)
    PMax2 = max;
  if (pos >= 0)
    PPos2 = pos;
  else
    PPos2 = PMax2 + pos;
  Synchronize(SyncProgress2);
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::SetState(HANDLE h, AnsiString state)
{
  DCB dcbCom;
  LogDebugAdd(state, NULL, 0);
  if (!GetCommState(h, &dcbCom))
  {
    DWORD err = GetLastError();
    LogDebugAdd("GetCommState Failed [("+IntToStr(err)+")"+SysErrorMessage(err)+"]", NULL, 0);
    return false;
  }
  if (!BuildCommDCB(state.c_str(), &dcbCom))
  {
    DWORD err = GetLastError();
    LogDebugAdd("BuildCommDCB Failed [("+IntToStr(err)+")"+SysErrorMessage(err)+"]", NULL, 0);
    return false;
  }
  //dcbCom.BaudRate = 600;
  //dcbCom.Parity = EVENPARITY;
  //dcbCom.ByteSize = 8;
  //dcbCom.StopBits = ONESTOPBIT;
  if (!SetCommState(h, &dcbCom))
  {
    DWORD err = GetLastError();
    LogDebugAdd("SetCommState Failed [("+IntToStr(err)+")"+SysErrorMessage(err)+"]", NULL, 0);
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::WriteBuf(HANDLE h, unsigned char buf[], int len)
{
  unsigned long wrote;
  int pos = 0;
  while (!Terminated && pos < len)
  {
    Progress2(pos, len);
    WriteFile(h, buf + pos, len - pos, &wrote, NULL);
    if (wrote == 0)
      return false;
    LogDebugAdd("S", buf + pos, wrote);
    pos += wrote;
  }
  FlushFileBuffers(h);
  if (!Terminated && pos == len)
    return true;
  else
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::ReadBuf(HANDLE h, unsigned char buf[], int len)
{
  int pos = 0;
  unsigned int t = GetTickCount();
  while (!Terminated && pos < len)
  {
    Progress2(pos, len);
    unsigned long read;
    if (!ReadFile(h, buf + pos, len - pos, &read, NULL))
      return false;
    if (read != 0)
    {
      LogDebugAdd("R", buf + pos, read);
      pos += read;
      t = GetTickCount();
    }
    else if (GetTickCount() - t > TimeOut)
      return false;
  }
  if (!Terminated && pos == len)
    return true;
  else
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::Wait(HANDLE h, unsigned char data[], int len)
{
  unsigned char buf;
  int pos = 0;
  unsigned int t = GetTickCount();
  while (!Terminated && pos < len)
  {
    unsigned long read;
    if (!ReadFile(h, &buf, 1, &read, NULL))
      return false;
    if (read != 0)
    {
      LogDebugAdd("R", &buf, read);
      if (buf == data[pos])
        pos ++;
      else if (buf == data[0])
        pos = 1;
      else
      {
        pos = 0;
        continue;
      }
      t = GetTickCount();
    }
    else if (GetTickCount() - t > TimeOut)
      return false;
  }
  if (!Terminated && pos == len)
    return true;
  else
    return false;
}
//---------------------------------------------------------------------------
/*
unsigned char __fastcall TMyThread::Command(HANDLE h, unsigned short cmd,
        unsigned char data[], int len, unsigned char cont)
{
  unsigned char buf[1024] = {0x01, 0x00, 0x00, 0x00, 0x02, cmd>>8, cmd};

  // 引数コピー
  if (len > 0)
    CopyMemory(buf + 7, data, len);

  // データ長
  len += 6;
  buf[2] = len>>8;
  buf[3] = len;

  len += 4 - 3;
  buf[len++] = cont;

  // チェックサム？
  unsigned short sum = 0;
  for (int i = 0; i < len; i ++)
    sum += buf[i];
  sum = ~sum;
  buf[len++] = sum>>8;
  buf[len++] = sum;

  int cnt;
  for (cnt = 0; cnt < 5 && !Terminated; cnt ++)
  {
    // コマンド送信
    if (!WriteBuf(h, buf, len))
      return 0;

    unsigned char res;
    do
    {
      // 返事受信
      if (!ReadBuf(h, &res, 1))
        return 0;
      if (res == 0x06)
        return 0x06;
      if (res == 0x07)
        continue;
      if (res != 0x15)
        return res;
    } while (res == 0x07);

    Sleep(WaitCmd);
  }
  if (Terminated || cnt == 5)
    return 0;

  return 0;
}
*/
//---------------------------------------------------------------------------
unsigned char __fastcall TMyThread::Send(HANDLE h, unsigned char data[], int len, unsigned char cont)
{
  unsigned char buf1[5] = {0x01, 0x80, 0x00, 0x00, 0x02};
  unsigned char buf2[3] = {cont};

  // データ長
  len += 4;
  buf1[2] = len>>8;
  buf1[3] = len;

  // チェックサム？
  unsigned short sum = cont;
  len -= 4;
  for (int i = 0; i < 5; i ++)
    sum += buf1[i];
  for (int i = 0; i < len; i ++)
    sum += data[i];
  sum = ~sum;
  buf2[1] = sum>>8;
  buf2[2] = sum;

  int cnt;
  for (cnt = 0; cnt < 5 && !Terminated; cnt ++)
  {
    // データ送信
    if (!WriteBuf(h, buf1, 5))
      return 0;
    if (len > 0 && !WriteBuf(h, data, len))
      return 0;
    if (!WriteBuf(h, buf2, 3))
      return 0;

    unsigned char res;
    do
    {
      // 返事受信
      if (!ReadBuf(h, &res, 1))
        return 0;
      if (res == 0x06)
        return 0x06;
      if (res == 0x07)
        continue;
      if (res != 0x15)
        return res;
    } while (res == 0x07);

    Sleep(WaitCmd);
  }
  if (Terminated || cnt == 5)
    return 0;

  return 0;
}
//---------------------------------------------------------------------------
int __fastcall TMyThread::Receive(HANDLE h, unsigned char buf[])
{
  unsigned short sum = 0xFFFF;
  unsigned short dlen;
  do
  {
    if (Terminated)
      return -1;

    unsigned char sbuf[2];
    if (!Wait(h, "\x01\x80", 2))
      return -1;

    if (!ReadBuf(h, sbuf, 2))
    {
      if (!WriteBuf(h, "\x15", 1))
        return -1;
      continue;
    }
    dlen = (sbuf[0]<<8) | sbuf[1];
    if (dlen > 0)
    {
      sum = 0x01 + 0x80 + sbuf[0] + sbuf[1];

      // 0x20 受信
      if (!ReadBuf(h, buf, 1))
      {
        if (!WriteBuf(h, "\x15", 1))
          return -1;
        continue;
      }
      sum += buf[0];
      dlen --;

      if (!ReadBuf(h, buf, dlen))
      {
        if (!WriteBuf(h, "\x15", 1))
          return -1;
        continue;
      }

      dlen -= 2;
      for (int i = 0; i < dlen; i ++)
        sum += buf[i];
      sum += (short)((buf[dlen]<<8) | buf[dlen+1]);
      if (sum != 0xFFFF)
      {
        if (!WriteBuf(h, "\x15", 1))
          break;
      }
    }
  } while (sum != 0xFFFF && !Terminated);
  if (Terminated)
    return false;

  if (!WriteBuf(h, "\x06", 1))
    return -1;

  return dlen;
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::Execute()
{
  unsigned char buf[0x10000];
  int len;
  unsigned long read;

  LogAdd("Thread Started.");

  // ポートオープン
  hCom = CreateFile(Port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (hCom == INVALID_HANDLE_VALUE)
  {
    LogAdd("ERROR: CAN'T OPEN '" + Port + "'");
    return;
  }
  LogDebugAdd(Port + " OPENED", NULL, 0);

  // バッファサイズ設定
  SetupComm(hCom, 4096, 4096);

  // 基本設定
  SetState(hCom, "baud=600 parity=e data=8 stop=1");

  // タイムアウト設定
  //*
  COMMTIMEOUTS comTimeout;
  comTimeout.ReadIntervalTimeout = 500;
  comTimeout.ReadTotalTimeoutConstant = 5000;
  comTimeout.ReadTotalTimeoutMultiplier = 0;
  comTimeout.WriteTotalTimeoutConstant = 5000;
  comTimeout.WriteTotalTimeoutMultiplier = 0;
  SetCommTimeouts(hCom, &comTimeout);
  //*/

  // USB設定
  if (usb == 1 || usb == 2)
  {
    Sleep(250);
    LogAdd("通信モード設定...");
    if (usb == 1)
      WriteBuf(hCom, "AT%Z\r\n", 6);
    else if (usb == 2)
      WriteBuf(hCom, "ATZ80\r\n", 7);
    if (!Wait(hCom, "CONNECT\r\n", 9))
    {
      LogAdd("設定失敗");
      return;
    }
    LogAdd("設定成功");
  }

  while (!Terminated)
  {
    if (!ReadBuf(hCom, buf, 1))
      continue;
    if (buf[0] == 0x8E)
    {
      WriteBuf(hCom, "\x9A\x99\x9A\x91\x92\x93\x94\x95\x96\x97\x98\xAA", 12);
      continue;
    }
    if (buf[0] == 0xB3)
    {
      ReadBuf(hCom, buf, 1);
      if (buf[0] == 0x0D)
      {
        ReadBuf(hCom, buf, 36);
        WriteBuf(hCom, "\x6c\x0d\x00\x02\x00\x07\x00\x05\x00\x00\x00\x03\x01\x08\x01\x03\x06\x04\x01\x00\x01\x08\x05\x03\x04\x08\x04\x09\x04\x02\x04\x06\x00\x01\x04\x08\x05\x05", 38);
      }
      continue;
    }
    if (buf[0] != 0xD0)
      continue;
    if (!Wait(hCom, "\x01\x05", 2))
      continue;
    if (!WriteBuf(hCom, "\xD0\x01\x06", 3))
      continue;
    if (!ReadBuf(hCom, buf, 1) || buf[0] != 0x05)
    {
      WriteBuf(hCom, "\x08", 1);
      LogAdd("Connect Request Error");
      continue;
    }
    if (!WriteBuf(hCom, "\x06", 1))
      continue;
    LogAdd("Client Connected.");

    bool err = false;
    while (!Terminated && !err)
    {
      if (!ReadBuf(hCom, buf, 1))
      {
        LogAdd("Command Read Error");
        err = true;
        break;
      }
      if (buf[0] == 0x04)
        break;
      if (buf[0] == 0x08)
      {
        LogAdd("Client Error");
        err = true;
        break;
      }
      if (buf[0] == 0x07)
        continue;
      if (buf[0] == 0x05)
      {
        Sleep(1000);
        WriteBuf(hCom, "\x06", 1);
        continue;
      }
      if (buf[0] != 0x01)
      {
        LogAdd("Command Unknown Error");
        ReadBuf(hCom, buf, 25);
        err = true;
        break;
      }

      unsigned short sum = 0x01;
      if (!ReadBuf(hCom, buf, 3)) // || buf[0] != 0x00
      {
        WriteBuf(hCom, "\x08", 1);
        LogAdd("Command Error");
        err = true;
        break;
      }
      len = (buf[1]<<8) | buf[2];
      sum += buf[0] + buf[1] + buf[2];
      if (!ReadBuf(hCom, buf, len))
      {
        WriteBuf(hCom, "\x15", 1);
        continue;
      }
      len -= 2;
      for (int i = 0; i < len; i ++)
        sum += buf[i];
      sum += (short)((buf[len]<<8) | buf[len+1]);
      if (sum != 0xFFFF)
      {
        WriteBuf(hCom, "\x15", 1);
        continue;
      }

      unsigned short cmd = (buf[1]<<8) | buf[2];
      if (cmd == 0x0000)
      { // SetProtocolVersion{Version(4B)}
        if (buf[3] | buf[5] | buf[6] == 0x00 && buf[4] == 0x01)
          WriteBuf(hCom, "\x06", 1);
        else
          WriteBuf(hCom, "\x18", 1);
      }
      else if (cmd == 0x0001)
      { // SetBaudRate{BaudRate(4B)}
        int rate = (buf[3]<<24) | (buf[4]<<16) | (buf[5]<<8) | buf[6];
        bool b = SetState(hCom, "baud=" + IntToStr(rate) + " parity=e data=8 stop=1");
        SetState(hCom, "baud=" + IntToStr(BaudRate) + " parity=e data=8 stop=1");
        if (b)
        {
          WriteBuf(hCom, "\x06", 1);
          Sleep(100);
          BaudRate = rate;
          SetState(hCom, "baud=" + IntToStr(BaudRate) + " parity=e data=8 stop=1");
        }
        else
          WriteBuf(hCom, "\x18", 1);
      }
      else if (cmd == 0x0002)
      { // SetParityBit{Parity(1B)}
        if (buf[3] == 0x01)
          WriteBuf(hCom, "\x06", 1);
        else
          WriteBuf(hCom, "\x18", 1);
      }
      else if (cmd == 0x0003)
      { // SetFrameSize{FrameSize(4B)}
        int f = (buf[3]<<24) | (buf[4]<<16) | (buf[5]<<8) | buf[6];
        LogDebugAdd("frame=" + IntToStr(f), NULL, 0);
        if (f <= FrameSize || SetupComm(hCom, f, f))
        {
          FrameSize = f;
          WriteBuf(hCom, "\x06", 1);
        }
        else
          WriteBuf(hCom, "\x18", 1);
      }
      else if (cmd == 0x0204)
      { // GetProfile{Number(2B)}
        unsigned short p = (buf[3]<<8) | buf[4];
        if (p == 0x0000)
        {
          WriteBuf(hCom, "\x06", 1);
          if (Send(hCom, "\x00\x02\x00\x00\x05\x00" // タイプ
                "\x00\x08YasuSoft" // メーカー名
                "\x00\x06KStore"        // デバイス名
                "\x00\x00\x00\x00"      // バージョン
                "\x00\x00\x00\x00"
                "\x0F\x42\x40\x00"      // 受信可能サイズ
                "\x00\x0B" // サポートコマンド
                  "\x00\x00" // SetProtocolVersion
                  "\x00\x01" // SetBaudRate
                  "\x00\x02" // SetParityBit
                  "\x00\x03" // SetFrameSize
                  "\x00\x10" // SetFile
                  "\x00\x21" // SetLockNo
                  "\x01\x10" // PutFile
                  "\x02\x04" // GetProfile
                  "\x02\x10" // GetFile
                  "\x02\x12" // GetFileInfo
                  "\x02\x14" // GetFileList
                        , 63, 0x03) == 0x04)
            break;
        }
        else if (p == 0x0100)
        {
          WriteBuf(hCom, "\x06", 1);
          if (Send(hCom, "\x00\x00\x00\x00\x00\x00\x00" // サポート
                "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" // メモリサイズ
                "\x00" // FuncTransサポート
                "\x00\x0A\x02\x10\x02\x12\x02\x14\x02\x04\x00\x10\x00\x00\x00\x01\x00\x03\x00\x02\x01\x10" // サポートコマンド
                "\x00\x00" // コントロールコマンド
                        , 60, 0x03) == 0x04)
            break;
        }
        else
          WriteBuf(hCom, "\x18", 1);
      }
      else if (cmd == 0x0214)
      { // GetFileList{}
        LogAdd("GetFileList");
        if (OpenDir == "")
          WriteBuf(hCom, "\x18", 1);
        else
        {
          WriteBuf(hCom, "\x06", 1);

          TStringList *list = new TStringList;
          TSearchRec rec;
          if (OpenDir[OpenDir.Length()] != '\\')
            OpenDir += "\\";
          if (FindFirst(OpenDir + "*.*", 0, rec) == 0)
          {
            do
            {
              list->Add(rec.Name);
            } while (FindNext(rec) == 0 && !Terminated);
            FindClose(rec);
          }

          buf[0] = list->Count >> 8;
          buf[1] = list->Count;
          len = 2;
          for (int i = 0; i < list->Count && !Terminated; i ++)
          {
            int slen = list->Strings[i].Length();
            for (int j = 1; j <= slen; j ++)
              buf[len++] = list->Strings[i][j];
            buf[len++] = 0;

            if (len >= FrameSize - 100 && i+1 < list->Count)
            {
              if (Send(hCom, buf, len, 0x17) == 0x04)
                break;
              len = 0;
            }
          }
          if (len > 0)
          {
            if (Send(hCom, buf, len, 0x03) == 0x04)
              break;
          }

          delete list;
        }
      }
      else if (cmd == 0x0010)
      { // SetFile{FileName(STR\0)}
        File = (char*)buf+3;
        LogAdd("SetFile(" + File + ")");
        if (OpenDir != "" && OpenDir[OpenDir.Length()] != '\\')
          OpenDir += "\\";
        if (File == "" || !FileExists(OpenDir + File))
          WriteBuf(hCom, "\x18", 1);
        else
          WriteBuf(hCom, "\x06", 1);
      }
      else if (cmd == 0x0212)
      { // GetFileInfo{}
        LogAdd("GetFileInfo");
        if (OpenDir != "" && OpenDir[OpenDir.Length()] != '\\')
          OpenDir += "\\";

        int fp;
        if (File == "" || !FileExists(OpenDir + File)
         || (fp = FileOpen(OpenDir + File, fmOpenRead)) == -1)
          WriteBuf(hCom, "\x18", 1);
        else
        {
          int fs = FileSeek(fp, 0, 2);
          FileClose(fp);

          WriteBuf(hCom, "\x06", 1);
          len = 0;
          // ファイル名\0
          CopyMemory(buf, File.c_str(), File.Length()+1);
          len += File.Length()+1;
          // ？
          ZeroMemory(buf + len, 4);
          len += 4;
          // ファイルサイズ
          buf[len++] = fs >> 24;
          buf[len++] = fs >> 16;
          buf[len++] = fs >> 8;
          buf[len++] = fs;
          // 日時\0
          TDateTime dt = FileDateToDateTime(FileAge(OpenDir + File));
          AnsiString d = dt.FormatString("yyyymmddhhnnss00");
          CopyMemory(buf + len, d.c_str(), d.Length()+1);
          len += d.Length()+1;
          // ？
          ZeroMemory(buf + len, 16);
          len += 16;
          if (Send(hCom, buf, len, 0x03) == 0x04)
            break;
        }
      }
      else if (cmd == 0x0210)
      { // GetFile{FileName(STR\0)}
        AnsiString f = (char*)buf+3;
        LogAdd("GetFile(" + f + ")");
        if (OpenDir != "" && OpenDir[OpenDir.Length()] != '\\')
          OpenDir += "\\";

        if (File == "" || !FileExists(OpenDir + File))
          WriteBuf(hCom, "\x18", 1);
        else
        {
          TMemoryStream *mem = new TMemoryStream;
          try
          {
            mem->LoadFromFile(OpenDir + File);

            WriteBuf(hCom, "\x06", 1);
            Progress1(mem->Position, mem->Size);
            do
            {
              len = mem->Read(buf, FrameSize - 10);
              if (Send(hCom, buf, len, mem->Position < mem->Size ? 0x17 : 0x03) != 0x06)
              {
                err = true;
                break;
              }
              Progress1(mem->Position, mem->Size);
            } while (mem->Position < mem->Size && !Terminated);
          }
          catch (...)
          {
            WriteBuf(hCom, "\x18", 1);
          }
          delete mem;
        }
      }
      else if (cmd == 0x0110)
      { // PutFile{FileName(STR\0),FileSize(4B)}
        AnsiString f = (char*)buf+3;
        int fs = 3 + f.Length() + 1;
        fs = (buf[fs]<<24) | (buf[fs+1]<<16) | (buf[fs+2]<<8) | buf[fs+3];

        LogAdd("PutFile(" + f + "," + fs + ")");
        if (SaveDir != "" && SaveDir[SaveDir.Length()] != '\\')
          SaveDir += "\\";

        AnsiString of = f;
        int n = 1;
        while (FileExists(SaveDir + f) && n < 100)
          f = ChangeFileExt(of,"") + "(" + IntToStr(n++) + ")" + ExtractFileExt(of);

        if (SaveDir == "" || n == 100)
          WriteBuf(hCom, "\x18", 1);
        else
        {
          WriteBuf(hCom, "\x06", 1);

          TMemoryStream *mem = new TMemoryStream;
          try
          {
            int dlen;
            Progress1(mem->Position, fs);
            do
            {
              if ((dlen = Receive(hCom, buf)) == -1)
              {
                err = true;
                break;
              }
              dlen --;
              mem->Write(buf, dlen);
              Progress1(mem->Position, fs);
            } while (buf[dlen] == 0x17 && !Terminated);
            if (mem->Position == fs)
              mem->SaveToFile(SaveDir + f);
          }
          catch (...)
          {
            LogAdd("SaveToFile失敗");
          }
          delete mem;
        }
      }
      else if (cmd == 0x0021)
      { // SetLockNo{(STR)}
        WriteBuf(hCom, "\x06", 1);
      }
      else
        WriteBuf(hCom, "\x18", 1);
    }

    if (err || Terminated)
      WriteBuf(hCom, "\x08", 1);
    else
      WriteBuf(hCom, "\x04", 1);
    LogAdd("Client Disconnected.");

    FrameSize = 4096;
    SetupComm(hCom, 4096, 4096);
    LogDebugAdd("frame=4096", NULL, 0);
    BaudRate = 600;
    SetState(hCom, "baud=600 parity=e data=8 stop=1");
  }

  CloseHandle(hCom);
  hCom = INVALID_HANDLE_VALUE;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender)
{
  IniFile = ChangeFileExt(Application->ExeName, ".ini");
  Debug = false;
  wcmd = -1;
  MyThread = NULL;
  ButtonClearClick(Sender);
  EditOpenDir->Text = GetCurrentDir();
  EditSaveDir->Text = GetCurrentDir();

  try
  {
    TIniFile *Reg = new TIniFile(IniFile);
    try
    {
      Width = Reg->ReadInteger("KStore", "Width", Width);
      Height = Reg->ReadInteger("KStore", "Height", Height);

      ComboPort->Text = Reg->ReadString("KStore", "Port", ComboPort->Text);
      ComboUSB->ItemIndex = Reg->ReadInteger("KStore", "USB", ComboUSB->ItemIndex);

      EditOpenDir->Text = Reg->ReadString("KStore", "OpenDir", EditOpenDir->Text);
      CheckOpen->Checked = (EditOpenDir->Text != "");
      EditSaveDir->Text = Reg->ReadString("KStore", "SaveDir", EditSaveDir->Text);
      CheckSave->Checked = (EditSaveDir->Text != "");

      Debug = Reg->ReadBool("KStore", "Debug", false);
      wcmd = Reg->ReadInteger("KStore", "WaitCmd", -1);
    }
    __finally
    {
      delete Reg;
    }
  }
  catch (...)
  {
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCloseQuery(TObject *Sender, bool &CanClose)
{
  try
  {
    TIniFile *Reg = new TIniFile(IniFile);
    try
    {
      Reg->WriteInteger("KStore", "Width", Width);
      Reg->WriteInteger("KStore", "Height", Height);

      Reg->WriteString("KStore", "Port", ComboPort->Text);
      Reg->WriteInteger("KStore", "USB", ComboUSB->ItemIndex);

      Reg->WriteString("KStore", "OpenDir", CheckOpen->Checked ? EditOpenDir->Text : AnsiString());
      Reg->WriteString("KStore", "SaveDir", CheckSave->Checked ? EditSaveDir->Text : AnsiString());
    }
    __finally
    {
      delete Reg;
    }
  }
  catch (...)
  {
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormDestroy(TObject *Sender)
{
  if (MyThread)
    MyThread->Terminate();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PanelTopResize(TObject *Sender)
{
  EditOpenDir->Width = PanelTop->Width - 95;
  EditSaveDir->Width = PanelTop->Width - 95;
  ButtonBrowseOpen->Left = PanelTop->Width - 25;
  ButtonBrowseSave->Left = PanelTop->Width - 25;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::LogChange(TObject *Sender)
{
  Log->Perform(EM_SCROLLCARET, 0, 0);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonOpenClick(TObject *Sender)
{
  if (MyThread)
  {
    MyThread->Terminate();
    //delete MyThread;
    //MyThread = NULL;

    //ButtonOpen->Caption = "&Connect";
    //Log->Lines->Add("Thread Terminated.");
    return;
  }

  MyThread = new TMyThread(true, ComboPort->Text, ComboUSB->ItemIndex);
  //MyThread->Debug = Debug;
  if (CheckOpen->Checked)
  {
    if (EditOpenDir->Text == "")
      EditOpenDir->Text = GetCurrentDir();
    MyThread->OpenDir = EditOpenDir->Text;
  }
  else
    MyThread->OpenDir = "";
  if (CheckSave->Checked)
  {
    if (EditSaveDir->Text == "")
      EditSaveDir->Text = GetCurrentDir();
    MyThread->SaveDir = EditSaveDir->Text;
  }
  else
    MyThread->OpenDir = "";
  MyThread->Debug = Debug;
  if (wcmd != -1) MyThread->WaitCmd = wcmd;
  MyThread->OnTerminate = ThreadTerminate;
  MyThread->FreeOnTerminate = true;
  MyThread->Resume();
  ButtonOpen->Caption = "&Close";
  //Log->Lines->Add("Thread Started.");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ThreadTerminate(TObject *Sender)
{
  MyThread = NULL;

  ProgressBar1->Position = 0;
  LabelProgress->Caption = "0/0";
  ProgressBar2->Position = 0;
  ButtonOpen->Caption = "&Open";
  Log->Lines->Add("Thread Terminated.");
  Log->Lines->Add("");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonClearClick(TObject *Sender)
{
  Log->Lines->Clear();
  Log->Lines->Add("Yasu software - KStore");
  Log->Lines->Add("");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonBrowseOpenClick(TObject *Sender)
{
  AnsiString dir = EditOpenDir->Text;
  //if (SelectDirectory(dir, TSelectDirOpts() << sdAllowCreate << sdPerformCreate << sdPrompt, 0))
  if (SelectDirectory("共有フォルダ", "", dir))
  {
    EditOpenDir->Text = dir;
    CheckOpen->Checked = true;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonBrowseSaveClick(TObject *Sender)
{
  AnsiString dir = EditSaveDir->Text;
  //if (SelectDirectory(dir, TSelectDirOpts() << sdAllowCreate << sdPerformCreate << sdPrompt, 0))
  if (SelectDirectory("保存フォルダ", "", dir))
  {
    EditSaveDir->Text = dir;
    CheckSave->Checked = true;
  }
}
//---------------------------------------------------------------------------

