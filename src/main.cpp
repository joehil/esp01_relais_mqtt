#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define E2 0

// Update these with values suitable for your network.

const char* ssid = "<ssid>";
const char* password = "<password>";
const char* mqtt_server = "<mqtt server>";

const char* clientId = "espRelais1";

WiFiClient espClient;
PubSubClient client(espClient);
void eventWiFi(WiFiEvent_t event);

char statrelais[4];
bool msgrec = true;
int count = 0;
char msg[50];
int ev2cnt = 0;
long cnt = 0;

void setup_wifi() {
  int loopcnt = 0;
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.setAutoReconnect(false);
  WiFi.onEvent(eventWiFi); 
  WiFi.hostname("espRelais1");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    loopcnt++;
    if (loopcnt > 20){
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/********************************************************
/*  Handle WiFi events                                  *
/********************************************************/
void eventWiFi(WiFiEvent_t event) {
     
  switch(event) {
    case WIFI_EVENT_STAMODE_CONNECTED:
      Serial.println("EV1");
    break;
    
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      Serial.println("EV2");
      WiFi.reconnect();
      ev2cnt++;
      if (ev2cnt > 20) 
         ESP.restart();
    break;
    
    case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
      Serial.println("EV3");
    break;
    
    case WIFI_EVENT_STAMODE_GOT_IP:
      Serial.println("EV4");
    break;
    
    case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
      Serial.println("EV5");
      ESP.restart();
    break;
    
    case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
      Serial.println("EV6");
    break;
    
    case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
      Serial.println("EV7");
      ESP.restart();
    break;
    
    case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
      Serial.println("EV8");
    break;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String tmp=topic;

  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;

  Serial.print("mqtt_callback topic=");
  Serial.println(tmp);
  Serial.println(message);
  
  if(tmp.indexOf("inTopic")>=0){
    if (message == "on"){
      Serial.println("relay on");
      digitalWrite(E2, 0);
      sprintf(statrelais,"%s","on");
      msgrec = true;
    }
    if (message == "off"){
      Serial.println("relay off");
      digitalWrite(E2, 1);
      sprintf(statrelais,"%s","off");
      msgrec = true;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  int loopcnt = 0;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // Attempt to connect
    if (client.connect(clientId)) {
      loopcnt = 0;
      Serial.println("connected");
      // Once connected, publish an announcement...
      strcat(msg,"/outTopic/IP");
      client.publish(msg, WiFi.localIP().toString().c_str());
      // ... and resubscribe
      strcpy(msg,clientId);
      strcat(msg,"/inTopic");
      client.subscribe(msg);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      loopcnt++;
      if (loopcnt > 20){
        if (WiFi.status() != WL_CONNECTED) {
          ESP.restart();
        }
      }
    }
  }
}

void setup() {
  pinMode(E2, OUTPUT);

  delay(500);

  digitalWrite(E2, 1);
  
  sprintf(statrelais,"%s","off");

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
    count=0;
  }
  client.loop();

  if (WiFi.status() != WL_CONNECTED){
    ESP.restart();
  }
 
  if (msgrec||(cnt > 6000000)){
    char* topic = "/outTopic/state";
    char* path = (char *) malloc(1 + strlen(clientId) + strlen(topic) + 20);
    strcpy(path, clientId);
    strcat(path, topic);
    client.publish(path, statrelais);
    msgrec = false;
    cnt = 0;
  }
  cnt++;
}
