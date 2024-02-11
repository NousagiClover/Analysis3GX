#include "cheats.hpp"

#define FREEADDRESS 0x01E81000

namespace CTRPluginFramework
{
  enum WriteMode
  {
    APPEND,
    OVERRIDE
  };

  void ErrorMessage(std::string text);
  void SuccessMessage(std::string text);
  void ConfMessage(std::string text);
  void VectorU32ToString(std::vector<u32> list, std::string &text, WriteMode writeMode);
  bool AccessCheck(u32 &address);
  bool isCovered(std::vector<u32> list, u32 value);
  u32 GetBranch(const u32 branchScr, const u32 branchDst);
  

  class Draw
  {
    private:
      static u32 posX;
      static u32 posY;
    public:
      static void DrawTopScr(std::string text, int posXAddValue = 0, int posYAddValue = 10);
      static void DrawBottomScr(std::string text, int posXAddValue = 0, int posYAddValue = 10);
      static void DrawPosition(int posX = 0, int posY = 0);
      static void GetDrawPosition(int &posX, int &posY);
  };

  class RangeWriteManager
  {
    private:
      static u32 freeAddress[];
      static u32 startAddress;
      static u32 endAddress;
      static u32 targetValue;
      static u32 writeValue;
      static u32 hits;
      static bool searched;
    public:
      static bool SearchWrite();
      static bool ReturnWrite();
      static void FreeAddressConf();
  };
}
