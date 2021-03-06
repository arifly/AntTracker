//=================================================================================================  
//================================================================================================= 
//
//                                       U T I L I T I E S
//
//================================================================================================= 
//=================================================================================================
 
//=================================================================================================   
//                           D I S P L A Y   S U P P O R T   -   ESP Only - for now
//================================================================================================= 

  #if defined displaySupport  
    void HandleDisplayButtons() {

      if (Pinfo != 99)  {   // if digital pin for info display enumerated
        infoButton = !digitalRead(Pinfo);  // low == pressed
      }
        
      if ((infoButton) && (!infoPressBusy)) { 
        infoPressBusy = true; 
        infoNewPress = true;          
        info_debounce_millis = millis();   

        info_millis = millis();                       
        if (show_log) {
          show_log = false; 
          info_millis = millis() + db_period;  
        } else {
          show_log = true;    
        }
      }
        
      if(millis() - info_debounce_millis > db_period) { 
        infoPressBusy = false; 
        infoButton = false; // for slow decay touch buttons
      }

      if (millis() - last_log_millis > 15000) { // after 15 seconds default to flight info screen
        last_log_millis = millis();             // and enable toggle button again
        show_log = false;
      }

      
      if (show_log) {
        if (infoNewPress) {     
          PaintLogScreen(row, show_last_row);  // one time     
          infoNewPress = false; 
          last_log_millis = millis();
        }
      } else {            // else show flight info
        DisplayFlightInfo();             
      }
      
      //Log.printf("busy=%d  new=%d log=%d  bounce=%d  info=%d\n", infoPressBusy, infoNewPress, show_log, info_debounce_millis, info_millis); 
      
     #if ((defined ESP32) || (defined ESP8266))   // Teensy does not have touch pins          
      if ( (Tup != 99) && (Tdn != 99) ) {         // if ESP touch pin-pair enumerated
        if (upButton) {
          Scroll_Display(up);
        }
        if (dnButton) {
          Scroll_Display(down);
        }     
      } else
      #endif
      
      if ( (Pup != 99) && (Pdn != 99) ) {   // if digital pin-pair enumerated
        upButton = !digitalRead(Pup);       // low == pressed
        if (upButton) {                 
          Scroll_Display(up);
        }
        dnButton = !digitalRead(Pdn);
        if (dnButton) {
          Scroll_Display(down);
        }        
      }        
    }

    //===================================
    #if ((defined ESP32) || (defined ESP8266)) 
    void IRAM_ATTR gotButtonUp(){
      upButton = true;
    }

    void IRAM_ATTR gotButtonDn(){
      dnButton = true;  
    }
    
    void IRAM_ATTR gotButtonInfo(){
      infoButton = true;
    }
    #endif 
    //===================================
   
    void Scroll_Display(scroll_t up_dn) {
      
      if (millis() - scroll_millis < 300) return;
      show_log = true;    
      scroll_millis = millis(); 
      
      if (up_dn == up) {
         scroll_row--;
         scroll_row = constrain(scroll_row, SCR_H_CH, row);
         upButton = false; 
         PaintLogScreen(scroll_row, show_last_row);   // paint down to scroll_row
      }
      if (up_dn == down) {
          scroll_row++; 
          scroll_row = constrain(scroll_row, SCR_H_CH, row);       
          dnButton = false; 
          PaintLogScreen(scroll_row, show_last_row);  // paint down to scroll_row      
      }   
    }

    //===================================
    void PaintLogScreen(uint8_t new_row, last_row_t last_row_action) { 
      if (display_mode != logg) { 
          SetupLogDisplayStyle();
          display_mode = logg; 
      }  
     
        #if (defined ST7789_Display) || (defined SSD1331_Display) ||  (defined ILI9341_Display)   
        //  hardware SPI pins defined in config.h 
          display.fillScreen(SCR_BACKGROUND);                 
        #elif (defined SSD1306_Display) 
          display.clearDisplay();
        #endif  
        display.setCursor(0,0);  
        int8_t first_row = (last_row_action==omit_last_row) ? (new_row - SCR_H_CH +1) : (new_row - SCR_H_CH); 
        int8_t last_row = (last_row_action==omit_last_row) ? new_row : (new_row );        
        for (int i = first_row ; i < last_row; i++) { // drop first line, display rest of old lines & leave space for new line          
          display.println(ScreenRow[i].x);
        }
   
        #if (defined SSD1306_Display)
          display.display();
        #endif 
    }
    
  #endif  // end of defined displaySupport
    
    //===================================
    void LogScreenPrintln(String S) {
    #if defined displaySupport   
    
      if (display_mode != logg) {
          SetupLogDisplayStyle();
          display_mode = logg; 
      }   
      if (row >= SCR_H_CH) {                 // if the new line exceeds the page lth, re-display existing lines
        PaintLogScreen(row, omit_last_row);
      }
      uint16_t lth = strlen(S.c_str());           // store the new line a char at a time
      if (lth > max_col-1) { 
        #if defined STM32F103C
            Log.print("Display width of "); Log.print(SCR_W_CH);
            Log.print(" exceeded for |"); Log.print(S); Log.println("|");  
          #else
            Log.printf("Display width of %d exceeded for |%s|\n", SCR_W_CH, S.c_str());  // SCR_W_CH = max_col-1
          #endif            
        lth = max_col-1;  // prevent array overflow
      }

      for (int i=0 ; i < lth ; i++ ) {
        ScreenRow[row].x[col] = S[i];
        col++;
      } 

      for (col=col ; col < max_col; col++) {    //  padd out the new line to eol
        ScreenRow[row].x[col] = '\0';
      } 

      display.println(ScreenRow[row].x);        // display the new line, which is always the last line
      #if (defined SSD1306_Display)
        display.display();
      #endif  

      col = 0;
      row++;
      if (row > max_row-1) {
        Log.println("Display rows exceeded!");
        row = max_row-1;  // prevent array overflow
      }
      last_log_millis = millis();             // and enable toggle button again
      show_log = true;          
    #endif       
    } // ready for next line

    //===================================
   
    void LogScreenPrint(String S) {
    #if defined displaySupport  

      if (display_mode != logg) {
          SetupLogDisplayStyle();
          display_mode = logg; 
      }   

     // scroll_row = row; 
      if (row >= SCR_H_CH) {              // if the new line exceeds the page lth, re-display existing lines
        PaintLogScreen(row, omit_last_row);
      }
      display.print(S);                         // the new line
      #if (defined SSD1306_Display)
        display.display();
      #endif 
       
      uint8_t lth = strlen(S.c_str());          // store the line a char at a time
      if (lth > SCR_W_CH) {
        #if defined STM32F103C
          Log.print("Display width of "); Log.print(SCR_W_CH);
          Log.print(" exceeded for |"); Log.print(S); Log.println("|");  
        #else
          Log.printf("Display width of %d exceeded for |%s|\n", SCR_W_CH, S.c_str());  // SCR_W_CH = max_col-1
        #endif   
        lth = max_col-1;  // prevent array overflow
      }  

      for (int i=0 ; i < lth ; i++ ) {
        ScreenRow[row].x[col] = S[i];
        col++;
      } 
      for (col=col ; col < max_col; col++) {  //  padd out to eol
        ScreenRow[row].x[col] = '\0';
      }
      
      if (col > max_col-1) {   // only if columns exceeded, increment row
        col = 0;
        row++;
      }
      last_log_millis = millis();             // and enable toggle button again
      show_log = true;
    #endif    
    } // ready for next line
    
    //===================================
    #if defined displaySupport  
    
    void DisplayFlightInfo() {
      uint16_t xx, yy; 
      if (display_mode != flight_info) {
          SetupInfoDisplayStyle();
          display_mode = flight_info; 
      }
      

      #if  (defined ILI9341_Display)


        if (millis() - info_millis > 200) {    // refresh rate
          info_millis = millis();  

          // artificial horizon
          draw_horizon(ap_roll, ap_pitch, SCR_W_PX, SCR_H_PX);

          display.setTextSize(2);    // 26 ch wide x 15 ch deep
          
          // sats visible
          xx = 0;
          yy = 0 ;          
          display.setCursor(xx, yy);  
          snprintf(snprintf_buf, snp_max, "Sats:%d", fr_numsats); 
          display.fillRect(xx +(4*CHAR_W_PX), yy, 2 * CHAR_W_PX, CHAR_H_PX, ILI9341_BLUE); // clear the previous line               
          display.println(snprintf_buf);  

          // heading (yaw)
          xx = 9 * CHAR_W_PX;
          yy = 0 ;          
          display.setCursor(xx, yy);  
          snprintf(snprintf_buf, snp_max, "Hdg:%.0f%", fr_yaw / 10);
          display.fillRect(xx+(4*CHAR_W_PX), yy, 4 * CHAR_W_PX, CHAR_H_PX, ILI9341_BLUE); // clear the previous line                                
          display.println(snprintf_buf);

          // Radio RSSI
          xx = 17 * CHAR_W_PX;
          yy = 0 ;          
          display.setCursor(xx, yy);  
          snprintf(snprintf_buf, snp_max, "RSSI:%ld%%", ap_rssi); 
          display.fillRect(xx+(4*CHAR_W_PX), yy, 4 * CHAR_W_PX, CHAR_H_PX, ILI9341_BLUE); // clear the previous line               
          display.println(snprintf_buf);
              
          // distance to home
          xx = 0;
          yy = 13.5 * CHAR_H_PX;        
          display.setCursor(xx, yy); 
          snprintf(snprintf_buf, snp_max, "Home:%d", pt_home_dist);    // m 
          display.fillRect(xx+(5*CHAR_W_PX), yy, (4*CHAR_W_PX), CHAR_H_PX, ILI9341_BLUE); // clear the previous line   
          display.println(snprintf_buf); 

          // arrow to home
          xx = 14 * CHAR_W_PX;
          yy = 13.5 * CHAR_H_PX;   
          draw_home_arrow(xx, yy, pt_home_angle, SCR_W_PX, SCR_H_PX);
   
          // altitude above home
          xx = 18 * CHAR_W_PX;
          yy = 14 * CHAR_W_PX;        
          display.setCursor(xx, yy); 
          snprintf(snprintf_buf, snp_max, "Alt:%d", cur.alt);    // mm => m 
          display.fillRect(xx+(4*CHAR_W_PX), yy, (4*CHAR_W_PX), CHAR_H_PX, ILI9341_BLUE); // clear the previous line   
          display.println(snprintf_buf); 

          // voltage
          xx = 0;
          yy = 16 * CHAR_W_PX;        
          display.setCursor(xx, yy); 
          snprintf(snprintf_buf, snp_max, "V:%.1fV", fr_bat1_volts  * 0.1F);     
          display.fillRect(xx+(2*CHAR_W_PX), yy, (6*CHAR_W_PX), CHAR_H_PX, ILI9341_BLUE); // clear the previous line   
          display.println(snprintf_buf); 
          
          // current
          xx = 9 * CHAR_W_PX;
          yy = 16 * CHAR_W_PX;        
          display.setCursor(xx, yy); 
          snprintf(snprintf_buf, snp_max, "A:%.0f", fr_bat1_amps * 0.1F);     
          display.fillRect(xx+(2*CHAR_W_PX), yy, (6*CHAR_W_PX), CHAR_H_PX, ILI9341_BLUE); // clear the previous line   
          display.println(snprintf_buf); 
          
          // Ah consumed
          xx = 18 * CHAR_W_PX;
          yy = 16 * CHAR_W_PX;        
          display.setCursor(xx, yy); 
          snprintf(snprintf_buf, snp_max, "Ah:%.1f", fr_bat1_mAh * 0.001F);     
          display.fillRect(xx+(3*CHAR_W_PX), yy, (5*CHAR_W_PX), CHAR_H_PX, ILI9341_BLUE); // clear the previous line   
          display.println(snprintf_buf);           
          
          // latitude and logitude
          display.setTextSize(1);          
          xx = 0;
          yy = 18 * CHAR_H_PX;
          display.setCursor(xx,yy);       
          snprintf(snprintf_buf, snp_max, "Lat:%.7f", cur.lat);
          display.fillRect(xx, yy, (15*CHAR_W_PX), CHAR_H_PX, ILI9341_BLUE); // clear the previous line        
          display.println(snprintf_buf);  
          xx = 18 * CHAR_W_PX;   
          yy = 18 * CHAR_H_PX;  
          display.setCursor(xx, yy);    
          snprintf(snprintf_buf, snp_max, "Lon:%.7f", cur.lon);
          display.fillRect(xx, yy, 21 * CHAR_W_PX, CHAR_H_PX, ILI9341_BLUE); // clear the previous line            
          display.println(snprintf_buf);  
          display.setTextSize(2);    // 26 ch wide x 15 ch deep
          display_mode = flight_info;
        }
      #else
        if (millis() - info_millis > 2000) {    // refresh rate
          info_millis = millis();  
          // Latitude
          xx = 0;
          yy = 0;
          display.setCursor(xx,yy);       
          snprintf(snprintf_buf, snp_max, "Lat %.7f", cur.lat);
          display.fillRect(xx+(4*CHAR_W_PX), yy, 11 * CHAR_W_PX, CHAR_H_PX, SCR_BACKGROUND); // clear the previous data           
          display.println(snprintf_buf);  

          // Longitude
          xx = 0;
          yy = 1.8 * CHAR_H_PX;    
          display.setCursor(xx, yy);                 
          snprintf(snprintf_buf, snp_max, "Lon %.7f", cur.lon);
          display.fillRect(xx+(4*CHAR_W_PX), yy, 11 * CHAR_W_PX, CHAR_H_PX, SCR_BACKGROUND);        
          display.println(snprintf_buf); 

          // Volts, Amps and Ah 
          xx = 0;
          yy = 3.6 * CHAR_H_PX;      
          display.setCursor(xx, yy);               
          snprintf(snprintf_buf, snp_max, "%.1fV %.0fA %.1fAh", fr_bat1_volts * 0.1F, fr_bat1_amps * 0.1F, fr_bat1_mAh * 0.001F);   
          display.fillRect(xx, yy, SCR_W_PX, CHAR_H_PX, SCR_BACKGROUND); // clear the whole line  
          display.println(snprintf_buf); 

          // Number of Sats and RSSI
          xx = 0;
          yy = 5.4 * CHAR_H_PX;      
          display.setCursor(xx, yy);            
          snprintf(snprintf_buf, snp_max, "Sats %2d RSSI %2d%%", fr_numsats, fr_rssi); 
          display.fillRect(xx+(5*CHAR_W_PX), yy, 3 * CHAR_W_PX, CHAR_H_PX, SCR_BACKGROUND);  
          display.fillRect(xx+(12*CHAR_W_PX), yy, 4 * CHAR_W_PX, CHAR_H_PX, SCR_BACKGROUND);   // blank rssi         
          display.println(snprintf_buf);       

           
          #if (defined SSD1306_Display)
            display.display();
          #endif 
  
        }
      #endif    
    } 
    #endif    

    //===================================
    #if defined displaySupport  
    
    void SetupLogDisplayStyle() {
       
      #if (defined ST7789_Display)      // LILYGO?? TTGO T-Display ESP32 1.14" ST7789 Colour LCD
        #if (SCR_ORIENT == 0)           // portrait
          display.setRotation(0);       // or 4 
          display.setTextFont(0);       // Original Adafruit font 0, try 0 thru 6 
        #elif (SCR_ORIENT == 1)         // landscape
          display.setRotation(3);       // or 1 
          display.setTextFont(1);        
        #endif   
         
      display.setTextSize(TEXT_SIZE);
      display.fillScreen(SCR_BACKGROUND);
      display.setTextColor(TFT_SKYBLUE);    
            
      //display.setTextColor(TFT_WHITE);
      //display.setTextColor(TFT_BLUE);  
      //display.setTextColor(TFT_GREEN, TFT_BLACK);
    
      #elif (defined SSD1306_Display)            // all  boards with SSD1306 OLED display
        display.clearDisplay(); 
        display.setTextColor(WHITE);  
        display.setTextSize(TEXT_SIZE);  
 
      #elif (defined SSD1331_Display)            // T2 board with SSD1331 colour TFT display
        //  software SPI pins defined in config.h 
        display.fillScreen(BLACK);
        display.setCursor(0,0);
        display.setTextSize(TEXT_SIZE);
        #define SCR_BACKGROUND BLACK  
        
      #elif (defined ILI9341_Display)           // ILI9341 2.8" COLOUR TFT SPI 240x320 V1.2  
        //  hardware SPI pins defined in config.h 
        display.fillScreen(ILI9341_BLUE);    
        display.setCursor(0,0);
        display.setTextSize(TEXT_SIZE);    // setup in config.h  
        #if (SCR_ORIENT == 0)              // portrait
          display.setRotation(2);          // portrait pins at the top rotation      
        #elif (SCR_ORIENT == 1)            // landscape
          display.setRotation(3);          // landscape pins on the left    
        #endif 
        #define SCR_BACKGROUND ILI9341_BLUE 
      #endif

    }
   #endif
    //===================================
    #if defined displaySupport  
    
    void SetupInfoDisplayStyle() {
    
      #if (defined ST7789_Display)      // LILYGO?? TTGO T-Display ESP32 1.14" ST7789 Colour LCD
        #if (SCR_ORIENT == 0)           // portrait
          display.setRotation(0);       // or 4 
          display.setTextSize(1);
          display.setTextFont(0);       // Original Adafruit font 0, try 0 thru 6 
        #elif (SCR_ORIENT == 1)         // landscape
          display.setRotation(3);       // or 1
          display.setTextSize(2);  
          display.setTextFont(1);        
        #endif    
      
      display.fillScreen(SCR_BACKGROUND);
      display.setTextColor(TFT_SKYBLUE);    
            
      //display.setTextColor(TFT_WHITE);
      //display.setTextColor(TFT_BLUE);  
      //display.setTextColor(TFT_GREEN, TFT_BLACK);
    
    #elif (defined SSD1306_Display)            // all  boards with SSD1306 OLED display
      display.clearDisplay(); 
      display.setTextColor(WHITE);  
      display.setTextSize(1);  
 
    #elif (defined SSD1331_Display)            // T2 board with SSD1331 colour TFT display
      //  SPI pins defined in config.h 
      display.fillScreen(BLACK);
      display.setTextColor(WHITE);  
      display.setTextSize(1);
      #define SCR_BACKGROUND BLACK  
      
    #elif (defined ILI9341_Display)            // ILI9341 2.8" COLOUR TFT SPI 240x320 V1.2  
      //  SPI pins defined in config.h 
      display.fillScreen(ILI9341_BLUE);
      display.setRotation(3);          // landscape pins on the left  
      display.setTextSize(2);     
      display.setCursor(0,0);
      #define SCR_BACKGROUND ILI9341_BLUE      
    #endif
   }
   #endif    

