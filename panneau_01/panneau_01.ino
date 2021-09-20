/*
   ----------------------------------------------------------------------------
   TECHNOLARP - https://technolarp.github.io/
   VU-METRE 01 - https://github.com/technolarp/panneau_01
   version 1.0 - 09/2021
   ----------------------------------------------------------------------------
*/

/*
   ----------------------------------------------------------------------------
   Pour ce montage, vous avez besoin de 
   24 leds neopixel
   ----------------------------------------------------------------------------
*/

/*
   ----------------------------------------------------------------------------
   PINOUT
   D0     NEOPIXEL
   ----------------------------------------------------------------------------
*/

#include <Arduino.h>

// WIFI
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// WEBSOCKET
AsyncWebSocket ws("/ws");

// TASK SCHEDULER
#define _TASK_OO_CALLBACKS
#include <TaskScheduler.h>
Scheduler globalScheduler;

// LED RGB
#include <technolarp_fastled.h>
M_fastled* aFastled;

// CONFIG
#include "config.h"
M_config aConfig;

// DIVERS
bool uneFois = true;
bool blinkUneFois = true;

// STATUTS
enum {
  PANNEAU_ACTIF = 0,
  PANNEAU_BLINK = 1
};

uint8_t statutPanneau = PANNEAU_ACTIF;

// HEARTBEAT
unsigned long int previousMillisHB;
unsigned long int currentMillisHB;
unsigned long int intervalHB;

// REFRESH
unsigned long int previousMillisRefresh;
unsigned long int currentMillisRefresh;
unsigned long int intervalRefresh;


/*
   ----------------------------------------------------------------------------
   SETUP
   ----------------------------------------------------------------------------
*/
void setup()
{
  Serial.begin(115200);

  // VERSION
  delay(500);
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F("----------------------------------------------------------------------------"));
  Serial.println(F("TECHNOLARP - https://technolarp.github.io/"));
  Serial.println(F("PANNEAU 01 - https://github.com/technolarp/panneau_01"));
  Serial.println(F("version 1.0 - 09/2021"));
  Serial.println(F("----------------------------------------------------------------------------"));
  
  // CONFIG OBJET
  Serial.println(F(""));
  Serial.println(F(""));
  aConfig.mountFS();
  aConfig.listDir("/");
  aConfig.listDir("/config");
  aConfig.listDir("/www");
  
  Serial.println(F("OBJECT CONFIG"));
  aConfig.printJsonFile("/config/objectconfig.txt");
  aConfig.readObjectConfig("/config/objectconfig.txt");

  Serial.println(F("NETWORK CONFIG"));
  aConfig.printJsonFile("/config/networkconfig.txt");
  aConfig.readNetworkConfig("/config/networkconfig.txt");

// LED RGB
  aFastled = new M_fastled(&globalScheduler);
  aFastled->setNbLed(aConfig.objectConfig.activeLeds);
  aFastled->setBrightness(aConfig.objectConfig.brightness);

  // animation led de depart  
  aFastled->allLedOff();
  for (int i = 0; i < aConfig.objectConfig.activeLeds * 2; i++)
  {
    aFastled->ledOn(i % aConfig.objectConfig.activeLeds, CRGB::Blue);
    delay(50);
    aFastled->ledOn(i % aConfig.objectConfig.activeLeds, CRGB::Black);
  }
  aFastled->allLedOff();
  
  // WIFI
  WiFi.disconnect(true);
  
  /*
  // AP MODE
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(aConfig.networkConfig.apIP, aConfig.networkConfig.apIP, aConfig.networkConfig.apNetMsk);
  WiFi.softAP(aConfig.networkConfig.apName, aConfig.networkConfig.apPassword);
  
  */
  // CLIENT MODE POUR DEBUG
  const char* ssid = "MYDEBUG";
  const char* password = "xxxxxxxxx";
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.printf("WiFi Failed!\n");        
  }
  /**/
  
  // WEB SERVER
  // Print ESP Local IP Address
  Serial.print(F("localIP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("softAPIP: "));
  Serial.println(WiFi.softAPIP());

  // Route for root / web page
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("config.html").setTemplateProcessor(processor);
  server.serveStatic("/config", LittleFS, "/config/");
  server.onNotFound(notFound);

  // WEBSOCKET
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Start server
  server.begin();

  // HEARTBEAT
  currentMillisHB = millis();
  previousMillisHB = currentMillisHB;
  intervalHB = 5000;

  previousMillisRefresh = currentMillisHB;
  currentMillisRefresh = currentMillisHB;
  intervalRefresh = 50;

  // SERIAL
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F("START !!!"));
}
/*
   ----------------------------------------------------------------------------
   FIN DU SETUP
   ----------------------------------------------------------------------------
*/




