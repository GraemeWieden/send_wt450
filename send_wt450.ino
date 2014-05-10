//    WT450 Emulator
//
//    Send WT450 protocol packets through an RF transmitter.
//
//    Thanks to Jaakko Ala-Paavola and colleagues for sharing
//    the protocol information and packet content for the WT450.
//
//    The following information is an abbreviated version of material from
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
//   I also believe the 1100 preamble could be used to allow devices to 
//   determine the pulse durations for long and short pulses.

// note that arduino has a max of 16383 for the delayMicroseconds function
int wt450TriggerPulse = 15000; // trigger time in microseconds
int wt450ShortPulse = 1000; // time in microseconds for a short pulse
int wt450LongPulse = 2000; // time in microseconds for a long pulse

int txPort = 9; // digital pin for transmitter

// the wt450 protocol sends two short pulses for a '1' but only one long pulse for a '0'
// that means we need to keep track of whether we're sending a high or a low pulse

boolean pulseState; // start with a high pulse and toggle whenever we send a '0'
boolean parityBit; // used maintain the parity state as we send bits in the packet

int testHum = 0;
float testTemp = 0;

void setup()
{
}

void loop()
{
  sendWT450Packet(15, 4, testHum, testTemp);
  testHum++;
  testTemp += 1.3; // just an arbitrary increment to test floating points
  
  // loop the test values at 100
  testHum = (testHum > 100 ? 0 : testHum);
  testTemp = (testTemp > 100 ? 0 : testTemp);
  
  delay(5000);
}

void sendWT450Packet(int house, int channel, int humidity, float temperature)
{
  unsigned int convertedTemperature = (temperature * 128.0) + 6400;
  int convertedChannel = channel - 1;
  int repeats = 2; // the actual WT450 sends three samples
  
  for(int i = 0; i < repeats; i++)
  {
    sendWT450Start();
    sendWT450Bits(B1100, 4); // always 1100
    sendWT450Bits(house, 4); // house 1 stored as 0001
    sendWT450Bits(convertedChannel, 2); // channel 1 stored as 00
    sendWT450Bits(B110, 3); // always 110
    sendWT450Bits(humidity, 7); // humidity
    sendWT450Bits(convertedTemperature, 15); // encoded temperature
    sendWT450Bits(parityBit, 1); // parity
    sendWT450End();
  }
}

void sendWT450Start()
{
  parityBit = 0;
  digitalWrite(txPort, LOW);
  delayMicroseconds(wt450TriggerPulse);    
  delayMicroseconds(wt450TriggerPulse); // send twice as delayMicroseconds has a max of 16383
  pulseState = HIGH; // next pulse state to use
}

void sendWT450End()
{
  // if the last pulse was low (next is high) we need to send a half short high pulse to complete the packet
  // note that this should always be the case because of the even parity bit
  if(pulseState == HIGH)
  {
    digitalWrite(txPort, HIGH);
    delayMicroseconds(wt450ShortPulse >> 1);
  }
  digitalWrite(txPort, LOW);
  delayMicroseconds(wt450TriggerPulse);  
  delayMicroseconds(wt450TriggerPulse);  
}

void sendWT450Bits(unsigned int data, int bits)
{
  unsigned int bitMask = 1;
  bitMask = bitMask << (bits - 1);
  for(int i = 0; i < bits; i++)
  {
    sendWT450Bit( (data&bitMask) == 0 ? 0 : 1);
    bitMask = bitMask >> 1;
  }
}

void sendWT450Bit(byte b)
{
  // single long pulse = '0'
  if(b == 0)
  {
    digitalWrite(txPort, pulseState);
    delayMicroseconds(wt450LongPulse);  
    pulseState = !pulseState;
  }
  // short pulse then short pulse = '1'
  else
  {
    digitalWrite(txPort, pulseState);
    delayMicroseconds(wt450ShortPulse);
    digitalWrite(txPort, !pulseState);
    delayMicroseconds(wt450ShortPulse);  
    // pulseState will be unchanged after two pulses
    parityBit = !parityBit;
  }
}


