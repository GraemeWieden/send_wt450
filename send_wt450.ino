//    The following information is an abbreviated of the content from
//    http://ala-paavola.fi/jaakko/doku.php?id=wt450h
//
//    +---+   +---+   +-------+       +  high
//    |   |   |   |   |       |       |
//    |   |   |   |   |       |       |
//    +   +---+   +---+       +-------+  low
//    
//    ^       ^       ^       ^       ^  clock cycle
//    |   1   |   1   |   0   |   0   |  translates as
//    
//    Each transmission is 36 bits long (i.e. 72 ms)
//    
//    Example transmission (House 1, Channel 1, RH 59 %, Temperature 23.5 °C)
//    110000010011001110110100100110011000
//    
//    b00 - b03  (4 bits): Constant, 1100, probably preamble
//    b04 - b07  (4 bits): House code (here: 0001 = HC 1)
//    b08 - b09  (2 bits): Channel code - 1 (here 00 = CC 1)
//    b10 - b12  (3 bits): Constant, 110
//    b13 - b19  (7 bits): Relative humidity (here 0111011 = 59 %)
//    b20 - b34 (15 bits): Temperature (see below)
//    b35 - b35  (1 bit) : Parity (xor of all bits should give 0)
//
//    Summary of bit fields:
//    1100 0001 00 110 0111011 010010011001100 0
//     c1   hc  cc  c2    rh          t        p
//
//    c1, c2 = constant field 1 and 2
//    hc, cc = house code and channel code
//    rh, t  = relative humidity, temperature
//    p      = parity bit
//
//   The temperature is transmitted as (temp + 50.0) * 128,

//   -------------------------------------------------------------------
//   Some additional things I've discovered from sampling transmissions:
//   The parity description didn't make sense to me so after some testing
//   it turns out that it's a simple even parity bit. This has the added
//   benefit of leaving the transmission in a low state at the end of the
//   final bit. To 'complete' the packet, a 500 μs pulse is sent.

// note that arduino has a max of 16383 for delayMicroseconds function
int wrt450TriggerPulse = 15000; // trigger time in microseconds
int wrt450ShortPulse = 1000; // time in microseconds for a short pulse
int wrt450LongPulse = 2000; // time in microseconds for a long pulse
int wrt450SeparatorPulse = 15000; // long low time in microseconds between repeats

int txPort = 9;

// the wt450 protocol sends two short pulses for a '1' but only one long pulse for a '0'
// that means we need to keep track of whether we're sending a high or a low pulse

boolean pulseState = HIGH; // start with a high pulse and toggle whenever we send a '0'
boolean parityBit;

int testHum = 14;
float testTemp = 0;
void setup()
{
  Serial.begin(9600);
  
  // DDRB = B11111110;  // sets low bit (digital pn 8) as input and digital pin 9 as output
  // only use floating point here for converting to binary
  float temperature = 23.5;
  int humidity = 59;
  
  int convertedTemperature = (temperature * 128.0) + 6400;
  // Serial.println(convertedTemperature);
}

void loop()
{
  // 
  byte testCode1 = B11000001;
  byte testCode2 = B00110011;
  byte testCode3 = B10010100;
  byte testCode4 = B10011001;
  byte testCode5 = B00001000;


  
  //sendWRT450Test();
  sendWRT450Packet(7, 4, testHum++, testTemp);
  testTemp += 1.3;
  //sendWRT450Packet(1, 1, 16, 23.5);
  //sendWRT450Start();
  //sendWRT450End();
  //sendWRT450Start();
    
  delay(3000);
}

String byteToBinary(byte b)
{
  String retVal = "";
  for (int i = 0; i < 8 - String(b,BIN).length(); i++)
  {
    retVal += "0";
  }
  retVal += String(b, BIN);
  return retVal;
}

void sendWRT450Start()
{
  parityBit = 0;
  digitalWrite(txPort, LOW);
  delayMicroseconds(wrt450TriggerPulse);    
  delayMicroseconds(wrt450TriggerPulse);  
  pulseState = HIGH; // next pulse state to use
}

