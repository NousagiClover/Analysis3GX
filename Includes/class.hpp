#include "cheats.hpp"

#define FREEADDRESS 0x01E81000

namespace CTRPluginFramework
{
  void ErrorMessage(std::string text);
  bool AccessCheck(u32 &address);

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
      static u32 freeAddress;
      static u32 startAddress;
      static u32 endAddress;
      static u32 targetValue;
      static u32 writeValue;
      static u32 hits;
      static bool searched;
    public:
      static bool SetFreeAddress();
      static bool SearchWrite();
      static bool ReturnWrite();
  };
}
