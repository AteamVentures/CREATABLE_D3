#include "temperature.h"
#include "ultralcd.h"
#ifdef ULTRA_LCD
#include "Marlin.h"
#include "language.h"
#include "cardreader.h"
#include "temperature.h"
#include "stepper.h"
#include "ConfigurationStore.h"
#include "lifetime_stats.h"

//#define ENABLE_MOVE_MENU

char step = 0;
char z_setup = 0;
float zoffset[3] = {0.0, 0.0, 0.0};
int manualLevelingDelay = 300;

int8_t encoderDiff; /* encoderDiff is updated from interrupt context and added to encoderPosition every LCD update */

/* Configuration settings */
int plaPreheatHotendTemp;
int plaPreheatHPBTemp;
int plaPreheatFanSpeed;

int absPreheatHotendTemp;
int absPreheatHPBTemp;
int absPreheatFanSpeed;

int preheatHotendTemp;
int preheatHPBTemp;
int preheatFanSpeed;

int filament_load_temp;

int heat_hold_minute;

bool leveling_flag = false;

// bool Enable_Mfg_test;
int firstTimeRunPhase;

int printR;
int printG;
int printB;
int brightValue;

int standByR;
int standByG;
int standByB;

int heatingR;
int heatingG;
int heatingB;

bool music_on;

int ledState;   // 0 : standby / 1 : printing / 2 : heating
bool ledUpdated;

#ifdef ULTIPANEL
static float manual_feedrate[] = MANUAL_FEEDRATE;
#endif // ULTIPANEL

/* !Configuration settings */

//Function pointer to menu functions.
typedef void (*menuFunc_t)();

uint8_t lcd_status_message_level;
char lcd_status_message[LCD_WIDTH+1] = WELCOME_MSG;

bool prevent_lcd_update = false;
bool onetime_flag;


extern unsigned long starttime;


#ifdef DOGLCD
#include "dogm_lcd_implementation.h"
#else
#include "ultralcd_implementation_hitachi_HD44780.h"
#endif

/** forward declerations **/

void copy_and_scalePID_i();
void copy_and_scalePID_d();

static void lcd_utilities_menu();
static void lcd_filament_menu();

/* Different menus */
static void lcd_status_screen();
#ifdef ULTIPANEL
static void lcd_main_menu();
static void lcd_tune_menu();
#ifdef ENABLE_MOVE_MENU
static void lcd_move_menu();
#endif

#ifdef DOGLCD
static void lcd_set_contrast();
#endif
static void lcd_control_retract_menu();
static void lcd_sdcard_menu();
static void lcd_quick_feedback();//Cause an LCD refresh, and give the user visual or audiable feedback that something has happend

/* Different types of actions that can be used in menuitems. */
static void menu_action_back(menuFunc_t data);
static void menu_action_submenu(menuFunc_t data);
static void menu_action_gcode(const char* pgcode);
static void menu_action_function(menuFunc_t data);
static void menu_action_sdfile(const char* filename, char* longFilename);
static void menu_action_sddirectory(const char* filename, char* longFilename);
static void menu_action_setting_edit_bool(const char* pstr, bool* ptr);
static void menu_action_setting_edit_custom_bool(const char* pstr, bool* ptr);
static void menu_action_setting_edit_custom_int3(const char* pstr, int* ptr, int minValue, int maxValue);

// static void menu_action_setting_edit_led_print(const char* pstr, int* ptr, int minValue, int maxValue);
// static void menu_action_setting_edit_led_standBy(const char* pstr, int* ptr, int minValue, int maxValue);
// static void menu_action_setting_edit_led_heating(const char* pstr, int* ptr, int minValue, int maxValue);
// void menu_edit_led_print ();
// void menu_edit_led_standBy ();
// void menu_edit_led_heating ();

