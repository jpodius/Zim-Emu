
// Zim Cartridge Emulator Nano
//  
// Copyright (c) <2015> <Joe Doubek>
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#include <SoftwareSerial.h>
#include <EEPROM.h>

#define NEVER_ENDING_FILAMENT   0      // If set, this will ignore filament used length writes
#define CARTRIDGE_ID            0x1234 // unique id for cartridge (set different for left and right)
#define CARTRIDGE_DATA_LENGTH   16
#define CARTRIDGE_EEPROM_LOC    0x00
#define CARTRIDGE_MAGIC_NUMBER  0x5C12 // should be 0x5C12. You can change->run->change back to reset EEPROM though.
#define RFID_BAUD_RATE          19200
#define RX_TIMEOUT              2000 // msecs timeout on receives

#define RFID_LEFT_RX_PIN  10 // D10
#define RFID_LEFT_TX_PIN  11 // D11

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

namespace RfidCommand
{
  enum Type
  {
    none              = 0x0000,
    initPort          = 0x0101,
    setNode           = 0x0102,
    setAntennaStatus  = 0x010C,
    request           = 0x0201,
    antiCollision     = 0x0202,
    select            = 0x0203,
    halt              = 0x0204,
    readData          = 0x0208,
    writeData         = 0x0213
  };
}

namespace SerialState
{
  enum Type
  {
    idle,
    start,
    len,
    address,
    funcCode,
    data,
    xorCheck,
    complete  
  };
}

class Cartridge
{
public:
  Cartridge();

  int                 magicNum_;
  CartridgeType::Type type_;
  Material::Type      material_;
  byte                red_;
  byte                green_;
  byte                blue_;
  long                initLen_;
  long                usedLen_;
  byte                tempPrint_;
  byte                tempStart_;
  int                 date_;
  byte                xor_;
};

Cartridge::Cartridge() :
    magicNum_(0x5C12),
    type_(CartridgeType::refillable),
    material_(Material::PLA),
    red_(0x00),
    green_(0x00),
    blue_(0x00),
    initLen_(200000),
    usedLen_(0),
    tempPrint_(0x55),
    tempStart_(0x5F),
    date_(0x0217),
    xor_(0)
{  
}

class Payload
{
public:
  Payload();

  int               addr_;
  int               len_;
  RfidCommand::Type funcCode_;
  int               index_;
  bool              msb_;
  byte              payload_[64];
};

Payload::Payload() :
                  addr_(0),
                  len_(0),
                  funcCode_(RfidCommand::none),
                  index_(0),
                  msb_(false)
{
}

class Rfid
{
public:
  Rfid();
  void runFsm();
  void handleRequest(RfidCommand::Type funcCode, byte * preq, int len);
  void sendResponse(byte * pPayload, int len);
  int  buildCartridgePayload(byte * pdata);
  void printCartridgeData();

  Cartridge cartridge_;
  SerialState::Type state_;
  Payload payload_;
  unsigned long timeout;
};

SoftwareSerial serial_(RFID_LEFT_RX_PIN, RFID_LEFT_TX_PIN);
Rfid rfid;

void setup()  
{ 
  Serial.begin(57600);
  Serial.println("Zim Cartridge Emulator Nano v1.0\n");

  // Load cartridge data from eeprom
  // or initialize if never set before
  Serial.println("Checking eeprom");
  Cartridge temp;
  EEPROM.get(CARTRIDGE_EEPROM_LOC, temp);
  if(temp.magicNum_ != CARTRIDGE_MAGIC_NUMBER)
  {
     Serial.println("Reinitializing eeprom");
     EEPROM.put(CARTRIDGE_EEPROM_LOC, rfid.cartridge_);
  }
  else
  {
    EEPROM.get(CARTRIDGE_EEPROM_LOC,rfid.cartridge_);
    Serial.println("Cartridge data restored from eeprom");
    rfid.printCartridgeData();
  }

  
  serial_.begin(RFID_BAUD_RATE);
}

// This is called repeatedly
void loop() 
{
  rfid.runFsm();
}

Rfid::Rfid() : state_(SerialState::idle),
               timeout(0)                          
{  
}

