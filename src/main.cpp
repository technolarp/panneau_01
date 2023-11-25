/*
   ----------------------------------------------------------------------------
   TECHNOLARP - https://technolarp.github.io/
   PANNEAU 01 - https://github.com/technolarp/panneau_01
   version 1.0 - 09/2023
   ----------------------------------------------------------------------------
*/

/*
   ----------------------------------------------------------------------------
   Pour ce montage, vous avez besoin de 
   8 ou + leds neopixel
   ----------------------------------------------------------------------------
*/

/*
   ----------------------------------------------------------------------------
   PINOUT
   A0     POTENTIOMETRE
   ----------------------------------------------------------------------------
*/

/*
TODO LIST

version 1.1
ajouter un mode blink aleatoire
ajouter un blink individuel sur chaque groupe
ajouter type led matrice et ring
ajouter sens matrice led
ajouter boutons pour commander le panneau
si reduction nbCouleur, faire uen passe sur les couleurs
option pour extinction aléatoire
*/

#include <Arduino.h>

// WIFI
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// WEBSOCKET
AsyncWebSocket ws("/ws");

char bufferWebsocket[300];
bool flagBufferWebsocket = false;

// CONFIG
#include "config.h"
M_config aConfig;

#define BUFFERSENDSIZE 1000
char bufferToSend[BUFFERSENDSIZE];

// FASTLED
#include <technolarp_fastled.h>
M_fastled aFastled;

// DIVERS
bool uneFois = true;

// STATUTS DE L OBJET
enum {
  OBJET_ACTIF = 0,
  OBJET_BLINK = 5
};

// REFRESH
uint32_t previousMillisRefresh;
uint32_t intervalRefresh;

// HEARTBEAT
uint32_t previousMillisHB;
uint32_t intervalHB;

