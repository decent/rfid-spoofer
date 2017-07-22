#include "Arduino.h"
#include "stdbool.h"


#define FIRST_RFID_BIT (63 - 9)
#define COIL_PIN 9
#define US_DELAY 256

uint64_t hexstring_to_uint64(String hex);
int hexchar_to_dec(char hexchar);
void transmit_rfid(uint64_t rfid_code);
void transmit_bit(uint8_t);
void manchester_set_pin(uint8_t clock_half, uint8_t signal);

void setup()
{
  pinMode(COIL_PIN, OUTPUT);
  Serial.begin(115200);
}

void loop()
{
  String hexstring;
  uint64_t rfid_code;

  digitalWrite(COIL_PIN, LOW);

  Serial.print("\nEnter Hex string: ");

  while(!Serial.available())
    ;

  hexstring = Serial.readString();

  if((rfid_code = hexstring_to_uint64(hexstring)) != 0)
  {
    Serial.print("\nConverted to uint64");
  }
  else
  {
    Serial.print("\nConverting to uint64 failed");
  }

  Serial.print("\nUint64:   ");
  for (int i = 63; i >= 0; i--)
  {
    Serial.print(rfid_code & (1ULL << i) ? 1 : 0);
  }

  for(uint8_t i; i < 5; i++)
  {
    transmit_rfid(rfid_code);
  }
}

int hexchar_to_dec(char hexchar)
{
  if (hexchar >= '0' && hexchar <= '9')
  {
    hexchar -= '0';
  }
  else if (hexchar >= 'A' && hexchar <= 'F')
  {
    hexchar -= 'A';
    hexchar += 10;
  }
  else if (hexchar >= 'a' && hexchar <= 'f')
  {
    hexchar -= 'a';
    hexchar += 10;
  }
  else
  {
    return -1; /* Some character wasn't hexadecimal */
  }

  return hexchar;
}

/*
 * Will return 0 if it failed somewhere (since a valid value will always at least
 * start with nine ones (start bits).
 */
uint64_t hexstring_to_uint64(String hex)
{
  char current_char_value;
  uint64_t current_bit_value;
  uint64_t return_value = 0;
  uint8_t shift_amount;

  if(hex.length() != 10) return 0; /* We require a 10 character string A-F */

  for(int i = 0; i < 10; i++)
  {
    current_char_value = hexchar_to_dec(hex.c_str()[i]);

    if(current_char_value == -1) return 0; /* Break out early if character wasn't "between" 0-F */

    for(int j = 3; j >= 0; j--) /* Go through every bit in the 4 bit value */
    {
      current_bit_value = current_char_value & (1 << j) ? 1 : 0;

      return_value |= current_bit_value << (FIRST_RFID_BIT - ((i * 5) + (3 - j))); /* Value itself */

      shift_amount = (FIRST_RFID_BIT - ((i * 5) + 4));
      return_value ^= current_bit_value << shift_amount; /* Row parity */

      shift_amount = (FIRST_RFID_BIT - (50 + (3 - j)));
      return_value ^= current_bit_value << shift_amount; /* Column parity */
    }
  }

  return (return_value | 0xFF80000000000000); /* Add start bits */
}

void transmit_rfid(uint64_t rfid_code)
{
  for(int i = 63; i >= 0; i--)
  {
    transmit_bit((rfid_code >> i) & 1);
  }
}

void transmit_bit(uint8_t bit_value)
{
  manchester_set_pin(0, bit_value);
  delayMicroseconds(US_DELAY);

  manchester_set_pin(1, bit_value);
  delayMicroseconds(US_DELAY);
}

/*
 * Function mainly taken from:
 * http://www.instructables.com/id/Stupid-Simple-Arduino-LF-RFID-Tag-Spoofer/
 */
void manchester_set_pin(uint8_t clock_half, uint8_t signal)
{
  uint8_t manchester_encoded = clock_half ^ signal;

  digitalWrite(COIL_PIN, !manchester_encoded);
}
