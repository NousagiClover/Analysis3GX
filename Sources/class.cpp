#include "class.hpp"

namespace CTRPluginFramework
{
  /* Draw */

  void Draw::topScrDraw(std::string text)
  {
    Screen topScr = OSD::GetTopScreen();
    topScr.Draw(text, 0, posY);
    Draw::posY += 10;
  }
  void Draw::topScrDraw(int pos)
  {
    Draw::posY = pos;
  }

  u32 Draw::posY = 0;

  /* RangeWriteManager */

  void RangeWriteManager::ErrorMessage(std::string text)
  {
    MessageBox(Color::Red << "エラー", text)();
  }
  bool RangeWriteManager::AccessCheck(u32 &address)
  {
    if (! Process::CheckAddress(address, MEMPERM_READWRITE))
    {
      ErrorMessage("アドレスにアクセス権限がありません\n" +
      Utils::Format("Address: 0x%08X", address));
      address = 0;
      return false;
    }
    return true;
  }
  bool RangeWriteManager::SetFreeAddress()
  {
    Keyboard kb("サーチ結果を記録するアドレスを入力して下さい\n"
    "0を入力すると初期設定になります");
    if (kb.Open(freeAddress, freeAddress) != 0)
    {
      freeAddress = FREEADDRESS;
      return true;
    }
    if (freeAddress == 0) freeAddress = FREEADDRESS;
    if (! AccessCheck(freeAddress))
    {
      freeAddress = FREEADDRESS;
    }
    OSD::Notify(Utils::Format("[SET] Search Memory: 0x%08X", freeAddress));
    return false;
  }
  bool RangeWriteManager::SearchDataInput()
  {
    Keyboard kbStart("開始アドレスを入力して下さい");
    Keyboard kbEnd("終了アドレスを入力して下さい");
    Keyboard kbTarget("検索対象となる値を入力して下さい");
    Keyboard kbWrite("置換する値を入力して下さい");
    if (kbStart.Open(startAddress, startAddress) != 0)
    {
      return true;
    }
    if (! AccessCheck(startAddress)) return true;
    if (startAddress == freeAddress)
    {
      ErrorMessage("サーチ結果記録アドレスは開始アドレスに設定できません\n" +
      Utils::Format("Address: %08X", startAddress));
      return true;
    }

    if (kbEnd.Open(endAddress, endAddress) != 0)
    {
      return true;
    }
    if (! AccessCheck(endAddress)) return true;

    if (kbTarget.Open(targetValue, targetValue) != 0)
    {
      return true;
    }

    if (kbWrite.Open(writeValue, writeValue) != 0)
    {
      return true;
    }
    return false;
  }
  bool RangeWriteManager::SearchWrite()
  {
    if (startAddress > endAddress)
    {
      ErrorMessage("開始アドレスが終了アドレスより大きいです\n" +
      Utils::Format("開始アドレス: %08X\n", startAddress) +
      Utils::Format("終了アドレス: %08X", endAddress));
      return true;
    }

    u32 rangeSize = (endAddress - startAddress + 4) / 4;
    u32 free = freeAddress;
    u32 value;

    for (int i = 0; i < rangeSize; i++)
    {
      Process::Read32(startAddress + i * 4, value);
      if (targetValue == value)
      {
        Process::Write32(startAddress + i * 4, writeValue);
        Process::Write32(free, startAddress + i * 4);
        free += 4;
        hits++;
        searched = true;
      }
    }
    OSD::Notify(Utils::Format("StartAddress: %08X", startAddress));
    OSD::Notify(Utils::Format("EndAddress  : %08X", endAddress));
    OSD::Notify(Utils::Format("Target      : %08X", targetValue));
    OSD::Notify(Utils::Format("Replace     : %08X", writeValue));
    OSD::Notify(Utils::Format("%d Address hits!", hits));
    return false;
  }
  bool RangeWriteManager::ReturnWrite()
  {
    if (! searched)
    {
      ErrorMessage("戻すデータが見つかりません");
      return true;
    }
    u32 address;
    for (int i = 0; i < hits; i++)
    {
      Process::Read32(freeAddress + i * 4, address);
      Process::Write32(address, targetValue);
    }
    hits = 0;
    searched = false;
    OSD::Notify("Reset!");
    return false;
  }

  u32 RangeWriteManager::freeAddress = FREEADDRESS;
  u32 RangeWriteManager::startAddress = 0;
  u32 RangeWriteManager::endAddress = 0;
  u32 RangeWriteManager::targetValue = 0;
  u32 RangeWriteManager::writeValue = 0;
  u32 RangeWriteManager::hits = 0;
  bool RangeWriteManager::searched = false;
}
