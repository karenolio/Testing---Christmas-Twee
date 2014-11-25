/* Setup shield-specific #include statements */
#include <SPI.h>
#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>
#include <Client.h>
#include <Temboo.h>
#include "TembooAccount.h" // Contains Temboo account information


#define ADAFRUIT_CC3000_IRQ 3
#define ADAFRUIT_CC3000_VBAT 5
#define ADAFRUIT_CC3000_CS 10



class TembooCC3KClient : public Client {
  public:
    TembooCC3KClient(Adafruit_CC3000& cc3k) : m_cc3k(cc3k) {
      m_cached = -1;
    }
    
    int connect(IPAddress ip, uint16_t port) {
      m_client = m_cc3k.connectTCP((uint32_t)ip, port); 
      m_cached = -1; 
      return m_client.connected()?1:0;
    }

    int connect(const char* host, uint16_t port) {
      uint32_t ip=0;
      if (m_cc3k.getHostByName(const_cast<char*>(host), &ip)) {
        m_client = m_cc3k.connectTCP(ip, port);
        m_cached = -1;
        return m_client.connected()?1:0;
      }
      return 0;
    }

    size_t write(uint8_t data) { 
      return m_client.write(data);
    }
    
    size_t write(const uint8_t* buf, size_t size) {
      return m_client.write((void*)buf, (uint16_t)size); 
    }
    
    int available() {
      return m_cached < 0 ? m_client.available():1; 
    }
    
    int read() {
      int c = m_cached < 0 ? m_client.read() : m_cached; 
      m_cached = -1; 
      return c; 
    }
    
    int read(uint8_t* buf, size_t size) {
      if (size <= 0) { return 0; }
      if (NULL == buf) { return -1; }
      if (m_cached >= 0) {
        buf[0] = (uint8_t)m_cached;
        m_cached = -1;
        buf++;
        size--;
      }
      return m_client.read((void*) buf, size);
    }

    int peek() {
      if (m_cached < 0) {
        m_cached = (available())?(read()):-1; 
      } 
      return m_cached;
    }

    void flush() {
      m_cached = -1;
      while(available()) {
        read(); 
      }
    }

    void stop() {
      while (m_client.connected()) {
        m_client.close(); 
        delay(10);
      }
      m_cached = -1;
    }
    
    uint8_t connected() {
      return (uint8_t)m_client.connected();
    }
    
    operator bool() {
      return true;
    } 
  
  private:
    Adafruit_CC3000& m_cc3k;
    Adafruit_CC3000_Client m_client;
    int16_t m_cached;
};

Adafruit_CC3000 cc3k = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT);

TembooCC3KClient client(cc3k);

//NEW CODE
int numRuns = 1;   // Execution count, so this doesn't run forever
int maxRuns = 4320;   // Maximum number of times the Choreo should be executed - 4320 is Every 10 minutes, for a month.
//int ledPin = 13; // Led for debug
//int buzzerPin = 2;  // buzzer's connected to pin 2
int lightsPin = 7; // The pin where your Xmas lights are connected to.
int timeToWait = 600000; //Delay between calls

String bodyMsgLast = "none"; // This variable holds the last text message read.
//NEW CODE

void setup() {
  Serial.begin(9600);
  
  // For debugging, wait until the serial console is connected.
  delay(4000);
  while(!Serial);

  status_t wifiStatus = STATUS_DISCONNECTED;
  while (wifiStatus != STATUS_CONNECTED) {
    Serial.print("WiFi:");
    if (cc3k.begin()) {
      if (cc3k.connectToAP("MFA IXD (a/n)", "1nter@ct10n", WLAN_SEC_WPA)) {
        wifiStatus = cc3k.getStatus();
      }
    }
    if (wifiStatus == STATUS_CONNECTED) {
      Serial.println("OK");
    } else {
      Serial.println("FAIL");
    }
    delay(5000);
  }
  
  //NEW CODE
  //Set Outputs
  //pinMode(ledPin, OUTPUT);
  pinMode(lightsPin, OUTPUT);
  //pinMode(buzzerPin, OUTPUT);


  //Turn lights off on boot up
  digitalWrite(lightsPin, HIGH);
  delay(5000); //simulates a button press for 5 seconds
  digitalWrite(lightsPin, LOW);
  delay(1000);
  
  //Console setup (Should be serial for non Yun arduinos)
  SPI.begin();
  Serial.begin();
  //  while (!Console); //Waits for Console to connect before starting. Disabled by default.
  
  //NEW CODE END

  Serial.println("Setup complete.\n");
}