//=================================================================================================  
String TimeString (unsigned long epoch){
 uint8_t hh = (epoch  % 86400L) / 3600;   // remove the days (86400 secs per day) and div the remainer to get hrs
 uint8_t mm = (epoch  % 3600) / 60;       // calculate the minutes (3600 ms per minute)
 uint8_t ss = (epoch % 60);               // calculate the seconds

  String S = "";
  if (hh<10) S += "0";
  S += String(hh);
  S +=":";
  if (mm<10) S += "0";
  S += String(mm);
  S +=":";
  if (ss<10) S += "0";
  S += String(ss);
  return S;
}
//=================================================================================================  
void PrintMavBuffer(const void *object) {
  
    const unsigned char * const bytes = static_cast<const unsigned char *>(object);
  int j;

uint8_t   tl;

uint8_t mavNum;

//Mavlink 1 and 2
uint8_t mav_magic;               // protocol magic marker
uint8_t mav_len;                 // Length of payload

//uint8_t mav_incompat_flags;    // MAV2 flags that must be understood
//uint8_t mav_compat_flags;      // MAV2 flags that can be ignored if not understood

uint8_t mav_seq;                // Sequence of packet
//uint8_t mav_sysid;            // ID of message sender system/aircraft
//uint8_t mav_compid;           // ID of the message sender component
uint8_t mav_msgid;            
/*
uint8_t mav_msgid_b1;           ///< first 8 bits of the ID of the message 0:7; 
uint8_t mav_msgid_b2;           ///< middle 8 bits of the ID of the message 8:15;  
uint8_t mav_msgid_b3;           ///< last 8 bits of the ID of the message 16:23;
uint8_t mav_payload[280];      ///< A maximum of 255 payload bytes
uint16_t mav_checksum;          ///< X.25 CRC_Out
*/

  
  if ((bytes[0] == 0xFE) || (bytes[0] == 0xFD)) {
    j = -2;   // relative position moved forward 2 places
  } else {
    j = 0;
  }
   
  mav_magic = bytes[j+2];
  if (mav_magic == 0xFE) {  // Magic / start signal
    mavNum = 1;
  } else {
    mavNum = 2;
  }
/* Mav1:   8 bytes header + payload
 * magic
 * length
 * sequence
 * sysid
 * compid
 * msgid
 */
  
  if (mavNum == 1) {
    Log.print("mav1: /");

    if (j == 0) {
      Printbyte(bytes[0], false, ' '); // CRC_Out1
      Printbyte(bytes[1], false, ' '); // CRC_Out2

      Log.print("/");
      }
    mav_magic = bytes[j+2];   
    mav_len = bytes[j+3];
 //   mav_incompat_flags = bytes[j+4];;
 //   mav_compat_flags = bytes[j+5];;
    mav_seq = bytes[j+6];
 //   mav_sysid = bytes[j+7];
 //   mav_compid = bytes[j+8];
    mav_msgid = bytes[j+9];

    //Log.print(TimeString(millis()/1000)); Log.print(": ");
  
    Log.print("seq="); Log.print(mav_seq); Log.print("\t"); 
    Log.print("len="); Log.print(mav_len); Log.print("\t"); 
    Log.print("/");
    for (int i = (j+2); i < (j+10); i++) {  // Print the header
       Printbyte(bytes[i], false, ' '); 
    }
    
    Log.print("  ");
    Log.print("#");
    Log.print(mav_msgid);
    if (mav_msgid < 100) Log.print(" ");
    if (mav_msgid < 10)  Log.print(" ");
    Log.print("\t");
    
    tl = (mav_len+10);                // Total length: 8 bytes header + Payload + 2 bytes CRC_Out
 //   for (int i = (j+10); i < (j+tl); i++) {  
    for (int i = (j+10); i <= (tl); i++) {    
      Printbyte(bytes[i], false, ' '); 
    }
    if (j == -2) {
      Log.print("//");
      Printbyte(bytes[mav_len + 8], false, ' '); 
      Printbyte(bytes[mav_len + 9], false, ' ');       
      }
    Log.println("//");  
  } else {

/* Mav2:   10 bytes
 * magic
 * length
 * incompat_flags
 * mav_compat_flags 
 * sequence
 * sysid
 * compid
 * msgid[11] << 16) | [10] << 8) | [9]
 */
    
    Log.print("mav2:  /");
    if (j == 0) {
      Printbyte(bytes[0], false, ' '); // CRC_Out1 
      Printbyte(bytes[1], false, ' '); // CRC_Out2            
      Log.print("/");
    }
    mav_magic = bytes[2]; 
    mav_len = bytes[3];
//    mav_incompat_flags = bytes[4]; 
  //  mav_compat_flags = bytes[5];
    mav_seq = bytes[6];
//    mav_sysid = bytes[7];
   // mav_compid = bytes[8]; 
    mav_msgid = (bytes[11] << 16) | (bytes[10] << 8) | bytes[9]; 

    //Log.print(TimeString(millis()/1000)); Log.print(": ");

    Log.print("seq="); Log.print(mav_seq); Log.print("\t"); 
    Log.print("len="); Log.print(mav_len); Log.print("\t"); 
    Log.print("/");
    for (int i = (j+2); i < (j+12); i++) {  // Print the header
     Printbyte(bytes[i], false, ' '); 
    }

    Log.print("  ");
    Log.print("#");
    Log.print(mav_msgid);
    if (mav_msgid < 100) Log.print(" ");
    if (mav_msgid < 10)  Log.print(" ");
    Log.print("\t");

 //   tl = (mav_len+27);                // Total length: 10 bytes header + Payload + 2B CRC_Out +15 bytes signature
    tl = (mav_len+22);                  // This works, the above does not!
    for (int i = (j+12); i < (tl+j); i++) {   
       if (i == (mav_len + 12)) {
        Log.print("/");
      }
      if (i == (mav_len + 12 + 2+j)) {
        Log.print("/");
      }
      Printbyte(bytes[i], false, ' ');       
    }
    Log.println();
  }

   Log.print("Raw: ");
   for (int i = 0; i < 40; i++) {  //  unformatted
      Printbyte(bytes[i], false, ' ');  
    }
   Log.println();
  
}
//=================================================================================================  
void Printbyte(byte b, bool LF, char delimiter) {
  
  if ( (b == 0x7E) && (LF) ) {//             || (b == 0x10)  || (b == 0x32)) {
    Log.println();
  } 
  if (b<=0xf) Log.print("0");
  Log.print(b,HEX);
  Log.write(delimiter);
}

