# M5StickC_SpO2
* For measuring SpO2 and Heart Rate on M5StickC.

* [M5StickC](https://m5stack.com/collections/m5-core/products/stick-c) and [Heart Unit (MAX30100)](https://m5stack.com/products/mini-heart-unit) are needed to work.

## Not guaranteed
The value from sensor is noisy and not-stable. This software reduce such noise, but it may still include issues.

# How to use
* Put your finger on the Heart Unit after turning on M5StickC.
* Measurement takes more than 30 sec (normally more than 60sec because of instability).
* After the measurement, such as the following result is shown.
* <img src="https://media.githubusercontent.com/media/shohara/M5StickC_SpO2/images/images/SpO2.jpg" width="400">

# Measurement
* This software gathers 30 probable samples.
* To get probable samples, analyze data to remove invalid (seems bad) samples.
* Re-measurement is automatically started if the measurements were doubtful by the analysis.
* The measurement as treat as valid is in the range as followings.
    * Heart Rate (HR): 40-250
    * SpO2: 70-100
* After getting 30 probable samples, the result is shown.

# Build
* Include MAX30100lib library before compiling ino file on Arduino IDE.
