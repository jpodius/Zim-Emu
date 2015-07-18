// Zim Cartridge Emulator
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#include <LiquidCrystal.h>

#include "Menu.h"
#include "Rfid.h"

// LCD
// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

ItemSelectedEnum::Type & operator--(ItemSelectedEnum::Type & selected) 
{ 
  if(selected > ItemSelectedEnum::color)
  {
    int val = static_cast<int>(selected);
    --val;
    selected = ItemSelectedEnum::Type(val);
  }
  return selected; 
} 

ItemSelectedEnum::Type & operator++(ItemSelectedEnum::Type & selected) 
{ 
  if(selected < ItemSelectedEnum::unused)
  {
    int val = static_cast<int>(selected);
    ++val;
    selected = ItemSelectedEnum::Type(val);
  }
  return selected; 
}


const byte Menu::TEMPERATURE_MAX = 250;
const byte Menu::TEMPERATURE_MIN = 150;
const byte Menu::TEMPERATURE_OFFSET = 100;
const uint8_t Menu::BUTTON_PORT = 0;
const unsigned long Menu::HOLD_EVENTS_START = 1200;
const unsigned long Menu::HOLD_EVENTS_MAX = 600;
const unsigned long Menu::HOLD_EVENTS_MIN = 50;
const long Menu::FILAMENT_LENGTH_MAX = 200000;//600000;

/// Ctor
Menu::Menu(Rfid * pLeft, Rfid * pRight) : 
                                adcSample_(0),
                                adcRead_(0),
                                filament_(FilamentSelectedEnum::left),
                                item_(ItemSelectedEnum::color),
                                button_(ButtonEnum::none),
                                buttonNow_(ButtonStateEnum::released),
                                buttonState_(ButtonStateEnum::released),
                                holdTimer_(0),
                                holdRate_(HOLD_EVENTS_START),
                                edit_(false),
                                refresh_(true),
                                pLeft_(pLeft),
                                pRight_(pRight),
                                pSelected_(pLeft)
//                                color_(ColorEnum::white),
//                                material_(Material::PLA),
//                                initialLength_(0),
//                                usedLength_(0),
//                                temperature_(0),
//                                temperatureFirst_(0)                                          
{  
}

/// Should be called after EEPROM has initialized cartridge parameters
void 
Menu::init()
{
  lcd.begin(16, 2);              // start the library
  lcd.setCursor(0,0);
  lcd.noCursor();
  lcd.print("Zim-Emu v1.0"); 
  delay(1000);
  updateLcd();
  
}

void Menu::updateLcd()
{
  Serial.println("Updating lcd");
  //readCartridgeParams();
  showSelected(item_, edit_);
}

void Menu::buttonDebounce()
{
    byte newRead;
    static byte lastRead = (int)ButtonEnum::none;
    static byte result = (int)ButtonEnum::none;
    static byte lastResult = (int)ButtonEnum::none;
    ButtonEnum::Type button = ButtonEnum::none;

    int adc = analogRead(BUTTON_PORT);
    if (adc < 50)   
      newRead = (int)ButtonEnum::right;
    else if (adc < 250) 
      newRead = (int)ButtonEnum::up; 
    else if (adc < 450) 
      newRead = (int)ButtonEnum::down; 
    else if (adc < 650)
      newRead = (int)ButtonEnum::left; 
    else if (adc < 850)  
      newRead = (int)ButtonEnum::select;
    else if (adc > 1000)
      newRead = (int)ButtonEnum::none;

    // Debounce (obtuse but effective)
    result = (newRead & lastRead & ~result) | (result & newRead) | (result & lastRead);
    lastRead = newRead;

    // Set resulting button state
    if(result != lastResult)
    {                  
        if(ButtonEnum::Type(result) != ButtonEnum::none)
        {
          button_ = ButtonEnum::Type(result);
          buttonNow_ = ButtonStateEnum::pressed;
        }
        else
        {
          buttonNow_ = ButtonStateEnum::released;
        }
        lastResult = result; 
    }
}

