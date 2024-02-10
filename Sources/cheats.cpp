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
    // 分差 / 4byte - ズレ補正
    int res = (dst - src) / 4 - 2;
    res = res & 0xFFFFFF; // 下3byteだけ抽出
    MessageBox("結果",
    Utils::Format("分岐値 : 0x%X\n"
    "リンク付き分岐 : 0xEB%06X\n"
    "無条件分岐 : 0xEA%06X", res, res, res))();
  }

  void regConf(MenuEntry* entry)
  {
    static u32 address;
    static u32 opcode;
    static bool patched = false;
    int srcBranch = 0;
    int dstBranch = 0;
    StringVector options = {"アドレス変更", "デフォルトパッチに戻す"};
    Keyboard kbOptions("選択して下さい", options);
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
      srcBranch = (0x1E81000 - address) / 4 - 2;
      srcBranch = (srcBranch & 0xFFFFFF) + 0xEA000000;
      dstBranch = (address - 0x1E81040) / 4 - 1;
      dstBranch = (dstBranch & 0xFFFFFF) + 0xEA000000;
      Process::Write32(address, srcBranch);
      {
        const u32 asmCode[] =
        {
          0xE58F003C, 0xE58F103C, 0xE58F203C, 0xE58F303C,
          0xE58F403C, 0xE58F503C, 0xE58F603C, 0xE58F703C,
          0xE58F803C, 0xE58F903C, 0xE58FA03C, 0xE58FB03C,
          0xE58FC03C, 0xE58FD03C, 0xE58FE03C, opcode, (u32)dstBranch
        };
        Process::CopyMemory((void*)0x1E81000, asmCode, sizeof(asmCode));
      }
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
      patched = false;
      break;
    
    default:
      ErrorMessage("不明な選択肢");
      break;
    }

    entry->SetGameFunc([](MenuEntry*){
      u32 regData[16];
      regData[15] = address + 8;
      u32 regDataAddress = 0x1E81044;
      for (int i = 0; i < 15; i++)
      {
        Process::Read32(regDataAddress + i * 4, regData[i]);
      }
      Draw::DrawPosition();
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

  bool isCovered(std::vector<u32> list, u32 value)
  {
    if (list.size() == 0) false;
    for (int i = 0; i < list.size(); i++)
    {
      if (list[i] == value) return true;
    }
    return false;
  }

  void maxMinValue(MenuEntry* entry)
  {
    static std::vector<u32> valueHistory;
    static u32 address = 0;
    static u32 oldAddress = 0;
    static u32 maxValue = 0;
    static u32 minValue = U32_MAX;
    static bool display = true;
    StringVector options = {"調査アドレス変更", "値の履歴", "履歴削除"};
    Keyboard kbOptions("実行する処理を選択して下さい", options);
    Keyboard kbAddress("調査するアドレスを入力して下さい");
    int menu = kbOptions.Open();
    if (menu < 0) return;
    std::string text;
    switch (menu)
    {
    case 0:
      if (kbAddress.Open(address, address) != 0) break;
      if (! AccessCheck(address)) break;
      if (valueHistory.size() != 0) break;
      if (MessageBox("確認", "履歴を削除しますか？", DialogType::DialogYesNo)())
        valueHistory = {};
        oldAddress = 0;
        maxValue = 0;
        minValue = U32_MAX;
      break;
    
    case 1:
      for (int i = 0; i < valueHistory.size(); i++)
      {
        text += Utils::Format("[%d] %08X\n", i, valueHistory[i]);
      }
      MessageBox("履歴", text)();
      break;

    case 2:
      valueHistory = {};
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
    StringVector options = {"範囲書き込み", "戻す", "サーチ結果メモリ設定"};
    Keyboard kb("実行したい処理を選択して下さい", options);
    menu = kb.Open();
    if (menu < 0) return;

    switch (menu)
    {
    case 0:
      if (RangeWriteManager::SearchWrite()) break;
      break;
    
    case 1:
      if (RangeWriteManager::ReturnWrite()) break;
      break;

    case 2:
      if (RangeWriteManager::SetFreeAddress()) break;
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
    StringVector options = {"空きメモリを確保", "メモリを破棄", "確保したメモリ確認"};
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
        ErrorMessage("メモリを確保しています\nメモリを破棄して下さい");
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
      if (! getFlag)
      {
        ErrorMessage("メモリを確保していません");
        break;
      }
      if (! MessageBox("確認", "メモリを破棄しますか？", DialogType::DialogYesNo)()) return;
      delete [] ptr;
      OSD::Notify(Utils::Format("Address: 0x%08X Delete!", ptr));
      ptr = nullptr;
      getFlag = false;
      break;

    case 2:
      MessageBox("確保したメモリ", Utils::Format("Address: 0x%08X", ptr))();
      break;
    
    default:
      ErrorMessage("不明な選択肢");
      break;
    }
  }
}
