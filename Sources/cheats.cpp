#include "cheats.hpp"
#include "class.hpp"

#define U32_MAX 0xFFFFFFFF

namespace CTRPluginFramework
{
  void calcBranch(MenuEntry* entry)
  {
    u32 src = 0;
    u32 dst = 0;
    Keyboard kb1("分岐元アドレスを入力して下さい");
    Keyboard kb2("分岐先アドレスを入力して下さい");
    if (kb1.Open(src) != 0) return;
    if (src % 4 != 0)
    {
      ErrorMessage("アドレスの入力が正しくありません");
      return;
    }
    if (kb2.Open(dst) != 0) return;
    if (dst % 4 != 0)
    {
      ErrorMessage("アドレスの入力が正しくありません");
      return;
    }
    u32 res = GetBranch(src, dst);
    MessageBox("結果",
    Utils::Format(
      "分岐値             : 0x%X\n"
      "リンク付き分岐 : 0xEB%06X\n"
      "無条件分岐       : 0xEA%06X", res, res, res))();
  }

  void regConf(MenuEntry* entry)
  {
    static u32 address;
    static u32 opcode;
    static bool patched = false;
    static u32 regData[16];
    u32 forwardBranch = 0;
    u32 returnBranch = 0;
    StringVector options = {"アドレス変更", (Color(255, 45, 45) << "デフォルトパッチに戻す")};
    std::string optionText[] = {"レジスタ確認パッチ : " + (Color::Red << "未適応")
                              , "レジスタ確認パッチ : " + (Color::Lime << "適応済み")};
    Keyboard kbOptions("実行したい処理を選択\n" + optionText[patched], options);
    Keyboard kbAddress("レジスタ確認したいアドレスを入力して下さい");
    int menu = kbOptions.Open();
    if (menu < 0) return;

    switch (menu)
    {
    case 0:
      if (patched)
      {
        ErrorMessage("レジスタ確認パッチが適応されています\nパッチをデフォルトに戻してください");
        break;
      }
      if (kbAddress.Open(address) != 0) break;
      if (! AccessCheck(address)) break;
      Process::Read32(address, opcode);
      forwardBranch = GetBranch(address, FREEADDRESS) + 0xEA000000;
      returnBranch = GetBranch(0x1E81040, address + 4) + 0xEA000000;
      Process::Write32(address, forwardBranch);
      for (int i = 0; i < 15; i++)
      {
        Process::Write32(FREEADDRESS + i * 4, 0xE58F003C + 0x1000 * i);
      }
      Process::Write32(0x1E8103C, opcode);
      Process::Write32(0x1E81040, returnBranch);
      OSD::Notify(Color::Lime << "Applied register confirmation patch!");
      patched = true;
      break;

    case 1:
      if (! patched)
      {
        ErrorMessage("レジスタ確認パッチが適応されていません");
        break;
      }
      Process::Write32(address, opcode);
      for (int i = 0; i < 15; i++)
      {
        Process::Write32(0x1E81044 + i * 4, 0);
      }
      regData[15] = 0;
      address = 0;
      opcode = 0;
      OSD::Notify(Color::Red << "Reset register confirmation patch!");
      patched = false;
      break;
    
    default:
      ErrorMessage("不明な選択肢");
      break;
    }

    entry->SetGameFunc([](MenuEntry*){
      if (address != 0) regData[15] = address + 8;
      u32 regDataAddress = 0x1E81044;
      for (int i = 0; i < 15; i++)
      {
        Process::Read32(regDataAddress + i * 4, regData[i]);
      }
      Draw::DrawPosition();
      Draw::DrawTopScr(Utils::Format("Address : %08X %08X", address, opcode));
      for (int i = 0; i < 13; i++)
      {
        Draw::DrawTopScr(Utils::Format("r%02d : %08X", i, regData[i]));
      }
      Draw::DrawTopScr(Utils::Format("sp  : %08X", regData[13]));
      Draw::DrawTopScr(Utils::Format("lr  : %08X", regData[14]));
      Draw::DrawTopScr(Utils::Format("pc  : %08X", regData[15]));
    });
  }

  void addressConf(MenuEntry* entry)
  {
    static u32 address[5] = {0,0,0,0,0};
    static int menu;
    StringVector options = {"アドレス 1", "アドレス 2", "アドレス 3", "アドレス 4", "アドレス 5"};
    Keyboard kbAdrSelect("監視するアドレスを選択して下さい", options);
    menu = kbAdrSelect.Open();
    if (menu < 0) return;
    Keyboard kbInAdr("監視するアドレスを入力して下さい\n" +
    Utils::Format("設定 : アドレス %d", menu + 1));
    if (kbInAdr.Open(address[menu]) != 0) return;
    if (! AccessCheck(address[menu]))
    {
      address[menu] = 0;
      return;
    }

    entry->SetGameFunc([](MenuEntry*){
      u32 value = 0;
      Draw::DrawPosition();
      for (int i = 0; i < 5; i++)
      {
        Process::Read32(address[i], value);
        if (address[i] == 0) value = 0;
        Draw::DrawTopScr(Utils::Format("Address: %08X %08X", address[i], value));
      }
    });
  }