//==================================
void PrintByte(byte b) {
  if (b<=0xf) Log.print("0");
    Log.print(b,HEX);
    Log.print(" ");
}

//=================================================================================================  
void PrintMavBuffer(int lth){
  for ( int i = 0; i < lth; i++ ) {
    Printbyte(inBuf[i], false, ' ');
  }
  Log.println();
}
//=================================================================================================  
void PrintFrsBuffer(byte *buf, uint8_t len){
  Log.print("len:"); Log.print(len); Log.print("  ");
  for ( int i = 0; i < len; i++ ) {
    Printbyte(buf[i], false, ' ');
  }
  Log.println();
}
//=================================================================================================  
bool PacketGood() {
// Allow 1 degree of lat and lon away from home, i.e. 60 nautical miles radius at the equator
// Allow 1km up and 300m down from home altitude
if (homeInitialised==0) {  //  You can't use the home co-ordinates for a reasonability test if you don't have them yet
  return true;
  }
if (cur.lat<(hom.lat-1.0) || cur.lat>(hom.lat+1.0)) {  // Also works for negative lat
  Log.print(" Bad lat! cur.lat=");
  Log.print(cur.lat,7);  
  Log.print(" hom.lat=");Log.print(hom.lat,7);
  Log.println("  Packet ignored");   
  return false; 
  }
if (cur.lon<(hom.lon-1.0) || cur.lon>(hom.lon+1.0)) { // Also works for negative lon
  Log.print(" Bad lon! cur.lon=");
  Log.print(cur.lon,7);  
  Log.print(" hom.lon=");Log.print(hom.lon,7);
  Log.println("  Packet ignored");  
  return false;  
  }
if (cur.alt<(hom.alt-300) || cur.alt>(hom.alt+1000)) {
  Log.print(" Bad alt! cur.alt=");
  Log.print(cur.alt,0);  
  Log.print(" hom.alt=");Log.print(hom.alt,0);
  Log.println("  Packet ignored");    
  return false;  
  }
if ((cur.alt-hom.alt)<-300 || (cur.alt-hom.alt)>1000) {
  Log.print(" Bad alt! cur.alt=");
  Log.print(cur.alt,0);  
  Log.print(" hom.alt=");Log.print(hom.alt,0);
  Log.println("  Packet ignored");    
  return false;  
  }
if (Heading_Source == 2) { //  Heading source from flight controller
  if (cur.hdg<0 || cur.hdg>360) {
    Log.print(" Bad hdg! cur.hdg=");
    Log.print(cur.hdg,0);  
    Log.print(" hom.hdg=");Log.print(hom.hdg,0);
    Log.println("  Packet ignored");    
    return false;  
   }
}
  
return true;
}
//=================================================================================================  

    void CheckForTimeouts() {
      
     if ((millis() - mavGood_millis) > ((2*timeout_secs) * 1000)) {
       mavGood = false; 
       hbGood = false;         
     }    

     if ((millis() - frGood_millis) > (timeout_secs * 1000)) {
       frGood = false;
       pwmGood = false;  
     }
     
     if ((millis() - pwmGood_millis) > (timeout_secs * 1000) ) {
       pwmGood = false;
     }

     if ((millis() - gpsGood_millis) > (timeout_secs * 1000) ) {
      gpsGood = false;        // If no GPS packet  
    }    
   
     ReportOnlineStatus();
    }   
    //===================================================================     
    
    void ReportOnlineStatus() {
  
       if (frGood != frPrev) {  // report on change of status
         frPrev = frGood;
         if (frGood) {
           Log.println("FrSky read good!");
           LogScreenPrintln("FrSky read ok");         
         } else {
          Log.println("FrSky read timeout!");
          LogScreenPrintln("FrSky timeout");         
         }
       }
       
       if (pwmGood != pwmPrev) {  // report on change of status
         pwmPrev = pwmGood;
         if (pwmGood) {
           Log.println("RC PWM good");
           LogScreenPrintln("RC PWM good");         
         } else {
          Log.println("RC PWM timeout");
          LogScreenPrintln("RC PWM timeout");         
         }
       }
       if (gpsGood != gpsPrev) {  // report on change of status
         gpsPrev = gpsGood;
         if (gpsGood) {
           Log.println("GPS good");
           LogScreenPrintln("GPS good");         
         } else {
          Log.println("GPS timeout");
          LogScreenPrintln("GPS timeout");         
         }
       }
       
    } 