static void menu_action_setting_edit_int3(const char* pstr, int* ptr, int minValue, int maxValue);
static void menu_action_setting_edit_float3(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_float32(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_float5(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_float51(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_float52(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_long5(const char* pstr, unsigned long* ptr, unsigned long minValue, unsigned long maxValue);
static void menu_action_setting_edit_callback_bool(const char* pstr, bool* ptr, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_int3(const char* pstr, int* ptr, int minValue, int maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float3(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float32(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float5(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float51(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float52(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_long5(const char* pstr, unsigned long* ptr, unsigned long minValue, unsigned long maxValue, menuFunc_t callbackFunc);


static void auto_leveling();

static void lcd_cooldown();

static void clean_nozzle();
static void manual_leveling();
static void leveling_procedure_0();
static void leveling_procedure_1();
static void leveling_procedure_1_5();
static void leveling_procedure_2();
static void leveling_procedure_2_5();
static void leveling_procedure_3();
static void leveling_procedure_3_5();
static void leveling_procedure_4();
static void leveling_procedure_4_5();
static void leveling_procedure_5();
static void leveling_procedure_5_5();
static void leveling_procedure_6();
static void leveling_procedure_7();
static void leveling_save();
static void edit_level_offset();
static void wait_offset();
static void move_offset();
static void lcd_move_z();



static void lcd_info_settings();

static void filament_setting_save();
static void filament_setting_plasave();
static void filament_setting_abssave();
static void filament_setting_pla();
static void filament_setting_abs();
static void filament_setting_custom();
static void filament_setting();


static void filament_load();
static void filament_load_new();
static void filament_load_wait();


static void lcd_firstTime_leveling();
static void lcd_firstTime_filament_load();
static void lcd_firstTime_greeting();



static void lcd_led_printing();
static void lcd_led_standBy();
static void lcd_led_heating();
static void lcd_led_setting();
static void lcd_preheat();


static void lcd_led_printR();
static void lcd_led_printG();
static void lcd_led_printB();

static void lcd_led_standByR();
static void lcd_led_standByG();
static void lcd_led_standByB();

static void lcd_led_heatingR();
static void lcd_led_heatingG();
static void lcd_led_heatingB();

static void lcd_led_brightValue();



#if defined(BEEPER) && BEEPER > -1
static void customTune(float freq, float duration);
#endif

int thermoErr;


float temp_level;

#define ENCODER_FEEDRATE_DEADZONE 10

#if !defined(LCD_I2C_VIKI)
  #ifndef ENCODER_STEPS_PER_MENU_ITEM
    #define ENCODER_STEPS_PER_MENU_ITEM 5
  #endif
  #ifndef ENCODER_PULSES_PER_STEP
    #define ENCODER_PULSES_PER_STEP 1
  #endif
#else
  #ifndef ENCODER_STEPS_PER_MENU_ITEM
    #define ENCODER_STEPS_PER_MENU_ITEM 2 // VIKI LCD rotary encoder uses a different number of steps per rotation
  #endif
  #ifndef ENCODER_PULSES_PER_STEP
    #define ENCODER_PULSES_PER_STEP 1
  #endif
#endif


/* Helper macros for menus */
#define START_MENU() do { \
    if (encoderPosition > 0x8000) encoderPosition = 0; \
    if (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM < currentMenuViewOffset) currentMenuViewOffset = encoderPosition / ENCODER_STEPS_PER_MENU_ITEM;\
    uint8_t _lineNr = currentMenuViewOffset, _menuItemNr; \
    bool wasClicked = LCD_CLICKED;\
    for(uint8_t _drawLineNr = 0; _drawLineNr < LCD_HEIGHT; _drawLineNr++, _lineNr++) { \
        _menuItemNr = 0;
#define MENU_ITEM(type, label, args...) do { \
    if (_menuItemNr == _lineNr) { \
        if (lcdDrawUpdate) { \
            const char* _label_pstr = PSTR(label); \
            if ((encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) { \
                lcd_implementation_drawmenu_ ## type ## _selected (_drawLineNr, _label_pstr , ## args ); \
            }else{\
                lcd_implementation_drawmenu_ ## type (_drawLineNr, _label_pstr , ## args ); \
            }\
        }\
        if (wasClicked && (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) {\
            lcd_quick_feedback(); \
            menu_action_ ## type ( args ); \
            return;\
        }\
    }\
    _menuItemNr++;\
} while(0)


#define MENU_ITEM_CUSTOM(type, label, line, args...) do { \
    if (_menuItemNr == _lineNr) { \
        if (lcdDrawUpdate) { \
            const char* _label_pstr = PSTR(label); \
            if ((encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) { \
                lcd_implementation_drawmenu_ ## type ## _selected (line, _label_pstr , ## args ); \
            }else{\
                lcd_implementation_drawmenu_ ## type (line, _label_pstr , ## args ); \
            }\
        }\
        if (wasClicked && (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) {\
            lcd_quick_feedback(); \
            menu_action_ ## type ( args ); \
            return;\
        }\
    }\
    _menuItemNr++;\
} while(0)


#define MENU_ITEM_E_CUSTOM(type, label, args...) do { \
    if (_menuItemNr == _lineNr) { \
        if (lcdDrawUpdate) { \
            const char* _label_pstr = PSTR(label); \
            if ((encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) { \
                lcd_implementation_drawmenu_ ## type ## _selected (_drawLineNr, _label_pstr , ## args ); \
            }else{\
                lcd_implementation_drawmenu_ ## type (_drawLineNr, _label_pstr , ## args ); \
            }\
        }\
        if (wasClicked && (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) {\
            lcd_quick_feedback(); \
            menu_action_ ## type ( args ); \
            return;\
        }\
    }\
    _menuItemNr++;\
} while(0)

// #define MENU_ITEM_E_LED(type, label, args...) do { \
//     if (_menuItemNr == _lineNr) { \
//         if (lcdDrawUpdate) { \
//             const char* _label_pstr = PSTR(label); \
//             if ((encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) { \
//                 lcd_implementation_drawmenu_ ## type ## _selected (_drawLineNr, _label_pstr , ## args ); \
//             }else{\
//                 lcd_implementation_drawmenu_ ## type (_drawLineNr, _label_pstr , ## args ); \
//             }\
//         }\
//         if (wasClicked && (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) {\
//             lcd_quick_feedback(); \
//             menu_action_ ## type ( args ); \
//             return;\
//         }\
//     }\
//     _menuItemNr++;\
// } while(0)

#define MENU_ITEM_DUMMY() do { _menuItemNr++; } while(0)
#define MENU_ITEM_EDIT(type, label, args...) MENU_ITEM(setting_edit_ ## type, label, PSTR(label) , ## args )
#define MENU_ITEM_EDIT_CUSTOM(type, label, args...) MENU_ITEM_E_CUSTOM(setting_edit_custom_ ## type, label, PSTR(label) , ## args )
// #define MENU_ITEM_EDIT_LED(type, label, args...) MENU_ITEM_E_LED(setting_edit_led_ ## type, label, PSTR(label) , ## args )
#define MENU_ITEM_EDIT_CALLBACK(type, label, args...) MENU_ITEM(setting_edit_callback_ ## type, label, PSTR(label) , ## args )
#define END_MENU() \
    if (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM >= _menuItemNr) encoderPosition = _menuItemNr * ENCODER_STEPS_PER_MENU_ITEM - 1; \
    if ((uint8_t)(encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) >= currentMenuViewOffset + LCD_HEIGHT) { currentMenuViewOffset = (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) - LCD_HEIGHT + 1; lcdDrawUpdate = 1; _lineNr = currentMenuViewOffset - 1; _drawLineNr = -1; } \
    } } while(0)

/** Used variables to keep track of the menu */
#ifndef REPRAPWORLD_KEYPAD
volatile uint8_t buttons;//Contains the bits of the currently pressed buttons.
#else
volatile uint8_t buttons_reprapworld_keypad; // to store the reprapworld_keypad shiftregister values
#endif
#ifdef LCD_HAS_SLOW_BUTTONS
volatile uint8_t slow_buttons;//Contains the bits of the currently pressed buttons.
#endif
uint8_t currentMenuViewOffset;              /* scroll offset in the current menu */
uint32_t blocking_enc;
uint8_t lastEncoderBits;
uint32_t encoderPosition;
#if (SDCARDDETECT > 0)
bool lcd_oldcardstatus;
#endif
#endif//ULTIPANEL

menuFunc_t currentMenu = lcd_status_screen; /* function pointer to the currently active menu */
uint32_t lcd_next_update_millis;
uint8_t lcd_status_update_delay;
uint8_t lcdDrawUpdate = 2; /* Set to none-zero when the LCD needs to draw, decreased after every draw. Set to 2 in LCD routines so the LCD gets atleast 1 full redraw (first redraw is partial) */

//prevMenu and prevEncoderPosition are used to store the previous menu location when editing settings.
menuFunc_t prevMenu = NULL;
uint16_t prevEncoderPosition;
//Variables used when editing values.
const char* editLabel;
void* editValue;
int32_t minEditValue, maxEditValue;
menuFunc_t callbackFunc;

// placeholders for Ki and Kd edits
float raw_Ki, raw_Kd;

/* Main status screen. It's up to the implementation specific part to show what is needed. As this is very display dependend */




static void lcd_firstTime_leveling(){
    lcd_implementation_draw_string(0, "First step is");
    lcd_implementation_draw_string(1, "leveling heat bed.");
    lcd_implementation_draw_string(2, "Please follow steps.");
    
    lcd_implementation_draw_string(3, "PUSH BUTTON TO NEXT ");
    if (LCD_CLICKED)
    {
        currentMenu = leveling_procedure_0;
        encoderPosition = 0;
        lcd_quick_feedback();
        firstTimeRunPhase++;
    }
}

static void lcd_firstTime_filament_load(){
    lcd_implementation_draw_string(0, "Next step is");
    lcd_implementation_draw_string(1, "loading filament.");
    lcd_implementation_draw_string(2, "Please follow steps.");

    lcd_implementation_draw_string(3, "PUSH BUTTON TO NEXT ");
    if (LCD_CLICKED)
    {
        currentMenu = filament_load;
        // currentMenu = filament_load_new;
        encoderPosition = 0;
        lcd_quick_feedback();
        firstTimeRunPhase++;
    }
}
static void lcd_firstTime_greeting(){
    lcd_implementation_draw_string(0, "It is finished!     ");
    lcd_implementation_draw_string(1, "Enjoy your printing!");
    lcd_implementation_draw_string(2, "Thank you!");

    lcd_implementation_draw_string(3, "PUSH BUTTON TO NEXT ");
    if (LCD_CLICKED)
    {
        currentMenu = lcd_status_screen;
        encoderPosition = 0;
        lcd_quick_feedback();
        firstTimeRunPhase++;
            prevent_lcd_update = false;
            Config_StoreSettings();

    }
}

static void lcd_status_screen()
{   
    ledUpdated = false;
    if(Enable_Mfg_Test){
        firstTimeRunPhase = 4;
    }   
    switch(firstTimeRunPhase){
        case 0:
            prevent_lcd_update = true;
            // lcd_update();
            lcd_implementation_draw_string(0, "Welcome. This is the");
            lcd_implementation_draw_string(1, "firsttime use guide.");
            lcd_implementation_draw_string(2, "Please follow steps.");
            
            lcd_implementation_draw_string(3, "PUSH BUTTON TO NEXT ");
            if (LCD_CLICKED)
            {
                // currentMenu = lcd_status_screen;
                encoderPosition = 0;
                lcd_quick_feedback();
                firstTimeRunPhase++;
            }
        break;

        case 1:
            currentMenu = lcd_firstTime_leveling; 
        break;
        case 2:
            currentMenu = lcd_firstTime_filament_load;
        break;
        case 3:
            currentMenu = lcd_firstTime_greeting;
        break;
        case 4:
            if(degHotend(active_extruder) < HEATER_0_MINTEMP || degBed() < BED_MINTEMP){
                if(thermoErr != -1)
                    thermoErr = 1;

            }else{
                thermoErr = 0;
            }


            if (lcd_status_update_delay)
                lcd_status_update_delay--;
            else
                lcdDrawUpdate = 1;
            if (lcdDrawUpdate)
            {
                lcd_implementation_status_screen();
                lcd_status_update_delay = 10;   /* redraw the main screen every second. This is easier then trying keep track of all things that change on the screen */
            }
            #ifndef IGNORE_THERMISTOR_ERR
            if(!thermoErr){
            #endif
                
            #ifdef ULTIPANEL
                if (LCD_CLICKED)
                {
                    currentMenu = lcd_main_menu;
                    encoderPosition = 0;
                    lcd_quick_feedback();
                }

            #if 0
                // Dead zone at 100% feedrate
                if ((feedmultiply < 100 && (feedmultiply + int(encoderPosition)) > 100) ||
                        (feedmultiply > 100 && (feedmultiply + int(encoderPosition)) < 100))
                {
                    encoderPosition = 0;
                    feedmultiply = 100;
                }

                if (feedmultiply == 100 && int(encoderPosition) > ENCODER_FEEDRATE_DEADZONE)
                {
                    feedmultiply += int(encoderPosition) - ENCODER_FEEDRATE_DEADZONE;
                    encoderPosition = 0;
                }
                else if (feedmultiply == 100 && int(encoderPosition) < -ENCODER_FEEDRATE_DEADZONE)
                {
                    feedmultiply += int(encoderPosition) + ENCODER_FEEDRATE_DEADZONE;
                    encoderPosition = 0;
                }
                else if (feedmultiply != 100)
                {
                    feedmultiply += int(encoderPosition);
                    encoderPosition = 0;
                }

                if (feedmultiply < 10)
                    feedmultiply = 10;
                if (feedmultiply > 999)
                    feedmultiply = 999;
            #endif    
            #endif//ULTIPANEL
            #ifndef IGNORE_THERMISTOR_ERR
            }else{
                if(thermoErr != -1){
                    lcd_cooldown();
                    thermoErr = -1;
                }
            }
            #endif
        break;

        default:
        break;  
    }

    
}


#ifdef ULTIPANEL
static void lcd_return_to_status()
{
    encoderPosition = 0;
    currentMenu = lcd_status_screen;
}

static void lcd_sdcard_pause()
{
    card.pauseSDPrint();
}
static void lcd_sdcard_resume()
{
    card.startFileprint();
}

static void lcd_cooldown()
{
    Cancle_heating = true;
    setTargetHotend0(0);
    setTargetHotend1(0);
    setTargetHotend2(0);
    setTargetBed(0);

    enquecommand_P(PSTR("M106 S255"));

    lcd_return_to_status();
}

static void lcd_sdcard_stop()
{
    card.sdprinting = false;
    card.closefile();

    Cancle_heating = true;
    autotempShutdown();
    lcd_cooldown();

    lifetime_stats_print_end();
    clear_command_queue();
    quickStop();
    
    if(SD_FINISHED_STEPPERRELEASE)
    {
        enquecommand_P(PSTR(SD_FINISHED_RELEASECOMMAND));
    }

    starttime = 0;
    lcd_cooldown();
    enquecommand_P(PSTR("G28"));
}

/* Menu implementation */
static void lcd_main_menu()
{
    START_MENU();

    MENU_ITEM(back, "BACK", lcd_status_screen);
#ifdef SDSUPPORT
    if (card.cardOK)
    {
        if (card.isFileOpen())
        {
            if (card.sdprinting)
                MENU_ITEM(function, "PAUSE PRINT", lcd_sdcard_pause);
            else
                MENU_ITEM(function, "RESUME PRINT", lcd_sdcard_resume);
            MENU_ITEM(function, "CANCEL PRINT", lcd_sdcard_stop);
            MENU_ITEM(submenu, "ADVANCED", lcd_tune_menu);

        }else{
            MENU_ITEM(submenu, "PRINT FROM SD", lcd_sdcard_menu);
#if SDCARDDETECT < 1
            MENU_ITEM(gcode, MSG_CNG_SDCARD, PSTR("M21"));  // SD-card changed by user
#endif
        }
    }else{
        MENU_ITEM(submenu, "NO SD CARD", lcd_sdcard_menu);
#if SDCARDDETECT < 1
        MENU_ITEM(gcode, MSG_INIT_SDCARD, PSTR("M21")); // Manually initialize the SD-card via user interface
#endif
    }
#endif
    if (!(movesplanned() || IS_SD_PRINTING)){
        if(!((target_temperature_bed == 0) && (target_temperature[0] == 0) &&  (target_temperature[1] == 0 ))){//&& target_temperature[2] == 0)){
            MENU_ITEM(function, "COOLDOWN", lcd_cooldown);
        }
    }
    if(!IS_SD_PRINTING){
        MENU_ITEM(submenu, "FILAMENT", lcd_filament_menu);
        MENU_ITEM(submenu, "PREHEAT",lcd_preheat);
        MENU_ITEM(submenu,"UTILITIES",lcd_utilities_menu);
        MENU_ITEM(submenu,"INFO/SETTINGS",lcd_info_settings);

    }

   
    // MENU_ITEM(submenu, "Maintenance", lcd_utilities_menu);
    
    END_MENU();
}

#ifdef SDSUPPORT
static void lcd_autostart_sd()
{
    card.lastnr=0;
    card.setroot();
    card.checkautostart(true);
}
#endif


// static void lcd_preheat_pla()
// {
//     setTargetHotend0(plaPreheatHotendTemp);
//     setTargetHotend1(plaPreheatHotendTemp);
// //    setTargetHotend2(plaPreheatHotendTemp);
//     setTargetBed(plaPreheatHPBTemp);
//     fanSpeed = plaPreheatFanSpeed;
//     setWatch(); // heater sanity check timer
//     lcd_return_to_status();
// }

// static void lcd_preheat_abs()
// {
//     setTargetHotend0(absPreheatHotendTemp);
//     setTargetHotend1(absPreheatHotendTemp);
//     // setTargetHotend2(absPreheatHotendTemp);
//     setTargetBed(absPreheatHPBTemp);
//     fanSpeed = absPreheatFanSpeed;
//     setWatch(); // heater sanity check timer
//     lcd_return_to_status();
// }


/* Filament menu implementation */
// static void lcd_filament_pla()
// {
//     char buffer[20];

//     sprintf(buffer,"  Nozzle temp : %d", plaPreheatHotendTemp);
//     lcd_implementation_draw_string(0, buffer);

//     sprintf(buffer,"  Bed temp : %d", plaPreheatHPBTemp);
//     lcd_implementation_draw_string(1, buffer);

//     START_MENU();

//     MENU_ITEM_CUSTOM(function, MSG_MENU_OK, 2, lcd_preheat_pla);
//     MENU_ITEM_CUSTOM(back, MSG_CANCEL, 3, lcd_filament_menu);
    
//     END_MENU();

// }

// static void lcd_filament_abs()
// {
   
//     char buffer[20];

//     sprintf(buffer,"  Nozzle temp : %d", absPreheatHotendTemp);
//     lcd_implementation_draw_string(0, buffer);

//     sprintf(buffer,"  Bed temp : %d", absPreheatHPBTemp);
//     lcd_implementation_draw_string(1, buffer);

//     START_MENU();

//     MENU_ITEM_CUSTOM(function, MSG_MENU_OK, 2, lcd_preheat_abs);
//     MENU_ITEM_CUSTOM(back, MSG_CANCEL, 3, lcd_filament_menu);
    
//     END_MENU();

// }


#ifdef BABYSTEPPING
static void lcd_babystep_x()
{
    if (encoderPosition != 0)
    {
        babystepsTodo[X_AXIS]+=(int)encoderPosition;
        encoderPosition=0;
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit(PSTR(MSG_BABYSTEPPING_X),"");
    }
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_tune_menu;
        encoderPosition = 0;
    }
}

static void lcd_babystep_y()
{
    if (encoderPosition != 0)
    {
        babystepsTodo[Y_AXIS]+=(int)encoderPosition;
        encoderPosition=0;
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit(PSTR(MSG_BABYSTEPPING_Y),"");
    }
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_tune_menu;
        encoderPosition = 0;
    }
}

static void lcd_babystep_z()
{
    if (encoderPosition != 0)
    {
        babystepsTodo[Z_AXIS]+=BABYSTEP_Z_MULTIPLICATOR*(int)encoderPosition;
        encoderPosition=0;
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit(PSTR(MSG_BABYSTEPPING_Z),"");
    }
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_tune_menu;
        encoderPosition = 0;
    }
}
#endif //BABYSTEPPING

static void lcd_tune_menu()
{
    START_MENU();
    MENU_ITEM(back, MSG_BACK, lcd_main_menu);
//    MENU_ITEM_EDIT(int3, MSG_SPEED, &feedmultiply, 10, 999);
#if EXTRUDERS > 1
        MENU_ITEM_EDIT(int3, "Right Nozzle", &target_temperature[0], 0, HEATER_0_MAXTEMP - 15);
    #if TEMP_SENSOR_1 != 0
        MENU_ITEM_EDIT(int3, "Left Nozzle", &target_temperature[1], 0, HEATER_1_MAXTEMP - 15);
    #endif

#else
        MENU_ITEM_EDIT(int3, MSG_NOZZLE, &target_temperature[0], 0, HEATER_0_MAXTEMP - 15);
    #if TEMP_SENSOR_1 != 0
        MENU_ITEM_EDIT(int3, MSG_NOZZLE1, &target_temperature[1], 0, HEATER_1_MAXTEMP - 15);
    #endif
#endif    

#if TEMP_SENSOR_2 != 0
    MENU_ITEM_EDIT(int3, MSG_NOZZLE2, &target_temperature[2], 0, HEATER_2_MAXTEMP - 15);
#endif
#if TEMP_SENSOR_BED != 0
    MENU_ITEM_EDIT(int3, MSG_BED, &target_temperature_bed, 0, BED_MAXTEMP - 15);
#endif
//    MENU_ITEM_EDIT(int3, MSG_FAN_SPEED, &fanSpeed, 0, 255);
    // MENU_ITEM_EDIT(int3, MSG_FLOW, &extrudemultiply, 100, 999);

#ifdef BABYSTEPPING
    #ifdef BABYSTEP_XY
      MENU_ITEM(submenu, MSG_BABYSTEP_X, lcd_babystep_x);
      MENU_ITEM(submenu, MSG_BABYSTEP_Y, lcd_babystep_y);
    #endif //BABYSTEP_XY
    MENU_ITEM(submenu, MSG_BABYSTEP_Z, lcd_babystep_z);
#endif
#ifdef FILAMENTCHANGEENABLE
     MENU_ITEM(gcode, MSG_FILAMENTCHANGE, PSTR("M600"));
#endif

    MENU_ITEM(submenu,"LED BRIGHTNESS", lcd_led_brightValue);

    END_MENU();
}


static void filament_extrude()
{
    float move_menu_scale=0.1;
    char buffer[20];
    if(!onetime_flag){
        enquecommand_P(PSTR("G28"));
        onetime_flag = true;
    }

    // setTargetHotend(filament_load_temp + 15, active_extruder); // +15 degree
    setTargetHotend(preheatHotendTemp + 15, active_extruder); // +15 degree

    prevent_lcd_update = true;


    // if (degHotend(active_extruder) < FILAMENT_LOAD_TEMP) lcd_implementation_draw_string(0, "Wait for heating up");
    // else lcd_implementation_draw_string(0, "Ready to extrude    ");

     if (degHotend(active_extruder) < preheatHotendTemp){//filament_load_temp) {
        lcd_implementation_draw_string(0, "WAIT FOR HEATING UP ");

        sprintf(buffer,"CURRENT TEMP:%d", (int)degHotend(active_extruder));
         
        lcd_implementation_draw_string(1, buffer);
        lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
        sprintf(buffer,"TARGET TEMP:%d", preheatHotendTemp);
        lcd_implementation_draw_string(2, buffer);
        lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
        lcd_implementation_draw_string(3, "   CLICK TO EXIT    ");

        

        encoderPosition = 0;

     }else { 
        lcd_implementation_draw_string(0, "   FOR EXTRUDING    ");
        lcd_implementation_draw_string(1, "  ROTATE CLOCKWISE  ");
        lcd_implementation_draw_string(2, "                    ");
        lcd_implementation_draw_string(3, "   CLICK TO EXIT    ");
    }

     // if (degHotend(active_extruder) >= FILAMENT_LOAD_TEMP) lcd_implementation_draw_string(1, "Wait for heating up");

    if ((encoderPosition != 0) && (degHotend(active_extruder) >= preheatHotendTemp))//filament_load_temp))
    {
        if (!blocks_queued()) {    
            refresh_cmd_timeout();
            current_position[E_AXIS] += float((int)encoderPosition) * move_menu_scale;
            encoderPosition = 0;
            #ifdef DELTA
            calculate_delta(current_position);
            #ifdef NONLINEAR_BED_LEVELING
            adjust_delta(current_position);
            #endif
            plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], manual_feedrate[Z_AXIS]/60, active_extruder);
            #else
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], manual_feedrate[Z_AXIS]/60, active_extruder);
            #endif
            lcdDrawUpdate = 1;
        }
    }

    // if (degHotend(active_extruder) >= preheatHotendTemp){//filament_load_temp) {
    //     lcd_implementation_draw_string(2, "Rotate left: Extrude");
    //     lcd_implementation_draw_string(3, "    Click to exit   ");
    // }

      manage_heater();
      manage_inactivity();
      fanSpeed = 255;


    // for debug    
    // if (lcdDrawUpdate)
    // {
    //     lcd_implementation_drawedit_custom(3,PSTR("E"), ftostr31(current_position[E_AXIS]));
    // }
    
    if (LCD_CLICKED)
    {
        quickStop();
        lcd_quick_feedback();
        
        currentMenu = lcd_return_to_status;
        encoderPosition = 0;
        current_position[E_AXIS] = 0;
        prevent_lcd_update = false;

        Cancle_heating = true;
        setTargetHotend0(0);
    }
}

// static void select_pla_temp()
// {
//     enquecommand_P(PSTR("G28"));

//     filament_load_temp = PLA_PREHEAT_HOTEND_TEMP;
//     currentMenu = filament_extrude;

// }

// static void select_abs_temp()
// {
//     enquecommand_P(PSTR("G28"));

//     filament_load_temp = ABS_PREHEAT_HOTEND_TEMP;
//     currentMenu = filament_extrude;
// }


// static void filament_select()
// {
    
//     lcd_implementation_draw_string(0, " Select the filament");
//     lcd_implementation_draw_string(1, "     to extrude     ");

//     START_MENU();

//     MENU_ITEM_CUSTOM(function, "PLA", 2, select_pla_temp);
//     MENU_ITEM_CUSTOM(function, "ABS", 3, select_abs_temp);

//     END_MENU();
// }


#if 0
// Filament handling function
static void forward_feeding()
{
    float old_max_feedrate_e = max_feedrate[E_AXIS];
    float old_retract_acceleration = retract_acceleration;
    max_feedrate[E_AXIS] = FILAMENT_LOAD_SPEED;
    retract_acceleration = FILAMENT_MOVE_ACCELERATION;
    
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);

    calculate_delta(current_position);
    plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], 1.0, FILAMENT_LOAD_SPEED, active_extruder);
    for(uint8_t n=0;n<6;n++)
    {
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], (n+1)*(FILAMENT_LOAD_LEN)/6, FILAMENT_LOAD_SPEED, active_extruder);
    }

    
    // Additional 100mm
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);

    for(uint8_t n=0;n<5;n++)
    {
        current_position[E_AXIS] += (FILAMENT_LOAD_LEN2/5); // fixed len
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], FILAMENT_LOAD_SPEED/10, active_extruder);
    }

    max_feedrate[E_AXIS] = old_max_feedrate_e;
    retract_acceleration = old_retract_acceleration;
}
#endif