// FUNCTION DECLARATIONS
uint16_t checkValeur(uint16_t valeur, uint16_t minValeur, uint16_t maxValeur);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len); 
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len); 
void handleWebsocketBuffer();
void notFound(AsyncWebServerRequest *request);
void checkCharacter(char* toCheck, const char* allowed, char replaceChar);
void sendUptime();
void sendMaxLed();
void sendStatut();
void sendObjectConfig();
void writeObjectConfig();
void sendNetworkConfig();
void writeNetworkConfig();
void convertStrToRGB(const char * source, uint8_t* r, uint8_t* g, uint8_t* b);
void panneauActif();
void panneauBlink();


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
  Serial.println(F("version 1.0 - 09/2023"));
  Serial.println(F("----------------------------------------------------------------------------"));
  
  // I2C RESET
  aConfig.i2cReset();
  
  // CONFIG OBJET
  Serial.println(F(""));
  Serial.println(F(""));
  aConfig.mountFS();
  aConfig.listDir("/");
  aConfig.listDir("/config");
  aConfig.listDir("/www");
  
  aConfig.printJsonFile("/config/objectconfig.txt");
  aConfig.readObjectConfig("/config/objectconfig.txt");

  aConfig.printJsonFile("/config/networkconfig.txt");
  aConfig.readNetworkConfig("/config/networkconfig.txt");

  // FASTLED
  aConfig.objectConfig.activeLeds = checkValeur(aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment,1,NB_LEDS_MAX);
  aConfig.writeObjectConfig("/config/objectconfig.txt");
  
  aFastled.setNbLed(aConfig.objectConfig.activeLeds);
  aFastled.setControlBrightness(aConfig.objectConfig.scintillementOnOff);
  aFastled.setIntervalControlBrightness(aConfig.objectConfig.intervalScintillement);
  
  // animation led de depart
  aFastled.animationDepart(50, aFastled.getNbLed()*2, CRGB::Blue);

  // WIFI
  WiFi.disconnect(true);

  Serial.println(F(""));
  Serial.println(F("connecting WiFi"));
  
  // AP MODE
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(aConfig.networkConfig.apIP, aConfig.networkConfig.apIP, aConfig.networkConfig.apNetMsk);
  bool apRC = WiFi.softAP(aConfig.networkConfig.apName, aConfig.networkConfig.apPassword);

  if (apRC)
  {
    Serial.println(F("AP WiFi OK"));
  }
  else
  {
    Serial.println(F("AP WiFi failed"));
  }

  // Print ESP soptAP IP Address
  Serial.print(F("softAPIP: "));
  Serial.println(WiFi.softAPIP());
  
  /*
  // CLIENT MODE POUR DEBUG
  const char* ssid = "SSID";
  const char* password = "PASSWORD*";
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  if (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.println(F("WiFi Failed!"));
  }
  else
  {
    Serial.println(F("WiFi OK"));
  }
    
  // Print ESP Local IP Address
  Serial.print(F("localIP: "));
  Serial.println(WiFi.localIP());
  */
  
  // WEB SERVER
  // Route for root / web page
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("config.html");
  server.serveStatic("/config", LittleFS, "/config/");
  server.onNotFound(notFound);

  // WEBSOCKET
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Start server
  server.begin();

  // HEARTBEAT
  previousMillisHB = millis();
  intervalHB = 10000;
  
  previousMillisRefresh = millis();
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
  // WEBSOCKET
  ws.cleanupClients();

  // FASTLED
  aFastled.updateAnimation();
  
  // CONTROL BRIGHTNESS
  aFastled.controlBrightness(aConfig.objectConfig.brightness);
  
  // gerer le statut de la serrure
  switch (aConfig.objectConfig.statutActuel)
  {
    case OBJET_ACTIF:
      // la serrure est fermee
      panneauActif();
      break;

    case OBJET_BLINK:
      // blink led pour identification
      panneauBlink();
      break;
      
    default:
      // nothing
      break;
  }
    
  // traiter le buffer du websocket
  if (flagBufferWebsocket)
  {
    flagBufferWebsocket = false;
    handleWebsocketBuffer();
  }

  // HEARTBEAT
  if(millis() - previousMillisHB > intervalHB)
  {
    previousMillisHB = millis();

    // envoyer l'uptime
    sendUptime();
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
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
   switch (type) 
    {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        // send config value to html
        sendObjectConfig();
        sendNetworkConfig();
        
        // send volatile info
        sendMaxLed();
        sendUptime();        
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
    data[len] = 0;
    sprintf(bufferWebsocket,"%s\n", (char*)data);
    Serial.print(len);
    Serial.print(bufferWebsocket);
    flagBufferWebsocket = true;
  }
}
  