/*
   ----------------------------------------------------------------------------
   LOOP
   ----------------------------------------------------------------------------
*/
void loop() 
{
// avoid watchdog reset
  yield();
  
  // WEBSOCKET
  ws.cleanupClients();
  
  // manage task scheduler
  globalScheduler.execute();

  // gerer le statut de la serrure
  switch (statutPanneau)
  {
    case PANNEAU_ACTIF:
      // la serrure est fermee
      panneauActif();
      break;

    case PANNEAU_BLINK:
      // blink led pour identification
      panneauBlink();
      break;
      
    default:
      // nothing
      break;
  }

  // HEARTBEAT
  currentMillisHB = millis();
  if(currentMillisHB - previousMillisHB > intervalHB)
  {
    previousMillisHB = currentMillisHB;

    // send new value to html
    sendUptime();
        
    Serial.println("heartbeat");
  }
}
/*
   ----------------------------------------------------------------------------
   FIN DU LOOP
   ----------------------------------------------------------------------------
*/





/*
   ----------------------------------------------------------------------------
   FONCTIONS ADDITIONNELLES
   ----------------------------------------------------------------------------
*/
String processor(const String& var)
{  
  if (var == "MAXLEDS")
  {
    return String(aFastled->getNbMaxLed());
  }

  if (var == "APNAME")
  {
    return String(aConfig.networkConfig.apName);
  }

  if (var == "OBJECTNAME")
  {
    return String(aConfig.objectConfig.objectName);
  }
   
  return String();
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
   switch (type) 
    {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        // send config value to html
        ws.textAll(aConfig.stringJsonFile("/config/objectconfig.txt"));
        ws.textAll(aConfig.stringJsonFile("/config/networkconfig.txt"));
    
        break;
        
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
        
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
        
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
  
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
  {
    char buffer[100];
    data[len] = 0;
    sprintf(buffer,"%s\n", (char*)data);
    Serial.print(buffer);
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, buffer);
    if (error)
    {
      Serial.println(F("Failed to deserialize buffer"));
    }
    else
    {
      
        // write config or not
        bool writeObjectConfig = false;
        bool sendObjectConfig = false;
        bool writeNetworkConfig = false;
        bool sendNetworkConfig = false;
        
        // modif object config
        if (doc.containsKey("new_objectName"))
        {
          strlcpy(  aConfig.objectConfig.objectName,
                    doc["new_objectName"],
                    sizeof(aConfig.objectConfig.objectName));
  
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        if (doc.containsKey("new_objectId")) 
        {
          uint16_t tmpValeur = doc["new_objectId"];
          aConfig.objectConfig.objectId = checkValeur(tmpValeur,1,1000);
  
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        if (doc.containsKey("new_groupId")) 
        {
          uint16_t tmpValeur = doc["new_groupId"];
          aConfig.objectConfig.groupId = checkValeur(tmpValeur,1,1000);
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        if (doc.containsKey("new_activeLeds")) 
        {
          aFastled->allLedOff();
          
          uint16_t tmpValeur = doc["new_activeLeds"];
          aConfig.objectConfig.activeLeds = checkValeur(tmpValeur,1,aFastled->getNbMaxLed());
          aFastled->setNbLed(aConfig.objectConfig.activeLeds);
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        if (doc.containsKey("new_brightness"))
        {
          uint16_t tmpValeur = doc["new_brightness"];
          aConfig.objectConfig.brightness = checkValeur(tmpValeur,0,255);
          aFastled->setBrightness(aConfig.objectConfig.brightness);
          aFastled->ledShow();
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        /*
        if (doc.containsKey("new_seuil1")) 
        {
          uint16_t tmpValeur = doc["new_seuil1"];
          aConfig.objectConfig.seuil1 = checkValeur(tmpValeur,1,aConfig.objectConfig.activeLeds);
          
          writeObjectConfig = true;
          sendObjectConfig = true;
          uneFois=true;
        }
  
        if (doc.containsKey("new_seuil2")) 
        {
          uint16_t tmpValeur = doc["new_seuil2"];
          aConfig.objectConfig.seuil2 = checkValeur(tmpValeur,1,aConfig.objectConfig.activeLeds);
          
          writeObjectConfig = true;
          sendObjectConfig = true;
          uneFois=true;
        }
        */

        if (doc.containsKey("new_couleur1")) 
        {
          String newColorStr = doc["new_couleur1"];
          convertStrToRGB(newColorStr,&aConfig.objectConfig.couleur1.red, &aConfig.objectConfig.couleur1.green, &aConfig.objectConfig.couleur1.blue);
          
          writeObjectConfig = true;
          sendObjectConfig = true;
          uneFois=true;
        }

        if (doc.containsKey("new_couleur2")) 
        {
          String newColorStr = doc["new_couleur2"];
          convertStrToRGB(newColorStr,&aConfig.objectConfig.couleur2.red, &aConfig.objectConfig.couleur2.green, &aConfig.objectConfig.couleur2.blue);
           
          writeObjectConfig = true;
          sendObjectConfig = true;
          uneFois=true;
        }

        if (doc.containsKey("new_couleur3")) 
        {
          String newColorStr = doc["new_couleur3"];
          convertStrToRGB(newColorStr,&aConfig.objectConfig.couleur3.red, &aConfig.objectConfig.couleur3.green, &aConfig.objectConfig.couleur3.blue);
          
          writeObjectConfig = true;
          sendObjectConfig = true;
          uneFois=true;
        }

        if (doc.containsKey("new_bouton")) 
        {
          JsonArray newBouton = doc["new_bouton"];
        
          uint8_t nouvellePosition = newBouton[0];
          uint8_t nouvelleCouleur = newBouton[1];
          
          aConfig.objectConfig.labelCouleur[nouvellePosition]=nouvelleCouleur;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
          uneFois=true;
        }

        /*
        if (doc.containsKey("new_labelNom")) 
        {
          JsonArray newLabel = doc["new_labelNom"];
        
          uint8_t nouvellePosition = newLabel[0];

          strlcpy(  aConfig.objectConfig.labelNom[nouvellePosition],
                    newLabel[1],
                    sizeof(aConfig.objectConfig.labelNom[nouvellePosition]));
          
          //writeObjectConfig = true;
          sendObjectConfig = true;
          uneFois=true;
        }*/
          
        // modif network config
        if (doc.containsKey("new_apName")) 
        {
          strlcpy(  aConfig.networkConfig.apName,
                    doc["new_apName"],
                    sizeof(aConfig.networkConfig.apName));
  
          // check for unsupported char
          checkCharacter(aConfig.networkConfig.apName, "ABCDEFGHIJKLMNOPQRSTUVWYZ", 'A');
          
          writeNetworkConfig = true;
          sendNetworkConfig = true;
        }
  
        if (doc.containsKey("new_apPassword")) 
        {
          strlcpy(  aConfig.networkConfig.apPassword,
                    doc["new_apPassword"],
                    sizeof(aConfig.networkConfig.apPassword));
  
          writeNetworkConfig = true;
        }
  
        if (doc.containsKey("new_apIP")) 
        {
          char newIPchar[16] = "";
  
          strlcpy(  newIPchar,
                    doc["new_apIP"],
                    sizeof(newIPchar));
  
          IPAddress newIP;
          if (newIP.fromString(newIPchar)) 
          {
            Serial.println("valid IP");
            aConfig.networkConfig.apIP = newIP;
  
            writeNetworkConfig = true;
          }
          
          sendNetworkConfig = true;
        }
  
        if (doc.containsKey("new_apNetMsk")) 
        {
          char newNMchar[16] = "";
  
          strlcpy(  newNMchar,
                    doc["new_apNetMsk"],
                    sizeof(newNMchar));
  
          IPAddress newNM;
          if (newNM.fromString(newNMchar)) 
          {
            Serial.println("valid netmask");
            aConfig.networkConfig.apNetMsk = newNM;
  
            writeNetworkConfig = true;
          }
  
          sendNetworkConfig = true;
        }
        
        // actions sur le esp8266
        if ( doc.containsKey("new_restart") && doc["new_restart"]==1 )
        {
          Serial.println(F("RESTART RESTART RESTART"));
          ESP.restart();
        }
  
        if ( doc.containsKey("new_refresh") && doc["new_refresh"]==1 )
        {
          Serial.println(F("REFRESH"));
          sendObjectConfig = true;
          sendNetworkConfig = true;
        }

        if ( doc.containsKey("new_defaultObjectConfig") && doc["new_defaultObjectConfig"]==1 )
        {
          Serial.println(F("reset to default object config"));
          aConfig.writeDefaultObjectConfig("/config/objectconfig.txt");
          
          sendObjectConfig = true;
          uneFois = true;
        }

        if ( doc.containsKey("new_defaultNetworkConfig") && doc["new_defaultNetworkConfig"]==1 )
        {
          Serial.println(F("reset to default network config"));
          aConfig.writeDefaultNetworkConfig("/config/networkconfig.txt");
          
          sendNetworkConfig = true;
        }

        if ( doc.containsKey("new_blink") && doc["new_blink"]==1 )
        {
          Serial.println(F("BLINK"));
          blinkUneFois = true;
          statutPanneau = PANNEAU_BLINK;
        }
  
        // modif config
        // write object config
        if (writeObjectConfig)
        {
          aConfig.writeObjectConfig("/config/objectconfig.txt");
          //aConfig.printJsonFile("/config/objectconfig.txt");
        }
  
        // resend object config
        if (sendObjectConfig)
        {
          ws.textAll(aConfig.stringJsonFile("/config/objectconfig.txt"));
        }
  
        // write network config
        if (writeNetworkConfig)
        {
          aConfig.writeNetworkConfig("/config/networkconfig.txt");
          //aConfig.printJsonFile("/config/networkconfig.txt");
        }
  
        // resend network config
        if (sendNetworkConfig)
        {
          ws.textAll(aConfig.stringJsonFile("/config/networkconfig.txt"));
        }
      
    }
 
    // clear json buffer
    doc.clear();
  }
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void checkCharacter(char* toCheck, char* allowed, char replaceChar)
{
  for (int i = 0; i < strlen(toCheck); i++)
  {
    if (!strchr(allowed, toCheck[i]))
    {
      toCheck[i]=replaceChar;
    }
    Serial.print(toCheck[i]);
  }
  Serial.println("");
}

uint16_t checkValeur(uint16_t valeur, uint16_t minValeur, uint16_t maxValeur)
{
  return(min(max(valeur,minValeur), maxValeur));
}

void sendUptime()
{
  uint32_t now = millis() / 1000;
  uint16_t days = now / 86400;
  uint16_t hours = (now%86400) / 3600;
  uint16_t minutes = (now%3600) / 60;
  uint16_t seconds = now % 60;
    
  String toSend = "{\"uptime\":\"";
  toSend+= String(days) + String("d ") + String(hours) + String("h ") + String(minutes) + String("m ") + String(seconds) + String("s");
  toSend+= "\"}";

  ws.textAll(toSend);
}

void convertStrToRGB(String source, uint8_t* r, uint8_t* g, uint8_t* b)
{
  uint32_t  number = (uint32_t) strtol( &source[1], NULL, 16);
  
  // Split them up into r, g, b values
  *r = number >> 16;
  *g = number >> 8 & 0xFF;
  *b = number & 0xFF;
}
/*
   ----------------------------------------------------------------------------
   FIN DES FONCTIONS ADDITIONNELLES
   ----------------------------------------------------------------------------
*/


void panneauActif()
{
  currentMillisRefresh = millis();
  if(currentMillisRefresh - previousMillisRefresh > intervalRefresh)
  {
    previousMillisRefresh = currentMillisRefresh;

    for (uint8_t i=0;i<aConfig.objectConfig.nombreLabel;i++)
    {
      for (uint8_t j=0;j<aConfig.objectConfig.ledParLabel;j++)
      {
        if (aConfig.objectConfig.labelCouleur[i]==1)
        {
          aFastled->setLed(i*aConfig.objectConfig.ledParLabel+j, aConfig.objectConfig.couleur1);
        }
        else if (aConfig.objectConfig.labelCouleur[i]==2)
        {
          aFastled->setLed(i*aConfig.objectConfig.ledParLabel+j, aConfig.objectConfig.couleur2);
        }
        else
        {
          aFastled->setLed(i*aConfig.objectConfig.ledParLabel+j, aConfig.objectConfig.couleur3);
        }
      }
      
      
    }
    aFastled->ledShow();
  }
  
  
}

void panneauBlink()
{
  if (!aFastled->isEnabled() && blinkUneFois)
  {
    blinkUneFois = false;
    aFastled->startAnimBlink(15, 200, CRGB::Blue, aConfig.objectConfig.activeLeds);
  }

  if (!aFastled->isAnimActive())
  {
    Serial.println(F("END TASK BLINK"));

    //blinkUneFois = true;
    uneFois = true;

    // retour au statut precedent
    statutPanneau = PANNEAU_ACTIF;
  }
}
