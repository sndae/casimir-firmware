@echo off
echo Program DIGREC ID to Flash
echo     Call with the following parameters: com id0 id1 id2 id3
echo     com: com port (e.g. com4)
echo     idn: ID in hexadecimal (no 0x prefix needed)

avrdude -p atmega1284p -c stk500v2 -P %1 -U eeprom:w:0x%2,0x%3,0x%4,0x%5:m
rem avrdude -p atmega1284p -c stk500v2 -P com4 -U eeprom:w:00,01,33,66,99:m