// Filament handling function
static void forward_feeding()
{
    float old_max_feedrate_e = max_feedrate[E_AXIS];
    float old_retract_acceleration = retract_acceleration;
    int divisionNumber = 1;
    max_feedrate[E_AXIS] = FILAMENT_LOAD_SPEED;
    retract_acceleration = FILAMENT_MOVE_ACCELERATION;
    
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);

    calculate_delta(current_position);
    plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], 1.0, FILAMENT_LOAD_SPEED, active_extruder);
    for(uint8_t n=0;n<divisionNumber;n++)
    {
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], (n+1)*(FILAMENT_LOAD_LEN)/divisionNumber, FILAMENT_LOAD_SPEED, active_extruder);
    }


    // while(current_position[E_AXIS] < FILAMENT_LOAD_LEN - 50){
    //     if(LCD_CLICKED){
    //         lcd_cooldown();
    //         lcd_quick_feedback();
    //         currentMenu = lcd_status_screen;
    //         encoderPosition = 0;
    //         prevent_lcd_update = false;
    //         return;
    //     }

    // }
    /*
    // Additional 100mm
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);

    for(uint8_t n=0;n<5;n++)
    {
        current_position[E_AXIS] += (FILAMENT_LOAD_LEN2/5); // fixed len
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], FILAMENT_LOAD_SPEED/10, active_extruder);
    }

    max_feedrate[E_AXIS] = old_max_feedrate_e;
    retract_acceleration = old_retract_acceleration;
    */
}

static void reverse_feeding()
{

    float old_max_feedrate_e = max_feedrate[E_AXIS];
    float old_retract_acceleration = retract_acceleration;
    int divisionNumber = 1;

    max_feedrate[E_AXIS] = FILAMENT_UNLOAD_SPEED;
    retract_acceleration = FILAMENT_MOVE_ACCELERATION;

    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);

    calculate_delta(current_position);
    plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], -1.0, FILAMENT_UNLOAD_SPEED, active_extruder);
    for(uint8_t n=0;n<divisionNumber;n++) {
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], (n+1)*-FILAMENT_UNLOAD_LEN/divisionNumber, FILAMENT_UNLOAD_SPEED, active_extruder);
    }

    max_feedrate[E_AXIS] = old_max_feedrate_e;
    retract_acceleration = old_retract_acceleration;
}


static void filament_load_new(){
    char buffer[20];
    setTargetHotend(FILAMENT_LOAD_TEMP, active_extruder);
    lcd_implementation_draw_string(0, "WAIT FOR HEATING UP ");

    sprintf(buffer,"CURRENT TEMP:%d", (int)degHotend(active_extruder));
    lcd_implementation_draw_string(1, "                    ");
    lcd_implementation_draw_string(1, buffer);
    lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
    sprintf(buffer,"TARGET TEMP:%d", preheatHotendTemp);
    lcd_implementation_draw_string(2, "                    ");
    lcd_implementation_draw_string(2, buffer);
    lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
    lcd_implementation_draw_string(3, "   CLICK TO EXIT    ");

    if (LCD_CLICKED)
    {
        lcd_cooldown();
        lcd_quick_feedback();
        currentMenu = lcd_status_screen;
        encoderPosition = 0;
        prevent_lcd_update = false;
        return;
    }    
    prevent_lcd_update = true;
    onetime_flag = true;

    currentMenu = filament_load_wait;
}

static void filament_load_wait(){
    char buffer[20];
    heatCount = millis();
    // setTargetHotend(FILAMENT_LOAD_TEMP, active_extruder);
    lcd_implementation_draw_string(0, "WAIT FOR HEATING UP ");

    sprintf(buffer,"CURRENT TEMP:%d", (int)degHotend(active_extruder));
    lcd_implementation_draw_string(1, "                    ");
    lcd_implementation_draw_string(1, buffer);
    lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
    sprintf(buffer,"TARGET TEMP:%d", preheatHotendTemp);
    lcd_implementation_draw_string(2, "                    ");

    lcd_implementation_draw_string(2, buffer);
    lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
    lcd_implementation_draw_string(3, "   CLICK TO EXIT    ");

    if (degHotend(active_extruder) >= FILAMENT_LOAD_TEMP-10) {
        if (onetime_flag) {
            enquecommand_P(PSTR("G28"));

            forward_feeding();

            onetime_flag = false;

            prevent_lcd_update = true;

            current_position[E_AXIS] = 0;
            plan_set_e_position(current_position[E_AXIS]);
        }

        currentMenu = filament_extrude;
    }

    if (LCD_CLICKED)
    {
        lcd_cooldown();
        lcd_quick_feedback();
        currentMenu = lcd_status_screen;
        encoderPosition = 0;
        prevent_lcd_update = false;
        return;
    }    


}



static void filament_load()
{
    char buffer[20];
    float target_temp;

    if (degHotend(active_extruder) >= preheatHotendTemp) target_temp = degHotend(active_extruder) + 10;//preheatHotendTemp + 10;// degHotend(active_extruder) + 10;
    else target_temp = preheatHotendTemp;

    setTargetHotend(target_temp, active_extruder);

    // while(degHotend(active_extruder) < FILAMENT_LOAD_TEMP){
    while(degHotend(active_extruder) < preheatHotendTemp){

        lcd_implementation_draw_string(0, "WAIT FOR HEATING UP ");

        sprintf(buffer,"CURRENT TEMP:%d", (int)degHotend(active_extruder));
         
        lcd_implementation_draw_string(1, buffer);
        lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
        sprintf(buffer,"TARGET TEMP:%d", preheatHotendTemp);
        lcd_implementation_draw_string(2, buffer);
        lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
        lcd_implementation_draw_string(3, "   CLICK TO EXIT    ");

        
        
        manage_heater();
        manage_inactivity();    

        onetime_flag = true;

        // Cancel
        if (LCD_CLICKED)
        {
            lcd_cooldown();
            lcd_quick_feedback();
            currentMenu = lcd_status_screen;
            encoderPosition = 0;
            prevent_lcd_update = false;
            return;
        }
    }


    if (degHotend(active_extruder) >= preheatHotendTemp) {
        if (onetime_flag) {
            // enquecommand_P(PSTR("G28"));

            forward_feeding();

            onetime_flag = false;

            prevent_lcd_update = true;

            current_position[E_AXIS] = 0;
            plan_set_e_position(current_position[E_AXIS]);
        }

        currentMenu = filament_extrude;
    }
}


static void filament_unload()
{
    char buffer[20];

    setTargetHotend(preheatHotendTemp, active_extruder);
    while(degHotend(active_extruder) < preheatHotendTemp){
        lcd_implementation_draw_string(0, "WAIT FOR HEATING UP ");

        sprintf(buffer,"CURRENT TEMP:%d", (int)degHotend(active_extruder));
         
        lcd_implementation_draw_string(1, buffer);
        lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
        sprintf(buffer,"TARGET TEMP:%d", preheatHotendTemp);
        lcd_implementation_draw_string(2, buffer);
        lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
        lcd_implementation_draw_string(3, "   CLICK TO EXIT    ");

        
        manage_heater();
        manage_inactivity();

        lcdDrawUpdate = 0;

        // Cancel
        if (LCD_CLICKED)
        {
            lcd_cooldown();
            lcd_quick_feedback();
            currentMenu = lcd_status_screen;
            encoderPosition = 0;
            return;
        }
    }

    // Clear screen
    lcd_implementation_clear();
    lcd_implementation_draw_string(0, "  Reverse material");

    reverse_feeding();

    lcd_cooldown();
    lcd_quick_feedback();
    currentMenu = lcd_status_screen;
}



#ifdef ENABLE_MOVE_MENU
float move_menu_scale;
static void lcd_move_menu_axis();

static void lcd_move_x()
{
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        current_position[X_AXIS] += float((int)encoderPosition) * move_menu_scale;
        if (min_software_endstops && current_position[X_AXIS] < X_MIN_POS)
            current_position[X_AXIS] = X_MIN_POS;
        if (max_software_endstops && current_position[X_AXIS] > X_MAX_POS)
            current_position[X_AXIS] = X_MAX_POS;
        encoderPosition = 0;
        #ifdef DELTA
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], manual_feedrate[X_AXIS]/60, active_extruder);
        #else
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], manual_feedrate[X_AXIS]/60, active_extruder);
        #endif
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit(PSTR("X"), ftostr31(current_position[X_AXIS]));
    }
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_move_menu_axis;
        encoderPosition = 0;
    }
}
static void lcd_move_y()
{
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        current_position[Y_AXIS] += float((int)encoderPosition) * move_menu_scale;
        if (min_software_endstops && current_position[Y_AXIS] < Y_MIN_POS)
            current_position[Y_AXIS] = Y_MIN_POS;
        if (max_software_endstops && current_position[Y_AXIS] > Y_MAX_POS)
            current_position[Y_AXIS] = Y_MAX_POS;
        encoderPosition = 0;
        #ifdef DELTA
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], manual_feedrate[Y_AXIS]/60, active_extruder);
        #else
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], manual_feedrate[Y_AXIS]/60, active_extruder);
        #endif
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit(PSTR("Y"), ftostr31(current_position[Y_AXIS]));
    }
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_move_menu_axis;
        encoderPosition = 0;
    }
}
static void lcd_move_z()
{
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        if (min_software_endstops && current_position[Z_AXIS] < Z_MIN_POS)
            current_position[Z_AXIS] = Z_MIN_POS;
        if (max_software_endstops && current_position[Z_AXIS] > Z_MAX_POS)
            current_position[Z_AXIS] = Z_MAX_POS;
        encoderPosition = 0;
        #ifdef DELTA
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], manual_feedrate[Z_AXIS]/60, active_extruder);
        #else
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], manual_feedrate[Z_AXIS]/60, active_extruder);
        #endif
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit(PSTR("Z"), ftostr31(current_position[Z_AXIS]));
    }
    if (LCD_CLICKED)
    {
        
        enquecommand_P(PSTR("G28"));
        lcd_quick_feedback();
        currentMenu = lcd_move_menu_axis;
        encoderPosition = 0;

    }
}

static void lcd_move_e()
{
    if (encoderPosition != 0)
    {
        current_position[E_AXIS] += float((int)encoderPosition) * move_menu_scale;
        encoderPosition = 0;
        #ifdef DELTA
        calculate_delta(current_position);
        plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], manual_feedrate[E_AXIS]/60, active_extruder);
        #else
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], manual_feedrate[E_AXIS]/60, active_extruder);
        #endif
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit(PSTR("Extruder"), ftostr31(current_position[E_AXIS]));
    }
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_move_menu_axis;
        encoderPosition = 0;
    }
}

