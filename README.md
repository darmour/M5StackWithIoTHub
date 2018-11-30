# M5StackWithIoTHub
Use two M5Stack devices to send color messages back an forth using Azure IoT Hub<BR>
  <IMG src='https://github.com/darmour/M5StackWithIoTHub/blob/master/IMG_20181130_131309981_HDR.jpg'> <BR>

The project connects two M5Stack devices to Azure IoT Hub.  When the devices are started up, they will try to connect to an array of wifi networks specified in the code - which you will need to modify for your available networks. Once wifi is connected, the device will connect to the Azure IoT Hub using the connection string specified in the code - which you need to modify. To obtain this value, you must first create the device twin in Azure. After you create the device twin you can find the connection string in the device details.

You can use the Arduino IDE to program your M5Stack. You can find the details about this here: https://www.arduino.cc/en/Main/Software

If you want to use VS Code instead of the Arduino IDE, follow the instructions here: https://github.com/Microsoft/vscode-arduino

For M5Stack you need to get the following library. View the ReadMe for additional instructions for the Arduino IDE config:  https://github.com/m5stack/arduino-esp32

This project was built using the esp-Azure "GetStarted" example. You will need to get the libraries and follow the instructions here: https://github.com/espressif/esp-azure


IoTHubTwoWayColor is the code for the M5Stack. The M5Devices are programmed to do the following when one of the three buttons are pushed:
<UL>
  <LI>Button 1: Read from the Grove Temperature/Humidity sensor (or use random values if the sensor is not present) and send it to Azure IoT Hub.</LI>
  <LI>Button 2: Rotate through an array of screen background colors.</LI>
  <LI>Button 3: Send the currently displayed background color to Azure IoT Hub.</LI>
</UL>

The M5Stack can also receive a message from Azure IoT Hub (note: your Azure IoT Hub must be using the "Standard pricing tier since the "free" pricing tier does not support bi-directional communication).  If the message contains one of the background colors, the device will change the background to the one specified in the message

The M5Stack can also receive commands from Azure IoT Hub.  If the "Clear" command is sent, the device will beep and clear the screen.

Additionally Azure Functions can be used to route the incoming color message from one of the M%Stack devices to a second M5Stack. Using the function, when you send a color from one M5Stack, the other M5Stack will change its color to the one that was sent to it.
