
#include <stdlib.h>         // exit() malloc()
#include <string.h>         // strcpy() strlen()

#include "DeviceInfo.H"

DeviceInfo::DeviceInfo() {
  imei = NULL;
  deviceId = NULL;
  serverId = NULL;
  userName = NULL;
  clientPassword = NULL;
  serverPassword = NULL;  
  DEVICE_ID_PREFIX = "IMEI:";
}

DeviceInfo::~DeviceInfo() {
  if (imei != NULL) {
    free(imei);
    imei = NULL;
  }

  if (deviceId != NULL) {
    free(deviceId);
    deviceId = NULL;
  }

  if (serverId != NULL) {
    free(serverId);
    serverId = NULL;
  }

  if (userName != NULL) {
    free(userName);
    userName = NULL;
  }
  
  if (clientPassword != NULL) {
    free(clientPassword);
    clientPassword = NULL;
  }
  
  if (serverPassword != NULL) {
    free(serverPassword);
    serverPassword = NULL;
  }
}

void DeviceInfo::setIMEI(char * aIMEI) {

  if (aIMEI == NULL) {
    return;
  }

  if (imei != NULL) {
    free(imei);
    imei = NULL;
  }

  int len = sizeof(char) * strlen(aIMEI) + 1;

  imei = (char *) malloc(len);
  memset(imei, '\0', len);
 
  strcpy(imei, aIMEI);

  userName = (char *) malloc(len);
  memset(userName, '\0', len);

  strcpy(userName, imei);

  len = sizeof(char) * (strlen(DEVICE_ID_PREFIX) + len) + 1;

  deviceId = (char *) malloc(len);
  memset(deviceId, '\0', len);

  strcpy(deviceId, DEVICE_ID_PREFIX);
  strcat(deviceId, imei);
  
}

void DeviceInfo::setDeviceId(char * aDeviceId) {
  if (deviceId != NULL) {
    free(deviceId);
    deviceId = NULL;
  }

  int len = sizeof(char) * strlen(aDeviceId) + 1;
  deviceId = (char *) malloc(len);
  memset(deviceId, '\0', len);
  deviceId = aDeviceId;
}

void DeviceInfo::setServerId(char * aServerId) {

  if (serverId != NULL) {
    free(serverId);
    serverId = NULL;
  }

  int len = sizeof(char) * strlen(aServerId) + 1;
  serverId = (char *) malloc(len);
  memset(serverId, '\0', len);
  serverId = aServerId;
}

void DeviceInfo::setUserName(char * aUserName) {
  if (userName != NULL) {
    free(userName);
    userName = NULL;
  }
  
  int len = sizeof(char) * strlen(aUserName) + 1;
  userName = (char *) malloc(len);
  memset(userName, '\0', len);
  userName = aUserName;
}

void DeviceInfo::setClientPassword(char * aClientPassword) {
  if (clientPassword != NULL) {
    free(clientPassword);
    clientPassword = NULL;
  }
  
  int len = sizeof(char) * strlen(aClientPassword) + 1;
  clientPassword = (char *) malloc(len);
  memset(clientPassword, '\0', len);
  clientPassword = aClientPassword;
}

void DeviceInfo::setServerPassword(char * aServerPassword) {

  if (serverPassword != NULL) {
    free(serverPassword);
    serverPassword = NULL;
  }

  int len = sizeof(char) * strlen(aServerPassword) + 1;
  serverPassword = (char *) malloc(len);
  memset(serverPassword, '\0', len);
  serverPassword = aServerPassword;
}

char * DeviceInfo::getIMEI() {
  return imei;
}

char * DeviceInfo::getDeviceId() {
  return deviceId;
}

char * DeviceInfo::getServerId() {
  return serverId;
}

char * DeviceInfo::getUserName() {
  return userName;
}

char * DeviceInfo::getClientPassword() {
  return clientPassword;
}

char * DeviceInfo::getServerPassword() {
  return serverPassword;
}


