#define I2C_VOL_DEV_ADDR 0x29
#define I2C_DEFAULT_DEV_ADDR 0x29
#define I2C_FREQ_DEV_ADDR 0x39

//I2c Settings
#define I2CInst i2c1
#define I2C_SDA_PIN 18
#define I2C_SCL_PIN 19

//#define I2C_BAUDRATE 200000 //400Khz
#define I2C_BAUDRATE 400000 //400Khz

#define VOL_XSHUT 20 //gpio wired to vol xshut pin
#define BUTTON1 15
#define BUTTON2 14

//over clock speed
#define PICOCLOCK 250000 