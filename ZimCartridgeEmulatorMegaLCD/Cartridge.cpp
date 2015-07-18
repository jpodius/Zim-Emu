// Zim Cartridge Emulator
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Cartridge.h"

//const long Cartridge::LENGTH_DEFAULT = 200000;
//const byte Cartridge::TEMPERATURE_DEFAULT_FIRST = 0x5F;
//const byte Cartridge::TEMPERATURE_DEFAULT_PRINT = 0x55;
  
CartridgeData::CartridgeData(int id) :
    id_(id),
    magicNum_(0x5C12),
    type_(CartridgeType::refillable),
    material_(Material::PLA),
    red_(0xFF),
    green_(0xFF),
    blue_(0xFF),
    initLen_(200000/*Cartridge::LENGTH_DEFAULT*/),
    usedLen_(0),
    tempPrint_(0x5F/*Cartridge::TEMPERATURE_DEFAULT_PRINT*/),
    tempFirst_(0x55/*Cartridge::TEMPERATURE_DEFAULT_FIRST*/),
    date_(0x0217),
    xor_(0)
{  
}

const char* Cartridge::Materials[]=
{
  "PLA",
  "ABS",
  "PVA"
};

const char* Cartridge::Colors[]=
{
  "Black",
  "White",
  "Gray",
  "Cyan",
  "Orange",
  "Brown",
  "Red",
  "Yellow",
  "Blue",
  "Green",
  "Purple",
  "Pink"
};

Cartridge::Cartridge(int id, int eepromLoc) : data_(id),
                                              eepromLoc_(eepromLoc)
{                                      
}

/// Return a color string based upon the color enum
String
Cartridge::getMaterial(Material::Type material)
{
  return Materials[material];
}


/// Return a color string based upon the color enum
String
Cartridge::getColor(ColorEnum::Type color)
{
  return Colors[color];
}

ColorEnum::Type
Cartridge::getColor(byte red, byte green, byte blue)
{
  unsigned long rgb = 0;
  rgb = (int)red & 0xFF;
  rgb <<= 8;
  rgb += (int)green & 0xFF;
  rgb <<= 8;
  rgb += (int)blue & 0xFF;
  
  switch(rgb)
  { 
    case ColorValue::black:
      return ColorEnum::black;
      break;

    case ColorValue::white:
      return ColorEnum::white;
      break;

    case ColorValue::gray:
      return ColorEnum::gray;
      break;

    case ColorValue::cyan:
      return ColorEnum::cyan;
      break;

    case ColorValue::orange:
      return ColorEnum::orange;
      break;

    case ColorValue::brown:
      return ColorEnum::brown;
      break;

    case ColorValue::red:
      return ColorEnum::red;
      break;

    case ColorValue::yellow:
      return ColorEnum::yellow;
      break;   

    case ColorValue::blue:
      return ColorEnum::blue;
      break;

    case ColorValue::green:
      return ColorEnum::green;
      break;

    case ColorValue::purple:
      return ColorEnum::purple;
      break;

    case ColorValue::pink:
      return ColorEnum::pink;
      break;                  

    default:
      return ColorEnum::white;
  }
  return ColorEnum::white;
}

/// Set the color on the cartridge based upon the color enum
void
Cartridge::setColor(ColorEnum::Type color)
{ 
  unsigned long rgb = 0;
  switch(color)
  { 
    case ColorEnum::black:
      rgb = ColorValue::black;
      break;

    case ColorEnum::white:
      rgb = ColorValue::white;
      break;

    case ColorEnum::gray:
      rgb = ColorValue::gray;
      break;

    case ColorEnum::cyan:
      rgb = ColorValue::cyan;
      break;

    case ColorEnum::orange:
      rgb = ColorValue::orange;
      break;

    case ColorEnum::brown:
      rgb = ColorValue::brown;
      break;

    case ColorEnum::red:
      rgb = ColorValue::red;
      break;

    case ColorEnum::yellow:
      rgb = ColorValue::yellow;
      break;   

    case ColorEnum::blue:
      rgb = ColorValue::blue;
      break;

    case ColorEnum::green:
      rgb = ColorValue::green;
      break;

    case ColorEnum::purple:
      rgb = ColorValue::purple;
      break;

    case ColorEnum::pink:
      rgb = ColorValue::pink;
      break;                  

    default:
      rgb = ColorValue::white;
      break;
  }

  data_.red_ = (rgb>>16) & 0xFF;
  data_.green_ = (rgb>>8) & 0xFF;
  data_.blue_ = rgb & 0xFF;
}

/// Set the color on the cartridge based upon the RGB value
void
Cartridge::setColor(byte red, byte green, byte blue)
{ 
  data_.red_ = red;
  data_.green_ = green;
  data_.blue_ = blue;
}

