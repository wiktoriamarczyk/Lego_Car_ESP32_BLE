#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Motor control digital output pins defined as global constants (4 wheel drive with 2 Lego motors):
const int ServoPin1    = 4;                  // L293D driver input 3A on pin no 10 connected to ESP32 digital output pin 4
const int ServoPin2    = 0;                  // L293D driver input 4A on pin no 15 connected to ESP32 digital output pin 0
const int ServoEnable  = 18;                 // L293D ENable(3,4) input on pin no 9 connected to ESP32 digital output pin 19
const int ServoChannel = 0;

const int MotorPin1    = 16;				 // L293D driver input 1A on pin no 2 connected to ESP32 digital output pin 16
const int MotorPin2    = 17;				 // L293D driver input 2A on pin no 7 connected to ESP32 digital output pin 17
const int MotorEnable  = 19;				 // L293D ENable(1,2) input on pin no 1 connected to ESP32 digital output pin 19
const int MotorChannel = 1; 

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" 

BLECharacteristic* pCharacteristic = 0;

MyCallback gCallback;

// Own BLE Callback class
class MyCallback : public BLECharacteristicCallbacks
{
  // Struct with motor data containing numbers of connected pins 
  struct MotorData
  {
      int Pin1    = 0;
      int Pin2    = 0;
      int Channel = 0;
  };
  // We have 2 motors 
  MotorData m_Motors[2];
  
public:
  // Initialization of structure fields with motor data
  MyCallback()
  {
      m_Motors[0].Pin1 = MotorPin1;
      m_Motors[0].Pin2 = MotorPin2;
      m_Motors[0].Channel = MotorChannel;
      m_Motors[1].Pin1 = ServoPin1;
      m_Motors[1].Pin2 = ServoPin2;
      m_Motors[1].Channel = ServoChannel;
  }

   // Value of motor tilt angle sent via bluetooth from the smartphone application
   signed char ValueX = 0;
   // Motor speed value sent via bluetooth
   signed char ValueY = 0;
   // Counter responsible for resetting the position of the car when no values were sent to the microcontroller for the specified time
   int         ResetCounter = -1;
   
   // Function in which the position of the car is updated depending on the values sent from the application on the smartphone
  void WriteToMotor(char MotorIndex , signed char Val)
  {
	// Values sent from the app are between -9 and 9 
    if( Val < -9 || Val > 9 )
      return;
    // Change the direction of the car's movement or the direction of the car's tilt depending on the sign of the sent value
    if( Val < 0 )
    {
      Val = -Val;
      
      digitalWrite(m_Motors[MotorIndex].Pin1, HIGH);
      digitalWrite(m_Motors[MotorIndex].Pin2, LOW);
    }
    else
    {
      digitalWrite(m_Motors[MotorIndex].Pin1, LOW);
      digitalWrite(m_Motors[MotorIndex].Pin2, HIGH);
    }
    // Convert the value sent from the smartphone app to be between 0 and 255
    ledcWrite(m_Motors[MotorIndex].Channel,  (Val*256)/9 );
  }
  
  // Function responsible for updating car speed and lean angle
  void Update()
  {
	// If a new value has not been sent from the smartphone application for a given time, reset the car's position
    if( ResetCounter >= 0 )
    {
        --ResetCounter;
        if( ResetCounter == 0 )
        {
          ValueX = 0;
          ValueY = 0;
          ResetCounter = -1;
        }
    }

      Serial.printf( "Data -> M:%d | S:%d\n" , ValueX , ValueY);
      WriteToMotor( 1 , ValueX );
      WriteToMotor( 0 , ValueY );
  }

    // Function called when values have been sent from the application on the smartphone via bluetooth
    virtual void onWrite(BLECharacteristic* pCharacteristic)override
    {
		std::string s = pCharacteristic->getValue();
		// 
		if( s.size()<2 )
			return;
		
		// Convert values sent as characters to numeric values and update car speed and lean angle
		ValueX = s[0]-'0';
		ValueY = s[1]-'0';
		// Reset the counter
		ResetCounter = 6;
		// Update car speed and lean angle
		Update();
    }
};

void setup() {
  Serial.begin(9600);
  Serial.println("LEGO ESP says Hi!");

  // Create the BLE Device with the BLE server name
  BLEDevice::init("WIKI_LEGO");
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  // Start a BLE service with the service UUID defined earlier.
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create BLE characteristic
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  // Setup callbacks
  pCharacteristic->setCallbacks( &gCallback );
  // Start the service
  pService->start();
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  // Functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");

   // Servo Setup
   // 1 arg - PWM channel
   // 2 arg - PWM signal frequency
   // 3 arg - signal's duty cycle resolution: 8-bit resolution means we can control motor tilt angle using a value from 0 to 255
   ledcSetup(ServoChannel, 2000, 8);
   // Attach the channel to the GPIO to be controlled
   // 1 arg - number of GPIO pin that will output the signal
   // 2 arg - channel that will generate the signal
   ledcAttachPin(ServoEnable, ServoChannel);

   // Setting pins as output
   pinMode(ServoPin1, OUTPUT);            
   pinMode(ServoPin2, OUTPUT);  
   // Servo PWM   
   pinMode(ServoEnable, OUTPUT);          

   // Servo pins initialization
   digitalWrite(ServoEnable, LOW);
   digitalWrite(ServoPin1, LOW);
   digitalWrite(ServoPin2, LOW);

   // Motor Setup
   ledcSetup(MotorChannel, 2000, 8);
   ledcAttachPin(MotorEnable, MotorChannel);

    // Setting pins as output
   pinMode(MotorPin1, OUTPUT);           
   pinMode(MotorPin2, OUTPUT); 
   // Motor PWM   
   pinMode(MotorEnable, OUTPUT);

   // Motor pins initialization
   digitalWrite(MotorEnable, LOW);
   digitalWrite(MotorPin1, LOW);
   digitalWrite(MotorPin2, LOW);
      
   ledcWrite(MotorChannel,  0 );
}

void loop() {
  delay(100);
  // Update car speed and lean angle
  gCallback.Update();
}
