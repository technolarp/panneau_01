#include <LittleFS.h>
#include <ArduinoJson.h> // arduino json v6  // https://github.com/bblanchon/ArduinoJson

// to upload config dile : https://github.com/earlephilhower/arduino-esp8266littlefs-plugin/releases
#define SIZE_ARRAY 20
#define MAX_SIZE_CODE 9

#include <IPAddress.h>
#include <FastLED.h>

class M_config
{
  public:
  // structure stockage
  struct OBJECT_CONFIG_STRUCT
  {
    uint16_t objectId;
  	uint16_t groupId;
      
  	char objectName[SIZE_ARRAY];
    char codeSerrure[MAX_SIZE_CODE];
  	
  	uint8_t activeLeds;
    uint8_t brightness;

    CRGB couleur1;
    CRGB couleur2;
    CRGB couleur3;
    
    uint8_t labelCouleur[8];
  };
  
  // creer une structure
  OBJECT_CONFIG_STRUCT objectConfig;

  struct NETWORK_CONFIG_STRUCT
  {
    IPAddress apIP;
    IPAddress apNetMsk;
    char apName[SIZE_ARRAY];
    char apPassword[SIZE_ARRAY];
  };
  
  // creer une structure
  NETWORK_CONFIG_STRUCT networkConfig;
  
  
  M_config()
  {
  }
  
  void mountFS()
  {
    Serial.println(F("Mount LittleFS"));
    if (!LittleFS.begin())
    {
      Serial.println(F("LittleFS mount failed"));
      return;
    }
  }
  
  void printJsonFile(const char * filename)
  {
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) 
    {
      Serial.println(F("Failed to open file for reading"));
    }
      