void handleWebsocketBuffer()
{
    DynamicJsonDocument doc(JSONBUFFERSIZE);
    
    DeserializationError error = deserializeJson(doc, bufferWebsocket);
    if (error)
    {
      Serial.println(F("Failed to deserialize buffer"));
    }
    else
    {
        // write config or not
        bool writeObjectConfigFlag = false;
        bool sendObjectConfigFlag = false;
        bool writeNetworkConfigFlag = false;
        bool sendNetworkConfigFlag = false;
        
        // modif object config
        if (doc.containsKey("new_objectName"))
        {
          strlcpy(  aConfig.objectConfig.objectName,
                    doc["new_objectName"],
                    sizeof(aConfig.objectConfig.objectName));
  
          char const * listeCheck = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-";
          checkCharacter(aConfig.objectConfig.objectName, listeCheck, '_');

          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
  
        if (doc.containsKey("new_objectId")) 
        {
          uint16_t tmpValeur = doc["new_objectId"];
          aConfig.objectConfig.objectId = checkValeur(tmpValeur,1,1000);
  
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
  
        if (doc.containsKey("new_groupId")) 
        {
          uint16_t tmpValeur = doc["new_groupId"];
          aConfig.objectConfig.groupId = checkValeur(tmpValeur,1,1000);
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
  
        if (doc.containsKey("new_brightness"))
        {
          uint16_t tmpValeur = doc["new_brightness"];
          aConfig.objectConfig.brightness = checkValeur(tmpValeur,0,255);
          FastLED.setBrightness(aConfig.objectConfig.brightness);
          uneFois=true;
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
        
        if (doc.containsKey("new_activeLeds")) 
        {
          FastLED.clear(); 
          
          uint16_t tmpValeur = doc["new_activeLeds"];
          aConfig.objectConfig.activeLeds = checkValeur(tmpValeur,1,NB_LEDS_MAX);
          uneFois=true;
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
        
        if (doc.containsKey("new_intervalScintillement"))
        {
          uint16_t tmpValeur = doc["new_intervalScintillement"];
          aConfig.objectConfig.intervalScintillement = checkValeur(tmpValeur,0,1000);
          aFastled.setIntervalControlBrightness(aConfig.objectConfig.intervalScintillement);
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }

        if (doc.containsKey("new_scintillementOnOff"))
        {
          uint16_t tmpValeur = doc["new_scintillementOnOff"];
          aConfig.objectConfig.scintillementOnOff = checkValeur(tmpValeur,0,1);
          aFastled.setControlBrightness(aConfig.objectConfig.scintillementOnOff);
          
          if (aConfig.objectConfig.scintillementOnOff == 0)
          {
            FastLED.setBrightness(aConfig.objectConfig.brightness);
          }
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }

        if (doc.containsKey("new_nbColonnes"))
        {
          uint16_t tmpValeur = doc["new_nbColonnes"];
          aConfig.objectConfig.nbColonnes = checkValeur(tmpValeur,1,5);
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }

        if (doc.containsKey("new_nbSegments"))
        {
          uint16_t tmpValeur = doc["new_nbSegments"];
          aConfig.objectConfig.nbSegments = checkValeur(tmpValeur,1,SIZE_INDEX_COULEURS);
          aConfig.objectConfig.activeLeds = checkValeur(aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment,1,NB_LEDS_MAX);
          aFastled.setNbLed(aConfig.objectConfig.activeLeds);
          aFastled.allLedOff(false);
                    
          uneFois=true;
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }

        if (doc.containsKey("new_ledParSegment"))
        {
          uint16_t tmpValeur = doc["new_ledParSegment"];
          aConfig.objectConfig.ledParSegment = checkValeur(tmpValeur,1,5);
          aConfig.objectConfig.activeLeds = checkValeur(aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment,1,NB_LEDS_MAX);
          aFastled.setNbLed(aConfig.objectConfig.activeLeds);
          aFastled.allLedOff(false);
          
          uneFois=true;
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }

        if (doc.containsKey("new_ledMatrixType"))
        {
          uint16_t tmpValeur = doc["new_ledMatrixType"];
          aConfig.objectConfig.ledMatrixType = checkValeur(tmpValeur,0,1);
          
          uneFois=true;
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }

        if (doc.containsKey("new_indexCouleur")) 
        {
          JsonArray newIndexCouleur = doc["new_indexCouleur"];
            
          for (uint8_t i=0;i<newIndexCouleur.size();i++)
          {
            uint16_t tmpValeurX = newIndexCouleur[i][0];
            uint16_t tmpValeurY = newIndexCouleur[i][1];
            
            uint8_t x = checkValeur(tmpValeurX,0,9);
            uint8_t y = checkValeur(tmpValeurY,1,49);
  
            aConfig.objectConfig.indexCouleur[x] = y;
          }
          
            
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;

          // update statut
          uneFois = true;
        }

        if (doc.containsKey("new_bouton")) 
        {
          JsonArray newBouton = doc["new_bouton"];
        
          uint8_t nouvellePosition = newBouton[0];
          uint8_t nouvelleCouleur = newBouton[1];
          
          aConfig.objectConfig.indexCouleur[nouvellePosition]=nouvelleCouleur;
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
          uneFois=true;
        }

        if (doc.containsKey("new_nbCouleurs"))
        {
          uint16_t tmpValeur = doc["new_nbCouleurs"];
          aConfig.objectConfig.nbCouleurs = checkValeur(tmpValeur,1,5);

          for (uint8_t i=0;i<SIZE_INDEX_COULEURS;i++)
          {
            aConfig.objectConfig.indexCouleur[i] = min<uint8_t>(aConfig.objectConfig.indexCouleur[i], aConfig.objectConfig.nbCouleurs-1);
          }
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
        
        if (doc.containsKey("new_couleurs")) 
        {
          JsonArray newCouleur = doc["new_couleurs"];
  
          uint8_t i = newCouleur[0];
          char newColorStr[8];
          strncpy(newColorStr, newCouleur[1], 8);
            
          uint8_t r;
          uint8_t g;
          uint8_t b;
            
          convertStrToRGB(newColorStr, &r, &g, &b);
          aConfig.objectConfig.couleurs[i].red=r;
          aConfig.objectConfig.couleurs[i].green=g;
          aConfig.objectConfig.couleurs[i].blue=b;
            
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
  
        if (doc.containsKey("new_label"))
        {
          JsonArray newLabel = doc["new_label"];
  
          uint8_t i = newLabel[0];
          strlcpy(  aConfig.objectConfig.labels[i],
                    newLabel[1],
                    SIZE_ARRAY);

          // check for unsupported char
          char const * listeCheck = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-";
          checkCharacter(aConfig.objectConfig.labels[i], listeCheck, '_');

          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
        
        if (doc.containsKey("new_statutActuel"))
        {
          aConfig.objectConfig.statutPrecedent=aConfig.objectConfig.statutActuel;
          
          uint16_t tmpValeur = doc["new_statutActuel"];
          aConfig.objectConfig.statutActuel=tmpValeur;

          uneFois=true;
          
          writeObjectConfigFlag = true;
          sendObjectConfigFlag = true;
        }
        
        // modif network config
        if (doc.containsKey("new_apName")) 
        {
          strlcpy(  aConfig.networkConfig.apName,
                    doc["new_apName"],
                    sizeof(aConfig.networkConfig.apName));
  
          // check for unsupported char
          char const * listeCheck = "ABCDEFGHIJKLMNOPQRSTUVWYXZ0123456789_-";
          checkCharacter(aConfig.networkConfig.apName, listeCheck, 'A');
          
          writeNetworkConfigFlag = true;
          sendNetworkConfigFlag = true;
        }
  
        if (doc.containsKey("new_apPassword")) 
        {
          strlcpy(  aConfig.networkConfig.apPassword,
                    doc["new_apPassword"],
                    sizeof(aConfig.networkConfig.apPassword));
  
          writeNetworkConfigFlag = true;
          sendNetworkConfigFlag = true;
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
  
            writeNetworkConfigFlag = true;
          }
          
          sendNetworkConfigFlag = true;
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
  
            writeNetworkConfigFlag = true;
          }
  
          sendNetworkConfigFlag = true;
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

          sendObjectConfigFlag = true;
          sendNetworkConfigFlag = true;
        }

        if ( doc.containsKey("new_defaultObjectConfig") && doc["new_defaultObjectConfig"]==1 )
        {
          aConfig.writeDefaultObjectConfig("/config/objectconfig.txt");
          Serial.println(F("reset to default object config"));

          aFastled.allLedOff();
          aFastled.setNbLed(aConfig.objectConfig.activeLeds);          
          aFastled.setControlBrightness(aConfig.objectConfig.scintillementOnOff);
          aFastled.setIntervalControlBrightness(aConfig.objectConfig.intervalScintillement);
          
          sendObjectConfigFlag = true;
          uneFois = true;
        }

        if ( doc.containsKey("new_defaultNetworkConfig") && doc["new_defaultNetworkConfig"]==1 )
        {
          aConfig.writeDefaultNetworkConfig("/config/networkconfig.txt");
          Serial.println(F("reset to default network config"));          
          
          sendNetworkConfigFlag = true;
        }

        // modif config
        // write object config
        if (writeObjectConfigFlag)
        {
          writeObjectConfig();
        
          // update statut
          uneFois = true;
        }
  
        // resend object config
        if (sendObjectConfigFlag)
        {
          sendObjectConfig();
        }
  
        // write network config
        if (writeNetworkConfigFlag)
        {
          writeNetworkConfig();
        }
  
        // resend network config
        if (sendNetworkConfigFlag)
        {
          sendNetworkConfig();
        }
    }
 
    // clear json buffer
    doc.clear();
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void checkCharacter(char* toCheck, const char* allowed, char replaceChar)
{
  for (uint8_t i = 0; i < strlen(toCheck); i++)
  {
    if (strchr(allowed, toCheck[i]) == NULL)
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
    
  char toSend[100];
  snprintf(toSend, 100, "{\"uptime\":\"%id %ih %im %is\"}", days, hours, minutes, seconds);

  ws.textAll(toSend);
}

void sendMaxLed()
{
  char toSend[20];
  snprintf(toSend, 20, "{\"maxLed\":%i}", NB_LEDS_MAX);
  
  ws.textAll(toSend);
}

void sendStatut()
{
  char toSend[100];
  snprintf(toSend, 100, "{\"statutActuel\":%i}", aConfig.objectConfig.statutActuel); 

  ws.textAll(toSend);
}

void sendObjectConfig()
{
  aConfig.stringJsonFile("/config/objectconfig.txt", bufferToSend, BUFFERSENDSIZE);
  ws.textAll(bufferToSend);
}

void writeObjectConfig()
{
  aConfig.writeObjectConfig("/config/objectconfig.txt");
}

void sendNetworkConfig()
{
  aConfig.stringJsonFile("/config/networkconfig.txt", bufferToSend, BUFFERSENDSIZE);
  ws.textAll(bufferToSend);
}

void writeNetworkConfig()
{
  aConfig.writeNetworkConfig("/config/networkconfig.txt");
}

void convertStrToRGB(const char * source, uint8_t* r, uint8_t* g, uint8_t* b)
{ 
  uint32_t  number = (uint32_t) strtol( &source[1], NULL, 16);
  
  // Split them up into r, g, b values
  *r = number >> 16;
  *g = number >> 8 & 0xFF;
  *b = number & 0xFF;
}

void panneauActif()
{
  if (uneFois)
  {
    uneFois = false;
    Serial.println(F("PANNEAU ACTIF"));
    
    uint8_t nbLigne = aConfig.objectConfig.nbSegments/aConfig.objectConfig.nbColonnes;
    uint8_t ledParLigne = aConfig.objectConfig.nbColonnes*aConfig.objectConfig.ledParSegment;
    
    if (aConfig.objectConfig.ledMatrixType==0)
    {
      // led strip is linear
      for (uint8_t i=0;i<aConfig.objectConfig.activeLeds;i++)
      {
        aFastled.indexMatrix[i]=i;
      }
    }
    else
    {
      // led strip is in snake shape
      for (uint8_t i=0;i<nbLigne;i++)
      {
        for (uint8_t j=0;j<ledParLigne;j++)
        {
          if (i%2==0)
          {
            aFastled.indexMatrix[i*ledParLigne+j]=i*ledParLigne+j;
          }
          else
          {
            aFastled.indexMatrix[i*ledParLigne+j]=i*ledParLigne+ledParLigne-1-j;
          }
        }
      }
    }
    
    for (uint8_t i=0;i<aConfig.objectConfig.nbSegments;i++)
    {
      for (uint8_t j=0;j<aConfig.objectConfig.ledParSegment;j++)
      {
        uint8_t indexLed = i*aConfig.objectConfig.ledParSegment+j;
        aFastled.ledOn(aFastled.indexMatrix[indexLed], aConfig.objectConfig.couleurs[aConfig.objectConfig.indexCouleur[i]], true);
      }
    }

    sendStatut();
  }
}

void panneauBlink()
{
  if (uneFois)
  {
    uneFois = false;
    Serial.println(F("PANNEAU BLINK"));

    sendStatut();

    aFastled.animationBlink02Start(100, 3000, CRGB::Blue, CRGB::Black);
  }

  // fin de l'animation blink
  if(!aFastled.isAnimActive()) 
  {
    uneFois = true;

    aConfig.objectConfig.statutActuel = aConfig.objectConfig.statutPrecedent;

    writeObjectConfig();
    sendObjectConfig();

    Serial.println(F("END VUMETRE BLINK "));
  }
}
/*
   ----------------------------------------------------------------------------
   FIN DES FONCTIONS ADDITIONNELLES
   ----------------------------------------------------------------------------
*/