static void lcd_move_menu_axis()
{
    START_MENU();
    MENU_ITEM(back, MSG_MOVE_AXIS, lcd_move_menu);
    MENU_ITEM(submenu, MSG_MOVE_X, lcd_move_x);
    MENU_ITEM(submenu, MSG_MOVE_Y, lcd_move_y);
    if (move_menu_scale < 10.0)
    {
        MENU_ITEM(submenu, MSG_MOVE_Z, lcd_move_z);
        MENU_ITEM(submenu, MSG_MOVE_E, lcd_move_e);
    }
    END_MENU();
}

static void lcd_move_menu_10mm()
{
    move_menu_scale = 10.0;
    lcd_move_menu_axis();
}
static void lcd_move_menu_1mm()
{
    move_menu_scale = 1.0;
    lcd_move_menu_axis();
}
static void lcd_move_menu_01mm()
{
    move_menu_scale = 0.1;
    lcd_move_menu_axis();
}

static void lcd_move_menu()
{
    START_MENU();
    MENU_ITEM(back, MSG_CONTROL, lcd_utilities_menu);
    MENU_ITEM(submenu, MSG_MOVE_10MM, lcd_move_menu_10mm);
    MENU_ITEM(submenu, MSG_MOVE_1MM, lcd_move_menu_1mm);
    MENU_ITEM(submenu, MSG_MOVE_01MM, lcd_move_menu_01mm);
    //TODO:X,Y,Z,E
    END_MENU();
}
#endif


// Maintenance menu implementation

#ifdef ENABLE_AUTO_BED_LEVELING
// Do auto leveling
static void auto_leveling()
{
    enquecommand_P(PSTR("G28"));
    enquecommand_P(PSTR("G29"));
#ifdef TEST    
    enquecommand_P(PSTR("G1 E0 F500"));
    enquecommand_P(PSTR("M104 S0"));
    enquecommand_P(PSTR("M106"));
#endif    

    lcd_return_to_status();
}



static void clean_nozzle()
{

    // lcd_implementation_draw_string(3, "                    ");    
    // lcd_implementation_draw_string(0, "Now, Cleaning Nozzle");
    // lcd_implementation_draw_string(1, "Please Wait Until   ");
    // lcd_implementation_draw_string(2, "Head going home     ");
    enquecommand_P(PSTR("V28"));
    // enquecommand_P(PSTR("M140 S100"));
    
    // START_MENU();

    // MENU_ITEM_CUSTOM(function, MSG_MENU_OK, 2, auto_leveling);
    // MENU_ITEM_CUSTOM(back, MSG_BACK, 3, lcd_utilities_menu);
    
    // END_MENU();

    if(!movesplanned()){
        currentMenu = lcd_status_screen;
    }
}
#endif


//**************************************************/


static void manual_leveling()
{
    lcd_implementation_draw_string(0, "Follow the Procedure");
    lcd_implementation_draw_string(1, " to Level Heat-Bed  ");

    START_MENU();
    
    MENU_ITEM_CUSTOM(function, MSG_MENU_OK, 2, leveling_procedure_0);
    MENU_ITEM_CUSTOM(back, "CANCEL", 3, lcd_utilities_menu);
    
    END_MENU();
}


static void leveling_procedure_0()
{
    for(int i = 0 ; i < 3 ; i++){
        for(int j = 0 ; j < 3 ; j++){
            bed_level[i][j] = 0;
        }
    }

    Config_StoreSettings();
    currentMenu = leveling_procedure_1;

    enquecommand_P(PSTR("G28"));
    enquecommand_P(PSTR("G1 F5000 Z10"));
    // enquecommand_P(PSTR("G1 F5000 X0 Y-110 Z10"));

    //currentMenu = leveling_procedure_1;
    encoderPosition = 0;
    prevent_lcd_update = true;
    leveling_flag = true;
}


static void leveling_procedure_1()
{
    refresh_cmd_timeout();

    lcd_implementation_draw_string(0, "Rotate all 3 knobs  ");
    lcd_implementation_draw_string(1, "till the bed matches");
    lcd_implementation_draw_string(2, "the bed level guide.");
    lcd_implementation_draw_string(3, "Then, push button.  ");


    if (LCD_CLICKED)
    {
       
        currentMenu = leveling_procedure_1_5;
    }
}

static void leveling_procedure_1_5()
{   
    delay(manualLevelingDelay);
    enquecommand_P(PSTR("G1 X0 Y-110"));
    enquecommand_P(PSTR("G1 F5000 Z0"));
    currentMenu = leveling_procedure_2;       
}


static void leveling_procedure_2()
{

    lcd_implementation_draw_string(0, "Rotate front knob   ");
    lcd_implementation_draw_string(1, "until nozzle-bed gap");
    lcd_implementation_draw_string(2, "is just paper-thin. ");
    lcd_implementation_draw_string(3, "Then, push button.  ");

    if (LCD_CLICKED)
    {
        currentMenu = leveling_procedure_2_5;
    }
}

static void leveling_procedure_2_5()
{
    delay(manualLevelingDelay);
    enquecommand_P(PSTR("G1 F5000 Z10"));
    enquecommand_P(PSTR("G1 X-95.3 Y55"));
    enquecommand_P(PSTR("G1 F5000 Z0"));
    currentMenu = leveling_procedure_3;       
}


static void leveling_procedure_3()
{

    lcd_implementation_draw_string(0, "Rotate left knob    ");
    lcd_implementation_draw_string(1, "until nozzle-bed gap");
    lcd_implementation_draw_string(2, "is just paper-thin. ");
    lcd_implementation_draw_string(3, "Then, push button.  ");
    if (LCD_CLICKED)
    {
        currentMenu = leveling_procedure_3_5;
    }
}

static void leveling_procedure_3_5()
{
    delay(manualLevelingDelay);
    enquecommand_P(PSTR("G1 F5000 Z10"));
    enquecommand_P(PSTR("G1 X95.3 Y55"));
    enquecommand_P(PSTR("G1 F5000 Z0"));
    currentMenu = leveling_procedure_4;
}

static void leveling_procedure_4()
{
    refresh_cmd_timeout();
        
    lcd_implementation_draw_string(0, "Rotate right knob   ");
    lcd_implementation_draw_string(1, "until nozzle-bed gap");
    lcd_implementation_draw_string(2, "is just paper-thin. ");
    lcd_implementation_draw_string(3, "Then, push button.  ");
   
    if (LCD_CLICKED) {   
        currentMenu = leveling_procedure_4_5;
    }
}

static void leveling_procedure_4_5()
{
    lcd_implementation_draw_string(0, "Bed Leveling Done!  ");
    lcd_implementation_draw_string(1, "Do a test print to  ");
    lcd_implementation_draw_string(2, "confirm leveling.   ");
    lcd_implementation_draw_string(3, "Wait for screen swap");
    
        
    enquecommand_P(PSTR("G28"));
    prevent_lcd_update = false;
    currentMenu = leveling_save;
}



static void leveling_save()
{

    lcd_implementation_draw_string(0, "                    ");
    lcd_implementation_draw_string(1, "                    ");
    lcd_implementation_draw_string(2, "                    ");
    lcd_implementation_draw_string(3, "                    ");    
    // delay(manualLevelingDelay);
    currentMenu = lcd_status_screen;
}


#endif

//**************************************************/




//**************************************************
#ifdef ENABLE_Z_OFFSET

static void print_bed_level() {
  for (int y = 0; y < ACCURATE_BED_LEVELING_POINTS; y++) {
    for (int x = 0; x < ACCURATE_BED_LEVELING_POINTS; x++) {
      SERIAL_PROTOCOL_F(bed_level[x][y], 2);
      SERIAL_PROTOCOLPGM(" ");
    }
    SERIAL_ECHOLN("");
  }
  
  SERIAL_PROTOCOL_F(get_level_offset(),2);
  SERIAL_ECHOLN("");
  
}

static void lcd_move_z()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =0.01;

    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        current_position[Z_AXIS] += float((int)encoderPosition) * move_menu_scale;
        if (min_software_endstops && current_position[Z_AXIS] < Z_MIN_POS)
            current_position[Z_AXIS] = Z_MIN_POS;
        if (max_software_endstops && current_position[Z_AXIS] > Z_MAX_POS)
            current_position[Z_AXIS] = Z_MAX_POS;

        // current_position[Z_AXIS] can be minus value but the limit is needed
        if (current_position[Z_AXIS] < -(Z_PROBE_OFFSET_FROM_EXTRUDER))
            current_position[Z_AXIS] = -(Z_PROBE_OFFSET_FROM_EXTRUDER);
        //
        
        encoderPosition = 0;

        if (!blocks_queued()) {
            #ifdef DELTA
            calculate_delta(current_position);
            #ifdef NONLINEAR_BED_LEVELING
            adjust_delta(current_position);
            #endif
            plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], manual_feedrate[Z_AXIS]/60, active_extruder);
            #else
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], manual_feedrate[Z_AXIS]/60, active_extruder);
            #endif
        }

        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(3,PSTR("Z"), ftostr32(current_position[Z_AXIS]));
    }
    lcd_implementation_draw_string(0, "Rotate the button.  ");
    lcd_implementation_draw_string(1, "And push it         ");
    lcd_implementation_draw_string(2, "      to set offset.");
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        
        set_level_offset(current_position[Z_AXIS]);
        for(int x = 0 ; x < ACCURATE_BED_LEVELING_POINTS ; x++){
            for(int y = 0 ; y < ACCURATE_BED_LEVELING_POINTS ; y++){
                bed_level[x][y]+=get_level_offset();
            }
        }
        Config_StoreSettings();
        print_bed_level();
        
        enquecommand_P(PSTR("G28"));

        currentMenu = lcd_return_to_status;
        encoderPosition = 0;
        
    }
}

static void edit_level_offset(){
    for(int x = 0 ; x < ACCURATE_BED_LEVELING_POINTS ; x++){
        for(int y = 0 ; y < ACCURATE_BED_LEVELING_POINTS ; y++){
            bed_level[x][y]-=get_level_offset();
        }
    }
    
    Config_StoreSettings();
    print_bed_level();
    enquecommand_P(PSTR("G28"));
    enquecommand_P(PSTR("G0 Z40 F5000"));
    enquecommand_P(PSTR("G0 Z20 F1000"));
    st_synchronize();

    enquecommand_P(PSTR("M400"));           // wait for finishing movement to go to z
    
    currentMenu = wait_offset;
}

static void wait_offset(){
    lcd_implementation_draw_string(0, "If the movement     ");
    lcd_implementation_draw_string(1, "        is finished,");
    lcd_implementation_draw_string(2, "Press the button    ");
    lcd_implementation_draw_string(3, "        to continue.");
  
    
    if(LCD_CLICKED){
        encoderPosition = 0;
        lcdDrawUpdate = 1;
        currentMenu = move_offset;
    }
    
}


static void move_offset(){
    lcd_implementation_draw_string(0, "Rotate the button.  ");
    lcd_implementation_draw_string(1, "And push it         ");
    lcd_implementation_draw_string(2, "      to set offset.");
    lcd_implementation_draw_string(3, "                    ");
    START_MENU();
    MENU_ITEM(submenu,MSG_MOVE_Z,lcd_move_z);
    END_MENU();

}


//**************************************************



static void goto_home()
{
    enquecommand_P(PSTR("G28"));
    lcd_return_to_status();
}


static void lcd_control_temperature_menu()
{

    START_MENU();
    MENU_ITEM(back, MSG_BACK, lcd_utilities_menu);

#if EXTRUDERS > 1
        MENU_ITEM_EDIT(int3, "Right Nozzle", &target_temperature[0], 0, HEATER_0_MAXTEMP - 15);
    #if TEMP_SENSOR_1 != 0
        MENU_ITEM_EDIT(int3, "Left Nozzle", &target_temperature[1], 0, HEATER_1_MAXTEMP - 15);
    #endif
#else    
        MENU_ITEM_EDIT(int3, MSG_NOZZLE, &target_temperature[0], 0, HEATER_0_MAXTEMP - 15);
    #if TEMP_SENSOR_1 != 0
        MENU_ITEM_EDIT(int3, MSG_NOZZLE1, &target_temperature[1], 0, HEATER_1_MAXTEMP - 15);
    #endif
#endif

#if TEMP_SENSOR_2 != 0
    MENU_ITEM_EDIT(int3, MSG_NOZZLE2, &target_temperature[2], 0, HEATER_2_MAXTEMP - 15);
#endif


#if TEMP_SENSOR_BED != 0
    MENU_ITEM_EDIT(int3, MSG_BED, &target_temperature_bed, 0, BED_MAXTEMP - 15);
#endif

    MENU_ITEM(function, "COOLDOWN", lcd_cooldown);

    END_MENU();
}

// static void lcd_control_temperature_preheat_pla_settings_menu()
// {
//     START_MENU();
//      MENU_ITEM(back, MSG_BACK, lcd_filament_menu);
// //    MENU_ITEM_EDIT(int3, MSG_FAN_SPEED, &plaPreheatFanSpeed, 0, 255);
//     MENU_ITEM_EDIT(int3, MSG_NOZZLE, &plaPreheatHotendTemp, 0, HEATER_0_MAXTEMP - 15);
// #if TEMP_SENSOR_BED != 0
//     MENU_ITEM_EDIT(int3, MSG_BED, &plaPreheatHPBTemp, 0, BED_MAXTEMP - 15);
// #endif
// #ifdef EEPROM_SETTINGS
//     MENU_ITEM(function, MSG_STORE_EPROM, Config_StoreSettings);
// #endif
//     END_MENU();
// }

