// Zim Cartridge Emulator 
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#ifndef Cartridge_h
#define Cartridge_h

#include <Arduino.h>


#define NEVER_ENDING_FILAMENT       0      // If set, this will ignore filament used length writes
#define CARTRIDGE_ID_LEFT           0x1234 // unique id for cartridge (set different for left and right)
#define CARTRIDGE_ID_RIGHT          0x5678 // unique id for cartridge (set different for left and right)
#define CARTRIDGE_DATA_LENGTH       16
#define CARTRIDGE_LEFT_EEPROM_LOC   0
#define CARTRIDGE_RIGHT_EEPROM_LOC  CARTRIDGE_LEFT_EEPROM_LOC + sizeof(Cartridge)
#define CARTRIDGE_MAGIC_NUMBER      0x5C12 // should be 0x5C12. You can change/program/changeback to reset EEPROM to defaults.

namespace CartridgeType
{
  enum Type
  {
    normal = 0x00,
    refillable = 0x01,
  };
  static const byte Mask = 0xF0;
}

namespace Material
{
  enum Type
  {
    PLA = 0x00,
    ABS = 0x01,
    PVA = 0x02,
  };
  static const byte Mask = 0x0F;
}

namespace ColorValue
{
  enum Type
  {
    black   = 0x000000,
    white   = 0xFFFFFF,
    gray    = 0xC0C0C0,
    cyan    = 0x00FFFF,
    orange  = 0xFFA500,
    brown   = 0xA52A2A,
    red     = 0xFF0000,
    yellow  = 0xFFFF00,
    blue    = 0x0000FF,
    green   = 0x008000,
    purple  = 0x800080,
    pink    = 0xFFC0CB
  };
}

namespace ColorEnum
{
  enum Type
  {
    black,
    white,
    gray,
    cyan,
    orange,
    brown,
    red,
    yellow,
    blue,
    green,
    purple,
    pink
  };
}



/// Cartridge information stored in EEPROM
class CartridgeData
{
public:
  CartridgeData(int id);
  
  int                 id_;
  int                 magicNum_;
  CartridgeType::Type type_;
  Material::Type      material_;
  byte                red_;
  byte                green_;
  byte                blue_;
  long                initLen_;
  long                usedLen_;
  byte                tempPrint_;
  byte                tempFirst_;
  int                 date_;
  byte                xor_;
};

class Cartridge
{ 
public:
  //static const long LENGTH_DEFAULT;
  //static const byte TEMPERATURE_DEFAULT_FIRST;
  //static const byte TEMPERATURE_DEFAULT_PRINT;

  Cartridge(int id, int eepromLoc);
  String getMaterial(Material::Type material);
  String getColor(ColorEnum::Type color);
  ColorEnum::Type getColor(byte red, byte green, byte blue);
  void setColor(ColorEnum::Type color); 
  void setColor(byte red, byte green, byte blue);  
  
  CartridgeData       data_;
  int                 eepromLoc_;
  static const char*  Materials[];
  static const char*  Colors[];
};

#endif
