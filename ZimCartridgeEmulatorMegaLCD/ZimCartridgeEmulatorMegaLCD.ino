//  Zim Cartridge Emulator Mega
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <EEPROM.h>
#include <LiquidCrystal.h>
#include "Menu.h"
#include "Rfid.h"
#include "Cartridge.h"

Cartridge cartridgeLeft(CARTRIDGE_ID_LEFT, CARTRIDGE_LEFT_EEPROM_LOC);
Cartridge cartridgeRight(CARTRIDGE_ID_RIGHT, CARTRIDGE_RIGHT_EEPROM_LOC);
Rfid rfidLeft("Left Cartridge", &Serial1, cartridgeLeft);
Rfid rfidRight("Right Cartridge", &Serial2, cartridgeRight);
Menu menu(&rfidLeft, &rfidRight);

void setup()  
{ 
  Serial.begin(57600);
  Serial.println("Zim Cartridge Emulator Mega v1.0\n");

  // Load cartridge data from eeprom or initialize if never set
  Serial.println("Checking eeprom");
  Cartridge temp(CARTRIDGE_ID_LEFT, CARTRIDGE_LEFT_EEPROM_LOC);
  EEPROM.get(CARTRIDGE_LEFT_EEPROM_LOC, temp);
  if(temp.data_.magicNum_ != CARTRIDGE_MAGIC_NUMBER)
  {
     Serial.println("Reinitializing eeprom");
     EEPROM.put(CARTRIDGE_LEFT_EEPROM_LOC, rfidLeft.cartridge_.data_);
     EEPROM.put(CARTRIDGE_RIGHT_EEPROM_LOC, rfidRight.cartridge_.data_);
  }
  else
  {
    EEPROM.get(CARTRIDGE_LEFT_EEPROM_LOC, rfidLeft.cartridge_.data_);
    EEPROM.get(CARTRIDGE_RIGHT_EEPROM_LOC, rfidRight.cartridge_.data_);
    Serial.println("Cartridge data restored from eeprom");
    rfidLeft.printCartridgeData();
    rfidRight.printCartridgeData();
  }

  rfidLeft.serial_->begin(RFID_BAUD_RATE);
  rfidRight.serial_->begin(RFID_BAUD_RATE); 
  menu.init();
}


// This is called repeatedly
void loop()
{
  rfidLeft.runFsm();
  rfidRight.runFsm();
  menu.runFsm();  
}
















