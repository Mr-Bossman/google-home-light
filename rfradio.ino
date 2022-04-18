#include <SPI.h>
#include <RFM69registers.h>

int led = 13;

int SSpin = 8;
SPIClass * spi_inst;
uint8_t readReg(uint8_t addr);
void writeReg(uint8_t addr, uint8_t value);
void init_radio();
void senddat(const void* buff, uint8_t bufferSize);
#define RF69_FSTEP  61.03515625 // == FXOSC / 2^19 = 32MHz / 2^19 (p13 in datasheet)

void light_on(){
  char txa[] = {0x55 ,0xd3 ,0xe5 ,0x43 ,0x01 ,0x40 ,0x02 ,0x10 ,0xa5 ,0xf8};
  char txb[] = {0x55 ,0xd3 ,0xe5 ,0x43 ,0x01 ,0x40};
  senddat(txa,sizeof(txa));
  senddat(txa,sizeof(txa));
  senddat(txa,sizeof(txa));
  senddat(txa,sizeof(txa));
  senddat(txb,sizeof(txb));
}

void light_off(){
  char txa[] = {0x55 ,0xd3 ,0xe5 ,0x43 ,0x01 ,0x40 ,0x02 ,0x10 ,0xa4 ,0xf7};
  char txb[] = {0x55 ,0xd3 ,0xe5 ,0x43 ,0x01 ,0x40};
  senddat(txa,sizeof(txa));
  senddat(txa,sizeof(txa));
  senddat(txa,sizeof(txa));
  senddat(txa,sizeof(txa));
  senddat(txb,sizeof(txb));
}
void test(){
  char txa[] = {0x55, 0xd3, 0xe5, 0x43, 0x01, 0x40, 0x02, 0x41, 0x02, 0x86, 0xaa, 0xaa};
  char txb[] = {0x55 ,0xd3 ,0xe5 ,0x43 ,0x01 ,0x40};
  for(int i = 0; i < 200; i++)
  senddat(txa,sizeof(txa));
  senddat(txb,sizeof(txb));
}
void blinkme(){
  light_on();
  digitalWrite(led, HIGH);
  delay(2000);
  light_off();
  digitalWrite(led, LOW);
  delay(2000);
}


