#include "cheats.hpp"
#include "class.hpp"

namespace CTRPluginFramework
{
  void calcBranch(MenuEntry* entry)
  {
    u32 src = 0;
    u32 dst = 0;
    Keyboard kb1("分岐元アドレスを入力して下さい");
    Keyboard kb2("分岐先アドレスを入力して下さい");
    if (kb1.Open(src) != 0) return;
    if (kb2.Open(dst) != 0) return;
    u32 res = (dst - src) / 4 + 2 - 4;
    MessageBox(Utils::Format("分岐値は「 0x%X 」です", res))();
  }

  void addressConf(MenuEntry* entry)
  {
    static u32 address[5] = {0,0,0,0,0};
    static int menu;
    StringVector options = {"Address 1", "Address 2", "Address 3", "Address 4", "Address 5"};
    Keyboard kbAdrSelect("監視するアドレスを選択して下さい", options);
    Keyboard kbInAdr("監視するアドレスを入力して下さい");
    menu = kbAdrSelect.Open();
    if (menu < 0) return;
    if (kbInAdr.Open(address[menu]) != 0) return;
    if (! Process::CheckAddress(address[menu], MEMPERM_READWRITE))
    {
      MessageBox(Color::Red << "エラー", Utils::Format("address: 0x%08X はアクセスできません", address[menu]))();
      address[menu] = 0;
      return;
    }

    entry->SetGameFunc([](MenuEntry*){
      u32 value = 0;
      for (int i = 0; i < 5; i++)
      {
        Process::Read32(address[i], value);
        if (! Process::CheckAddress(address[i], MEMPERM_READWRITE)) value = 0;
        Draw::topScrDraw(Utils::Format("Address: %08X %08X", address[i], value));
      }
      Draw::topScrDraw(0);
    });
  }

  void maxMinValue(MenuEntry* entry)
  {
    static u32 address = 0;
    Keyboard kb("調査するアドレスを入力して下さい");
    if (kb.Open(address, address) != 0) return;
    if (! Process::CheckAddress(address, MEMPERM_READWRITE))
    {
      MessageBox(Color::Red << "エラー", Utils::Format("address: 0x%08X はアクセスできません", address))();
      address = 0;
      return;
    }

    entry->SetGameFunc([](MenuEntry*){
      u32 oldAddress = 0;
      u32 value = 0;
      u32 maxValue = 0;
      u32 minValue = 0xFFFFFFFF;
      if (address != oldAddress)
      {
        maxValue = 0;
        minValue = 0xFFFFFFFF;
        oldAddress = address;
      }
      Process::Read32(address, value);
      if (value > maxValue) maxValue = value;
      if (value < minValue) minValue = value;
      Draw::topScrDraw(0);
      Draw::topScrDraw(Utils::Format("Add: %08X", address));
      Draw::topScrDraw(Utils::Format("Max: %08X", maxValue));
      Draw::topScrDraw(Utils::Format("Min: %08X", minValue));
      Draw::topScrDraw(Utils::Format("Now: %08X", value));
    });
  }

  void rangeWrite(MenuEntry* entry)
  {
    RangeWriteManager rwm;
    int menu;
    StringVector options = {"範囲書き込み", "戻す", "サーチ結果メモリ設定"};
    Keyboard kb("実行したい処理を選択して下さい", options);
    menu = kb.Open();
    if (menu < 0) return;

    switch (menu)
    {
    case 0:
      if (rwm.SearchDataInput()) break;
      if (rwm.SearchWrite()) break;
      break;
    
    case 1:
      if (rwm.ReturnWrite()) break;
      break;

    case 2:
      if (rwm.SetFreeAddress()) break;
      break;
    
    default:
      MessageBox(Color::Red << "エラー", "不明な選択肢")();
      break;
    }
  }
}
