// Zim Cartridge Emulator
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <EEPROM.h>
#include "Rfid.h"
#include "Menu.h"

Payload::Payload() :
                  addr_(0),
                  len_(0),
                  funcCode_(RfidCommand::none),
                  index_(0),
                  msb_(false)
{
}


Rfid::Rfid(String name, HardwareSerial * serial, Cartridge cartridge) :  
                                name_(name),
                                cartridge_(cartridge),
                                state_(SerialState::idle),
                                serial_(serial),
                                timeout_(0),
                                updated_(true)
{  
}

// State machine for parsing the YET-MF2 Mifare protocol
void 
Rfid::runFsm()
{
  byte rx = 0;

  if(state_ != SerialState::idle &&
     (millis() - timeout_) > RX_TIMEOUT)
  {
    Serial.println("Rx timeout");
    state_ = SerialState::idle;
  }

  if(serial_->available())
  {
    rx = (byte)serial_->read();
    Serial.print(rx, HEX);
    Serial.print(" ");
  
    switch(state_)
    {
      case SerialState::idle:
        if(rx == 0xAA)
        {        
          timeout_ = millis();
          state_ = SerialState::start;
        }
        break;
  
      case SerialState::start:
        if(rx == 0xBB)
        {
          payload_.msb_ = false;
          state_ = SerialState::len;
        }
        break;
  
      case SerialState::len:
        if(payload_.msb_ == false)
        {
          payload_.len_ = rx;
          payload_.msb_ = true;
        }
        else
        {
          payload_.len_ += rx<<8;
          payload_.len_ -= 5; // only get payload
          payload_.msb_ = false;
          state_ = SerialState::address;
        }
        break;
  
      case SerialState::address:
        if(payload_.msb_ == false)
        {
          payload_.addr_ = rx;
          payload_.msb_ = true;
        }
        else
        {
          payload_.addr_ += rx<<8;
          payload_.msb_ = false;
          state_ = SerialState::funcCode; 
        }
        break;
        
      case SerialState::funcCode:
        if(payload_.msb_ == false)
        {    
          payload_.funcCode_ = RfidCommand::Type(rx);
          payload_.msb_ = true;
        }
        else
        {
          int temp = (int)payload_.funcCode_;
          temp += rx<<8;
          payload_.funcCode_ = RfidCommand::Type(temp);
          payload_.index_ = 0;
          payload_.msb_ = false;     
          if(payload_.len_ == 0)
          {
            // no payload data
            state_ = SerialState::xorCheck;
          }
          else
          {
            state_ = SerialState::data;
          }
        }    
        break;
             
      case SerialState::data:        
        if(payload_.index_ < payload_.len_)
        {       
          payload_.payload_[payload_.index_++] = rx;
          break;
        }
        else
        {
          state_ = SerialState::xorCheck;
        }
        // falls through...
        
      case SerialState::xorCheck:
        // Doesn't currently perform an XOR error check.
        state_ = SerialState::complete;
        // falls through...
      
      case SerialState::complete:  
        Serial.print("\nReceived ");
        Serial.print(payload_.index_);
        Serial.print(" bytes from Zim for ");
        Serial.println(name_);
        Serial.print("FuncCode:");
        Serial.println(payload_.funcCode_, HEX);
        Serial.print("Payload Length:");
        Serial.println(payload_.len_, HEX);      
        Serial.print("Data:");
        for(int i=0; i<payload_.len_; ++i)
        {
          Serial.print("0x");
          Serial.print(payload_.payload_[i], HEX);
          Serial.print(" ");
        }
        Serial.println("");
           
        handleRequest(payload_.funcCode_, payload_.payload_, payload_.len_);
        state_ = SerialState::idle;
        break;
  
      default:
        break;
    }
  }  
}

