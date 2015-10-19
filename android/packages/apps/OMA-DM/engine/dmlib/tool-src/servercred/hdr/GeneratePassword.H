#ifndef GENERATEPASSWORD_H
#define GENERATEPASSWORD_H

class GeneratePassword 
{
  
 public:

  GeneratePassword();

  ~GeneratePassword();

  char * generateClientPasswordKey(char * deviceId);

  char * generateKeyFromDict(char * deviceId, char dict[]);
  
  char * generateServerPasswordKey(char * deviceId);
  
  char * generateClientPassword(char * deviceId, char * serverId);
  
  char * generateServerPassword(char * deviceId, char * serverId);

  char * generateClientPassword();
  
  char * generateServerPassword();
  
  char * encodeDeviceId(char * deviceId);
  
  char * get36BasedIMEI(char * deviceId);
  
  char * generatePassword(char * deviceId, char * serverId, char * key);
  
  char * encodeHex(char data[]);

  long long convertChar2Long(char * input);

  void shuffle(char & buffer);

  void setServerId(const char * sid);

  void setIMEI(const char * did);

  char * getIMEI();

  char * getServerId();

 private:

  char * serverId;
  char * imei;
  char clientPasswordDict [15];
  char serverPasswordDict[15];
  char hexTable[16];
  int MD5_HASH_LENGTH;
};
  
#endif