    StaticJsonDocument<1024> doc;
    
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      Serial.println(F("Failed to deserialize file"));
      Serial.println(error.c_str());
    }
    else
    {
      serializeJsonPretty(doc, Serial);
      Serial.println();
    }
    
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }

  
  
  void listDir(const char * dirname)
  {
    Serial.printf("Listing directory: %s", dirname);
    Serial.println();
  
    Dir root = LittleFS.openDir(dirname);
  
    while (root.next())
	  {
      File file = root.openFile("r");
      Serial.print(F("  FILE: "));
      Serial.print(root.fileName());
      Serial.print(F("  SIZE: "));
      Serial.print(file.size());
      Serial.println();
      file.close();
    }

    Serial.println();
  }
  
  
  void readObjectConfig(const char * filename)
  {
    // lire les données depuis le fichier littleFS
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) 
    {
      Serial.println(F("Failed to open file for reading"));
      return;
    }
    else
    {
      Serial.println(F("File opened"));
    }
  
    StaticJsonDocument<1024> doc;
    
	  // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      Serial.println(F("Failed to deserialize file"));
      Serial.println(error.c_str());
    }
    else
    {
  		// Copy values from the JsonObject to the Config
  		objectConfig.objectId = doc["objectId"];
  		objectConfig.groupId = doc["groupId"];
  		objectConfig.activeLeds = doc["activeLeds"];
      objectConfig.brightness = doc["brightness"];
      
  		if (doc.containsKey("objectName"))
  		{ 
  			strlcpy(  objectConfig.objectName,
  			          doc["objectName"],
  			          sizeof(objectConfig.objectName));
  		}

      if (doc.containsKey("couleur1"))
      {
        JsonArray couleur = doc["couleur1"];
        
        objectConfig.couleur1.red = couleur[0];
        objectConfig.couleur1.green = couleur[1];
        objectConfig.couleur1.blue =  couleur[2];
      }

      if (doc.containsKey("couleur2"))
      {
        JsonArray couleur = doc["couleur2"];
        
        objectConfig.couleur2.red = couleur[0];
        objectConfig.couleur2.green = couleur[1];
        objectConfig.couleur2.blue =  couleur[2];
      }

      if (doc.containsKey("couleur3"))
      {
        JsonArray couleur = doc["couleur3"];
        
        objectConfig.couleur3.red = couleur[0];
        objectConfig.couleur3.green = couleur[1];
        objectConfig.couleur3.blue =  couleur[2];
      }

      if (doc.containsKey("labelCouleur"))
      {
        JsonArray labelCouleurTmp = doc["labelCouleur"];
        
        for (uint8_t i=0;i<8;i++)
        {
          uint8_t tmpValue = labelCouleurTmp[i];
          objectConfig.labelCouleur[i]=tmpValue;
        }
      }
    }
  		
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }

  void readNetworkConfig(const char * filename)
  {
    // lire les données depuis le fichier littleFS
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) 
    {
      Serial.println(F("Failed to open file for reading"));
      return;
    }
    else
    {
      Serial.println(F("File opened"));
    }
  
    StaticJsonDocument<1024> doc;
    
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      Serial.println(F("Failed to deserialize file"));
      Serial.println(error.c_str());
    }
    else
    {
      // Copy values from the JsonObject to the Config
      if (doc.containsKey("apIP"))
      { 
        JsonArray apIP = doc["apIP"];
        
        networkConfig.apIP[0] = apIP[0];
        networkConfig.apIP[1] = apIP[1];
        networkConfig.apIP[2] = apIP[2];
        networkConfig.apIP[3] = apIP[3];
      }

      if (doc.containsKey("apNetMsk"))
      { 
        JsonArray apNetMsk = doc["apNetMsk"];
        
        networkConfig.apNetMsk[0] = apNetMsk[0];
        networkConfig.apNetMsk[1] = apNetMsk[1];
        networkConfig.apNetMsk[2] = apNetMsk[2];
        networkConfig.apNetMsk[3] = apNetMsk[3];
      }
          
      if (doc.containsKey("apName"))
      { 
        strlcpy(  networkConfig.apName,
                  doc["apName"],
                  sizeof(networkConfig.apName));
      }

      if (doc.containsKey("apPassword"))
      { 
        strlcpy(  networkConfig.apPassword,
                  doc["apPassword"],
                  sizeof(networkConfig.apPassword));
      }
    }
      
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }
  
  void writeObjectConfig(const char * filename)
  {
    // Delete existing file, otherwise the configuration is appended to the file
    LittleFS.remove(filename);
  
    // Open file for writing
    File file = LittleFS.open(filename, "w");
    if (!file) 
    {
      Serial.println(F("Failed to create file"));
      return;
    }

    // Allocate a temporary JsonDocument
    StaticJsonDocument<1024> doc;

    String newObjectName= String(objectConfig.objectName);
    doc["objectName"] = newObjectName;

    doc["objectId"] = objectConfig.objectId;
    doc["groupId"] = objectConfig.groupId;
    doc["activeLeds"] = objectConfig.activeLeds;
    doc["brightness"] = objectConfig.brightness;

    StaticJsonDocument<128> docCouleur1;
    JsonArray arrayCouleur1 = docCouleur1.to<JsonArray>();
    arrayCouleur1.add(objectConfig.couleur1.red);
    arrayCouleur1.add(objectConfig.couleur1.green);
    arrayCouleur1.add(objectConfig.couleur1.blue);
    
    doc["couleur1"]=arrayCouleur1;

    StaticJsonDocument<128> docCouleur2;
    JsonArray arrayCouleur2 = docCouleur2.to<JsonArray>();
    arrayCouleur2.add(objectConfig.couleur2.red);
    arrayCouleur2.add(objectConfig.couleur2.green);
    arrayCouleur2.add(objectConfig.couleur2.blue);
    
    doc["couleur2"]=arrayCouleur2;

    StaticJsonDocument<128> docCouleur3;
    JsonArray arrayCouleur3 = docCouleur3.to<JsonArray>();
    arrayCouleur3.add(objectConfig.couleur3.red);
    arrayCouleur3.add(objectConfig.couleur3.green);
    arrayCouleur3.add(objectConfig.couleur3.blue);
    
    doc["couleur3"]=arrayCouleur3;

    StaticJsonDocument<128> docLabelCouleur;
    JsonArray arrayLabelCouleur = docLabelCouleur.to<JsonArray>();
    for (uint8_t i=0;i<8;i++)
    {
      arrayLabelCouleur.add(objectConfig.labelCouleur[i]);
    }
    
    doc["labelCouleur"]=arrayLabelCouleur;
    
    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) 
    {
      Serial.println(F("Failed to write to file"));
    }
    else
    {
      Serial.println(F("write success"));
    }

    // Close the file (File's destructor doesn't close the file)
    file.close();
    Serial.println(F("file closed"));
  }

  void writeNetworkConfig(const char * filename)
  {
    // Delete existing file, otherwise the configuration is appended to the file
    LittleFS.remove(filename);
  
    // Open file for writing
    File file = LittleFS.open(filename, "w");
    if (!file) 
    {
      Serial.println(F("Failed to create file"));
      return;
    }

    // Allocate a temporary JsonDocument
    StaticJsonDocument<1024> doc;

    String newApName="";
    String newApPassword="";
    
    for (int i=0;i<SIZE_ARRAY;i++)
    {
      newApName+= networkConfig.apName[i];
      newApPassword+= networkConfig.apPassword[i];
    }
    
    doc["apName"] = newApName;
    doc["apPassword"] = newApPassword;

    StaticJsonDocument<128> docIp;
    JsonArray arrayIp = docIp.to<JsonArray>();
    arrayIp.add(networkConfig.apIP[0]);
    arrayIp.add(networkConfig.apIP[1]);
    arrayIp.add(networkConfig.apIP[2]);
    arrayIp.add(networkConfig.apIP[3]);

    StaticJsonDocument<128> docNetMask;
    JsonArray arrayNetMask = docNetMask.to<JsonArray>();
    arrayNetMask.add(networkConfig.apNetMsk[0]);
    arrayNetMask.add(networkConfig.apNetMsk[1]);
    arrayNetMask.add(networkConfig.apNetMsk[2]);
    arrayNetMask.add(networkConfig.apNetMsk[3]);
    
    doc["apIP"]=arrayIp;
    doc["apNetMsk"]=arrayNetMask;

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) 
    {
      Serial.println(F("Failed to write to file"));
    }
    
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }
  
  void writeDefaultObjectConfig(const char * filename)
  {
  	objectConfig.objectId = 1;
  	objectConfig.groupId = 1;
  	objectConfig.activeLeds = 16;
    objectConfig.brightness = 80;

    objectConfig.couleur1.red = 0;
    objectConfig.couleur1.green = 255;
    objectConfig.couleur1.blue =  0;
  
    objectConfig.couleur2.red = 255;
    objectConfig.couleur2.green = 128;
    objectConfig.couleur2.blue =  0;
  
    objectConfig.couleur3.red = 255;
    objectConfig.couleur3.green = 0;
    objectConfig.couleur3.blue =  0;

    for (uint8_t i=0;i<8;i++)
    {
      objectConfig.labelCouleur[i] = 1;
    }

    strlcpy(  objectConfig.objectName,
    			          "panneau",
    			          sizeof("panneau"));
  	
  	writeObjectConfig(filename);
  }

  void writeDefaultNetworkConfig(const char * filename)
  {
    strlcpy(  networkConfig.apName,
                    "VUMETRE",
                    sizeof("VUMETRE"));
    
    strlcpy(  networkConfig.apPassword,
                    "",
                    sizeof(""));
  
    networkConfig.apIP[0]=192;
    networkConfig.apIP[1]=168;
    networkConfig.apIP[2]=1;
    networkConfig.apIP[3]=1;
  
    networkConfig.apNetMsk[0]=255;
    networkConfig.apNetMsk[1]=255;
    networkConfig.apNetMsk[2]=255;
    networkConfig.apNetMsk[3]=0;
      
    writeNetworkConfig(filename);
  }

  String stringJsonFile(const char * filename)
  {
    String result = "";
    
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) 
    {
      Serial.println(F("Failed to open file for reading"));
    }
      
    StaticJsonDocument<1024> doc;
    
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      Serial.println(F("Failed to deserialize file"));
      Serial.println(error.c_str());
    }
    else
    {
      serializeJson(doc, result);
    }
    
    // Close the file (File's destructor doesn't close the file)
    file.close();
    return(result);
  }
};
