
// Zim Cartridge Emulator 
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#ifndef Menu_h
#define Menu_h

#include <Arduino.h>
#include "Rfid.h"

namespace ButtonEnum
{
  enum Type
  {
    right,
    up,
    down,
    left,
    select,
    none = 0xFF
  };
}

namespace ButtonStateEnum
{
  enum Type
  {
    pressed,
    hold,
    released
  };
}

namespace FilamentSelectedEnum
{
  enum Type
  {
    left = 0,
    right = 1
  };
  static const byte Mask = 0xF0;
}

namespace ItemSelectedEnum
{
  enum Type
  {
    color,
    type,
    temp,
    tempFirst,
    len,
    used,
    unused
  };
  static const byte Mask = 0xF0;
}

class Menu
{
private:
  static const byte TEMPERATURE_MAX;
  static const byte TEMPERATURE_MIN;
  static const byte TEMPERATURE_OFFSET;
  static const uint8_t BUTTON_PORT;
  static const unsigned long HOLD_EVENTS_START;
  static const unsigned long HOLD_EVENTS_MAX;
  static const unsigned long HOLD_EVENTS_MIN;
  static const long FILAMENT_LENGTH_MAX;
    
public:
  Menu(Rfid * pLeft, Rfid * pRight);
  void init();
  void updateLcd();  
  void buttonDebounce();
  void runFsm();
  void onButtonPressed(ButtonEnum::Type button);
  void onButtonHeld(ButtonEnum::Type button);
  void onButtonReleased(ButtonEnum::Type button);
  void incSelected(ItemSelectedEnum::Type item);
  void decSelected(ItemSelectedEnum::Type item);
  void saveSelected(ItemSelectedEnum::Type item);
  void showSelected(ItemSelectedEnum::Type item, bool edit = false);
  
private:  
  void readCartridgeParams();

  int                         adcSample_;
  int                         adcRead_;
  FilamentSelectedEnum::Type  filament_;
  ItemSelectedEnum::Type      item_;
  ButtonEnum::Type            button_;
  ButtonStateEnum::Type       buttonNow_;
  ButtonStateEnum::Type       buttonState_;
  unsigned long               holdTimer_;
  unsigned long               holdRate_;  
  bool                        edit_;
  bool                        refresh_;

  // Cartridge pointers
  Rfid * pLeft_;
  Rfid * pRight_;
  Rfid * pSelected_;
  
  // Editable cartridge parameters
  ColorEnum::Type color_;
  Material::Type  material_;
  long            initialLength_;
  long            usedLength_;
  byte            temperature_;    
  byte            temperatureFirst_;
};

#endif


















