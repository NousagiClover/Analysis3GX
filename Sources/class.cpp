#include "class.hpp"

namespace CTRPluginFramework
{
  /* Global */

  void ErrorMessage(std::string text)
  {
    MessageBox(Color::Red << "エラー", text)();
  }
  void SuccessMessage(std::string text)
  {
    MessageBox(Color::Lime << "成功", text)();
  }
  void ConfMessage(std::string text)
  {
    MessageBox("確認", text)();
  }
  bool AccessCheck(u32 &address)
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
  void VectorU32ToString(std::vector<u32> list, std::string &text, WriteMode writeMode = OVERRIDE)
  {
    if (writeMode == OVERRIDE) text = "";
    for (int i = 0; i < list.size(); i++)
    {
      text += Utils::Format("[No.%d] %08X\n", i + 1, list[i]);
    }
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
  u32 GetBranch(const u32 branchScr, const u32 branchDst)
  {
    int res = (branchDst - branchScr) / 4 - 2;
    res &= 0xFFFFFF;
    return (u32)res;
  }
  

  /* Draw */

  void Draw::DrawTopScr(std::string text, int posXAddValue, int posYAddValue)
  {
    const Screen &topScr = OSD::GetTopScreen();
    topScr.Draw(text, posX, posY);
    Draw::posX += posXAddValue;
    Draw::posY += posYAddValue;
  }
  void Draw::DrawBottomScr(std::string text, int posXAddValue, int posYAddValue)
  {
    const Screen &bottom = OSD::GetBottomScreen();
    bottom.Draw(text, posX, posY);
    Draw::posX += posXAddValue;
    Draw::posY += posYAddValue;
  }
  void Draw::DrawPosition(int posX, int posY)
  {
    Draw::posX = posX;
    Draw::posY = posY;
  }
  void Draw::GetDrawPosition(int &posX, int &posY)
  {
    posX = Draw::posX;
    posY = Draw::posY;
  }

  u32 Draw::posX = 0;
  u32 Draw::posY = 0;


  /* RangeWriteManager */

  bool RangeWriteManager::SearchWrite()
  {
    if (searched)
    {
      ErrorMessage("サーチデータが戻されていません。\n戻すを選択して下さい");
      return true;
    }
    Keyboard kbStart("開始アドレスを入力して下さい");
    Keyboard kbEnd("終了アドレスを入力して下さい");
    Keyboard kbTarget("検索対象となる値を入力して下さい");
    Keyboard kbWrite("置換する値を入力して下さい");
    if (kbStart.Open(startAddress, startAddress) != 0)
    {
      return true;
    }
    if (! AccessCheck(startAddress)) return true;

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

    if (startAddress > endAddress)
    {
      ErrorMessage("開始アドレスが終了アドレスより大きいです\n" +
      Utils::Format("開始アドレス: %08X\n", startAddress) +
      Utils::Format("終了アドレス: %08X", endAddress));
      return true;
    }

    u32 rangeSize = (endAddress - startAddress + 4) / 4;
    u32* freeAdrPtr = freeAddress;
    u32 value;

    for (int i = 0; i < rangeSize; i++)
    {
      u32 address = startAddress + i * 4;
      if (! AccessCheck(address))
      {
        searched = true;
        return true;
      }
      Process::Read32(address, value);
      if (targetValue == value)
      {
        Process::Write32(address, writeValue);
        Process::Write32((u32)freeAdrPtr, address);
        freeAdrPtr++;
        hits++;
      }
    }
    OSD::Notify(Utils::Format("StartAddress: %08X", startAddress));
    OSD::Notify(Utils::Format("EndAddress  : %08X", endAddress));
    OSD::Notify(Utils::Format("Target      : %08X", targetValue));
    OSD::Notify(Utils::Format("Replace     : %08X", writeValue));
    OSD::Notify(Utils::Format("%d Address hits!", hits));
    searched = true;
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
      Process::Read32((u32)freeAddress + i * 4, address);
      Process::Write32(address, targetValue);
    }
    hits = 0;
    searched = false;
    OSD::Notify("Reset!");
    return false;
  }
  void RangeWriteManager::FreeAddressConf()
  {
    MessageBox("確認", Utils::Format("サーチ結果記録アドレス\nAddress: 0x%08X", freeAddress))();
  }

  u32 RangeWriteManager::freeAddress[2048];
  u32 RangeWriteManager::startAddress = 0;
  u32 RangeWriteManager::endAddress = 0;
  u32 RangeWriteManager::targetValue = 0;
  u32 RangeWriteManager::writeValue = 0;
  u32 RangeWriteManager::hits = 0;
  bool RangeWriteManager::searched = false;
}