void loop() {
  if (numRuns <= maxRuns) {
    Serial.println("Running GetLastMessageThatContains - Run #" + String(numRuns++));

    TembooChoreo GetLastMessageThatContainsChoreo(client);

    // Invoke the Temboo client
    GetLastMessageThatContainsChoreo.begin();

    // Set Temboo account credentials
    GetLastMessageThatContainsChoreo.setAccountName("karenolio");
    GetLastMessageThatContainsChoreo.setAppKeyName("myFirstApp");
    GetLastMessageThatContainsChoreo.setAppKey("3546218b287d4140811c2b0426b125a7");

    // Set profile to use for execution
    // DO WE STILL NEED THIS??
    //GetLastMessageThatContainsChoreo.setProfile("TwilioAccount");
    
    // Set Choreo inputs
    GetLastMessageThatContainsChoreo.addInput("AuthToken", "7919cad306355bfcebba3979f14adf2b"); //Twilio Authentication Token
    GetLastMessageThatContainsChoreo.addInput("Filter", "lights"); // Filter for incoming messages holding this word
    GetLastMessageThatContainsChoreo.addInput("AccountSID", "AC6556d2a6d561c34bf460f7a47679235e"); //Twilio account ID
    GetLastMessageThatContainsChoreo.addInput("ResponseMode", "simple"); //Response Mode

    // Set Choreo inputs //WE COMMENTED THIS OUT BC ITS ABOVE
    //String FilterValue = "lights";
    //GetLastMessageThatContainsChoreo.addInput("Filter", FilterValue);

    // Identify the Choreo to run
    GetLastMessageThatContainsChoreo.setChoreo("/Library/Twilio/SMSMessages/GetLastMessageThatContains");

    // Run the Choreo; when results are available, print them to serial
    GetLastMessageThatContainsChoreo.run();
    String bodyMsg; // This contains the whole Message
    while(GetLastMessageThatContainsChoreo.available()) {
      char c = GetLastMessageThatContainsChoreo.read();
      Serial.print(c);//c is one character at at time from the whole message. It's being printed to the console.
    }
    //GetLastMessageThatContainsChoreo.close();
    bodyMsg += c; //The characters are being fed to the bodyMsg string
  }
    
    if (bodyMsg != bodyMsgLast) { //Only runs if this message is different than the one stored.
      if (bodyMsg.substring(17, 19) == "on") { //This only works if the 7th to 9 letters are "on"".
        // This works if you're seinding the message "Lights on"
        // Characters before Lights on are other info from Twilio


        // Turn lights on
        //digitalWrite(ledPin, HIGH); //turns on debug LED
        digitalWrite(lightsPin, HIGH);
        delay(800);
        digitalWrite(lightsPin, LOW); //Simulated button press for less than a second
        Console.println("Lights are on");
        //tone(buzzerPin, 2000, 3000); //beeps for 3 seconds
      }  else if (bodyMsg.substring (17, 20) == "off") { //reads "off" from a message saying "Lights off"
        //digitalWrite(ledPin, LOW); //turns off debug LED
        //tone(buzzerPin, 4200, 1000); //beeps
        digitalWrite(lightsPin, HIGH);
        delay(5000); //simulates a 5 second button press to turn the lights off
        digitalWrite(lightsPin, LOW);
        delay(1000);
        Console.println("Lights are off");


      }


      bodyMsgLast = bodyMsg; //Copies this message to the Last message variable
    } else {
      Serial.println("Identical to Last"); //if identical, do nothing. 
    }
      Serial.println();
      Serial.println("Waiting...");
      delay(timeToWait); // wait a period between GetLastMessageThatContains calls


  } else {
    Serial.println ("Done. Restart me for another run");
    abort();
  }

    
  //Serial.println("\nWaiting...\n");
  //delay(30000); // wait 30 seconds between GetLastMessageThatContains calls
}