// State machine for parsing the YET-MF2 Mifare protocol
void 
Rfid::runFsm()
{
  byte rx = 0;

  if(state_ != SerialState::idle &&
     (millis() - timeout) > RX_TIMEOUT)
  {
    Serial.println("Rx timeout");
    state_ = SerialState::idle;
  }
  
  if(serial_.available())
  {
    rx = (byte)serial_.read();
    Serial.print(rx, HEX);
    Serial.print(" ");

    switch(state_)
    {
      case SerialState::idle:
        if(rx == 0xAA)
        {      
          timeout = millis();  
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
        Serial.println(" bytes from Zim");
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
// Format is: uint16 header (0xAABB)- uint16 len - uint16 nodeId - uint16 func code - uint8 status - uint8 n data - uint8 XOR
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
  Serial.println(" bytes to Zim");

  for(int i=0; i<rspLen; ++i)
  {
      serial_.write(rsp[i]);
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
      payload[2] = CARTRIDGE_ID>>8;
      payload[3] = CARTRIDGE_ID & 0xFF;
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
        cartridge_.magicNum_ = preq[1]<<8;
        cartridge_.magicNum_ |= preq[2];
        cartridge_.type_ = CartridgeType::Type(preq[3]>>4);
        cartridge_.material_ = Material::Type(preq[3] & 0x0F);
        cartridge_.red_ = preq[4];
        
      }
      else if(page == 7)
      {
        cartridge_.green_    = preq[1];
        cartridge_.blue_     = preq[2];
        cartridge_.initLen_  = ((long)preq[3])<<12;
        cartridge_.initLen_ |= ((long)preq[4])<<4;        
      }
      else if(page == 8)
      {
        cartridge_.initLen_ |= ((long)preq[1])>>4;
        cartridge_.usedLen_  = ((long)preq[1])<<16;
        cartridge_.usedLen_ |= ((long)preq[2])<<8;
        cartridge_.usedLen_ |= ((long)preq[3]); 
        cartridge_.tempPrint_ = preq[4];
        
      }
      else if(page == 9)
      {
        cartridge_.tempStart_ = preq[1];
        cartridge_.date_    = preq[2]<<8;
        cartridge_.date_   |= preq[3];
        cartridge_.xor_  = preq[4];  

        Serial.println("Cartridge data received from Zim:");
        printCartridgeData();
        
#if NEVER_ENDING_FILAMENT == 1
        Serial.println("Your spool runneth over");
        cartridge_.usedLen_ = 0;
#endif

        Serial.println("Saving cartridge data to eeprom");
        EEPROM.put(CARTRIDGE_EEPROM_LOC, cartridge_);       
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
  pdata[index++] = cartridge_.magicNum_>>8;
  pdata[index++] = cartridge_.magicNum_ & 0xFF;
  pdata[index++] = cartridge_.type_<<4 | cartridge_.material_ & Material::Mask;
  pdata[index++] = cartridge_.red_;
  pdata[index++] = cartridge_.green_;
  pdata[index++] = cartridge_.blue_;
  pdata[index++] = (cartridge_.initLen_>>12) & 0xFF;
  pdata[index++] = (cartridge_.initLen_>>4) & 0xFF;
  pdata[index++] = (cartridge_.initLen_<<4 | cartridge_.usedLen_>>16) & 0xFF;
  pdata[index++] = (cartridge_.usedLen_>>8) & 0xFF;
  pdata[index++] = (cartridge_.usedLen_) & 0xFF;
  pdata[index++] = cartridge_.tempPrint_;
  pdata[index++] = cartridge_.tempStart_;
  pdata[index++] = cartridge_.date_>>8;
  pdata[index++] = cartridge_.date_ & 0xFF;

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
  Serial.print("Magic Number: 0x");
  Serial.println(cartridge_.magicNum_, HEX);
  Serial.print("Type:");
  Serial.println(cartridge_.type_);
  Serial.print("Material:");
  Serial.println(cartridge_.material_);
  Serial.print("Red: 0x");
  Serial.println(cartridge_.red_, HEX);
  Serial.print("Green: 0x");
  Serial.println(cartridge_.green_, HEX);
  Serial.print("Blue: 0x");
  Serial.println(cartridge_.blue_, HEX);
  Serial.print("Initial Length: ");
  Serial.println(cartridge_.initLen_);
  Serial.print("Used Length: ");
  Serial.println(cartridge_.usedLen_);
  Serial.print("Temp Print: ");
  Serial.println(cartridge_.tempPrint_);
  Serial.print("Temp Start: ");
  Serial.println(cartridge_.tempStart_);
  Serial.print("Date: 0x");
  Serial.println(cartridge_.date_, HEX);
  Serial.print("Xor: 0x");
  Serial.println(cartridge_.xor_, HEX); 
}