void sendWRT450End()
{
  // if the last pulse was low (next is high) we need to send a half short high pulse to complete the packet
  if(pulseState == HIGH)
  {
    digitalWrite(txPort, HIGH);
    delayMicroseconds(wrt450ShortPulse >> 1);
  }
  digitalWrite(txPort, LOW);
  delayMicroseconds(wrt450TriggerPulse);  
  delayMicroseconds(wrt450TriggerPulse);  
}

/*
void sendWRT450End()
{
  digitalWrite(txPort, pulseState);
  delayMicroseconds(wrt450TriggerPulse);  
}
*/

void sendWRT450Bit(byte b)
{
  // single long pulse = '0'
  if(b == 0)
  {
    digitalWrite(txPort, pulseState);
    delayMicroseconds(wrt450LongPulse);  
    pulseState = !pulseState;
  }
  // short pulse then short pulse = '1'
  else
  {
    digitalWrite(txPort, pulseState);
    delayMicroseconds(wrt450ShortPulse);
    digitalWrite(txPort, !pulseState);
    delayMicroseconds(wrt450ShortPulse);  
    // pulseState will be unchanged after two pulses
    parityBit = !parityBit;
  }
}

//    b00 - b03  (4 bits): Constant, 1100, probably preamble
//    b04 - b07  (4 bits): House code (here: 0001 = HC 1)
//    b08 - b09  (2 bits): Channel code - 1 (here 00 = CC 1)
//    b10 - b12  (3 bits): Constant, 110
//    b13 - b19  (7 bits): Relative humidity (here 0111011 = 59 %)
//    b20 - b34 (15 bits): Temperature (see below)
//    b35 - b35  (1 bit) : Parity (xor of all bits should give 0)

void sendWRT450Test()
{
  int repeats = 2;
  for(int i = 0; i < repeats; i++)
  {
    sendWRT450Start();
    sendWRT450Bits(B1100, 4); // always 1100
    sendWRT450Bits(B0001, 4); // house code
    sendWRT450Bits(B00, 2); // channel code
    sendWRT450Bits(B110, 3); // always 110
    sendWRT450Bits(B0111100, 7); // humidity
    sendWRT450Bits(9420, 15); // temperature
    sendWRT450Bits(B0, 1); // parity
    sendWRT450End();
  }
}

void sendWRT450Packet(int house, int channel, int humidity, float temperature)
{
  unsigned int convertedTemperature = (temperature * 128.0) + 6400;
  int convertedChannel = channel - 1;
  int repeats = 2;
  
  for(int i = 0; i < repeats; i++)
  {
    sendWRT450Start();
    sendWRT450Bits(B1100, 4); // always 1100
    sendWRT450Bits(house, 4); // house 1 stored as 0001
    sendWRT450Bits(convertedChannel, 2); // channel 1 stored as 00
    sendWRT450Bits(B110, 3); // always 110
    sendWRT450Bits(humidity, 7); // humidity
    sendWRT450Bits(convertedTemperature, 15); // encoded temperature
    sendWRT450Bits(parityBit, 1); // parity
    sendWRT450End();
  }
}

void sendWRT450Bits(unsigned int data, int bits)
{
  //Serial.println(data);
  unsigned int bitMask = 1;
  bitMask = bitMask << (bits - 1);
  for(int i = 0; i < bits; i++)
  {
    sendWRT450Bit( (data&bitMask) == 0 ? 0 : 1);
    bitMask = bitMask >> 1;
  }
  //Serial.println();
}

void sendWRT450Byte(byte b)
{
  byte bitMask = B10000000;
  for(int i = 0; i < 8; i++)
  {
    sendWRT450Bit(b&bitMask);
    bitMask = bitMask >> 1;
  }
}

void sendWRT450Nibble(byte b)
{
  byte bitMask = B00001000;
  for(int i = 0; i < 4; i++)
  {
    sendWRT450Bit(b&bitMask);
    bitMask = bitMask >> 1;
  }
}


