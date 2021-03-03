// 9600 NMEA
#if (Telemetry_In == 0)    //  Serial
uint16_t Lth=0;

uint8_t Mav1 = 0;
uint8_t Mav2 = 0;
uint8_t SPort = 0;
uint8_t FPort1 = 0;
uint8_t FPort2 = 0;
uint8_t LTM = 0;
uint8_t MSP = 0;
uint8_t NMEA_GPS = 0;

uint16_t  q = 0;
uint16_t  r = 0;



char ch[3]; // rolling input bytes

// **********************************************************
uint8_t getProtocol() { 
     
   #if defined AutoBaud
      baud = getBaud(rxPin);
      Log.print("Baud rate detected is ");  Log.print(baud); Log.println(" b/s"); 
      String s_baud=String(baud);   // integer to string. "String" overloaded
      LogScreenPrintln("Telem at "+ s_baud);   
   #else 
     baud = 57600;  // default if not autobaud
     Log.print("Default baud rate is ");  Log.print(baud); Log.println(" b/s"); 
     String s_baud=String(baud);   // integer to string. "String" overloaded
     LogScreenPrintln("Default b/s:"+ s_baud);  
   #endif  
   uint8_t proto = detectProtocol(baud);
   return proto;
}

//***************************************************
uint16_t detectProtocol(uint32_t baud) {

    #if ( (defined ESP8266) || (defined ESP32) ) 
       delay(10);
      inSerial.begin(baud, SERIAL_8N1, rxPin, txPin, rxInvert); 
    #elif (defined TEENSY3X) 
      frSerial.begin(frBaud); // Teensy 3.x    tx pin hard wired
       if (rxInvert) {          // For S.Port not F.Port
         UART0_C3 = 0x10;       // Invert Serial1 Tx levels
         UART0_S2 = 0x10;       // Invert Serial1 Rx levels;       
       }
       #if (defined frOneWire )  // default
         UART0_C1 = 0xA0;        // Switch Serial1 to single wire (half-duplex) mode  
       #endif    
    #else
      inSerial.begin(baud);
    #endif     
  
    chr = NxtChar();
    while (Lth>0) {  //  ignore 0x00 for this routine
      
      if (chr==0xFE) {   
        int pl = NxtChar();   // Payload length
        if (pl>=8 && pl<=37) {
          Mav1++;             // Found candidate Mavlink 1
          if (Mav1>10) {
            inSerial.end();
            return 1; 
          }
        }
      }

      if (chr==0xFD) {
        Mav2++;                     // Found candidate Mavlink 2
        if (Mav2>10) {
          inSerial.end();
          return 2;    
        }
      }

      if (chr==0x7E)                             // Could be S.Port or F.Port1
        if (ch[1]==0x7E) {                       // must be FPort1
          FPort1++;
          if (SPort) SPort--;                    // remove the trailing 0x7E count                           
          if (FPort1>10) {
            inSerial.end();
            return 4;    
          }
      } else {                                   // else it's SPort
        SPort++;                                
        if (SPort>10) {
          inSerial.end();
          return 3;        
        }
      } 
      
      if ( ( (ch[2] == 0x0D) || (ch[2] == 0x18)|| (ch[2] == 0x23) ) && ( (chr == 0xf1) || (chr == 0xf1) ) )  {  // fp2 OTA     
        FPort2++; 
        if (FPort2>10) {
          inSerial.end();
          return 5;             
        }
      } else  
      if ( (ch[2] == 0x08) && ( (chr >= 0) && (chr < 0x1b) ) )   {     // fp2 downlink  
        FPort2++; 
        if (FPort2>10) {
          inSerial.end();
          return 5;  
        }             
      } 
      
      /*
      if (chr==0x24) {    //  'S'
        chr = NxtChar(); // start2 should be 0x54 ('T')
        if (chr==0x54) LTM++;                     // Found candidate LTM
      }
     */
     
      if ((ch[0]=='$') && (ch[1]=='T') && (ch[2]=='<')) {
        LTM++;       // Found candidate LTM
        if (LTM>10) {
          inSerial.end();
          return 6; 
        }
      }
      
      if ((ch[0]=='$') && (ch[1]=='M') && (ch[2]=='<')) {
        MSP++;       // Found candidate MSP 
        if (MSP>10) {
          inSerial.end();
          return 7; 
        }
      }

      if ((ch[0]=='$') && (ch[1]=='G') && (ch[2]=='P')) {
        NMEA_GPS++; // Found candidate GPS
        if (NMEA_GPS>10) {
          inSerial.end();
          return 8;      
        }
      }

    #if defined Debug_All || defined Debug_Protocol
      Log.print("Mav1=");  Log.println(Mav1);
      Log.print("Mav2=");  Log.println(Mav2);
      Log.print("SPort=");  Log.println(SPort);
      Log.print("FPort1=");  Log.println(FPort1);  
      Log.print("FPort2=");  Log.println(FPort2);          
      Log.print("LTM=");  Log.println(LTM);
      Log.print("MSP=");  Log.println(MSP);
      Log.print("NMEA_GPS=");  Log.println(NMEA_GPS);
    #endif  
  
    chr = NxtChar(); 
    ch[0] = ch[1];   // rolling input bytes
    ch[1] = ch[2];
    ch[2] = chr;
      
    }
   delay(5); 
}