// static void lcd_control_temperature_preheat_abs_settings_menu()
// {
//     START_MENU();
//      MENU_ITEM(back, MSG_BACK, lcd_filament_menu);
// //    MENU_ITEM_EDIT(int3, MSG_FAN_SPEED, &absPreheatFanSpeed, 0, 255);
//     MENU_ITEM_EDIT(int3, MSG_NOZZLE, &absPreheatHotendTemp, 0, HEATER_0_MAXTEMP - 15);
// #if TEMP_SENSOR_BED != 0
//     MENU_ITEM_EDIT(int3, MSG_BED, &absPreheatHPBTemp, 0, BED_MAXTEMP - 15);
// #endif
// #ifdef EEPROM_SETTINGS
//     MENU_ITEM(function, MSG_STORE_EPROM, Config_StoreSettings);
// #endif
//     END_MENU();
// }

static void lcd_maintenance_runtime_stats()
{
    char buffer[20];
    lcd_implementation_draw_string(0, "    CREATABLE D3    ");

    sprintf(buffer, "POWER TIME %lu:%lu", lifetime_minutes / 60, lifetime_minutes % 60);
    lcd_implementation_draw_string(1, buffer);

    sprintf(buffer, "PRINT TIME %lu:%lu", lifetime_print_minutes / 60, lifetime_print_minutes % 60);
    lcd_implementation_draw_string(2, buffer);



    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_status_screen;
        encoderPosition = 0;
    }

    // Clear
    if (encoderPosition > 10) {
        lcd_implementation_draw_string(3, "RESET RUNTIME STATS ");
        init_lifetime_stats();
    }else{
        lcd_implementation_draw_string(3, "PUSH BUTTON TO EXIT ");
    }

}


static void lcd_maintenance_version()
{
    char buffer[20];
    lcd_implementation_draw_string(0, "    CREATABLE D3    ");
    sprintf(buffer,"   %s", STRING_CONFIG_H_AUTHOR);
    lcd_implementation_draw_string(1, buffer);
    sprintf(buffer,"%s", STRING_VERSION_CONFIG_H);
    lcd_implementation_draw_string(2, buffer);
    lcd_implementation_draw_string(3, "PUSH BUTTON TO EXIT ");
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_status_screen;
        encoderPosition = 0;
    }

}



static void lcd_preheat_operate()
{
    setTargetHotend0(preheatHotendTemp);
    setTargetHotend1(preheatHotendTemp);
//    setTargetHotend2(plaPreheatHotendTemp);
    setTargetBed(preheatHPBTemp);
    fanSpeed = preheatFanSpeed;
    setWatch(); // heater sanity check timer
    lcd_return_to_status();
}

static void lcd_preheat()
{
    char buffer[20];

    sprintf(buffer,"  Nozzle temp : %d", preheatHotendTemp);
    lcd_implementation_draw_string(0, buffer);

    sprintf(buffer,"  Bed temp : %d", preheatHPBTemp);
    lcd_implementation_draw_string(1, buffer);

    START_MENU();

    MENU_ITEM_CUSTOM(function, "START", 2, lcd_preheat_operate);
    MENU_ITEM_CUSTOM(back, MSG_CANCEL, 3, lcd_main_menu);
    
    END_MENU();
}
static void filament_change()
{
    START_MENU();   
    MENU_ITEM(back, MSG_BACK, lcd_filament_menu);
    MENU_ITEM(submenu, "LOAD FILAMENT", filament_load);
    MENU_ITEM(submenu, "UNLOAD FILAMENT", filament_unload);
    // MENU_ITEM(submenu, "LOAD FILAMENT", filament_load_new);
    // MENU_ITEM(submenu, "UNLOAD FILAMENT", filament_unload);
    
    END_MENU();
}



static void filament_setting_save()
{
    Config_StoreSettings();
    currentMenu = lcd_status_screen;
}

static void filament_setting_plasave()
{
    preheatHotendTemp = plaPreheatHotendTemp;
    preheatHPBTemp = plaPreheatHPBTemp;
    filament_setting_save();
}

static void filament_setting_abssave()
{
    preheatHotendTemp = absPreheatHotendTemp;
    preheatHPBTemp = absPreheatHPBTemp;
    filament_setting_save();
}



static void filament_setting_pla()
{
    char buffer[20];

    sprintf(buffer,"  Nozzle temp : %d", plaPreheatHotendTemp);
    lcd_implementation_draw_string(0, buffer);

    sprintf(buffer,"  Bed temp : %d", plaPreheatHPBTemp);
    lcd_implementation_draw_string(1, buffer);
    START_MENU();   
   
    MENU_ITEM_CUSTOM(function, "SAVE & EXIT",2, filament_setting_plasave);
    MENU_ITEM_CUSTOM(back, MSG_BACK,3, filament_setting);
    END_MENU();
}


static void filament_setting_abs()
{
    char buffer[20];

    sprintf(buffer,"  Nozzle temp : %d", absPreheatHotendTemp);
    lcd_implementation_draw_string(0, buffer);

    sprintf(buffer,"  Bed temp : %d", absPreheatHPBTemp);
    lcd_implementation_draw_string(1, buffer);

    START_MENU();   
    MENU_ITEM_CUSTOM(function, "SAVE & EXIT",2,filament_setting_abssave);
    MENU_ITEM_CUSTOM(back, MSG_BACK, 3,filament_setting);
       
    

    END_MENU();
}

static void filament_setting_custom()
{
    START_MENU();
    
//    MENU_ITEM_EDIT(int3, MSG_FAN_SPEED, &plaPreheatFanSpeed, 0, 255);
    MENU_ITEM_EDIT_CUSTOM(int3, MSG_NOZZLE, &preheatHotendTemp, 0, HEATER_0_MAXTEMP - 15);
#if TEMP_SENSOR_BED != 0
    MENU_ITEM_EDIT_CUSTOM(int3, MSG_BED, &preheatHPBTemp, 0, BED_MAXTEMP - 15);
#endif
#ifdef EEPROM_SETTINGS
    MENU_ITEM_CUSTOM(function, "SAVE & EXIT", 2,filament_setting_save);
    MENU_ITEM_CUSTOM(back, MSG_BACK, 3,filament_setting);
#endif
    END_MENU();
}

static void filament_setting()
{
    START_MENU();   
    
    MENU_ITEM(back, MSG_BACK, lcd_filament_menu);

    MENU_ITEM(submenu, "PLA", filament_setting_pla);
    MENU_ITEM(submenu, "ABS", filament_setting_abs);
    MENU_ITEM(submenu, "CUSTOM SETTINGS", filament_setting_custom);
    
    END_MENU();
}


static void lcd_filament_menu()
{
    if(onetime_flag){
        onetime_flag = false;
    }
    START_MENU();
    
#ifdef SDSUPPORT
    #ifdef MENU_ADDAUTOSTART
      MENU_ITEM(function, MSG_AUTOSTART, lcd_autostart_sd);
    #endif
#endif

    // MENU_ITEM(submenu, "Preheat", lcd_preheat);
    MENU_ITEM(back, MSG_BACK, lcd_main_menu);
    MENU_ITEM(submenu, "EXTRUDE FILAMENT", filament_extrude);
    MENU_ITEM(submenu, "CHANGE FILAMENT", filament_change);
    MENU_ITEM(submenu, "SETTING FILAMENT", filament_setting);
    
    // MENU_ITEM(submenu, MSG_PREHEAT_PLA_SETTINGS, lcd_control_temperature_preheat_pla_settings_menu);
    // MENU_ITEM(submenu, MSG_PREHEAT_ABS_SETTINGS, lcd_control_temperature_preheat_abs_settings_menu);
    END_MENU();
}

static void lcd_control_motion_menu()
{
    START_MENU();
    MENU_ITEM(back, MSG_BACK, lcd_utilities_menu);
#ifdef ENABLE_AUTO_BED_LEVELING
    MENU_ITEM_EDIT(float32, MSG_ZPROBE_ZOFFSET, &zprobe_zoffset, -10, 50);
#endif
    MENU_ITEM_EDIT(float5, MSG_ACC, &acceleration, 500, 99000);
    MENU_ITEM_EDIT(float3, MSG_VXY_JERK, &max_xy_jerk, 1, 990);
    MENU_ITEM_EDIT(float52, MSG_VZ_JERK, &max_z_jerk, 0.1, 990);
    MENU_ITEM_EDIT(float3, MSG_VE_JERK, &max_e_jerk, 1, 990);
//    MENU_ITEM_EDIT(float3, MSG_VMAX MSG_X, &max_feedrate[X_AXIS], 1, 999);
//    MENU_ITEM_EDIT(float3, MSG_VMAX MSG_Y, &max_feedrate[Y_AXIS], 1, 999);
//    MENU_ITEM_EDIT(float3, MSG_VMAX MSG_Z, &max_feedrate[Z_AXIS], 1, 999);
//    MENU_ITEM_EDIT(float3, MSG_VMAX MSG_E, &max_feedrate[E_AXIS], 1, 999);
//    MENU_ITEM_EDIT(float3, MSG_VMIN, &minimumfeedrate, 0, 999);
//    MENU_ITEM_EDIT(float3, MSG_VTRAV_MIN, &mintravelfeedrate, 0, 999);
    MENU_ITEM_EDIT_CALLBACK(long5, MSG_AMAX MSG_X, &max_acceleration_units_per_sq_second[X_AXIS], 100, 99000, reset_acceleration_rates);
    MENU_ITEM_EDIT_CALLBACK(long5, MSG_AMAX MSG_Y, &max_acceleration_units_per_sq_second[Y_AXIS], 100, 99000, reset_acceleration_rates);
    MENU_ITEM_EDIT_CALLBACK(long5, MSG_AMAX MSG_Z, &max_acceleration_units_per_sq_second[Z_AXIS], 100, 99000, reset_acceleration_rates);
    MENU_ITEM_EDIT_CALLBACK(long5, MSG_AMAX MSG_E, &max_acceleration_units_per_sq_second[E_AXIS], 100, 99000, reset_acceleration_rates);
    MENU_ITEM_EDIT(float5, MSG_A_RETRACT, &retract_acceleration, 100, 99000);
//    MENU_ITEM_EDIT(float52, MSG_XSTEPS, &axis_steps_per_unit[X_AXIS], 5, 9999);
//    MENU_ITEM_EDIT(float52, MSG_YSTEPS, &axis_steps_per_unit[Y_AXIS], 5, 9999);
//    MENU_ITEM_EDIT(float51, MSG_ZSTEPS, &axis_steps_per_unit[Z_AXIS], 5, 9999);
//    MENU_ITEM_EDIT(float51, MSG_ESTEPS, &axis_steps_per_unit[E_AXIS], 5, 9999);
#ifdef ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED
    MENU_ITEM_EDIT(bool, MSG_ENDSTOP_ABORT, &abort_on_endstop_hit);
#endif
    END_MENU();
}


static void lcd_led_printing(){
    START_MENU();
    MENU_ITEM(back, MSG_BACK, lcd_led_setting);
    MENU_ITEM(submenu, "PRINT RED", lcd_led_printR);
    MENU_ITEM(submenu, "PRINT GREEN", lcd_led_printG);
    MENU_ITEM(submenu, "PRINT BLUE", lcd_led_printB);
    END_MENU();



}

static void lcd_led_printR()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(printR , printG , printB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        printR += (int)(float((int)encoderPosition) * move_menu_scale);
        if (printR < 0)
            printR = 0;
        if (printR > 255)
            printR = 255;
        
        encoderPosition = 0;

        set_led_strip_color(printR , printG , printB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("PRINT RED"), itostr3(printR));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_printing;
        encoderPosition = 0;


    }
}

static void lcd_led_printG()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(printR , printG , printB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        printG += (int)(float((int)encoderPosition) * move_menu_scale);
        if (printG < 0)
            printG = 0;
        if (printG > 255)
            printG = 255;
        
        encoderPosition = 0;

        set_led_strip_color(printR , printG , printB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("PRINT GREEN"), itostr3(printG));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_printing;
        encoderPosition = 0;


    }
}


static void lcd_led_printB()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(printR , printG , printB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        printB += (int)(float((int)encoderPosition) * move_menu_scale);
        if (printB < 0)
            printB = 0;
        if (printB > 255)
            printB = 255;
        
        encoderPosition = 0;

        set_led_strip_color(printR , printG , printB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("PRINT BLUE"), itostr3(printB));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_printing;
        encoderPosition = 0;


    }
}





static void lcd_led_standBy(){
    START_MENU();
    MENU_ITEM(back, MSG_BACK, lcd_led_setting);
    MENU_ITEM(submenu, "STANDBY RED", lcd_led_standByR);
    MENU_ITEM(submenu, "STANDBY GREEN", lcd_led_standByG);
    MENU_ITEM(submenu, "STANDBY BLUE", lcd_led_standByB);
    END_MENU();


}

static void lcd_led_standByR()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(standByR , standByG , standByB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        standByR += (int)(float((int)encoderPosition) * move_menu_scale);
        if (standByR < 0)
            standByR = 0;
        if (standByR > 255)
            standByR = 255;
        
        encoderPosition = 0;

        set_led_strip_color(standByR , standByG , standByB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("STANDBY RED"), itostr3(standByR));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_standBy;
        encoderPosition = 0;


    }
}

static void lcd_led_standByG()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(standByR , standByG , standByB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        standByG += (int)(float((int)encoderPosition) * move_menu_scale);
        if (standByG < 0)
            standByG = 0;
        if (standByG > 255)
            standByG = 255;
        
        encoderPosition = 0;

        set_led_strip_color(standByR , standByG , standByB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("STANDBY GREEN"), itostr3(standByG));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_standBy;
        encoderPosition = 0;


    }
}


static void lcd_led_standByB()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(standByR , standByG , standByB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        standByB += (int)(float((int)encoderPosition) * move_menu_scale);
        if (standByB < 0)
            standByB = 0;
        if (standByB > 255)
            standByB = 255;
        
        encoderPosition = 0;

        set_led_strip_color(standByR , standByG , standByB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("STANDBY BLUE"), itostr3(standByB));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_standBy;
        encoderPosition = 0;
    }
}




