// Zim Cartridge Emulator 
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#ifndef Rfid_h
#define Rfid_h

#include <Arduino.h>
#include "Cartridge.h"

#define RFID_BAUD_RATE              19200 // don't change
#define RX_TIMEOUT                  2000 // msecs timeout on receives

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


class Rfid
{
public:
  Rfid(String name, HardwareSerial * serial, Cartridge cartridge);
  void runFsm();
  void handleRequest(RfidCommand::Type funcCode, byte * preq, int len);
  void sendResponse(byte * pPayload, int len);
  int  buildCartridgePayload(byte * pdata);
  void printCartridgeData();
  void saveCartridgeData();
  bool isUpdated();

  String name_;
  Cartridge cartridge_;
  SerialState::Type state_;
  Payload payload_;
  HardwareSerial * serial_;
  unsigned long timeout_; 
  bool updated_;
};

#endif
