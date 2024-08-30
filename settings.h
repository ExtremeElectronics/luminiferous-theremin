#define I2C_LEFT_DEV_ADDR 0x29
#define I2C_DEFAULT_DEV_ADDR 0x29
#define I2C_RIGHT_DEV_ADDR 0x39

//speaker
#define soundIO1 6
#define soundIO2 7


//I2c Settings
#define I2CInst i2c1
#define I2C_SDA_PIN 18
#define I2C_SCL_PIN 19

//#define I2C_BAUDRATE 200000 //400Khz
#define I2C_BAUDRATE 400000 //400Khz

//TOF xshut used to change I2C addrss
#define LEFT_XSHUT 20 //gpio wired to vol xshut pin

//buttons
#define LEFT_BUTTON 17
#define RIGHT_BUTTON 16

//over clock speed
#define PICOCLOCK 250000 

//neopixel
#define PIXEL 8