static void lcd_led_heating(){
    START_MENU();
    MENU_ITEM(back, MSG_BACK, lcd_led_setting);
    MENU_ITEM(submenu, "HEATING RED", lcd_led_heatingR);
    MENU_ITEM(submenu, "HEATING GREEN", lcd_led_heatingG);
    MENU_ITEM(submenu, "HEATING BLUE", lcd_led_heatingB);
    END_MENU();    



}


static void lcd_led_heatingR()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(heatingR , heatingG , heatingB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        heatingR += (int)(float((int)encoderPosition) * move_menu_scale);
        if (heatingR < 0)
            heatingR = 0;
        if (heatingR > 255)
            heatingR = 255;
        
        encoderPosition = 0;

        set_led_strip_color(heatingR , heatingG , heatingB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("STANDBY RED"), itostr3(heatingR));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_heating;
        encoderPosition = 0;


    }
}

static void lcd_led_heatingG()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(heatingR , heatingG , heatingB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        heatingG += (int)(float((int)encoderPosition) * move_menu_scale);
        if (heatingG < 0)
            heatingG = 0;
        if (heatingG > 255)
            heatingG = 255;
        
        encoderPosition = 0;

        set_led_strip_color(heatingR , heatingG , heatingB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("STANDBY GREEN"), itostr3(heatingG));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_heating;
        encoderPosition = 0;


    }
}


static void lcd_led_heatingB()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        set_led_strip_color(heatingR , heatingG , heatingB );       
        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        heatingB += (int)(float((int)encoderPosition) * move_menu_scale);
        if (heatingB < 0)
            heatingB = 0;
        if (heatingB > 255)
            heatingB = 255;
        
        encoderPosition = 0;

        set_led_strip_color(heatingR , heatingG , heatingB );
        
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("STANDBY BLUE"), itostr3(heatingB));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_heating;
        encoderPosition = 0;
    }
}


static void lcd_led_brightValue()
{
    // float move_menu_scale =0.1;
    float move_menu_scale =1;
    if(!ledUpdated){
        if(IS_SD_PRINTING){
            set_led_strip_color(printR , printG , printB );
        }else{
            if(isHeat){
                set_led_strip_color(heatingR , heatingG , heatingB );
            }else{
                set_led_strip_color(standByR , standByG , standByB );
            }
        }        ledUpdated = true;
        ledState = 1;
    }
    if (encoderPosition != 0)
    {
        refresh_cmd_timeout();
        brightValue += (int)(float((int)encoderPosition) * move_menu_scale);
        if (brightValue < 0)
            brightValue = 0;
        if (brightValue > 100)
            brightValue = 100;
        
        encoderPosition = 0;
        if(IS_SD_PRINTING){
            set_led_strip_color(printR , printG , printB );
        }else{
            if(isHeat){
                set_led_strip_color(heatingR , heatingG , heatingB );
            }else{
                set_led_strip_color(standByR , standByG , standByB );
            }
        }
        lcdDrawUpdate = 1;
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit_custom(1,PSTR("LED BRIGHTNESS"), itostr3(brightValue));
    }
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\ 
    

    if (LCD_CLICKED)
    {
        ledState = 0;
        ledUpdated = false;
        lcd_quick_feedback();
        Config_StoreSettings();
        currentMenu = lcd_led_setting;
        encoderPosition = 0;
    }
}




static void lcd_led_setting(){
    START_MENU();
    MENU_ITEM(back, MSG_BACK, lcd_info_settings);
    MENU_ITEM(submenu, "PRINTING COLOR", lcd_led_printing);
    MENU_ITEM(submenu, "STANDBY COLOR", lcd_led_standBy);
    MENU_ITEM(submenu, "HEATING COLOR", lcd_led_heating);
    MENU_ITEM(submenu, "LED BRIGHTNESS", lcd_led_brightValue);

    // MENU_ITEM_EDIT_LED(standBy, "LED BRIGHTNESS", &brightValue, 0, 100);

    END_MENU();



}


static void lcd_buzzer_setting(){
    START_MENU();
    MENU_ITEM_EDIT_CUSTOM(bool, "BUZZER", &music_on);
    END_MENU();


    // customTune(261.6,1);

    // customTune(329.6,1);

    // customTune(392.0,1);
    // lcd_return_to_status();
}


static void lcd_info_settings()
{
    START_MENU();
    
   MENU_ITEM(back, MSG_BACK, lcd_main_menu);
#ifdef ENABLE_MOVE_MENU
    MENU_ITEM(submenu, "Motion setting", lcd_control_motion_menu);
    MENU_ITEM(submenu, MSG_MOVE_AXIS, lcd_move_menu);
#endif
    // MENU_ITEM(submenu, "ALARM MUSIC", lcd_buzzer_setting);
    MENU_ITEM_EDIT(bool, "ALARM MUSIC", &music_on);

    MENU_ITEM(submenu, "LED COLOR", lcd_led_setting);
    MENU_ITEM_EDIT_CUSTOM(int3, "HEATING MINUTE", &heat_hold_minute, 1, 60);
    

    MENU_ITEM(submenu, "FIRMWARE VERSION", lcd_maintenance_version);
    MENU_ITEM(submenu, "RUNTIME STATUS", lcd_maintenance_runtime_stats);
    

// #ifdef EEPROM_SETTINGS
//     MENU_ITEM(function, MSG_STORE_EPROM, Config_StoreSettings);
//     MENU_ITEM(function, MSG_LOAD_EPROM, Config_RetrieveSettings);
// #endif

    MENU_ITEM(function, "RESTORE DEFAULT", Config_ResetDefault);
    
    END_MENU();
}


static void lcd_utilities_menu()
{
    START_MENU();
    MENU_ITEM(back, "BACK", lcd_main_menu);
    MENU_ITEM(function, "HOME HEAD", goto_home);
#ifdef ENABLE_AUTO_BED_LEVELING
// #ifdef TEST    
//     MENU_ITEM(submenu, MSG_AUTO_LEVEL, heating_for_leveling);
// #else
    MENU_ITEM(submenu, "BED LEVELING", manual_leveling);
// #endif    

#endif
   /**************************************************
#ifdef ENABLE_Z_OFFSET    
    MENU_ITEM(submenu, "Setting Z-offset", edit_level_offset);
#endif
    //**************************************************/
    

    MENU_ITEM(submenu, "TEMPERATURE", lcd_control_temperature_menu);
    // MENU_ITEM(submenu, "System", lcd_info_settings)
    MENU_ITEM(submenu, "CLEAN NOZZLE", clean_nozzle);      
    

    END_MENU();
}


#ifdef DOGLCD
static void lcd_set_contrast()
{
    if (encoderPosition != 0)
    {
        lcd_contrast -= encoderPosition;
        if (lcd_contrast < 0) lcd_contrast = 0;
        else if (lcd_contrast > 63) lcd_contrast = 63;
        encoderPosition = 0;
        lcdDrawUpdate = 1;
        u8g.setContrast(lcd_contrast);
    }
    if (lcdDrawUpdate)
    {
        lcd_implementation_drawedit(PSTR(MSG_CONTRAST), itostr2(lcd_contrast));
    }
    if (LCD_CLICKED)
    {
        lcd_quick_feedback();
        currentMenu = lcd_control_menu;
        encoderPosition = 0;
    }
}
#endif

#ifdef FWRETRACT
static void lcd_control_retract_menu()
{
    START_MENU();
    MENU_ITEM(back, MSG_CONTROL, lcd_control_menu);
    MENU_ITEM_EDIT(bool, MSG_AUTORETRACT, &autoretract_enabled);
    MENU_ITEM_EDIT(float52, MSG_CONTROL_RETRACT, &retract_length, 0, 100);
    MENU_ITEM_EDIT(float3, MSG_CONTROL_RETRACTF, &retract_feedrate, 1, 999);
    MENU_ITEM_EDIT(float52, MSG_CONTROL_RETRACT_ZLIFT, &retract_zlift, 0, 999);
    MENU_ITEM_EDIT(float52, MSG_CONTROL_RETRACT_RECOVER, &retract_recover_length, 0, 100);
    MENU_ITEM_EDIT(float3, MSG_CONTROL_RETRACT_RECOVERF, &retract_recover_feedrate, 1, 999);
    END_MENU();
}
#endif


#if SDCARDDETECT == -1
static void lcd_sd_refresh()
{
    card.initsd();
    currentMenuViewOffset = 0;
}
#endif
static void lcd_sd_updir()
{
    card.updir();
    currentMenuViewOffset = 0;
}

void lcd_sdcard_menu()
{
    if (lcdDrawUpdate == 0 && LCD_CLICKED == 0)
        return;	// nothing to do (so don't thrash the SD card)
    uint16_t fileCnt = card.getnrfilenames();
    START_MENU();
    MENU_ITEM(back, MSG_BACK, lcd_main_menu);
    card.getWorkDirName();
    if(card.filename[0]=='/')
    {
#if SDCARDDETECT == -1
        MENU_ITEM(function, LCD_STR_REFRESH MSG_REFRESH, lcd_sd_refresh);
#endif
    }else{
        MENU_ITEM(function, LCD_STR_FOLDER "..", lcd_sd_updir);
    }

    for(uint16_t i=0;i<fileCnt;i++)
    {
        if (_menuItemNr == _lineNr)
        {
            #ifndef SDCARD_RATHERRECENTFIRST
              card.getfilename(i);
            #else
              card.getfilename(fileCnt-1-i);
            #endif
            if (card.filenameIsDir)
            {
                MENU_ITEM(sddirectory, MSG_CARD_MENU, card.filename, card.longFilename);
            }else{
                MENU_ITEM(sdfile, MSG_CARD_MENU, card.filename, card.longFilename);
            }
        }else{
            MENU_ITEM_DUMMY();
        }
    }
    END_MENU();
}

#define menu_edit_type(_type, _name, _strFunc, scale) \
    void menu_edit_ ## _name () \
    { \
        if ((int32_t)encoderPosition < minEditValue) \
            encoderPosition = minEditValue; \
        if ((int32_t)encoderPosition > maxEditValue) \
            encoderPosition = maxEditValue; \
        if (lcdDrawUpdate) \
            lcd_implementation_drawedit(editLabel, _strFunc(((_type)encoderPosition) / scale)); \
        if (LCD_CLICKED) \
        { \
            *((_type*)editValue) = ((_type)encoderPosition) / scale; \
            lcd_quick_feedback(); \
            currentMenu = prevMenu; \
            encoderPosition = prevEncoderPosition; \
        } \
    } \
    void menu_edit_custom ## _name () \
    { \
        if ((int32_t)encoderPosition < minEditValue) \
            encoderPosition = minEditValue; \
        if ((int32_t)encoderPosition > maxEditValue) \
            encoderPosition = maxEditValue; \
        if (lcdDrawUpdate) \
            lcd_implementation_drawedit(editLabel, _strFunc(((_type)encoderPosition) / scale)); \
        if (LCD_CLICKED) \
        { \
            *((_type*)editValue) = ((_type)encoderPosition) / scale; \
            lcd_quick_feedback(); \
            currentMenu = prevMenu; \
            encoderPosition = prevEncoderPosition; \
            Config_StoreSettings();\
        } \
        lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");\
        lcd_implementation_draw_string(2, "                    ");\
        lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");\   
    } \
    void menu_edit_callback_ ## _name () \
    { \
        if ((int32_t)encoderPosition < minEditValue) \
            encoderPosition = minEditValue; \
        if ((int32_t)encoderPosition > maxEditValue) \
            encoderPosition = maxEditValue; \
        if (lcdDrawUpdate) \
            lcd_implementation_drawedit(editLabel, _strFunc(((_type)encoderPosition) / scale)); \
        if (LCD_CLICKED) \
        { \
            *((_type*)editValue) = ((_type)encoderPosition) / scale; \
            lcd_quick_feedback(); \
            currentMenu = prevMenu; \
            encoderPosition = prevEncoderPosition; \
            (*callbackFunc)();\
        } \
    } \
    static void menu_action_setting_edit_ ## _name (const char* pstr, _type* ptr, _type minValue, _type maxValue) \
    { \
        prevMenu = currentMenu; \
        prevEncoderPosition = encoderPosition; \
         \
        lcdDrawUpdate = 2; \
        currentMenu = menu_edit_ ## _name; \
         \
        editLabel = pstr; \
        editValue = ptr; \
        minEditValue = minValue * scale; \
        maxEditValue = maxValue * scale; \
        encoderPosition = (*ptr) * scale; \
    }\
    static void menu_action_setting_edit_custom_ ## _name (const char* pstr, _type* ptr, _type minValue, _type maxValue) \
    { \
        prevMenu = currentMenu; \
        prevEncoderPosition = encoderPosition; \
         \
        lcdDrawUpdate = 2; \
        currentMenu = menu_edit_custom ## _name; \
         \
        editLabel = pstr; \
        editValue = ptr; \
        minEditValue = minValue * scale; \
        maxEditValue = maxValue * scale; \
        encoderPosition = (*ptr) * scale; \
    }\
    static void menu_action_setting_edit_callback_ ## _name (const char* pstr, _type* ptr, _type minValue, _type maxValue, menuFunc_t callback) \
    { \
        prevMenu = currentMenu; \
        prevEncoderPosition = encoderPosition; \
         \
        lcdDrawUpdate = 2; \
        currentMenu = menu_edit_callback_ ## _name; \
         \
        editLabel = pstr; \
        editValue = ptr; \
        minEditValue = minValue * scale; \
        maxEditValue = maxValue * scale; \
        encoderPosition = (*ptr) * scale; \
        callbackFunc = callback;\
    }
menu_edit_type(int, int3, itostr3, 1)
menu_edit_type(float, float3, ftostr3, 1)
menu_edit_type(float, float32, ftostr32, 100)
menu_edit_type(float, float5, ftostr5, 0.01)
menu_edit_type(float, float51, ftostr51, 10)
menu_edit_type(float, float52, ftostr52, 100)
menu_edit_type(unsigned long, long5, ftostr5, 0.01)


// static void menu_action_setting_edit_led_print (const char* pstr, int* ptr, int minValue, int maxValue) 
// { 
//     prevMenu = currentMenu; 
//     prevEncoderPosition = encoderPosition; 
     
