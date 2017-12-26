// o----------------------------------------------------------o
// | "The5K" - An (almost) 5kiB Accu Keyer Clone Iambic Keyer |
// | * A **real** clone of this little jevel with any Arduino |
// |                  V: 1.0 - Build: 20171226 ( 4508 bytes ) |
// |                                          (c)2017 - PP5VX |
// o----------------------------------------------------------o
// | PSE TAKE NOTE !                                          |
// | This work is licensed in World, as: "CC-BY-NC-3.0"       |
// | And is licensed **only** in Brazil, as: (c)2017 - PP5VX  |
// o----------------------------------------------------------o
// | Portions: (c)K1EL - (c)KC4IFB - (c)KC2WAG                |
// o----------------------------------------------------------o
//
// o----------------------------------------------------------o
// | Note - PSE READ IN FULL !                                |
// o----------------------------------------------------------o
// |  Do you remember the Accu Keyer ( WB4VVF ) ?             |
// |  I remember, because used it too much at my CW begins    |
// |  So, I searched for a "real clone", and...   nothing     |
// |  Now, don't look **anymore** for something "similar"     |
// |  ==> This is a **real** Accu Keyer, Mode "B" full clone  |
// |  But, I added Weight, and space code is almost 5kiB      |
// |  ==> It works full on an ATTiny45 or 85, with some mods  |
// |  Enjoy, as I enjoyed it **too much** on the 70's         |
// |                              73/DX/SYOS de PP5VX (Bone)  |
// |                                pp5vx (.--.-.) amsat.org  |
// o----------------------------------------------------------o 
//
// o----------------------------------------------------------o
// |                     Hardware Settings                    |
// o----------------------------------------------------------o
// | Pin | Function                                           |
// o----------------------------------------------------------o
// |  A0 | Speed Pot:  100k ( LIN )                           |
// |  A1 | Tone Pot:   100k ( LIN )                           |
// |  A0 | Weight Pot: 100k ( LIN )                           |
// |   8 | Left  Paddle Input: Dits ------- on "Tip"          |
// |   9 | Right Paddle Input: Dashes ----- on "Ring"         |
// |  11 | TX Keying: 2N7000 or 2N7002 ( classic )            |
// |  12 | Sidetone Output: 2N2222 to a 2" Speaker            |
// |  13 | ===> PSE: NEVER USE PIN 13 TO KEY THE TX !         |
// o----------------------------------------------------------o

// Some of PP5VX's fingers are here...
#define Speed_Pin  A0 // Speed
#define Tone_Pin   A1 // Tone
#define Weight_Pin A2 // Weight
#define LPin        8 // Left Paddle Input
#define RPin        9 // Right Paddle Input
// == TX KEY Output is ONLY on Pin 11
// PSE: Don't use Pin 13, they FLICK on Boot Up
// And this, **effectively** KEYS your TX !
#define KEYpin     11

#define ST_Pin     12 // Sidetone Output

#define DIT_L    0x01 // Dit latch
#define DASH_L   0x02 // Dash latch
#define DIT_PROC 0x04 // Dit is being processed
#define IAMBICB  0x10 // 0: Iambic A, 1: Iambic B

// All of PP5VX's fingers are here...
#define MIN_WPM 10 // Min Speed     ( ...all keyers begins here - try 13 or 15, is my advice ! )
#define DefaWPM 35 // Default Speed ( ...Default Speed for me, of course: But NOT IMPLEMENTED here )
#define MAX_WPM 55 // Max Speed     ( ...top speed that for some is too high - but here is my choice )

// All of PP5VX's fingers are here...
#define MIN_WGT 40 // Min Weight: 40%     ( ...remember that "chompy" CW ? This is it ! )
#define DefaWGT 50 // Default Weight: 50% ( ...normal weight, for normal people )
#define MAX_WGT 75 // Max Weight: 75%     ( ...the "big gun" weight, it's funny with 1 kW )

// All of PP5VX's fingers are here...
#define MIN_HZ  400 // Min Tone     ( ...I don't like low frequencies on CW, but is your choice )
#define DefaHZ  500 // Default Tone ( ...the mostly used tone frequency in whole World ! )
#define MAX_HZ  700 // Max Tone     ( ...no one use above this to do **real** CW on the Bands ! LOL )

unsigned long ditTime;
unsigned char keyerControl, keyerState;
unsigned int  key_speed, key_weight, key_tone;
unsigned int  kte      = 2.3; // o---------------------------------------------------------------------------o
                              // | You bet that Ten PP5VX's fingers (and all brains) are here... (LOL)       |
                              // | Your choice of some Weight "trimming" is on [kte]                         |
                              // | Weight Constant are: 1 = Light - 2 = Heavy (Default) - 3: = Very Heavy    |
                              // | Don't use TOO LIGHT or TOO HEAVY, or no one can undestand your fist !     |
                              // | I suggest a little bit above 2 ( try: 2.2, 2.3 or 2.5 - I use 2.3 )       |
                              // | You can also look the NEDDLE of your RF OUTPUT METER for a "decent value" |
                              // o---------------------------------------------------------------------------o                              
 