/// State machine for LCD menu
void 
Menu::runFsm()
{
  if(pSelected_->isUpdated() == true)
  {
    updateLcd();  
  }
  
  buttonDebounce(); 
  switch(buttonState_)
  {
    case ButtonStateEnum::released:
      if(buttonNow_ == ButtonStateEnum::pressed)
      {
        holdTimer_ = millis();
        holdRate_ = HOLD_EVENTS_START;
        buttonState_ = ButtonStateEnum::pressed;
        onButtonPressed(button_);  
      }
      break;
      
    case ButtonStateEnum::pressed:
      if(buttonNow_ ==  ButtonStateEnum::released)
      {
        buttonState_ = ButtonStateEnum::released;
        onButtonReleased(button_);
      }
      else if(millis() - holdTimer_ > holdRate_)
      {
        holdTimer_ = millis();
        holdRate_ = HOLD_EVENTS_MAX;
        buttonState_ = ButtonStateEnum::hold;
        onButtonHeld(button_);
      }
      break;

    case ButtonStateEnum::hold:
      if(buttonNow_ ==  ButtonStateEnum::released)
      {
        buttonState_ = ButtonStateEnum::released;
        onButtonReleased(button_);
      }
      else if(millis() - holdTimer_ > holdRate_)
      {
        holdTimer_ = millis();
        if(holdRate_ > HOLD_EVENTS_MIN)
          holdRate_ -= 50;
        onButtonHeld(button_);
      }
      break;

    default:
      break;
  }
}

void Menu::onButtonPressed(ButtonEnum::Type button)
{
  switch(button)
  {
    case ButtonEnum::left:
      filament_ = FilamentSelectedEnum::left;
      refresh_ = true;
      pSelected_ = pLeft_;
      //readCartridgeParams();
      break;
    
    case ButtonEnum::right:
      filament_ = FilamentSelectedEnum::right;
      refresh_ = true;
      pSelected_ = pRight_;
     // readCartridgeParams();
      break;

    case ButtonEnum::up:
      if(!edit_)
      {
        --item_;
        refresh_ = true;
      }
      else
        incSelected(item_);
      break;

    case ButtonEnum::down:
      if(!edit_)
      {
        ++item_;
        refresh_ = true;
      }
      else
        decSelected(item_);
      break;

    case ButtonEnum::select:
      if(edit_)
      { 
        // Select pressed while editing an item
        pSelected_->saveCartridgeData();
        edit_ = false;
      }
      else if(item_ != ItemSelectedEnum::unused)
      {
        // Select pressed while selecting an items (can't edit "unused" item)
        edit_ = true; 
      }
      refresh_ = true;
      break;

    default:
      break;
  }

  if(refresh_)
  {
    refresh_ = false;
    updateLcd();
  }
}

void Menu::onButtonHeld(ButtonEnum::Type button)
{
  switch(button)
  {  
    case ButtonEnum::up:
      if(edit_)
        incSelected(item_);
      break;

    case ButtonEnum::down:
      if(edit_)
        decSelected(item_);
      break;
      
    default:
      break;
  }

  if(refresh_)
  {
    refresh_ = false;
    updateLcd();
  }      
  
}

void Menu::onButtonReleased(ButtonEnum::Type button)
{

}

/// Increment the selected item
void Menu::incSelected(ItemSelectedEnum::Type item)
{
  switch(item)
  {
    case ItemSelectedEnum::color:
      pSelected_->cartridge_.nextColor();
      break;

    case ItemSelectedEnum::type:
      //++material_;
      pSelected_->cartridge_.nextMaterial();
      break;

    case ItemSelectedEnum::temp:
      if((pSelected_->cartridge_.data_.tempPrint_+TEMPERATURE_OFFSET) < TEMPERATURE_MAX)
      {
        pSelected_->cartridge_.data_.tempPrint_++;
      }
      break;

    case ItemSelectedEnum::tempFirst:
      if((pSelected_->cartridge_.data_.tempFirst_+TEMPERATURE_OFFSET) < TEMPERATURE_MAX)
      {
        pSelected_->cartridge_.data_.tempFirst_++;
      }
      break;      
      
    case ItemSelectedEnum::len:
      if(pSelected_->cartridge_.data_.initLen_ < FILAMENT_LENGTH_MAX)
      {      
        pSelected_->cartridge_.data_.initLen_ += 1000;
        if(pSelected_->cartridge_.data_.initLen_ > FILAMENT_LENGTH_MAX)
          pSelected_->cartridge_.data_.initLen_ = FILAMENT_LENGTH_MAX;
      }
      break;

    case ItemSelectedEnum::used:
      if(pSelected_->cartridge_.data_.usedLen_ < FILAMENT_LENGTH_MAX)   
      {   
        pSelected_->cartridge_.data_.usedLen_ += 1000;    
        if(pSelected_->cartridge_.data_.usedLen_ > FILAMENT_LENGTH_MAX)
          pSelected_->cartridge_.data_.usedLen_ = FILAMENT_LENGTH_MAX;
      }
      break;

    default:
      break;
  }
  updateLcd();
}

