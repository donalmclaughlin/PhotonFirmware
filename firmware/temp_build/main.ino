#include "ArduCAM.h"
#include "memorysaver.h"
SYSTEM_THREAD(ENABLED);

#define VERSION_SLUG "7n"

//Connect to TCP Client Server on PC
TCPClient client;

//Elastic IP
#define SERVER_ADDRESS "54.85.56.7"

#define SERVER_TCP_PORT 5550

#define TX_BUFFER_MAX 1024

uint8_t buffer[TX_BUFFER_MAX + 1];
int tx_buffer_index = 0;
int led = D7;           //Onboard led
int ledouter = A0;      //LED outer, on if image capturing is in progress
int button = D4;        //Push Button Switch
int illled1 = D3;       //Illumination LED 1
int illled2 = D2;       //Illumination LED 2
int conled = A1;        //Connection LED - On if cant connect to TCP server
int buttonpush = 0;     

// set pin A2 as the slave select for the ArduCAM shield
const int SPI_CS = A2;

ArduCAM myCAM(OV5640, SPI_CS);

//----------------------------------------------SETUP----------------------------------------------//
void setup()
{
pinMode(led, OUTPUT);
pinMode(ledouter, OUTPUT);
pinMode(illled1, OUTPUT);
pinMode(illled2,OUTPUT);
pinMode(conled, OUTPUT);
pinMode(button, INPUT_PULLUP);
  Particle.publish("status", "Good morning, Version: " + String(VERSION_SLUG));
  delay(1000);

  uint8_t vid,pid;
  uint8_t temp;

  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();

  Serial.begin(115200);
  Serial.println("ArduCAM Start!");

  // set the SPI_CS as an output:
  pinMode(SPI_CS, OUTPUT);

  // Initialise Serial Peripheral Interface (SPI)
  SPI.begin();


  while(1) {
    Particle.publish("status", "checking for camera");
    Serial.println("Checking for camera...");

    //Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if(temp != 0x55)
    {
      Serial.println("SPI interface Error!");
      Serial.println("myCam.read_reg said " + String(temp));
      delay(5000);
    }
    else {
      break;
    }
    Particle.process();
  }

    Particle.publish("status", "Camera found.");


while(1){
  //Check if the camera module type is OV5640
  myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
/*   if ((vid != 0x56) || (pid != 0x42)){
    Serial.println(F("Can't find OV5640 module!"));
    Particle.publish("status", "Not found, camera says " + String::format("%d:%d", vid, pid));
    delay(5000);
    continue;
  } 
  else {*/
    Serial.println(F("OV5640 detected."));
    Particle.publish("status", "OV5640 detected: " + String::format("%d:%d", vid, pid));
    break;
 // }
}


  Serial.println("Camera found, initializing...");
    //myCAM.write_reg(ARDUCHIP_MODE, 0x01);		 	//Switch to CAM

    //Change MCU mode
    myCAM.set_format(JPEG);
    delay(100);

    myCAM.InitCAM();
    delay(100);

//    myCAM.set_format(JPEG);
//    delay(100);

    myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
    //myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
    delay(100);

    myCAM.clear_fifo_flag();
    delay(100);

    myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
    delay(100);

    myCAM.OV5640_set_JPEG_size(OV5640_320x240);
  

    // wait 1 second
    delay(1000);


    client.connect(SERVER_ADDRESS, SERVER_TCP_PORT);
}


//----------------------------------------------LOOP----------------------------------------------//

