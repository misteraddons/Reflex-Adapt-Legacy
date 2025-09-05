/*******************************************************************************
 * PlayStation input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles up to 2 input ports.
 *
 * Uses PsxNewLib
 * https://github.com/SukkoPera/PsxNewLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/DigitalIO/DigitalIO.h"
#include "src/PsxNewLib/PsxControllerHwSpi.h"


//Guncon config
//0=Mouse, 1=Joy, 2=Joy OffScreenEdge (MiSTer)
//#define GUNCON_FORCE_MODE 2

//NeGcon config
//0=Default, 1=MiSTer Wheel format with paddle
//#define NEGCON_FORCE_MODE 1
//If you dont want to force a mode but instead change the default:
//Don't enable the force mode and edit the isNeGconMiSTer variable below as you desire.

#if REFLEX_PIN_VERSION == 1
  const byte PIN_PS1_ATT = 2;
#else
  const byte PIN_PS1_ATT = 21;//A3
#endif

const byte PIN_PS2_ATT = 5;
const byte PIN_PS3_ATT = 10;
const byte PIN_PS4_ATT = 18;//A0
const byte PIN_PS5_ATT = 19;//A1
const byte PIN_PS6_ATT = 20;//A2

const uint8_t SPECIALMASK_POPN = 0xE;
const uint8_t SPECIALMASK_JET  = 0xC;

static const uint8_t PS_INTERVAL_DEFAULT  = 250;
static const uint16_t PS_INTERVAL_JET  = 1000;
static const uint16_t PS_INTERVAL_DS2  = 3000;

class ReflexInputPsx : public RZInputModule {
  private:
    PsxController* psx;//variable to hold current reading port

    PsxController* psxlist[2] {
      new PsxControllerHwSpi<PIN_PS1_ATT>(),
      new PsxControllerHwSpi<PIN_PS2_ATT>()
    };

    bool haveController[2] { false, false };
    PsxControllerProtocol lastProto[2] { PSPROTO_UNKNOWN, PSPROTO_UNKNOWN };
    uint8_t outputIndex { 0 };

    void tryEnableRumble() {
      if (psx->enterConfigMode()) {
        psx->enableRumble();
        psx->enableAnalogButtons();
        if (psx->getControllerType() == PSCTRL_DUALSHOCK2) {
          sleepTime = PS_INTERVAL_DS2;
        }
        psx->exitConfigMode();
      }
    }

    // PS2 Analog firmware - specialized for DualShock 2 pressure-sensitive buttons

    #ifdef ENABLE_REFLEX_PAD
      const Pad padPsx[16] = {
        { PSB_SELECT,    2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PSB_L3,        3, 4*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_R3,        3, 5*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_START,     2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PSB_PAD_UP,    1, 1*6, UP_ON, UP_OFF },
        { PSB_PAD_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
        { PSB_PAD_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
        { PSB_PAD_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
        { PSB_L2,        0, 2*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { PSB_R2,        0, 9*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { PSB_L1,        0, 0*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { PSB_R1,        0, 7*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { PSB_TRIANGLE,  1, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_CIRCLE,    2, 9*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_CROSS,     3, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_SQUARE,    2, 7*6, FACEBTN_ON, FACEBTN_OFF }
      };

      const Pad padPsxPopN[11] = {
        { PSB_SELECT,    0, 3*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PSB_START,     0, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PSB_CIRCLE,    1, 1*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_CROSS,     1, 3*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_SQUARE,    1, 5*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_PAD_UP,    1, 7*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_TRIANGLE,  2, 0*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_R1,        2, 2*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_L1,        2, 4*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_R2,        2, 6*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_L2,        2, 8*6, FACEBTN_ON, FACEBTN_OFF }
      };
    
      void loopPadDisplayCharsPsx(const uint8_t index, const PsxControllerProtocol padType, void* p, const bool force) {
        // Show all DS2 buttons
        for(uint8_t i = 0; i < (sizeof(padPsx) / sizeof(Pad)); ++i){
          const Pad pad = padPsx[i];
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, p && static_cast<PsxController*>(p)->buttonPressed(static_cast<PsxButton>(pad.padvalue)), pad.on, pad.off, force);
        }
      }
    
      void ShowDefaultPadPsx(const uint8_t index, const PsxControllerProtocol padType) {
        display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(padDivision[index].firstCol, 7);
    
        // Show protocol type and analog button availability
        switch(padType) {
          case PSPROTO_DUALSHOCK2:
            display.print(F("DS2"));
            break;
          case PSPROTO_DUALSHOCK:
            display.print(F("DS1"));
            break;
          case PSPROTO_DIGITAL:
            display.print(F("DIG"));
            break;
          case PSPROTO_FLIGHTSTICK:
            display.print(F("FLY"));
            break;
          case PSPROTO_NEGCON:
            display.print(F("NEG"));
            break;
          case PSPROTO_JOGCON:
            display.print(F("JOG"));
            break;
          case PSPROTO_GUNCON:
            display.print(F("GUN"));
            break;
          default:
            display.print(PSTR_TO_F(PSTR_NONE));
            return;
        }
        
        // Show if analog button data is available
        if (psx && psx->getAnalogButtonData() != nullptr) {
          display.print(F("+"));  // Indicates analog button data available
        }
      
        if (index < 2) {
          loopPadDisplayCharsPsx(index, padType, NULL, true);
        }
      }
    #endif

    void handleDpad() {
      state[outputIndex].dpad = 0
        | (psx->buttonPressed(PSB_PAD_UP)    ? GAMEPAD_MASK_UP    : 0)
        | (psx->buttonPressed(PSB_PAD_DOWN)  ? GAMEPAD_MASK_DOWN  : 0)
        | (psx->buttonPressed(PSB_PAD_LEFT)  ? GAMEPAD_MASK_LEFT  : 0)
        | (psx->buttonPressed(PSB_PAD_RIGHT) ? GAMEPAD_MASK_RIGHT : 0)
      ;
    }

    bool loopDualShock() {
      static byte lastLX[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
      static byte lastLY[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
      static byte lastRX[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
      static byte lastRY[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
    
      #ifdef ENABLE_REFLEX_PAD
        static PsxControllerProtocol lastPadType[] = { PSPROTO_UNKNOWN, PSPROTO_UNKNOWN };
        PsxControllerProtocol currentPadType[] = { PSPROTO_UNKNOWN, PSPROTO_UNKNOWN };
        const uint8_t inputPort = outputIndex; //todo fix
      #endif
    
      byte analogX = ANALOG_IDLE_VALUE;
      byte analogY = ANALOG_IDLE_VALUE;
      //word convertedX, convertedY;
    
      const bool digitalStateChanged = psx->buttonsChanged();//check if any digital value changed (dpad and buttons)
      bool stateChanged = digitalStateChanged;
      
      const PsxControllerProtocol proto = psx->getProtocol();
    
      #ifdef ENABLE_REFLEX_PAD
        if(proto != lastPadType[inputPort])
          ShowDefaultPadPsx(inputPort, proto);
        currentPadType[inputPort] = proto;
      #endif
    
      
      switch (proto) {
      case PSPROTO_DUALSHOCK:
      case PSPROTO_DUALSHOCK2:
      {
        handleDpad();
    
//        uint16_t buttonData = 0;
        // Controller buttons - Triangle and Cross remain digital, Circle/Square/L2/R2 are analog
        const uint8_t* analogData = psx->getAnalogButtonData();
        
        state[outputIndex].buttons = 0
          | (psx->buttonPressed(PSB_TRIANGLE) ? GAMEPAD_MASK_B4 : 0)
          | (psx->buttonPressed(PSB_CROSS)    ? GAMEPAD_MASK_B1 : 0)
          | (psx->buttonPressed(PSB_L1)       ? GAMEPAD_MASK_L1 : 0)
          | (psx->buttonPressed(PSB_R1)       ? GAMEPAD_MASK_R1 : 0)
          | (psx->buttonPressed(PSB_SELECT)   ? GAMEPAD_MASK_S1 : 0)
          | (psx->buttonPressed(PSB_START)    ? GAMEPAD_MASK_S2 : 0)
          | (psx->buttonPressed(PSB_L3)       ? GAMEPAD_MASK_L3 : 0)
          | (psx->buttonPressed(PSB_R3)       ? GAMEPAD_MASK_R3 : 0)
        ;
    
    
    //if(proto != PSPROTO_DIGITAL)
    
        //analog sticks
        if (psx->getLeftAnalog(analogX, analogY)) {
          state[outputIndex].lx = convertAnalog(analogX);
          state[outputIndex].ly = convertAnalog(analogY);
        } else {
          analogX = ANALOG_IDLE_VALUE;
          analogY = ANALOG_IDLE_VALUE;
          state[outputIndex].lx = convertAnalog(analogX);
          state[outputIndex].ly = convertAnalog(analogY);
        }
        
        if (lastLX[outputIndex] != analogX || lastLY[outputIndex] != analogY)
          stateChanged = true;
          
        lastLX[outputIndex] = analogX;
        lastLY[outputIndex] = analogY;
    
        // Read pressure data for analog buttons
        if (analogData != nullptr) {
          state[outputIndex].pressureCircle = analogData[5];
          state[outputIndex].pressureSquare = analogData[7];
          state[outputIndex].pressureL2 = analogData[10];
          state[outputIndex].pressureR2 = analogData[11];
        } else {
          state[outputIndex].pressureCircle = 0;
          state[outputIndex].pressureSquare = 0;
          state[outputIndex].pressureL2 = 0;
          state[outputIndex].pressureR2 = 0;
        }
        
        state[outputIndex].pressureRight = 0;
        state[outputIndex].pressureLeft = 0;
        state[outputIndex].pressureUp = 0;
        state[outputIndex].pressureDown = 0;
        state[outputIndex].pressureL1 = 0;
        state[outputIndex].pressureR1 = 0;
        state[outputIndex].pressureTriangle = 0;
        state[outputIndex].pressureCross = 0;
        
        // Handle right analog stick normally
        if (psx->getRightAnalog(analogX, analogY)) {
          state[outputIndex].rx = convertAnalog(analogX);
          state[outputIndex].ry = convertAnalog(analogY);
        } else {
          analogX = ANALOG_IDLE_VALUE;
          analogY = ANALOG_IDLE_VALUE;
          state[outputIndex].rx = convertAnalog(analogX);
          state[outputIndex].ry = convertAnalog(analogY);
        }


        if (lastRX[outputIndex] != analogX || lastRY[outputIndex] != analogY)
          stateChanged = true;
    
        lastRX[outputIndex] = analogX;
        lastRY[outputIndex] = analogY;
    
        if(stateChanged) {
          #ifdef ENABLE_REFLEX_PAD
            if (inputPort < 2) {
              loopPadDisplayCharsPsx(inputPort, proto, psx, false);
            }
          #endif
        }
    
        break;
      }
      default:
        break;
      }
    
    
      #ifdef ENABLE_REFLEX_PAD
        /*for (uint8_t i = 0; i < 2; i++) {
          if(lastPadType[i] != currentPadType[i] && currentPadType[i] == PSPROTO_UNKNOWN) {
            ShowDefaultPadPsx(i, currentPadType[i]);
          }
          lastPadType[i] = currentPadType[i];
        }*/
        lastPadType[inputPort] = currentPadType[inputPort];
      #endif
    
      return digitalStateChanged;
    }


  public:
    ReflexInputPsx() : RZInputModule() { }


    const char* getUsbId() override {
      static const char* usbId_ds2 { "RZMPsDS2" };
      return usbId_ds2;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_PSX;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(5*6);
//        display.print(F("PLAYSTATION"));
//      }
//    #endif

    void setup() override {
      //initialize the "attention" pins as OUTPUT HIGH.
      fastPinMode(PIN_PS1_ATT, OUTPUT);
      fastPinMode(PIN_PS2_ATT, OUTPUT);
    
      fastDigitalWrite(PIN_PS1_ATT, HIGH);
      fastDigitalWrite(PIN_PS2_ATT, HIGH);
      
      psx = psxlist[0];

      sleepTime = PS_INTERVAL_DEFAULT;
    
      //if forcing specific mode
    #ifdef ENABLE_REFLEX_PSX_JOG
      PsxControllerProtocol proto = (deviceMode == RZORD_PSX_JOG ? PSPROTO_JOGCON : PSPROTO_UNKNOWN);
    #else
      PsxControllerProtocol proto = PSPROTO_UNKNOWN;
    #endif
    
      if (psx->begin ()) {
        //delay(150);//200
        //haveController = true;
        haveController[0] = true;
        //const PsxControllerProtocol proto = psx->getProtocol();
    
    
        //if not forced a mode, then read from currenct connected controller
        if(proto == PSPROTO_UNKNOWN)
          proto = psx->getProtocol();

        lastProto[0] = proto;
    
        // Only support DualShock 2 controllers

        tryEnableRumble();


      } else { //no controller connected
        // DS2 only - no special controller handling
      }
    
      // DualShock 2 setup
      totalUsb = 2;//MAX_USB_STICKS;
      for (uint8_t i = 0; i < totalUsb; i++) {
        hasLeftAnalogStick[i] = true;
        hasRightAnalogStick[i] = true;
      }
    }//end setup

    void setup2() override {
      // DualShock 2 setup - nothing needed
    }

    bool read() override {
      static bool isReadSuccess[] = {false,false};
      static bool isEnabled[] = {false,false};
      bool stateChanged = false;
    
      outputIndex = 0;
    
      // DualShock 2 only - no special controller handling needed
    
      //if (millis() - last >= POLLING_INTERVAL) {
      //  last = millis();
    
    
        //nothing detected yet
        if (!isEnabled[0] && !isEnabled[1]) {
          for (uint8_t i = 0; i < 2; i++) {
            //isEnabled[i] = haveController[i] || psxlist[i]->begin();
            isEnabled[i] = haveController[i] || (haveController[i] = psxlist[i]->begin());
            //haveController[i] = haveController[i] || psxlist[i]->begin();
            //isEnabled[i] = haveController[i];
          }
          #if defined REFLEX_USE_OLED_DISPLAY && defined ENABLE_PSX_GENERAL_OLED
          //display.setCursor(0, 7);
          //display.clearToEOL();
          //display.print(F("Ports "));
          setOledDisplay(true);
          //clearOledLineAndPrint(0, 7 - oledDisplayFirstRow, F("Ports "));
          //clearOledLineAndPrint(0, 6 - oledDisplayFirstRow, F("Connect a single pad."));
          //clearOledLineAndPrint(0, 7 - oledDisplayFirstRow, F("Connect two pads and reset device."));
    
          //clearOledLineAndPrint(0, 5 - oledDisplayFirstRow, F("To begin, connect:"));
          //clearOledLineAndPrint(0, 6 - oledDisplayFirstRow, F("- A single pad."));
          //clearOledLineAndPrint(0, 7 - oledDisplayFirstRow, F("- Two pads, then reset device."));
    
          //clearOledLineAndPrint(6, 6 - oledDisplayFirstRow, F("CONNECT A CONTROLLER"));
          //clearOledLineAndPrint(4*6, 7 - oledDisplayFirstRow, F("TO INITIALIZE"));
    
          display.clear(0, 127, 7, 7);
          display.setRow(7);
    
          // Show firmware identifier
          display.setCol(0);
          display.print(F("DS2 FW"));
    
          for (uint8_t i = 0; i < 2; i++) {
            //const uint8_t firstCol = i == 0 ? 0 : 12*6;
            display.setCol(padDivision[i].firstCol);
            if (!isEnabled[0] && !isEnabled[1])
              display.print(PSTR_TO_F(PSTR_NONE));  
            else if (!isEnabled[i])
              display.print(PSTR_TO_F(PSTR_NA));
          }
    /*      
          if(!isEnabled[0] && !isEnabled[1]) {
            
            for (uint8_t i = 0; i < 2; i++) {
              const uint8_t firstCol = i == 0 ? 0 : 12*6;
              display.setCol(firstCol);
              display.print(PSTR_TO_F(PSTR_NONE));
            }
          }
    
          if(isEnabled[0] || isEnabled[1]) {
            for (uint8_t i = 0; i < 2; i++) {
              //display.setCol(36 + (i*12)); //each char is 6 cols
              const uint8_t firstCol = i == 0 ? 0 : 12*6;
              display.setCol(firstCol); //each char is 6 cols
              if(isEnabled[i])
                display.print(PSTR_TO_F(PSTR_NONE));
              else
                display.print(PSTR_TO_F(PSTR_NA));
            }
          }
          */
    
          
          //for (uint8_t i = 0; i < 2; i++) {
          //  if(isEnabled[i]){
          //    display.setCol(36 + (i*12)); //each char is 6 cols
          //    display.print(i+1);
          //  }
          //}
          #endif
        }
    
    
        //read all ports
    
        for (uint8_t i = 0; i < 2; i++) {
          psx = psxlist[i];
          isReadSuccess[i] = false;
    
          if (!isEnabled[i])
            continue;
          
          if (!haveController[i]) {
            if (psx->begin()) {
              haveController[i] = true;
              tryEnableRumble();
              ShowDefaultPadPsx(i, psx->getProtocol());
            }
          } else {
            const PsxControllerProtocol proto = psx->getProtocol();
            if(lastProto[i] != proto)
              tryEnableRumble();
            lastProto[i] = proto;
            #ifdef PSX_COMBINE_RUMBLE
              psx->setRumble ((rumble[i].left_power | rumble[i].right_power) != 0x0, (rumble[i].left_power | rumble[i].right_power));
            #else
              psx->setRumble (rumble[i].right_power != 0x0, rumble[i].left_power);
            #endif
            isReadSuccess[i] = psx->read();
            if (!isReadSuccess[i]){ //debug (F("Controller lost.")); debug (F(" last values: x = ")); debug (lastX); debug (F(", y = ")); debugln (lastY);
              haveController[i] = false;
              ShowDefaultPadPsx(i, PSPROTO_UNKNOWN);
            }
          }
          // DS2 supports both ports
        }
    
    
        for (uint8_t i = 0; i < totalUsb; i++) {
          if (haveController[i] && isReadSuccess[i]) {
            psx = psxlist[i];
            stateChanged |= loopDualShock();
          }
          outputIndex++;
        }
    
      //} end if (millis() - last >= POLLING_INTERVAL)
      return stateChanged;//haveController[0] || haveController[1];
    }//end read

};