/// Decrement the selected item
void Menu::decSelected(ItemSelectedEnum::Type item)
{
  switch(item)
  {
    case ItemSelectedEnum::color:
      pSelected_->cartridge_.prevColor();
      break;

    case ItemSelectedEnum::type:
      pSelected_->cartridge_.prevMaterial();
      break;

    case ItemSelectedEnum::temp:
      if((pSelected_->cartridge_.data_.tempPrint_+TEMPERATURE_OFFSET) > TEMPERATURE_MIN)
      {
        pSelected_->cartridge_.data_.tempPrint_--;
      }
      break;

    case ItemSelectedEnum::tempFirst:
      if((pSelected_->cartridge_.data_.tempFirst_+TEMPERATURE_OFFSET) > TEMPERATURE_MIN)      
      {
        pSelected_->cartridge_.data_.tempFirst_--;
      }
      break;       

    case ItemSelectedEnum::len:
      if(pSelected_->cartridge_.data_.initLen_ > 0)
      {      
        pSelected_->cartridge_.data_.initLen_ -= 1000;
        if(pSelected_->cartridge_.data_.initLen_ < 0)
        {
          pSelected_->cartridge_.data_.initLen_ = 0;
        }
      }
      break;

    case ItemSelectedEnum::used:
      if(pSelected_->cartridge_.data_.usedLen_ > 0)
      {     
        pSelected_->cartridge_.data_.usedLen_ -= 1000;
        if(pSelected_->cartridge_.data_.usedLen_ < 0)
        {
          pSelected_->cartridge_.data_.usedLen_ = 0;   
        }
      }
      break;

    default:
      break;
  }
  updateLcd();
}

/// Show the selected item on the LCD
void
Menu::showSelected(ItemSelectedEnum::Type item, bool edit)
{
  lcd.clear();
  lcd.setCursor(0,0);
  if(filament_ == FilamentSelectedEnum::left)
  {  
    lcd.print("Left Filament");
  }
  else
  {
    lcd.print("Right Filament");
  } 

  lcd.setCursor(0,1);
  switch(item)
  {
    case ItemSelectedEnum::color:
      lcd.print("Color:");
      lcd.print(pSelected_->cartridge_.getColorStr());
      break;

    case ItemSelectedEnum::type:
      lcd.print("Type:");
      lcd.print(pSelected_->cartridge_.getMaterialStr());
      break;

    case ItemSelectedEnum::temp:
      lcd.print("Temp:");
      lcd.print(pSelected_->cartridge_.data_.tempPrint_ + TEMPERATURE_OFFSET);      
      lcd.print("C");
      break;

    case ItemSelectedEnum::tempFirst:
      lcd.print("TempFirst:");
      lcd.print(pSelected_->cartridge_.data_.tempFirst_ + TEMPERATURE_OFFSET);
      lcd.print("C");
      break;       

    case ItemSelectedEnum::len:
      {
        lcd.print("Length:");
        float len = pSelected_->cartridge_.data_.initLen_/1000.0;
        lcd.print(len);
        lcd.print("m");
      }
      break;

    case ItemSelectedEnum::used:
      {
        lcd.print("Used:");
        float len = pSelected_->cartridge_.data_.usedLen_/1000.0;
        lcd.print(len);
        lcd.print("m");
      }
      break;

    case ItemSelectedEnum::unused:
      {
        lcd.print("Unused:");
        float len = (pSelected_->cartridge_.data_.initLen_ - pSelected_->cartridge_.data_.usedLen_)/1000.0;
        lcd.print(len);
        lcd.print("m");
      }
      break;                        

    default:
      break;
  }

  if(edit_ && item != ItemSelectedEnum::unused)
    lcd.print("*");
}






