//=================================================================================================  
uint32_t epochNow() {
return (epochSync + (millis() - millisSync) / 1E3);
}
//=================================================================================================  
void LostPowerCheckAndRestore(uint32_t epochSyn) {  // this function only ever called by a time enabled protocol
  if ((!timeGood) || (epochSyn == 0)) return;
  
  epochSync = epochSyn;
  millisSync = millis();
 
  if (lostPowerCheckDone) return;

  #if defined Debug_All || defined Debug_Time || defined Debug_Home
    Log.print("Checking for RestoreHomeFromFlash conditions:"); 
    Log.print("  epochHome="); Log.print(TimeString(epochHome())); 
    Log.print("  epochNow="); Log.println(TimeString(epochNow()));
  #endif 

  
  if ((epochNow() -  epochHome()) < 300) {  //  within 5 minutes
    RestoreHomeFromFlash();
    homeInitialised=1;           
  }
  
  lostPowerCheckDone = true;
}
//=================================================================================================  
void SaveHomeToFlash() {

  EEPROMWritelong(0, epochNow());   // epochHome
  EEPROMWritelong(1, hom.lon*1E6);  // float to long
  EEPROMWritelong(2, hom.lat*1E6);
  EEPROMWritelong(3, hom.alt*10);
  EEPROMWritelong(4, hom.hdg*10);  
  
#if defined Debug_All || defined Debug_EEPROM || defined Debug_Time || defined Debug_Home
  Log.print("  homSaved="); Log.print(homSaved);
  Log.print("  home.lon="); Log.print(hom.lon, 6);
  Log.print("  home.lat="); Log.print(hom.lat, 6);
  Log.print("  home.alt="); Log.print(hom.alt, 1);
  Log.print("  home.hdg="); Log.println(hom.hdg, 1);

#endif   
}