//***************************************************
byte NxtChar() {
byte x;

  Lth=inSerial.available();     //   wait for more data

  while (Lth==0) {
 
    q++;      
    if (q>100) {
      #if defined Debug_All || defined Debug_Protocol
        Log.print("."); 
      #endif     
      LogScreenPrintln("No telemetry");
      q=0;
    }

    delay(100);
    Lth=inSerial.available();
  }
  q=0;
  // Data is available

  x = inSerial.read();
  
  r++;
  
  #if defined Debug_All || defined Debug_Protocol
   // Log.print((char)x); 
     Printbyte(x, false, ' ');  
   
   if ((r % 50) == 0) {
      Log.println();
      r = 0;
    }
  #endif  
  
  return x;
}
// **********************************************************
uint32_t getBaud(uint8_t rxPin) {
  Log.print("AutoBaud - sensing rxPin "); Log.println(rxPin);
 // Log.printf("AutoBaud - sensing rxPin %2d \n", rxPin );
  uint8_t i = 0;
  uint8_t col = 0;
  pinMode(rxPin, INPUT);       
  digitalWrite (rxPin, HIGH); // pull up enabled for noise reduction ?

  uint32_t gb_baud = GetConsistent(rxPin);
  while (gb_baud == 0) {
    if(ftgetBaud) {
      ftgetBaud = false;
    }

    i++;
    if ((i % 5) == 0) {
      Log.print(".");
      col++; 
    }
    if (col > 60) {
      Log.println(); 
        Log.print("No telemetry found on pin "); Log.println(rxPin);
  //    Log.printf("No telemetry found on pin %2d\n", rxPin); 
      col = 0;
      i = 0;
    }
    gb_baud = GetConsistent(rxPin);
  } 
  if (!ftgetBaud) {
    Log.println();
  }

  //Log.print("Telem found at "); Log.print(gb_baud);  Log.println(" b/s");
  //LogScreenPrintln("Telem found at " + String(gb_baud));

  return(gb_baud);
}
//************************************************
uint32_t GetConsistent(uint8_t rxPin) {
  uint32_t t_baud[5];

  while (true) {  
    t_baud[0] = SenseUart(rxPin);
    delay(10);
    t_baud[1] = SenseUart(rxPin);
    delay(10);
    t_baud[2] = SenseUart(rxPin);
    delay(10);
    t_baud[3] = SenseUart(rxPin);
    delay(10);
    t_baud[4] = SenseUart(rxPin);
    #if defined Debug_All || defined Debug_Baud
      Log.print("  t_baud[0]="); Log.print(t_baud[0]);
      Log.print("  t_baud[1]="); Log.print(t_baud[1]);
      Log.print("  t_baud[2]="); Log.print(t_baud[2]);
      Log.print("  t_baud[3]="); Log.println(t_baud[3]);
    #endif  
    if (t_baud[0] == t_baud[1]) {
      if (t_baud[1] == t_baud[2]) {
        if (t_baud[2] == t_baud[3]) { 
          if (t_baud[3] == t_baud[4]) {   
            #if defined Debug_All || defined Debug_Baud    
              Log.print("Consistent baud found="); Log.println(t_baud[3]); 
            #endif   
            return t_baud[3]; 
          }          
        }
      }
    }
  }
}
//************************************************
uint32_t SenseUart(uint8_t  rxPin) {

uint32_t pw = 999999;  //  Pulse width in uS
uint32_t min_pw = 999999;
uint32_t su_baud = 0;
const uint32_t su_timeout = 5000; // uS !  Default timeout 1000mS!

  #if defined Debug_All || defined Debug_Baud
    Log.printf("rxPin:%d  rxInvert:%d\n", rxPin, rxInvert);  
  #endif  

  if (rxInvert) {
    while(digitalRead(rxPin) == 0){ };  // idle_low, wait for high bit (low pulse) to start
  } else {
    while(digitalRead(rxPin) == 1){ };  // idle_high, wait for high bit (high pulse) to start  
  }

  for (int i = 0; i < 10; i++) {

    if (rxInvert) {               //  Returns the length of the pulse in uS
      pw = pulseIn(rxPin,HIGH, su_timeout);     
    } else {
      pw = pulseIn(rxPin,LOW, su_timeout);    
    }    

    if (pw !=0) {
      min_pw = (pw < min_pw) ? pw : min_pw;  // Choose the lowest
      //Log.printf("i:%d  pw:%d  min_pw:%d\n", i, pw, min_pw);      
    } 
  } 
  #if defined Debug_All || defined Debug_Baud
    Log.printf("pw:%d  min_pw:%d\n", pw, min_pw);
  #endif

  switch(min_pw) {   
    case 1:     
     su_baud = 921600;
      break;
    case 2:     
     su_baud = 460800;
      break;     
    case 4 ... 11:     
     su_baud = 115200;
      break;
    case 12 ... 19:  
     su_baud = 57600;
      break;
     case 20 ... 28:  
     su_baud = 38400;
      break; 
    case 29 ... 39:  
     su_baud = 28800;
      break;
    case 40 ... 59:  
     su_baud = 19200;
      break;
    case 60 ... 79:  
     su_baud = 14400;
      break;
    case 80 ... 149:  
     su_baud = 9600;
      break;
    case 150 ... 299:  
     su_baud = 4800;
      break;
     case 300 ... 599:  
     su_baud = 2400;
      break;
     case 600 ... 1199:  
     su_baud = 1200;  
      break;                        
    default:  
     su_baud = 0;    // no signal        
 }
  return su_baud;
} 
#endif

    //======================================================
    pol_t getPolarity(uint8_t rxpin) {
      uint16_t hi = 0;
      uint16_t lo = 0;  
      const uint16_t mx = 1000;
      while ((lo <= mx) && (hi <= mx)) {

        if (digitalRead(rxpin) == 0) { 
          lo++;
        } else 
        if (digitalRead(rxpin) == 1) {  
          hi++;
        }

        if (lo > mx) {
          if (hi == 0) {         
            return no_traffic;
          } else {
            return idle_low; 
          }
        }
        if (hi > mx) {
          if (lo == 0) {
            return no_traffic;
          } else {
            return idle_high; 
          }
        }
        delayMicroseconds(200);
       //Log.printf("rxpin:%d  %d\t\t%d\n",rxpin, lo, hi);  
     } 
    } 