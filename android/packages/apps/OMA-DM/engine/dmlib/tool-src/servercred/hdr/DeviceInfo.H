#ifndef DEVICEINFO_H
#define DEVICEINFO_H

class DeviceInfo
{

 public:

  DeviceInfo();
  ~DeviceInfo();

  void setIMEI(char * aIMEI);
  void setDeviceId(char * did);
  void setServerId(char * sid);
  void setUserName(char * aUserName);
  void setClientPassword(char * aClientPassword);
  void setServerPassword(char * aServerPassword);

  char * getIMEI();
  char * getDeviceId();
  char * getServerId();
  char * getUserName();
  char * getClientPassword();
  char * getServerPassword();

 private:

  char * imei;
  char * deviceId;
  char * serverId;
  char * userName;
  char * clientPassword;
  char * serverPassword;
  char * DEVICE_ID_PREFIX;
};

#endif