enum KSTYPE {IDLE, CHK_DIT, CHK_DASH, KEYED_PREP, KEYED, INTER_ELEMENT };
 
// ======================================================================
// Keep in mind, that is almost know that all Hams that use Iambic Keyers
// can use **only** "Stereo Plugs"  ( like the "P2" or "P10", in Brazil )
// So, except in China or some small city in USA (hi), wire these "Stereo
// Plugs", as follows: DITS on TIP, DASHES on RING and naturally the GND.
// What is this "so complicated' for some Hams ? Don't change standards !
// In the code below, ALL that I look over the Internet are INVERTED !
// But don't worry, below they don't need the "Paddle Swap" ( PDLSWP )
// They are **correctly sourced** ( DITS ans DASHES ), as described above
void update_PaddleLatch()
{ if (digitalRead(LPin) == LOW) keyerControl |= DIT_L;  // DITS   on TIP ( Stereo Plug )
  if (digitalRead(RPin) == LOW) keyerControl |= DASH_L; // DASHES on RING ( Stereo Plug )
}

// ==================================
void setup()
{ pinMode(LPin,        INPUT_PULLUP); // Left = DITS    ( Getting RFI here ? Use a 100nF to GND )
  pinMode(RPin,        INPUT_PULLUP); // Right = DASHES ( Getting RFI here ? Use a 100nF to GND )

  pinMode(ST_Pin,      OUTPUT);    
  digitalWrite(ST_Pin, LOW);

  pinMode(KEYpin,      OUTPUT);
  digitalWrite(KEYpin, LOW);

  keyerState   = IDLE;
  keyerControl = IAMBICB;
  ReadParameters();
}

// ====================================================================
// All of PP5VX's fingers are here...
// ====================================================================
void ReadParameters()
{ key_speed  = map(analogRead(Speed_Pin),  10, 1000, MIN_WPM, MAX_WPM);
  ditTime    = 1200/key_speed;
  key_tone   = map(analogRead(Tone_Pin),   10, 1000, MIN_HZ,  MAX_HZ);
  key_weight = map(analogRead(Weight_Pin), 10, 1000, MIN_WGT, MAX_WGT);
}

// Some of PP5VX's fingers are here...
void loop()
{ static long ktimer;
  // ReadParameters();
  switch (keyerState)
  { case IDLE: if ((digitalRead(LPin) == LOW) || (digitalRead(RPin) == LOW) || (keyerControl & 0x03))
               { update_PaddleLatch(); keyerState = CHK_DIT; }
               ReadParameters();
               break;
 
    case CHK_DIT: if (keyerControl & DIT_L)
                  { keyerControl |= DIT_PROC;
                    // Some of PP5VX's fingers are here...
                    // ktimer = ditTime;    // If you don't use Weight Control
                    ktimer = ditTime*(key_weight/50.0);     // Dit Time+Weight
                    keyerState = KEYED_PREP;
                  } else keyerState = CHK_DASH;
                  break;
 
    case CHK_DASH: if (keyerControl & DASH_L)
                  { // Some of PP5VX's fingers are here...
                    // ktimer = ditTime*3;   // If you don't use Weight Control
                    ktimer = ditTime*(3*(key_weight/50.0)); // Dash Time+Weight                    
                    keyerState = KEYED_PREP; }
                  else keyerState = IDLE;
                  break;
 
    case KEYED_PREP: digitalWrite(KEYpin, HIGH);
                     tone(ST_Pin, key_tone);
                     ktimer += millis();
                     keyerControl &= ~(DIT_L + DASH_L);
                     keyerState = KEYED;
                     break;
 
    case KEYED: if (millis() > ktimer)
                { digitalWrite(KEYpin, LOW);
                  noTone(ST_Pin);
                  // Some of PP5VX's fingers are here...
                  // ktimer = millis() + ditTime;         // If you don't use Weight Control
                  ktimer = millis()+ditTime*(kte-(key_weight/50.0));  // Weight Control here
                  keyerState = INTER_ELEMENT; }
                else if (keyerControl & IAMBICB) update_PaddleLatch();
                break;
 
    case INTER_ELEMENT: update_PaddleLatch();
                        if (millis() > ktimer)
                        { if (keyerControl & DIT_PROC)
                          {  keyerControl &= ~(DIT_L + DIT_PROC);
                             keyerState = CHK_DASH; }
                          else  { keyerControl &= ~(DASH_L);
                                  keyerState = IDLE;  }
                        }
                        break;
  } // end Case
}
