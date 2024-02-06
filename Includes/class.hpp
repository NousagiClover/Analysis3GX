#include "cheats.hpp"

#define FREEADDRESS 0x01E81000

namespace CTRPluginFramework
{
  class Draw
  {
    private:
      static u32 posY;
    public:
      static void topScrDraw(std::string text);
      static void topScrDraw(int pos);
  };

  class RangeWriteManager
  {
    private:
      void ErrorMessage(std::string text);
      bool AccessCheck(u32 &address);
      static u32 freeAddress;
      static u32 startAddress;
      static u32 endAddress;
      static u32 targetValue;
      static u32 writeValue;
      static u32 hits;
      static bool searched;
    public:
      bool SetFreeAddress();
      bool SearchDataInput();
      bool SearchWrite();
      bool ReturnWrite();
  };
}