void loop()
{
    if (!client.connected()) {
        //client.stop();
        Particle.publish("status", "Attempting to reconnect to TCP Server...");
        if (!client.connect(SERVER_ADDRESS, SERVER_TCP_PORT)) {
            digitalWrite(conled, HIGH);
            delay(1000);
            return;
        }
    }
    digitalWrite(conled, LOW);
int buttonState = digitalRead(button);
int imgcount = 0;
if (buttonState == LOW){
    buttonpush = 0;
    digitalWrite(ledouter, HIGH);
    // digitalWrite(illled1, HIGH);
    // digitalWrite(illled2, HIGH);
    analogWrite(illled1, 1);
    analogWrite(illled2, 1);
   // analogWrite(illled1, 4096);

    Particle.publish("status", "Taking a picture...");
    Serial.println("Taking a picture...");
   digitalWrite(led, HIGH);

//CHANGE IMAGE RESOLUTION-------------------------------------------------
	//myCAM.OV5640_set_JPEG_size(OV5640_320x240);   //works
    //myCAM.OV5640_set_JPEG_size(OV5640_1600x1200); //doesn't work
    //myCAM.OV5640_set_JPEG_size(OV5640_1280x960);  // doesn't work?
    //myCAM.OV5640_set_JPEG_size(OV5640_640x480);    // ?

    myCAM.OV5640_set_JPEG_size(OV5640_2592x1944); //works
//------------------------------------------------------------------------
    delay(100);

    myCAM.flush_fifo();
    delay(100);

    myCAM.clear_fifo_flag();
    delay(100);

    myCAM.start_capture();
    delay(100);


    unsigned long start_time = millis(),
                  last_publish = millis();


//
//  wait for the photo to be done
//
    while(!myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK)) {
        Particle.process();
        delay(10);

        unsigned long now = millis();
        if ((now - last_publish) > 1000) {
            Particle.publish("status", "waiting for photo " + String(now-start_time));
            last_publish = now;
        }

        if ((now-start_time) > 30000) {
            Particle.publish("status", "bailing...");
            break;
        }
    }
    delay(100);

    int length = myCAM.read_fifo_length();
    Particle.publish("status", "Image size is " + String(length));
    Serial.println("Image size is " + String(length));

    uint8_t temp = 0xff, temp_last = 0;
    int bytesRead = 0;



    if(myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
    {
        delay(100);
        Serial.println(F("ACK CMD CAM Capture Done."));
        Particle.publish("status", "Capture done");


        myCAM.set_fifo_burst();



        tx_buffer_index = 0;
        temp = 0;
/*
        //while (bytesRead < length)
        while(length){
            temp_last = temp;
            temp = myCAM.read_fifo()
            if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
        {
            buffer[tx_buffer_index++] = temp;
            client.write(buffer, tx_buffer_index);
                Particle.publish("Writing to server");
                tx_buffer_index = 0;
                //Run background loop to maintain connection with cloud
                Particle.process();
        }

        }
*/

        while( (temp != 0xD9) | (temp_last != 0xFF) )
        {
            temp_last = temp;//
            temp = myCAM.read_fifo();//
            bytesRead++;


            buffer[tx_buffer_index++] = temp;//
            //tx_buffer_index >= 1024
            if (tx_buffer_index >= TX_BUFFER_MAX) {
              //  if (buttonpush ==0){
             //   if(imgcount==0){
                client.write(buffer, tx_buffer_index);
                Particle.publish("Writing to server");
            //Particle.publish(buffer.length);
                tx_buffer_index = 0;
                //Run background loop to maintain connection with cloud
                Particle.process();
                // imgcount=1;
                // buttonpush = 1;
                // }else{
                //   digitalWrite(conled, HIGH);
                //   delay(10000);
                //   digitalWrite(conled, LOW);
                //   break;
                // }
           //     }
                // else{
                //     digitalWrite(conled, HIGH);
                //     delay(3000);
                //     digitalWrite(conled, LOW);
                //     break;
                // }
            }

            if (bytesRead > 2048000) {
                // failsafe
                break;
            }
        }


        if (tx_buffer_index != 0) {
            client.write(buffer, tx_buffer_index);
            Particle.publish("Writing to server small if");
        }

        //Clear the capture done flag
        myCAM.clear_fifo_flag();

        Serial.println(F("End of Photo"));
    }

    //Serial.println("sleeping 10 seconds");
    //Particle.publish("status", "Sleeping 10 seconds");
    digitalWrite(ledouter, LOW);
    digitalWrite(led, LOW);
//    digitalWrite(illled1, LOW);
//    digitalWrite(illled2, LOW);
    analogWrite(illled1, 0);
    analogWrite(illled2, 0);
    buttonState = HIGH;
   // delay(10 * 1000);
}else{
    digitalWrite(ledouter, LOW);
}
}