//=================================================================================================  
void StoreEpochPeriodic() {
  uint32_t epochPeriodic = epochNow();
  EEPROMWritelong(5, epochPeriodic); // Seconds

  if (homeInitialised) {
    EEPROMWritelong(0, epochPeriodic); // UPDATE epochHome
    #if defined Debug_All || defined Debug_EEPROM || defined Debug_Time || defined Debug_Home
      Log.print("epochHome stored="); Log.println(TimeString(epochPeriodic));
    #endif  
  }
  
  #if defined Debug_All || defined Debug_EEPROM || defined Debug_Time || defined Debug_Home
  Log.print("epochPeriodic stored="); Log.println(TimeString(epochPeriodic));
  #endif  
}

//=================================================================================================  
uint32_t epochHome() {

uint32_t epHome = EEPROMReadlong(0);

 #if defined Debug_All || defined Debug_EEPROM
   Log.print("epochHome="); Log.println(TimeString(epHome));

 #endif
 return epHome;    
}
//=================================================================================================  
void RestoreHomeFromFlash() {

  hom.lon = EEPROMReadlong(1) / 1E6; //long back to float
  hom.lat = EEPROMReadlong(2) / 1E6;
  hom.alt = EEPROMReadlong(3) / 10;
  hom.hdg = EEPROMReadlong(4) / 10;
  
  Log.println("Home data restored from Flash"); 
  LogScreenPrintln("Home data restored");
  LogScreenPrintln("from Flash. Go Fly!");
  
  #if defined Debug_All || defined Debug_EEPROM || defined Debug_Time || defined Debug_Home
    Log.print("  home.lon="); Log.print(hom.lon, 6);
    Log.print("  home.lat="); Log.print(hom.lat, 6);
    Log.print("  home.alt="); Log.print(hom.alt, 0);
    Log.print("  home.hdg="); Log.println(hom.hdg, 0);
  #endif  
}
//=================================================================================================  
  
