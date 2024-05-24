# Customizations & Fixes

The following customizations were applied on top of the stock firmware. 

-  Fn + M + O Changes the side light to display on either left side or right side or alternating sides every 1 minute.
-  Numlock indicator using side leds while color
-  Keyboard has 3 sleep levels: 
  1. Leds off, RF module on, MCU on
  2. Leds off, RF module off, MCU on
  3. Leds off, RF module off, MCU standby/sleep 

  By default keyboard will go to sleep level 1 in 2 minutes and sleep level 2 in 6 minutes (when using 2.4G dongle) or in 30 minutes (when using BT connection)
  If 2.4G is selected and there is no connectivity in 15 seconds keyboard will enter level 3 sleep.
  If BT is selected and there is no connectivity in 60 seconds keyboard will enter level 3 sleep.

  To change sleep mode you can use Fn + ]:
  When on default mode (explained above, the right side leds will flash 4 times green)

  The next mode will flash 4 times orange (This is actually the default sleep policy for Nuphy firmware version 1.1.3). 
  In this mode the following applies:
  Keyboard will go to sleep level 1 in 6 minutes
  If 2.4G or BT is selected and there is no connectivity in 120 seconds(BT) or 30 seconds (RF 2.4G) keyboard will enter level 2 sleep.

  On the last sleep mode the right side leds will flash 4 times red.
  In this mode the keyboard will never sleep.

  Side indicators will flash blue for 0.3s when board enters level 3 sleep mode, as an indicator.

-  Fn + B will show current battery levels using numbers from 1 to 0 (0 means 100%). The color will be red, yellow, light green, dark green) based on battery level.
   Last led will have a different color based on battery percentage as well. 
   Example:
   if 1 to 4 lights up and the last led that lights up 5 is dark green color you will have between 47 and 50% battery
   if 1 to 4 lights up and the last led that lights up 5 is light green color you will have between 44 and 46 % battery
   if 1 to 4 lights up and the last led that lights up 5 is red color you will have between 41 and 43 % battery

-  The custom MAC keys now have double functionality:
    MAC_VOICE if Mac mode selected and F5             if Win mode is selected
    TASK      if Mac mode selected and KC_CALC        if Win mode is selected
    SEARCH    if Mac mode selected and KC_LCTL + KC_F if Win mode is selected
    PRT_SCR   if Mac mode selected and PrintScreen    if Win mode is selected
    PRT_AREA  if Mac mode selected and PrintScreen    if Win mode is selected

-  New custom key: NUMLOCK_INS. On longpress it will act as NUMLOCK and on short press as INS.
-  New algorithm to prevent unnecessary EEPROM writes. When you change RGB modes or side lights it will only save the setting to EEPROM after 30 seconds.
   If for instance your need to press mode button for 5 times to reach your favourite RGB effect, normally you will get 1 EEPROM save for each press (5 EEPROM writes).
   With the new algorithm you will get 1 write to EEPROM after 40 seconds from last press. As a side efect if you change something and you shutdown the keyboard immediately you will lose the change.

-  Bluetooth and RF will signal on corresponding number keys when trying to connect (Blue color and keys 1 to 3 for BT_1 to BT_3 or Green color and key 4 when it is trying to connect using 2.4G)

-  Side light brightness levels are now 7 (instead of 6) from 0 (disabled) to 6 (full). A new lower step has been introduced. Default level after reset is 1 (the lowest ilumination possible for side leds)

-  The system indicators now have 3 levels of brightness (before there was only 1 level) that are dependend on the side light brightness. Thus for example capslock will no longer light at full brightness if side leds have a lower brightness selected.

- Default startup LED brightness is set to half of max. Default RGB effect is RGB_MATRIX_CYCLE_LEFT_RIGHT
- Default tapping has been reduced to 6ms (from 8ms default) and debounce decreased to 1 ms
- Deep sleep algorithm (level 3 sleep) is applied using code from nuphy / jincao1
- Fix keyboard randomly crashing/freezing (algorithm from jincao1 with small modifications)
- Fix LED lights not powering down when not used. Because of how the keyboard is build, both rgb and side leds need to be off, they can't be controlled individually (algorithm from jincao1)
- Fix keystrokes being lost on wake. Wake keystrokes will appear after a very short delay while board re-establishes connection. BT may not be as reliable as the dongle.
  This is achieved through a buffer of 64 key actions (key down and key up are 2 actions). The buffer is cleared if connection is not established within 1s after the last action.
  Key events after the buffer is full will also be dropped. (Buffer algorithm taken from jincao1)
- Enhance keyboard reports transmission logic to greatly reduce stuck/lost key strokes. It may still occasionally drop/repeat keys but it's rare. (algorithm from jincao1 with small modifications)
- Side light algorithms are modified in order to reduce firmware size and to make animations more smooth. On the lowest level of brightness for side leds you might see some jerkiness on some of the breath algorithm. This is because the lowest level of brightness is lower than standard Nuphy.

## Author

[@adi4086](https://github.com/adi4086)

