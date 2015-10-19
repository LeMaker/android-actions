#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "DeviceInfo.H"
#include "GeneratePassword.H"

using namespace std;

int  main(int argc, char *argv[])
{
  GeneratePassword gp;

  if (argv[1] != NULL) {

    if (strcmp(argv[1], "--help") == 0) {
      printf("Usage: GeneratePassword [IMEI] [SERVER_ID]\n\n");
      printf("Example: GeneratePassword 000000011234564 motorola\n\n");
      printf("Default: IMEI = 123456789012345\n");
      printf("         SERVER_ID = openwave.com\n");
      exit(0);
    }

    char * imei = argv[1];
    gp.setIMEI((const char *)imei);

    if (argv[2] != NULL) {
      char * serverId = argv[2];
      gp.setServerId((const char *)serverId);
    }
  }

  DeviceInfo deviceInfo;
  deviceInfo.setIMEI(gp.getIMEI());
  deviceInfo.setServerId(gp.getServerId());
  deviceInfo.setClientPassword(gp.generateClientPassword());
  deviceInfo.setServerPassword(gp.generateServerPassword());

  printf("[Device Info]\n");
  printf("IMEI: %s\n", deviceInfo.getIMEI());
  printf("Device ID: %s\n", deviceInfo.getDeviceId());
  printf("Server ID: %s\n", deviceInfo.getServerId());
  printf("User Name: %s\n", deviceInfo.getUserName());
  printf("Client Password: %s\n", deviceInfo.getClientPassword());
  printf("Server Password: %s\n", deviceInfo.getServerPassword());

  return 0;
}
