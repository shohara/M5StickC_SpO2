# M5StickC_SpO2
* For measuring SpO2 and Heart Rate on M5StickC.

* [M5StickC](https://m5stack.com/collections/m5-core/products/stick-c) and [Heart Unit (MAX30100)](https://m5stack.com/products/mini-heart-unit) are needed to work.

## Not guaranteed
The value is noisy and no-stability, it may include issues.

# This app
* Measurement takes more than 30 sec (normally more than 60sec because of instability).
* Analyze data to remove invalid (seems bad) measurement.
* Re-measurement is automatically started if the measurements were not trusted by the analysis.
* The measurement as treat as valid is in the range as followings.
    * Heart Rate (HR): 40-250
    * SpO2: 70-100
* After getting the valid 30 measurement, it analyzes data. 
![preview](https://media.githubusercontent.com/media/shohara/M5StickC_SpO2/images/images/SpO2.jpg)

# Build
* Include MAX30100lib library before compiling ino file on Arduino IDE.