void setup() {
  pinMode(led, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  spi_inst = &SPI;
  spi_inst->setDataMode(SPI_MODE0);
  spi_inst->setBitOrder(MSBFIRST);
  spi_inst->setClockDivider(SPI_CLOCK_DIV16);
  spi_inst->begin();
  Serial.begin(115200);
  init_radio();
  bool val = false;
  while(1){
    int cont = 0;
    if(readReg(0x28)&(1<<6)){
      while(readReg(0x28)&(1<<6)){
        unsigned char tmp = readReg(0);
        cont++;
        if(cont == 10){
          if(tmp == 0xf8){
            digitalWrite(led, HIGH);
            Serial.println("on");
            val = true;
          } else if(tmp == 0xf7){
            digitalWrite(led, LOW);
            Serial.println("off");
            val = false;
          }
        }
        char buf[6];
        //snprintf ( buf, 6,"%02x ",tmp);
        //Serial.print(buf);
        delayMicroseconds(850);
      }
      //Serial.println();
    }
    if(digitalRead(2)){
      if(val){
        digitalWrite(led, LOW);
        light_off();
        val = false;
      }else{
        digitalWrite(led, HIGH);
        light_on();
        val = true;
      }
      while(digitalRead(2));
    }
    if (Serial.available() > 0)
    {
      char input = Serial.read();
      if (input == 'b'){
         blinkme();
      }
      if (input == '1'){
         Serial.println("on");
         digitalWrite(led, HIGH);
         light_on();
         val = true;
      }
      if (input == '^'){
        if(val){
          Serial.println("off");
          digitalWrite(led, LOW);
          light_off();
          val = false;
        }else{
          Serial.println("on");
          digitalWrite(led, HIGH);
          light_on();
          val = true;
        }
      }
      if (input == '0'){
         Serial.println("off");
         digitalWrite(led, LOW);
         light_off();
         val = false;
      }
      if (input == 't'){
         test();
      }
    }
    }
}

void loop() {
}

void senddat(const void* buff, uint8_t bufferSize){
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
    while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // wait for ModeReady
    for (uint8_t i = 0; i < bufferSize; i++)
        writeReg(0,((uint8_t*) buff)[i]);
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
    while ((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) == 0x00); // wait for PacketSent
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
}
void init_radio(){
    const uint8_t CONFIG[][2] =
  {
    /* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
    /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_11 }, // no shaping
    /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_15000}, // default: 4.8 KBPS
    /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_15000},
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_50000}, // default: 5KHz, (FDEV + BitRate / 2 <= 500KHz)
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_50000},

    /* 0x07 */ { REG_FRFMSB, (uint8_t) (RF_FRFMSB_917) },
    /* 0x08 */ { REG_FRFMID, (uint8_t) (RF_FRFMID_917) },
    /* 0x09 */ { REG_FRFLSB, (uint8_t) (RF_FRFLSB_917) },

    // looks like PA1 and PA2 are not implemented on RFM69W/CW, hence the max output power is 13dBm
    // +17dBm and +20dBm are possible on RFM69HW
    // +13dBm formula: Pout = -18 + OutputPower (with PA0 or PA1**)
    // +17dBm formula: Pout = -14 + OutputPower (with PA1 and PA2)**
    // +20dBm formula: Pout = -11 + OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
    /* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | RF_PALEVEL_OUTPUTPOWER_11111},
    /* 0x13 */ { REG_OCP, RF_OCP_OFF }, // over current protection (default is 95mA)
    /* 0x19 */ { REG_LNA, RF_LNA_GAINSELECT_AUTO}, // (BitRate < 2 * RxBw)
    // RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4KHz)
    /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2 }, // (BitRate < 2 * RxBw)
    //for BR-19200: /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_3 },
    /* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_10 | RF_DIOMAPPING1_DIO2_00 }, // DIO0 is the only IRQ we're using
    /* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, // DIO5 ClkOut disable for power saving
    /* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // writing to this bit ensures that the FIFO & status flags are reset
    /* 0x29 */ { REG_RSSITHRESH, 220 }, // must be set to dBm = (-Sensitivity / 2), default is 0xE4 = 228 so -114dBm
    /* 0x2D */ { REG_PREAMBLELSB, 8}, // default 3 preamble bytes 0xAAAAAA
    /* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0 },
    /* 0x2F */ { REG_SYNCVALUE1, 0x2d },      // attempt to make this compatible with sync1 byte of RFM12B lib
    /* 0x30 */ { REG_SYNCVALUE2, 0xd4 }, // NETWORK ID
    /* 0x30 */ { REG_SYNCVALUE3, 0x55 }, // NETWORK ID
    //* 0x31 */ { REG_SYNCVALUE4, 0xBB },
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | RF_PACKET1_ADRSFILTERING_OFF },
    /* 0x38 */ { REG_PAYLOADLENGTH, 120}, // in variable length mode: the max frame size, not used in TX
    ///* 0x39 */ { REG_NODEADRS, nodeID }, // turned off because we're not using address filtering
    /* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, // TX on FIFO not empty
    /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_OFF | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //for BR-19200: /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
    {255, 0}
  };

  digitalWrite(SSpin, HIGH);
  pinMode(SSpin, OUTPUT);
  for (uint8_t i = 0; CONFIG[i][0] != 255; i++)
    writeReg(CONFIG[i][0], CONFIG[i][1]);
  writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
  while (((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00)); // wait for ModeReady
  uint32_t freqHz = 916500000;
  freqHz /= RF69_FSTEP; // divide down by FSTEP to get FRF
  writeReg(REG_FRFMSB, freqHz >> 16);
  writeReg(REG_FRFMID, freqHz >> 8);
  writeReg(REG_FRFLSB, freqHz);
  writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
}
uint8_t readReg(uint8_t addr)
{
  digitalWrite(SSpin, LOW);
  spi_inst->transfer(addr & 0x7F);
  uint8_t regval = spi_inst->transfer(0);
  digitalWrite(SSpin, HIGH);
  return regval;
}

void writeReg(uint8_t addr, uint8_t value)
{
  digitalWrite(SSpin, LOW);
  spi_inst->transfer(addr | 0x80);
  spi_inst->transfer(value);
  digitalWrite(SSpin, HIGH);
}