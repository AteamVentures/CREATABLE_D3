##Firmware Change History
Ver. 1.1.6
- Fix the wrong retraction speed set after load the filament

Ver. 1.1.5
- Supports LED pin of both mainboards(Ver 1.1 & 1.2)
- Add Go home movement before Load / Unload Filament

Ver. 1.1.4
- Fix pin mapping for LED R and G of changed mainboard(Ver 1.2)
- Add definition for selecting cases => #define BALL_BARING_FAN & #define BOARD_VER1_2

Ver. 1.1.3
- Change Error Temperature from 5 to -80

Ver. 1.1.2

- Auto fan control enable. Nozzle temperature is under 50 celcius degrees, fan will be turned off.
- When get serial data, motor will be hold. Originally, motor hold time was 60 seconds. However, we change it to 6000 seconds. 


Ver. 1.1.1

- When get serial data, reset the heat-hold-count.
- Change the default heat-hold-time from 10 minutes to 30 minutes.

Ver. 1.1.0

- Comment Utilities > Clean Nozzle Function
- Change Manual Z Home Position from 232 to 230 for changed bed clip

Ver. 1.0.9

- PID tuning
- Change Kp, Ki, Kd value
- Change PID_MAX Value
- Change PID_FUNCTIONAL_RANGE from 10 to 30

Ver. 1.0.8

- Change DEFAULT_AXIS_STEPS_PER_UNIT Constant for feeder in Configuration.h
- Auto Leveling Code is Disabled

Ver. 1.0.7

- Filament load function debug
- Enable/Disable Unlimit Heating Code add 
	- V130 : Enable Unlimit Heating
	- V131 : Disable Unlimit Heating	
- Change wording (Select Filament)

Ver. 1.0.6

- Delete D2 firmware files
- Change Delta Radius Values in configuration.h file
- Add D3 Delta Radius Values image

Ver. 1.0.5

- While SD Card Printing, stepper deactivate time is changed from 60 to 9999 seconds

Ver. 1.0.4

- Apply 3 point manual leveling
- Add LED control
- Add Buzzer on/off