// Generates responses for Mifare protocol.
// Format is: uint16 header (0xAABB) - uint16 len - uint16 nodeId - uint16 func code - uint8 status - uint8 n data - uint8 XOR
void 
Rfid::sendResponse(byte * pPayload, int len)
{
  int  index = 0;
  int  pktLen = 0;
  byte rsp[50];
  int  rspLen = 0;

  pktLen = len + 6; // includes crc
  rsp[index++] = 0xAA;
  rsp[index++] = 0xBB;
  rsp[index++] = pktLen & 0xFF;
  rsp[index++] = pktLen>>8;
  rsp[index++] = payload_.addr_ & 0xFF;
  rsp[index++] = payload_.addr_>>8;
  rsp[index++] = payload_.funcCode_ & 0xFF;
  rsp[index++] = payload_.funcCode_>>8;
  rsp[index++] = 0; // status: success

  // stuff payload
  if(pPayload != NULL)
  {
    for(int i=0; i<len; ++i)
    {
      if(pPayload[i] == 0xAA)
      {
        rsp[index++] = 0x00; // escape 0xAA
      }
      rsp[index++] = pPayload[i];    
    }
  }

  // compute XOR
  byte xorVal = 0;
  for(int i=6; i<index; ++i)
  {
     xorVal ^= rsp[i];
  }

  rsp[index++] = xorVal;
  rspLen = index;

  Serial.print("Sending ");
  Serial.print(rspLen);
  Serial.print(" bytes to Zim for ");
  Serial.println(name_);

  for(int i=0; i<rspLen; ++i)
  {
      serial_->write(rsp[i]);
  }
}

// Handles Mifare requests specific to Zim, and sends appropriate responses
void 
Rfid::handleRequest(RfidCommand::Type funcCode, byte * preq, int len)
{
  switch(funcCode)
  {
    case RfidCommand::initPort:
    {
   
      Serial.println("Initialize Port");   
      sendResponse(NULL, 0);
    }
    break;
    
    case RfidCommand::setAntennaStatus:
    {   
      Serial.println("Set Antenna Status");     
      // No response
    }
    break;
    
    case RfidCommand::request:
    {
      Serial.println("Mifare Request");
      byte payload[] = {0x44, 0x00};
      sendResponse(payload, sizeof(payload));
    }
    break;
    
    case RfidCommand::antiCollision:
    {
      Serial.println("Mifare Anticollision");
      byte payload[4];
      payload[0] = 0x88;
      payload[1] = 0x04;
      payload[2] = cartridge_.data_.id_>>8;
      payload[3] = cartridge_.data_.id_ & 0xFF;
      sendResponse(payload, sizeof(payload));
    } 
    break;

    case RfidCommand::select:
    {
      Serial.println("Mifare Select");
      byte payload[] = {0x04};
      sendResponse(payload, sizeof(payload));
    } 
    break;

    case RfidCommand::halt:
    {
      Serial.println("Mifare Halt");
      sendResponse(NULL, 0);
    } 
    break;

    case RfidCommand::readData:
    {
      Serial.println("Mifare Read");
      byte payload[CARTRIDGE_DATA_LENGTH];
      int len = buildCartridgePayload(payload);
      sendResponse(payload, len);
    }
    break;

    case RfidCommand::writeData:
    {
      byte page = preq[0];
      Serial.print("Mifare Write for page ");
      Serial.println(page);

      if(page == 6)
      {
        cartridge_.data_.magicNum_ = preq[1]<<8;
        cartridge_.data_.magicNum_ |= preq[2];
        cartridge_.data_.type_ = CartridgeType::Type(preq[3]>>4);
        cartridge_.data_.material_ = Material::Type(preq[3] & 0x0F);
        cartridge_.data_.red_ = preq[4];
        
      }
      else if(page == 7)
      {
        cartridge_.data_.green_    = preq[1];
        cartridge_.data_.blue_     = preq[2];
        cartridge_.data_.initLen_  = ((long)preq[3])<<12;
        cartridge_.data_.initLen_ |= ((long)preq[4])<<4;        
      }
      else if(page == 8)
      {
        cartridge_.data_.initLen_ |= ((long)preq[1]&0xF0)>>4;
        cartridge_.data_.usedLen_  = ((long)preq[1]&0x0F)<<16;
        cartridge_.data_.usedLen_ |= ((long)preq[2])<<8;
        cartridge_.data_.usedLen_ |= ((long)preq[3]); 
        cartridge_.data_.tempPrint_ = preq[4];
        
      }
      else if(page == 9)
      {
        cartridge_.data_.tempFirst_ = preq[1];
        cartridge_.data_.date_    = preq[2]<<8;
        cartridge_.data_.date_   |= preq[3];
        cartridge_.data_.xor_  = preq[4];  

        Serial.print("Cartridge data received from Zim for ");
        Serial.print(name_);
        Serial.println(":");
        printCartridgeData();
        
#if NEVER_ENDING_FILAMENT == 1
        Serial.println("Your spool runneth over");
        cartridge_.data_.usedLen_ = 0;
#endif
        saveCartridgeData();
      }
      
      sendResponse(NULL, 0);
    }
    break;
    
    default:
      Serial.print("Unhandled request: 0x");
      Serial.println(funcCode, HEX);
    break;    
  }
  Serial.println("");
}

