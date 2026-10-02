"""Run the real SNES input loop with simulated controller ports and USB.

Requires g++; no Arduino hardware is used. SNES_INPUT_HEADER can select an older
Input_Snes.h to verify that the regression scenarios fail before the fix.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

HARDWARE = r'''
#define SNES_ENABLE_VBOY
#define MAX_USB_STICKS 2
#define JOYSTICK_DEFAULT_REPORT_ID 3
#define JOYSTICK_TYPE_GAMEPAD 5
#define dstart(x)
#define bitWrite(value, bit, on) ((on) ? (value |= 1UL << (bit)) : (value &= ~(1UL << (bit))))
uint8_t UDCON = 0;
constexpr int DETACH = 0;
uint32_t clockMs = 0;
unsigned long millis() { return clockMs; }
void delay(unsigned long ms) { clockMs += ms; }
void delayMicroseconds(unsigned long) {}
uint8_t totalUsb = 2;
unsigned long sleepTime = 50;
uint8_t hatTable[16] = {};
uint32_t hostButtons[2] = {};
unsigned reports[2] = {};
struct {
  bool ready = true;
  unsigned attaches = 0;
  void attach() { ready = false; ++attaches; UDCON &= ~(1 << DETACH); }
  bool configured() { return ready; }
} USBDevice;

template <uint8_t... Pins> struct SnesPort {
  SnesController controller;
  SnesControllerState input;
  uint8_t count = 1;
  void begin() {}
  void update() { controller.copyCurrentToLast(); controller.currentState = input; }
  uint8_t getMultitapPorts() { return 0; }
  uint8_t getControllerCount() { return count; }
  SnesController& getSnesController(uint8_t) { return controller; }
};
struct Joy1_ {
  unsigned index;
  uint32_t buttons = 0;
  const char* serial;
  Joy1_(const char* s, unsigned id, unsigned, unsigned) : index(id - 3), serial(s) {}
  void setSerial(const char* s) { serial = s; }
  void resetState() { buttons = 0; }
  void setButtons(uint32_t b) { buttons = b; }
  void setHatSwitch(uint8_t) {}
  void sendState() {
    if (USBDevice.configured()) { hostButtons[index] = buttons; ++reports[index]; }
  }
};
Joy1_* usbStick[2];
'''

SCENARIOS = r'''
void tick(unsigned ms = 1) { clockMs += ms; snesLoop(); }
int main(int argc, char** argv) {
  assert(argc == 2);
  const std::string scenario = argv[1];
  snes1.input = {0, 0, 0xFFFF}; // SNES, neutral
  snes2.input = {0, SNES_B, 0xFFFF}; // SNES, holding B
  snesSetup();
  tick();
  assert(hostButtons[1] == 2);
  snes1.input = {4, 0, 0}; // swap port 1 to VB
  tick();
  if (scenario == "debounce") {
    tick(20);
    snes1.input = {0, 0, 0xFFFF};
    tick(40);
    assert(USBDevice.attaches == 0);
    return 0;
  }
  tick(31);
  assert(std::string(usbStick[0]->serial) == "ReflexVboy");
  if (scenario == "configuration") {
    assert(!USBDevice.configured());
    return 0;
  }
  // Model the host discarding reports from the old device and taking its time
  // to configure the new one. Inputs can still change during this interval.
  USBDevice.ready = false;
  hostButtons[0] = hostButtons[1] = 0;
  if (scenario == "changed") snes2.input.digital = SNES_A;
  if (scenario == "removed") { snes2.count = 0; snes2.input = {0, 0, 0}; }
  tick(1000);
  tick(1000);
  const auto before0 = reports[0], before1 = reports[1];
  USBDevice.ready = true;
  tick();
  assert(reports[0] == before0 + 1 && reports[1] == before1 + 1);
  assert(hostButtons[1] == (scenario == "held" ? 2U : scenario == "changed" ? 4U : 0U));
  tick();
  assert(reports[0] == before0 + 1 && reports[1] == before1 + 1);
  // A reverse swap must also restore an unchanged second controller.
  snes1.input = {0, 0, 0xFFFF};
  tick();
  tick(31);
  assert(std::string(usbStick[0]->serial) == "ReflexSNESNTT");
  hostButtons[0] = hostButtons[1] = 0;
  USBDevice.ready = true;
  tick();
  assert(reports[1] == before1 + 2);
  assert(hostButtons[1] == (scenario == "held" ? 2U : scenario == "changed" ? 4U : 0U));
}
'''


class SnesUsbTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        # Use the actual controller state/decoding code; mock only the pin IO.
        library = (ROOT / "Reflex/src/SnesLib/SnesLib.h").read_text()
        controller = library.split("enum SnesDeviceType_Enum", 1)[1].split(
            "#ifdef SNES_ENABLE_MULTITAP //single port", 1
        )[0]
        header = Path(os.environ.get("SNES_INPUT_HEADER", ROOT / "Reflex/Input_Snes.h"))
        module = "\n".join(
            line for line in header.read_text().splitlines()
            if not line.startswith('#include "src/')
        )
        source = Path(cls.temp.name) / "snes.cpp"
        source.write_text(
            "#include <cstdint>\n#include <cassert>\n#include <string>\n"
            + "enum SnesDeviceType_Enum" + controller + HARDWARE + module + SCENARIOS
        )
        cls.binary = Path(cls.temp.name) / "snes"
        subprocess.run(["g++", "-std=c++17", str(source), "-o", str(cls.binary)], check=True)

    def test_usb_configuration_is_reset(self):
        subprocess.run([str(self.binary), "configuration"], check=True)

    def test_held_input_restored_in_both_swap_directions(self):
        subprocess.run([str(self.binary), "held"], check=True)

    def test_latest_input_restored_after_slow_enumeration(self):
        subprocess.run([str(self.binary), "changed"], check=True)

    def test_disconnected_controller_gets_neutral_report(self):
        subprocess.run([str(self.binary), "removed"], check=True)

    def test_transient_vb_detection_does_not_reconnect(self):
        subprocess.run([str(self.binary), "debounce"], check=True)


if __name__ == "__main__":
    unittest.main()