//     lcdDrawUpdate = 2; 
//     currentMenu = menu_edit_led_print; 
     
//     editLabel = pstr; 
//     editValue = ptr; 
//     minEditValue = minValue * 1; 
//     maxEditValue = maxValue * 1; 
//     encoderPosition = (*ptr) * 1; 
//     // set_led_strip_color( ## name ## R * 100 / brightValue ,  ## name ## G * 100 / brightValue , ## name ## B * 100 / brightValue );
// }

// void menu_edit_led_print ()
// {
//     if ((int32_t)encoderPosition < minEditValue)
//         encoderPosition = minEditValue;
//     if ((int32_t)encoderPosition > maxEditValue)
//         encoderPosition = maxEditValue;
//     if (lcdDrawUpdate)
//         lcd_implementation_drawedit(editLabel, itostr3(((int)encoderPosition) / 1));
//     if (LCD_CLICKED)
//     {
//         *((int*)editValue) = ((int)encoderPosition) / 1;
//         lcd_quick_feedback();
//         currentMenu = prevMenu;
//         encoderPosition = prevEncoderPosition;
//         Config_StoreSettings();
//     }
//     lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");
//     lcd_implementation_draw_string(2, "                    ");
//     lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");   

//     set_led_strip_color( printR * 100 / brightValue ,  printG * 100 / brightValue , printB * 100 / brightValue );

// }


// static void menu_action_setting_edit_led_standBy (const char* pstr, int* ptr, int minValue, int maxValue) 
// { 
//     prevMenu = currentMenu; 
//     prevEncoderPosition = encoderPosition; 
     
//     lcdDrawUpdate = 2; 
//     currentMenu = menu_edit_led_standBy; 
     
//     editLabel = pstr; 
//     editValue = ptr; 
//     minEditValue = minValue * 1; 
//     maxEditValue = maxValue * 1; 
//     encoderPosition = (*ptr) * 1; 
//     // set_led_strip_color( ## name ## R * 100 / brightValue ,  ## name ## G * 100 / brightValue , ## name ## B * 100 / brightValue );
// }

// void menu_edit_led_standBy ()
// {
//     if ((int32_t)encoderPosition < minEditValue)
//         encoderPosition = minEditValue;
//     if ((int32_t)encoderPosition > maxEditValue)
//         encoderPosition = maxEditValue;
//     if (lcdDrawUpdate)
//         lcd_implementation_drawedit(editLabel, itostr3(((int)encoderPosition) / 1));
//     if (LCD_CLICKED)
//     {
//         *((int*)editValue) = ((int)encoderPosition) / 1;
//         lcd_quick_feedback();
//         currentMenu = prevMenu;
//         encoderPosition = prevEncoderPosition;
//         Config_StoreSettings();
//     }
//     lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");
//     lcd_implementation_draw_string(2, "                    ");
//     lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");   

//     set_led_strip_color( standByR * 100 / brightValue ,  standByG * 100 / brightValue , standByB * 100 / brightValue );

// }


// static void menu_action_setting_edit_led_heating (const char* pstr, int* ptr, int minValue, int maxValue) 
// { 
//     prevMenu = currentMenu; 
//     prevEncoderPosition = encoderPosition; 
     
//     lcdDrawUpdate = 2; 
//     currentMenu = menu_edit_led_heating; 
     
//     editLabel = pstr; 
//     editValue = ptr; 
//     minEditValue = minValue * 1; 
//     maxEditValue = maxValue * 1; 
//     encoderPosition = (*ptr) * 1; 
//     // set_led_strip_color( ## name ## R * 100 / brightValue ,  ## name ## G * 100 / brightValue , ## name ## B * 100 / brightValue );
// }

// void menu_edit_led_heating ()
// {
//     if ((int32_t)encoderPosition < minEditValue)
//         encoderPosition = minEditValue;
//     if ((int32_t)encoderPosition > maxEditValue)
//         encoderPosition = maxEditValue;
//     if (lcdDrawUpdate)
//         lcd_implementation_drawedit(editLabel, itostr3(((int)encoderPosition) / 1));
//     if (LCD_CLICKED)
//     {
//         *((int*)editValue) = ((int)encoderPosition) / 1;
//         lcd_quick_feedback();
//         currentMenu = prevMenu;
//         encoderPosition = prevEncoderPosition;
//         Config_StoreSettings();
//     }
//     lcd_implementation_draw_string(0, "   ROTATE BUTTON    ");
//     lcd_implementation_draw_string(2, "                    ");
//     lcd_implementation_draw_string(3, "PUSH BUTTON TO SAVE");   

//     set_led_strip_color( heatingR * 100 / brightValue ,  heatingG * 100 / brightValue , heatingB * 100 / brightValue );

// }


#ifdef REPRAPWORLD_KEYPAD
	static void reprapworld_keypad_move_z_up() {
    encoderPosition = 1;
    move_menu_scale = REPRAPWORLD_KEYPAD_MOVE_STEP;
		lcd_move_z();
  }
	static void reprapworld_keypad_move_z_down() {
    encoderPosition = -1;
    move_menu_scale = REPRAPWORLD_KEYPAD_MOVE_STEP;
		lcd_move_z();
  }
	static void reprapworld_keypad_move_x_left() {
    encoderPosition = -1;
    move_menu_scale = REPRAPWORLD_KEYPAD_MOVE_STEP;
		lcd_move_x();
  }
	static void reprapworld_keypad_move_x_right() {
    encoderPosition = 1;
    move_menu_scale = REPRAPWORLD_KEYPAD_MOVE_STEP;
		lcd_move_x();
	}
	static void reprapworld_keypad_move_y_down() {
    encoderPosition = 1;
    move_menu_scale = REPRAPWORLD_KEYPAD_MOVE_STEP;
		lcd_move_y();
	}
	static void reprapworld_keypad_move_y_up() {
		encoderPosition = -1;
		move_menu_scale = REPRAPWORLD_KEYPAD_MOVE_STEP;
    lcd_move_y();
	}
	static void reprapworld_keypad_move_home() {
		enquecommand_P((PSTR("G28"))); // move all axis home
	}
#endif

/** End of menus **/


static void lcd_quick_feedback()
{
    lcdDrawUpdate = 2;
    blocking_enc = millis() + 500;
    lcd_implementation_quick_feedback();
}

/** Menu action functions **/
static void menu_action_back(menuFunc_t data)
{
    currentMenu = data;
    encoderPosition = 0;
}
static void menu_action_submenu(menuFunc_t data)
{
    currentMenu = data;
    encoderPosition = 0;
}
static void menu_action_gcode(const char* pgcode)
{
    enquecommand_P(pgcode);
}
static void menu_action_function(menuFunc_t data)
{
    (*data)();
}
static void menu_action_sdfile(const char* filename, char* longFilename)
{
    char cmd[30];
    char* c;



    // implant start gcode
    // enquecommand_P(PSTR("G28"));    //Go Home
    // enquecommand_P(PSTR("G21"));    //metric values
    // enquecommand_P(PSTR("G90"));    //absolute positioning
    // enquecommand_P(PSTR("M82"));    //set extruder to absolute mode
    // enquecommand_P(PSTR("M106"));   //start with the fan on
    
    // enquecommand_P(PSTR("G92 E0")); //set extruder position to 0
    
    // //set extruder temp
    // for(int i = 0 ; i < 30 ; i++){
    //     cmd[i] = '\0';
    // }
    // sprintf_P(cmd, PSTR("M104 S%d"), preheatHotendTemp);
    // for(c = &cmd[6]; *c; c++)
    //     *c = tolower(*c);
    // enquecommand(cmd);
    // //set extruder temp

    // //get bed heating up
    // for(int i = 0 ; i < 30 ; i++){
    //     cmd[i] = '\0';
    // }
    // sprintf_P(cmd, PSTR("M140 S%d"), preheatHPBTemp);
    // for(c = &cmd[6]; *c; c++)
    //     *c = tolower(*c);
    // enquecommand(cmd);
    // //get bed heating up
    
    // enquecommand_P(PSTR("G1 Z100 F5000"));
    // enquecommand_P(PSTR("G1 X-123"));
    // enquecommand_P(PSTR("G1 Z1"));

    // //set extruder temp and wait
    // for(int i = 0 ; i < 30 ; i++){
    //     cmd[i] = '\0';
    // }
    // sprintf_P(cmd, PSTR("M109 S%d"), preheatHotendTemp);
    // for(c = &cmd[6]; *c; c++)
    //     *c = tolower(*c);
    // enquecommand(cmd);
    // //set extruder temp and wait

    // //get bed heating up and wait
    // for(int i = 0 ; i < 30 ; i++){
    //     cmd[i] = '\0';
    // }
    // sprintf_P(cmd, PSTR("M190 S%d"), preheatHPBTemp);
    // for(c = &cmd[6]; *c; c++)
    //     *c = tolower(*c);
    // enquecommand(cmd);
    // //get bed heating up and wait

    // enquecommand_P(PSTR("G92 E-32"));
    // enquecommand_P(PSTR("G1 E0 F3000"));
    // enquecommand_P(PSTR("G1 E70 F200"));
    // enquecommand_P(PSTR("G92 E0"));

    // enquecommand_P(PSTR("G21"));

    // // start gcode 



    sprintf_P(cmd, PSTR("M23 %s"), filename);
    for(c = &cmd[4]; *c; c++)
        *c = tolower(*c);
    enquecommand(cmd);

  

    // enquecommand_P(PSTR("V128"));    


    enquecommand_P(PSTR("M24"));
    lcd_return_to_status();
}
static void menu_action_sddirectory(const char* filename, char* longFilename)
{
    card.chdir(filename);
    encoderPosition = 0;
}
static void menu_action_setting_edit_bool(const char* pstr, bool* ptr)
{
    *ptr = !(*ptr);
}

static void menu_action_setting_edit_custom_bool(const char* pstr, bool* ptr)
{
    *ptr = !(*ptr);
}
#endif//ULTIPANEL

/** LCD API **/
void lcd_init()
{
    lcd_implementation_init();

#ifdef LED_STRIP
    // setup LED Strip
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
#endif    

#ifdef NEWPANEL
    pinMode(BTN_EN1,INPUT);
    pinMode(BTN_EN2,INPUT);
    WRITE(BTN_EN1,HIGH);
    WRITE(BTN_EN2,HIGH);
  #if BTN_ENC > 0
    pinMode(BTN_ENC,INPUT);
    WRITE(BTN_ENC,HIGH);
  #endif
  #ifdef REPRAPWORLD_KEYPAD
    pinMode(SHIFT_CLK,OUTPUT);
    pinMode(SHIFT_LD,OUTPUT);
    pinMode(SHIFT_OUT,INPUT);
    WRITE(SHIFT_OUT,HIGH);
    WRITE(SHIFT_LD,HIGH);
  #endif
#else  // Not NEWPANEL
  #ifdef SR_LCD_2W_NL // Non latching 2 wire shiftregister
     pinMode (SR_DATA_PIN, OUTPUT);
     pinMode (SR_CLK_PIN, OUTPUT);
  #elif defined(SHIFT_CLK) 
     pinMode(SHIFT_CLK,OUTPUT);
     pinMode(SHIFT_LD,OUTPUT);
     pinMode(SHIFT_EN,OUTPUT);
     pinMode(SHIFT_OUT,INPUT);
     WRITE(SHIFT_OUT,HIGH);
     WRITE(SHIFT_LD,HIGH);
     WRITE(SHIFT_EN,LOW);
  #else
     #ifdef ULTIPANEL
     #error ULTIPANEL requires an encoder
     #endif
  #endif // SR_LCD_2W_NL
#endif//!NEWPANEL

#if defined (SDSUPPORT) && defined(SDCARDDETECT) && (SDCARDDETECT > 0)
    pinMode(SDCARDDETECT,INPUT);
    WRITE(SDCARDDETECT, HIGH);
    lcd_oldcardstatus = IS_SD_INSERTED;
#endif//(SDCARDDETECT > 0)
#ifdef LCD_HAS_SLOW_BUTTONS
    slow_buttons = 0;
#endif
    lcd_buttons_update();
#ifdef ULTIPANEL
    encoderDiff = 0;
#endif
}

