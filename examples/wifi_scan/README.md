# GUI Provision Example

Starts a UI to configure Wi-Fi credential.
## How to use example

### Hardware Required

* An ESP32-S3-HMI-DevKit development board
* A USB-C cable for Power supply and programming

### Configure the Project

Open the project configuration menu (`idf.py menuconfig`). 

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

## Example Output

Run the example, you will see a simple provision interface.

After scanning, the AP lists will show on screen, you can chose an AP and enter credential to connect.

The AP information, such as BSSID, RSSI will show on screen.