  void maxMinValue(MenuEntry* entry)
  {
    static std::vector<u32> valueHistory;
    static u32 address = 0;
    static u32 oldAddress = 0;
    static u32 maxValue = 0;
    static u32 minValue = U32_MAX;
    static bool display = true;
    static bool firstRun = true;
    StringVector options = {"調査アドレス変更", "値の履歴を確認", "履歴をファイルに保存",
      (Color(255, 45, 45) << "履歴を削除")};
    Keyboard kbOptions("実行する処理を選択して下さい", options);
    Keyboard kbAddress("調査するアドレスを入力して下さい");
    Keyboard kbFileName("保存するファイル名を入力して下さい (拡張子不要)");
    int menu = kbOptions.Open();
    if (menu < 0) return;
    std::string text;
    std::string fileName = "";
    std::string fileNameExt = "";
    File file;

    switch (menu)
    {
    case 0:
      if (! firstRun)
      {
        if (! MessageBox("確認", "履歴は削除されますがよろしいですか？", DialogType::DialogOkCancel)()) break;
      } else {
        firstRun = false;
      }
      if (kbAddress.Open(address, address) != 0) break;
      if (! AccessCheck(address)) break;
      valueHistory = {};
      oldAddress = 0;
      maxValue = 0;
      minValue = U32_MAX;
      break;
    
    case 1:
      VectorU32ToString(valueHistory, text, APPEND);
      MessageBox("履歴", text)();
      break;

    case 2:
      if (valueHistory.size() == 0)
      {
        ErrorMessage("履歴がありません");
        break;
      }
      if (kbFileName.Open(fileName) != 0) break;
      fileNameExt = fileName + ".txt";
      if (! File::Exists(fileNameExt))
      {
        File::Create(fileNameExt);
      } else {
        MessageBox msg("確認",
          Utils::Format(
            "同じファイル名が存在してます\n"
            "ファイルを上書きしてもよろしいですか？"), DialogType::DialogYesNo);
        if (! msg()) break;
        File::Remove(fileNameExt);
        File::Create(fileNameExt);
      }
      File::Open(file, fileNameExt);
      text = Utils::Format("[Address: %08X]\n", address);
      VectorU32ToString(valueHistory, text, APPEND);
      file.WriteLine(text);
      file.Flush();
      file.Close();
      ConfMessage(Utils::Format("3gxDir:/%s\nに保存しました", fileNameExt.c_str()));
      break;

    case 3:
      valueHistory = {};
      ConfMessage("履歴を削除しました");
      break;

    default:
      ErrorMessage("不明な選択肢");
      break;
    }

    entry->SetGameFunc([](MenuEntry*){
      u32 value = 0;
      if (address != oldAddress)
      {
        maxValue = 0;
        minValue = 0xFFFFFFFF;
        oldAddress = address;
      }
      Process::Read32(address, value);
      if (value > maxValue) maxValue = value;
      if (value < minValue) minValue = value;
      if (! isCovered(valueHistory, value)) valueHistory.push_back(value);
      if (Controller::IsKeyPressed(Start)) display = display ? false : true;
      if (! display) return;
      Draw::DrawPosition(0, 190);
      Draw::DrawTopScr(Utils::Format("Adr: %08X", address));
      Draw::DrawTopScr(Utils::Format("Max: %08X", maxValue));
      Draw::DrawTopScr(Utils::Format("Min: %08X", minValue));
      Draw::DrawTopScr(Utils::Format("Now: %08X", value));
      Draw::DrawTopScr("Press START : Show / Hide");
    });
  }

  void rangeWrite(MenuEntry* entry)
  {
    int menu;
    StringVector options = {"範囲書き込み", "サーチ結果記録アドレス確認", (Color(255, 45, 45) << "戻す")};
    Keyboard kb("実行したい処理を選択して下さい", options);
    menu = kb.Open();
    if (menu < 0) return;

    switch (menu)
    {
    case 0:
      if (RangeWriteManager::SearchWrite()) break;
      break;
    
    case 1:
      RangeWriteManager::FreeAddressConf();
      break;

    case 2:
      if (RangeWriteManager::ReturnWrite()) break;
      break;
    
    default:
      ErrorMessage("不明な選択肢");
      break;
    }
  }

  void getFreeMemory(MenuEntry* entry)
  {
    static bool getFlag = false;
    static u8* ptr = nullptr;
    StringVector options = {"空きメモリを確保", "確保したメモリ確認", (Color(255, 45, 45) << "メモリを開放")};
    u32 size = 0;
    u8* p;
    Keyboard kbOptions("実行したい処理を選択して下さい", options);
    Keyboard kbSize("希望の容量を入力して下さい (バイト単位)");
    kbSize.IsHexadecimal(false);
    int menu = kbOptions.Open();
    if (menu < 0) return;
    switch (menu)
    {
    case 0:
      if (getFlag)
      {
        ErrorMessage("メモリを確保しています\nメモリを開放して下さい");
        return;
      }
      if (kbSize.Open(size) != 0) return;
      ptr = new u8[size];
      p = ptr;
      for (int i = 0; i < size; i++)
      {
        *p = 0xDF;
        p++;
      }
      MessageBox("成功",
      Utils::Format("メモリの確保に成功しました\nAddress: 0x%08X", ptr))();
      getFlag = true;
      break;

    case 1:
      MessageBox("確保したメモリ", Utils::Format("Address: 0x%08X", ptr))();
      break;

    case 2:
      if (! getFlag)
      {
        ErrorMessage("メモリを確保していません");
        break;
      }
      if (! MessageBox("確認", "メモリを開放しますか？", DialogType::DialogYesNo)()) return;
      delete [] ptr;
      OSD::Notify(Utils::Format("Address: 0x%08X Delete!", ptr));
      ptr = nullptr;
      getFlag = false;
      break;

    default:
      ErrorMessage("不明な選択肢");
      break;
    }
  }
}