void lcd_update()
{
    unsigned long milsec = millis();

    if(IS_SD_PRINTING){
        if(!ledUpdated){
            set_led_strip_color(printR , printG, printB);
            ledUpdated = true;
        }
    }else{
        if(isHeat){
            if(!ledUpdated){
            set_led_strip_color(heatingR , heatingG, heatingB);
            ledUpdated = true;
        }
        }else{
            if(!ledUpdated){
            set_led_strip_color(standByR , standByG, standByB);
            ledUpdated = true;
        }

        }

        if(isHeat){
            // lcd.setCursor(10,1);
            // lcd.print((unsigned long)heat_hold_minute - (milsec - heatCount)/1000/60);
            double heat_time_diff;
            if(milsec - heatCount < 0){
                heat_time_diff = (unsigned long)heat_hold_minute - (milsec - heatCount+sizeof(unsigned long))/1000/60;
            }else{
                heat_time_diff = (unsigned long)heat_hold_minute - (milsec - heatCount)/1000/60;
            }

          if(heat_time_diff  <=0){
                // analogWrite(LED_G, 250);
                Cancle_heating = true;
                setTargetHotend0(0);
                setTargetHotend1(0);
                setTargetHotend2(0);
                setTargetBed(0);

                enquecommand_P(PSTR("M106 S255"));
                isHeat = 2;
                ledUpdated = false;
            }
            if(degTargetHotend(0) == 0 && degTargetBed() ==0){
                isHeat = 0;
                ledUpdated = false;   
            }
        }else{
            if(degTargetHotend(0) != 0 || degTargetBed() !=0){
                isHeat=1;
                ledUpdated = false;
            }
            heatCount = milsec;

        }

        // lcd.setCursor(19,0);
        // lcd.print(firstTimeRunPhase);
    }
    




    static unsigned long timeoutToStatus = 0;

    #ifdef LCD_HAS_SLOW_BUTTONS
    slow_buttons = lcd_implementation_read_slow_buttons(); // buttons which take too long to read in interrupt context
    #endif

    lcd_buttons_update();

    #if (SDCARDDETECT > 0)
    if((IS_SD_INSERTED != lcd_oldcardstatus))
    {
        lcdDrawUpdate = 2;
        lcd_oldcardstatus = IS_SD_INSERTED;
        lcd_implementation_init(); // to maybe revive the lcd if static electricty killed it.

        if(lcd_oldcardstatus)
        {
            card.initsd();
            LCD_MESSAGEPGM(MSG_SD_INSERTED);
        }
        else
        {
            card.release();
            LCD_MESSAGEPGM(MSG_SD_REMOVED);
        }
    }
    #endif//CARDINSERTED

    if (lcd_next_update_millis < millis())
    {
#ifdef ULTIPANEL
		#ifdef REPRAPWORLD_KEYPAD
        	if (REPRAPWORLD_KEYPAD_MOVE_Z_UP) {
        		reprapworld_keypad_move_z_up();
        	}
        	if (REPRAPWORLD_KEYPAD_MOVE_Z_DOWN) {
        		reprapworld_keypad_move_z_down();
        	}
        	if (REPRAPWORLD_KEYPAD_MOVE_X_LEFT) {
        		reprapworld_keypad_move_x_left();
        	}
        	if (REPRAPWORLD_KEYPAD_MOVE_X_RIGHT) {
        		reprapworld_keypad_move_x_right();
        	}
        	if (REPRAPWORLD_KEYPAD_MOVE_Y_DOWN) {
        		reprapworld_keypad_move_y_down();
        	}
        	if (REPRAPWORLD_KEYPAD_MOVE_Y_UP) {
        		reprapworld_keypad_move_y_up();
        	}
        	if (REPRAPWORLD_KEYPAD_MOVE_HOME) {
        		reprapworld_keypad_move_home();
        	}
		#endif
        if (abs(encoderDiff) >= ENCODER_PULSES_PER_STEP)
        {
            lcdDrawUpdate = 1;
            encoderPosition += encoderDiff / ENCODER_PULSES_PER_STEP;
            encoderDiff = 0;
            timeoutToStatus = millis() + LCD_TIMEOUT_TO_STATUS;
        }
        if (LCD_CLICKED)
            timeoutToStatus = millis() + LCD_TIMEOUT_TO_STATUS;
#endif//ULTIPANEL

#ifdef DOGLCD        // Changes due to different driver architecture of the DOGM display
        blink++;     // Variable for fan animation and alive dot
        u8g.firstPage();
        do
        {
            u8g.setFont(u8g_font_6x10_marlin);
            u8g.setPrintPos(125,0);
            if (blink % 2) u8g.setColorIndex(1); else u8g.setColorIndex(0); // Set color for the alive dot
            u8g.drawPixel(127,63); // draw alive dot
            u8g.setColorIndex(1); // black on white
            (*currentMenu)();
            if (!lcdDrawUpdate)  break; // Terminate display update, when nothing new to draw. This must be done before the last dogm.next()
        } while( u8g.nextPage() );
#else
        (*currentMenu)();
#endif

#ifdef LCD_HAS_STATUS_INDICATORS
        lcd_implementation_update_indicators();
#endif

#ifdef ULTIPANEL
        if(timeoutToStatus < millis() && currentMenu != lcd_status_screen && !prevent_lcd_update)
        {
            lcd_return_to_status();
            lcdDrawUpdate = 2;
        }
#endif//ULTIPANEL
        if (lcdDrawUpdate == 2)
            lcd_implementation_clear();
        if (lcdDrawUpdate)
            lcdDrawUpdate--;
        lcd_next_update_millis = millis() + 100;
    }
}

void lcd_setstatus(const char* message)
{
    if (lcd_status_message_level > 0)
        return;
    strncpy(lcd_status_message, message, LCD_WIDTH);
    lcdDrawUpdate = 2;
}
void lcd_setstatuspgm(const char* message)
{
    if (lcd_status_message_level > 0)
        return;
    strncpy_P(lcd_status_message, message, LCD_WIDTH);
    lcdDrawUpdate = 2;
}
void lcd_setalertstatuspgm(const char* message)
{
    lcd_setstatuspgm(message);
    lcd_status_message_level = 1;
#ifdef ULTIPANEL
    lcd_return_to_status();
#endif//ULTIPANEL
}
void lcd_reset_alert_level()
{
    lcd_status_message_level = 0;
}

#ifdef DOGLCD
void lcd_setcontrast(uint8_t value)
{
    lcd_contrast = value & 63;
    u8g.setContrast(lcd_contrast);
}
#endif

#ifdef ULTIPANEL
/* Warning: This function is called from interrupt context */
void lcd_buttons_update()
{
#ifdef NEWPANEL
    uint8_t newbutton=0;
    if(READ(BTN_EN1)==0)  newbutton|=EN_A;
    if(READ(BTN_EN2)==0)  newbutton|=EN_B;
  #if BTN_ENC > 0
    if((blocking_enc<millis()) && (READ(BTN_ENC)==0))
        newbutton |= EN_C;
  #endif
    buttons = newbutton;
    #ifdef LCD_HAS_SLOW_BUTTONS
    buttons |= slow_buttons;
    #endif
    #ifdef REPRAPWORLD_KEYPAD
      // for the reprapworld_keypad
      uint8_t newbutton_reprapworld_keypad=0;
      WRITE(SHIFT_LD,LOW);
      WRITE(SHIFT_LD,HIGH);
      for(int8_t i=0;i<8;i++) {
          newbutton_reprapworld_keypad = newbutton_reprapworld_keypad>>1;
          if(READ(SHIFT_OUT))
              newbutton_reprapworld_keypad|=(1<<7);
          WRITE(SHIFT_CLK,HIGH);
          WRITE(SHIFT_CLK,LOW);
      }
      buttons_reprapworld_keypad=~newbutton_reprapworld_keypad; //invert it, because a pressed switch produces a logical 0
	#endif
#else   //read it from the shift register
    uint8_t newbutton=0;
    WRITE(SHIFT_LD,LOW);
    WRITE(SHIFT_LD,HIGH);
    unsigned char tmp_buttons=0;
    for(int8_t i=0;i<8;i++)
    {
        newbutton = newbutton>>1;
        if(READ(SHIFT_OUT))
            newbutton|=(1<<7);
        WRITE(SHIFT_CLK,HIGH);
        WRITE(SHIFT_CLK,LOW);
    }
    buttons=~newbutton; //invert it, because a pressed switch produces a logical 0
#endif//!NEWPANEL

    //manage encoder rotation
    uint8_t enc=0;
    if(buttons&EN_A)
        enc|=(1<<0);
    if(buttons&EN_B)
        enc|=(1<<1);
    if(enc != lastEncoderBits)
    {
        switch(enc)
        {
        case encrot0:
            if(lastEncoderBits==encrot3)
                encoderDiff++;
            else if(lastEncoderBits==encrot1)
                encoderDiff--;
            break;
        case encrot1:
            if(lastEncoderBits==encrot0)
                encoderDiff++;
            else if(lastEncoderBits==encrot2)
                encoderDiff--;
            break;
        case encrot2:
            if(lastEncoderBits==encrot1)
                encoderDiff++;
            else if(lastEncoderBits==encrot3)
                encoderDiff--;
            break;
        case encrot3:
            if(lastEncoderBits==encrot2)
                encoderDiff++;
            else if(lastEncoderBits==encrot0)
                encoderDiff--;
            break;
        }
    }
    lastEncoderBits = enc;
}

void lcd_buzz(long duration, uint16_t freq)
{
#ifdef LCD_USE_I2C_BUZZER
  lcd.buzz(duration,freq);
#endif
}

bool lcd_clicked()
{
  return LCD_CLICKED;
}
#endif//ULTIPANEL

#ifdef LED_STRIP
// res, green, blue : 0~255)
void set_led_strip_color(int red, int green, int blue)
{
  analogWrite(LED_R, (int)(float(red)  / 100.0  * float(brightValue)));
  analogWrite(LED_G, (int)(float(green)  / 100.0  * float(brightValue)));
  analogWrite(LED_B, (int)(float(blue)  / 100.0  * float(brightValue)));
}
#endif

/********************************/
/** Float conversion utilities **/
/********************************/
//  convert float to string with +123.4 format
char conv[8];

char *ftostr3(const float &x)
{
   return itostr3((int)x);
}

char *ftostr_plus3(const float &x)
{
   return itostr31((int)x);
}

char *itostr2(const uint8_t &x)
{
  //sprintf(conv,"%5.1f",x);
  int xx=x;
  conv[0]=(xx/10)%10+'0';
  conv[1]=(xx)%10+'0';
  conv[2]=0;
  return conv;
}

//  convert float to string with +123.4 format
char *ftostr31(const float &x)
{
  int xx=x*10;
  conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[1]=(xx/1000)%10+'0';
  conv[2]=(xx/100)%10+'0';
  conv[3]=(xx/10)%10+'0';
  conv[4]='.';
  conv[5]=(xx)%10+'0';
  conv[6]=0;
  return conv;
}

//  convert float to string with 123.4 format
char *ftostr31ns(const float &x)
{
  int xx=x*10;
  //conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[0]=(xx/1000)%10+'0';
  conv[1]=(xx/100)%10+'0';
  conv[2]=(xx/10)%10+'0';
  conv[3]='.';
  conv[4]=(xx)%10+'0';
  conv[5]=0;
  return conv;
}

char *ftostr32(const float &x)
{
  long xx=x*100;
  if (xx >= 0)
    // conv[0]=(xx/10000)%10+'0';
    conv[0]=(xx/10000)%10+'+';
  else
    conv[0]='-';
  xx=abs(xx);
  conv[1]=(xx/1000)%10+'0';
  conv[2]=(xx/100)%10+'0';
  conv[3]='.';
  conv[4]=(xx/10)%10+'0';
  conv[5]=(xx)%10+'0';
  conv[6]=0;
  return conv;
}


char *itostr31(const int &x)
{
  int xx = x;

  conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[1]=(xx/100)%10+'0';
  conv[2]=(xx/10)%10+'0';
  conv[3]=(xx)%10+'0';
  conv[4]=0;

  return conv;
}

char *itostr3(const int &xx)
{
  if (xx >= 100)
    conv[0]=(xx/100)%10+'0';
  else
    conv[0]=' ';
  if (xx >= 10)
    conv[1]=(xx/10)%10+'0';
  else
    conv[1]=' ';
  conv[2]=(xx)%10+'0';
  conv[3]=0;
  return conv;
}

char *itostr3left(const int &xx)
{
  if (xx >= 100)
  {
    conv[0]=(xx/100)%10+'0';
    conv[1]=(xx/10)%10+'0';
    conv[2]=(xx)%10+'0';
    conv[3]=0;
  }
  else if (xx >= 10)
  {
    conv[0]=(xx/10)%10+'0';
    conv[1]=(xx)%10+'0';
    conv[2]=0;
  }
  else
  {
    conv[0]=(xx)%10+'0';
    conv[1]=0;
  }
  return conv;
}

char *itostr4(const int &xx)
{
  if (xx >= 1000)
    conv[0]=(xx/1000)%10+'0';
  else
    conv[0]=' ';
  if (xx >= 100)
    conv[1]=(xx/100)%10+'0';
  else
    conv[1]=' ';
  if (xx >= 10)
    conv[2]=(xx/10)%10+'0';
  else
    conv[2]=' ';
  conv[3]=(xx)%10+'0';
  conv[4]=0;
  return conv;
}

//  convert float to string with 12345 format
char *ftostr5(const float &x)
{
  long xx=abs(x);
  if (xx >= 10000)
    conv[0]=(xx/10000)%10+'0';
  else
    conv[0]=' ';
  if (xx >= 1000)
    conv[1]=(xx/1000)%10+'0';
  else
    conv[1]=' ';
  if (xx >= 100)
    conv[2]=(xx/100)%10+'0';
  else
    conv[2]=' ';
  if (xx >= 10)
    conv[3]=(xx/10)%10+'0';
  else
    conv[3]=' ';
  conv[4]=(xx)%10+'0';
  conv[5]=0;
  return conv;
}

//  convert float to string with +1234.5 format
char *ftostr51(const float &x)
{
  long xx=x*10;
  conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[1]=(xx/10000)%10+'0';
  conv[2]=(xx/1000)%10+'0';
  conv[3]=(xx/100)%10+'0';
  conv[4]=(xx/10)%10+'0';
  conv[5]='.';
  conv[6]=(xx)%10+'0';
  conv[7]=0;
  return conv;
}

//  convert float to string with +123.45 format
char *ftostr52(const float &x)
{
  long xx=x*100;
  conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[1]=(xx/10000)%10+'0';
  conv[2]=(xx/1000)%10+'0';
  conv[3]=(xx/100)%10+'0';
  conv[4]='.';
  conv[5]=(xx/10)%10+'0';
  conv[6]=(xx)%10+'0';
  conv[7]=0;
  return conv;
}

// Callback for after editing PID i value
// grab the pid i value out of the temp variable; scale it; then update the PID driver
void copy_and_scalePID_i()
{
#ifdef PIDTEMP
  Ki = scalePID_i(raw_Ki);
  updatePID();
#endif
}

// Callback for after editing PID d value
// grab the pid d value out of the temp variable; scale it; then update the PID driver
void copy_and_scalePID_d()
{
#ifdef PIDTEMP
  Kd = scalePID_d(raw_Kd);
  updatePID();
#endif
}

#endif //ULTRA_LCD



#if defined(BEEPER) && BEEPER > -1
static void customTune(float freq, float duration){
  float microSec;
  int repeat;

  SET_OUTPUT(BEEPER);

  microSec = 1000000/freq;
  repeat = duration * 1000000 / microSec;

  for(int i =0 ; i <repeat ; i++){
    WRITE(BEEPER, HIGH);
    delayMicroseconds(microSec/2);
    WRITE(BEEPER, LOW);
    delayMicroseconds(microSec/2);
     
  }
}

#endif