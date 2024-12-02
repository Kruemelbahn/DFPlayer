# DFPlayer

DFPlayer is an ATmega328-board to control a soundmodul "DFPlayer - mini".<br>
<img src=Images/DFPlayer_mini.jpg><br>

For more details please refer to the [manual](Documentation/AVRSound_DFPlayer.pdf).<br>

### Sound-FRED
There is also a hardware-version for a SoundFred available:<br>
<img src=Images/Sound-FRED.jpg><br>
For compilation as SoundFRED active/deactivate #defines in DFPlayer.ino and use this [boarddefinition](Boarddefinition%20for%20SoundFRED.txt).<br>

For more details please refer to the [manual](Documentation/AVRSound_DFPlayer-Sound-FRED.pdf).<br>

A battery tray for holding the battery can be found [here](https://github.com/Kruemelbahn/3D-Printables/blob/main/Geh%C3%A4use%20und%20Komponenten/Akkuschale-SoundFred.stl)

### Requested libraries
DFPlayer requires my libraries listed below in addition to various Arduino standard libraries:<br> 
- [HeartBeat](https://www.github.com/Kruemelbahn/HeartBeat)
- [OLEDPanel](https://www.github.com/Kruemelbahn/OLEDPanel)