int
Rfid::buildCartridgePayload(byte * pdata)
{
  int index = 0;
  pdata[index++] = cartridge_.data_.magicNum_>>8;
  pdata[index++] = cartridge_.data_.magicNum_ & 0xFF;
  pdata[index++] = cartridge_.data_.type_<<4 | cartridge_.data_.material_ & Material::Mask;
  pdata[index++] = cartridge_.data_.red_;
  pdata[index++] = cartridge_.data_.green_;
  pdata[index++] = cartridge_.data_.blue_;
  pdata[index++] = (cartridge_.data_.initLen_>>12) & 0xFF;
  pdata[index++] = (cartridge_.data_.initLen_>>4) & 0xFF;
  pdata[index++] = (cartridge_.data_.initLen_<<4 | cartridge_.data_.usedLen_>>16) & 0xFF;
  pdata[index++] = (cartridge_.data_.usedLen_>>8) & 0xFF;
  pdata[index++] = (cartridge_.data_.usedLen_) & 0xFF;
  pdata[index++] = cartridge_.data_.tempPrint_;
  pdata[index++] = cartridge_.data_.tempFirst_;
  pdata[index++] = cartridge_.data_.date_>>8;
  pdata[index++] = cartridge_.data_.date_ & 0xFF;

  byte xorVal = 0;
  for(int i=0; i<index; ++i)
  {
    xorVal ^= pdata[i];
  }
  pdata[index++] = xorVal;
  return index;
}

void 
Rfid::printCartridgeData()
{
  Serial.print("Name: ");
  Serial.println(name_);
  Serial.print("ID: 0x");
  Serial.println(cartridge_.data_.id_, HEX);
  Serial.print("EEPROM Location: 0x");
  Serial.println(cartridge_.eepromLoc_, HEX);
  Serial.print("Magic Number: 0x");
  Serial.println(cartridge_.data_.magicNum_, HEX);
  Serial.print("Type:");
  Serial.println(cartridge_.data_.type_);
  Serial.print("Material:");
  Serial.println(cartridge_.data_.material_);
  Serial.print("Red: 0x");
  Serial.println(cartridge_.data_.red_, HEX);
  Serial.print("Green: 0x");
  Serial.println(cartridge_.data_.green_, HEX);
  Serial.print("Blue: 0x");
  Serial.println(cartridge_.data_.blue_, HEX);
  Serial.print("Initial Length: ");
  Serial.println(cartridge_.data_.initLen_);
  Serial.print("Used Length: ");
  Serial.println(cartridge_.data_.usedLen_);
  Serial.print("Temp Print: ");
  Serial.println(cartridge_.data_.tempPrint_);
  Serial.print("Temp Start: ");
  Serial.println(cartridge_.data_.tempFirst_);
  Serial.print("Date: 0x");
  Serial.println(cartridge_.data_.date_, HEX);
  Serial.print("Xor: 0x");
  Serial.println(cartridge_.data_.xor_, HEX);
  Serial.println("");
}

void Rfid::saveCartridgeData()
{
  Serial.print("Saving cartridge data to eeprom for: ");
  Serial.print(name_);
  Serial.print(" at location: ");
  Serial.println(cartridge_.eepromLoc_, HEX);
  EEPROM.put(cartridge_.eepromLoc_, cartridge_.data_);
  updated_ = true;
}

bool Rfid::isUpdated()
{
  bool rval = updated_;
  updated_ = false;
  return rval;
}

