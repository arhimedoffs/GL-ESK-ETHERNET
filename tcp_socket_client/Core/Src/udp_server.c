#include "main.h"
#include "lwip.h"
#include "sockets.h"
#include "cmsis_os.h"
#include <string.h>

#if (USE_UDP_SERVER_PRINTF == 1)
#include <stdio.h>
#define UDP_SERVER_PRINTF(...) do { printf("[udp_server.c: %s: %d]: ",__func__, __LINE__);printf(__VA_ARGS__); } while (0)
#else
#define UDP_SERVER_PRINTF(...)
#endif

#define PORTNUM 5678

typedef enum
{
	CMD_INCORRECT = -1,
	CMD_LED_OFF = 0,
	CMD_LED_ON,
	CMD_LED_TOGGLE,
} TLEDcommand;

#define MSG_CMD_ERROR "Error!\n"
#define MSG_CMD_OK "OK\n"

static struct sockaddr_in serv_addr, client_addr;
static int socket_fd;
static uint16_t nport;

static int udpServerInit(void)
{
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd == -1) {
		UDP_SERVER_PRINTF("socket() error\n");
		return -1;
	}

	nport = PORTNUM;
	nport = htons((uint16_t)nport);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = nport;

	if(bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1) {
		UDP_SERVER_PRINTF("bind() error\n");
		close(socket_fd);
		return -1;
	}

	UDP_SERVER_PRINTF("Server is ready\n");

	return 0;
}

static int parseCommand(const char *cmd, int cmdLen) {
	const char *ptr = cmd;
	const char *ptrEnd = cmd+cmdLen;
	TLEDcommand command = CMD_INCORRECT;
	Led_TypeDef leds[4] = {LED3, LED4, LED5, LED6};
	int ledNumber = -1;
	int retVal = 0;

	if (strncmp(ptr, "led", strlen("led")) != 0) {
		return -1;
	}
	ptr += strlen("led");
	if (ptr >= ptrEnd)
		return -1;

	ledNumber = (int)((*ptr++) - '3');
	if (ledNumber < 0 || ledNumber > 3)
		return -2;
	ptr++;
	if (ptr >= ptrEnd)
		return -1;

	if (strncmp(ptr, "on", strlen("on")) == 0) {
		command = CMD_LED_ON;
	} else if (strncmp(ptr, "off", strlen("off")) == 0) {
		command = CMD_LED_OFF;
	} else if (strncmp(ptr, "toggle", strlen("toggle")) == 0) {
		command = CMD_LED_TOGGLE;
	} else {
		command = CMD_INCORRECT;
	}

	switch (command) {
	case CMD_LED_OFF:
		BSP_LED_Off(leds[ledNumber]);
		break;
	case CMD_LED_ON:
		BSP_LED_On(leds[ledNumber]);
		break;
	case CMD_LED_TOGGLE:
		BSP_LED_Toggle(leds[ledNumber]);
		break;
	default:
		retVal = -1;
	}
	return retVal;
}

void StartUdpServerTask(void const * argument)
{
    int received;
	int addr_len;
	char buffer[80];

	osDelay(5000);// wait 5 sec to init lwip stack

	if(udpServerInit() < 0) {
		UDP_SERVER_PRINTF("udpSocketServerInit() error\n");
		return;
	}

	for(;;)
	{
		  bzero(&client_addr, sizeof(client_addr));
		  addr_len = sizeof(client_addr);

		  received = recvfrom(socket_fd, buffer, sizeof(buffer)-1, 0, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);

		  if (received == -1) {
			  UDP_SERVER_PRINTF("recvfrom() error\n");
			  continue;
		  }
		  buffer[received] = '\0';

		  UDP_SERVER_PRINTF("Client: %s\n", inet_ntoa(client_addr.sin_addr));

		  if (parseCommand(buffer, received) < 0)
			  sendto(socket_fd, MSG_CMD_ERROR, strlen(MSG_CMD_ERROR), 0, (struct sockaddr *)&client_addr, (socklen_t)addr_len);
		  else
			  sendto(socket_fd, MSG_CMD_OK, strlen(MSG_CMD_OK), 0, (struct sockaddr *)&client_addr, (socklen_t)addr_len);
	}
}