#if (Telemetry_In == 2) ||  (Telemetry_In == 3)   //  Mav WiFi or FrSky UDP
 
  void SetupWiFi() { 

    bool apMode = false;  // used when STA fails to connect
     //===============================  S T A T I O N   =============================
     
    #if (WiFi_Mode == 2) || (WiFi_Mode == 3)  // STA or SPA>AP
      uint8_t retry = 0;
      Log.print("Trying to connect to ");  
      Log.print(STAssid); 
      LogScreenPrintln("WiFi trying ..");

      WiFi.disconnect(true);   // To circumvent "wifi: Set status to INIT" error bug
      delay(500);
      WiFi.mode(WIFI_STA);
      delay(500);
      
      WiFi.begin(STAssid, STApw);
      while (WiFi.status() != WL_CONNECTED){
        retry++;
        if (retry > 20) {
          Log.println();
          Log.println("Failed to connect in STA mode");
          LogScreenPrintln("Failed in STA Mode");
          wifiSuDone = true;
          
          #if (WiFi_Mode ==  3)       
            apMode = true;            // Rather go establish an AP instead
            Log.println("Starting AP instead.");
            LogScreenPrintln("Starting AP instead");  
            //new from Target0815:
            Log.println("WiFi-Reset ...");
            WiFi.mode(WIFI_MODE_NULL);    
            delay(500);             
          #endif  
          
          break;
        }
        delay(500);
        Serial.print(".");
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        localIP = WiFi.localIP();
        Log.println();
        Log.println("WiFi connected!");
        Log.print("Local IP address: ");
        Log.print(localIP);

        #if (Telemetry_In == 2) && (WiFi_Protocol == 1)   // Mav TCP
            Log.print("  TCP port: ");
            Log.println(TCP_localPort);    //  UDP port is printed lower down
        #else 
            Log.println();
        #endif 
         
        wifi_rssi = WiFi.RSSI();
        Log.print("WiFi RSSI:");
        Log.print(wifi_rssi);
        Log.println(" dBm");

        LogScreenPrintln("Connected! My IP =");
        LogScreenPrintln(localIP.toString());
        
        #if (WiFi_Protocol == 1)        // TCP                                                
          #if (Telemetry_In  == 2)     // We are a client and need to connect to a server
             outbound_clientGood = NewOutboundTCPClient();
          #endif
        #endif
        #if (Telemetry_In == 2)                // Mavlink 
          #if (WiFi_Protocol == 2)         // Mav UDP 
            mav_udp_object.begin(UDP_localPort);
            #if defined STM32F103C
              Log.print("Mav UDP instance started, listening on IP "); Log.print(localIP.toString().c_str());
              Log.print(", UDP port "); Log.println(UDP_localPort);  
            #else
              Log.printf("Mav UDP instance started, listening on IP %s, UDP port %d\n", localIP.toString().c_str(), UDP_localPort);
            #endif   
            LogScreenPrint("UDP port = ");  LogScreenPrintln(String(UDP_localPort));
          #endif
        #endif

        #if (Telemetry_In == 3)               // FrSky
          frs_udp_object.begin(UDP_localPort+1);
          #if defined STM32F103C
            Log.print("Frs UDP instance started, listening on IP "); Log.print(localIP.toString().c_str());
            Log.print(", UDP port "); Log.println(UDP_localPort+1);  
          #else
            Log.printf("Frs UDP instance started, listening on IP %s, UDP port %d\n", localIP.toString().c_str(), UDP_localPort+1);
          #endif           
          LogScreenPrint("UDP port = ");  LogScreenPrintln(String(UDP_localPort+1));       
        #endif
        
        wifiSuDone = true;
        wifiSuGood = true;
      } 
    #endif
     //===============================  Access Point   =============================  

    #if (WiFi_Mode == 1)  // AP
      apMode = true;
    #endif

    if (apMode)   {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(APssid, APpw, APchannel);
      localIP = WiFi.softAPIP();
      Log.print("AP IP address: ");
      Log.print (localIP); 
      Log.print("  SSID: ");
      Log.println(String(APssid));
      LogScreenPrintln("WiFi AP SSID =");
      LogScreenPrintln(String(APssid));

        #if (WiFi_Protocol == 1)   // TCP                                                
          #if (Telemetry_In  == 2)     // We are a client and need to connect to a server
             outbound_clientGood = NewOutboundTCPClient();
          #endif
        #endif
        #if (Telemetry_In == 2)                // Mavlink 
          #if (WiFi_Protocol == 2)         // Mav UDP 
            mav_udp_object.begin(UDP_localPort);
            Log.printf("Mav UDP instance started, listening on IP %s, UDP port %d\n", localIP.toString().c_str(), UDP_localPort);
            LogScreenPrint("UDP port = ");  LogScreenPrintln(String(UDP_localPort));
          #endif
        #endif

        #if (Telemetry_In == 3)               // FrSky
          frs_udp_object.begin(UDP_localPort);
          Log.printf("Frs UDP instance started, listening on IP %s, UDP port %d\n", localIP.toString().c_str(), UDP_localPort);
          LogScreenPrint("UDP port = ");  LogScreenPrintln(String(UDP_localPort));       
        #endif
      
      wifiSuGood = true;
 
    }           

      
    #ifndef Start_WiFi  // if not button override
      delay(2000);      // debounce button press
    #endif  

    wifiSuDone = true;
 }   

  //=================================================================================================  
  #if (WiFi_Protocol == 1)    //  WiFi TCP    
  
  bool NewOutboundTCPClient() {
  static uint8_t retry = 3;

    WiFiClient newClient;        
    while (!newClient.connect(TCP_remoteIP, TCP_remotePort)) {
      Log.print("Local outgoing tcp client connect failed, retrying ");  Log.println(retry);
      retry--;
      if (retry == 0) {
         Log.println("Tcp client connect aborted!");
         return false;
      }
      nbdelay(4000);
    }
    active_client_idx = 0;     // use the first client object for  our single outgoing session   
    clients[0] = new WiFiClient(newClient); 
    Log.print("Local tcp client connected to remote server IP:"); Log.print(TCP_remoteIP);
    Log.print(" remote Port:"); Log.println(TCP_remotePort);
    nbdelay(1000);
    LogScreenPrintln("Remote server IP =");
    snprintf(snprintf_buf, snp_max, "%d.%d.%d.%d", TCP_remoteIP[0], TCP_remoteIP[1], TCP_remoteIP[2], TCP_remoteIP[3] );        
    LogScreenPrintln(snprintf_buf); 

    
 //   LogScreenPrintln(TCP_remoteIP.toString()); 
    return true;
  }
  #endif
  //=================================================================================================  
   #if (Telemetry_In == 2) &&  (WiFi_Protocol == 2) //  WiFi && UDP - Print the remote UDP IP the first time we get it  
   void PrintRemoteIP() {
    if (FtRemIP)  {
      FtRemIP = false;
      Log.print("Client connected: Remote UDP IP: "); Log.print(UDP_remoteIP);
      Log.print("  Remote  UDP port: "); Log.println(UDP_remotePort);
      LogScreenPrintln("Client connected");
      LogScreenPrintln("Remote UDP IP =");
      LogScreenPrintln(UDP_remoteIP.toString());
      LogScreenPrintln("Remote UDP port =");
      LogScreenPrintln(String(UDP_remotePort));
     }
  }
  #endif
  
 #endif  

  //=================================================================================================  

  uint32_t Get_Volt_Average1(uint16_t mV)  {
    if (bat1.avg_mV < 1) bat1.avg_mV = mV;  // Initialise first time
    bat1.avg_mV = (bat1.avg_mV * 0.6666) + (mV * 0.3333);  // moving average
    return bat1.avg_mV;
  }
  //=================================================================================================  
uint32_t Get_Current_Average1(uint16_t cA)  {   // in 100*milliamperes (1 = 100 milliampere)
 
  if (bat1.avg_cA < 1){
    bat1.avg_cA = cA;  // Initialise first time
  }
  bat1.avg_cA = (bat1.avg_cA * 0.6666F) + (cA * 0.333F);  // moving average
  return bat1.avg_cA;
  }
  //=================================================================================================   

  uint32_t Abs(int32_t num) {
    if (num<0) 
      return (num ^ 0xffffffff) + 1;
    else
      return num;  
  }  
