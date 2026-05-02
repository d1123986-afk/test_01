#ifndef QT_SOKOL_CLIENT_NETWORK
#define QT_SOKOL_CLIENT_NETWORK

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/tcp.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

#include <QString>
#include <QDebug>

#define USBIP_VERSION 0x00000111

/* макросы модулей ядра, необходимые клиенту */
#define USBIP_CORE_MOD_NAME	"usbip-core"
#define USBIP_VHCI_DRV_NAME	"vhci_hcd"

/* sysfs constants */
#ifdef _WIN32
    #define SYSFS_MNT_PATH         ""
    #define SOKOL_SYSFS_PATH		256
    #define SOKOL_SYSFS_BUS_ID      32
    /* Windows USBIP driver paths */
    #define USBIP_VHCI_DEVICE_NAME  "USBIPVHCI"
    #define VHCI_STATE_PATH         ""
#else
    #define SYSFS_MNT_PATH         "/sys"
    #define SYSFS_BUS_NAME         "bus"
    #define SYSFS_BUS_TYPE         "usb"
    #define SYSFS_DRIVERS_NAME     "drivers"
    #define SOKOL_SYSFS_PATH		256
    #define SOKOL_SYSFS_BUS_ID      32
    #define VHCI_STATE_PATH             "/var/run/vhci_hcd"
#endif

#define ST_OK	0x00
#define ST_NA	0x01
#define ST_DEV_BUSY	0x02
#define ST_DEV_ERR	0x03
#define ST_NODEV	0x04
#define ST_ERROR	0x05

struct sokol_usb_interface {

	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t padding;	/* alignment */

} __attribute__((packed));

struct sokol_usb_device {

    char sokolDevicePath[SOKOL_SYSFS_PATH];
    char sokolDeviceBusid[SOKOL_SYSFS_BUS_ID];

    uint32_t sokolDeviceBusnum;
    uint32_t sokolDeviceDevnum;
    uint32_t sokolDeviceSpeed;

    uint16_t sokolDeviceVendorId;
    uint16_t sokolDeviceProductId;
    uint16_t sokolDeviceBcdDevice;

    uint8_t sokolDeviceClass;
    uint8_t sokolDeviceSubClass;
    uint8_t sokolDeviceProtocol;
    uint8_t sokolConfigurationValue;
    uint8_t sokolNumConfigurations;
    uint8_t sokolNumInterfaces;

} __attribute__((packed));

#define OP_REQUEST	(0x80 << 8)
#define OP_REPLY	(0x00 << 8)

struct op_common {
	uint16_t version;
	uint16_t code;
    uint32_t status;
} __attribute__((packed));

/* ---------------------------------------------------------------------- */
/* Dummy Code */
#define OP_UNSPEC		0x00
#define OP_REQ_UNSPEC	OP_UNSPEC
#define OP_REP_UNSPEC	OP_UNSPEC

struct op_auth_reply{
    int returncode;
}__attribute__((packed));

/* ---------------------------------------------------------------------- */
/* Структура логин/пароль*/
struct auth{
	char l_log[64];
	char p_pass[64];
}__attribute__((packed));

/* ---------------------------------------------------------------------- */
/* Import a remote USB device. */
#define OP_IMPORT	0x03
#define OP_REQ_IMPORT	(OP_REQUEST | OP_IMPORT)
#define OP_REP_IMPORT   (OP_REPLY   | OP_IMPORT)

struct op_import_request {
    char busid[SOKOL_SYSFS_BUS_ID];
} __attribute__((packed));

struct op_import_reply {
    struct sokol_usb_device udev;
//	struct usbip_usb_interface uinf[];
} __attribute__((packed));

/* ---------------------------------------------------------------------- */
/* Retrieve the list of exported USB devices. */
#define OP_DEVLIST			 0x05
#define OP_REQ_DEVLIST		(OP_REQUEST | OP_DEVLIST)
#define OP_REP_DEVLIST		(OP_REPLY   | OP_DEVLIST)


struct op_devlist_reply {
	uint32_t ndev;
} __attribute__((packed));

class SokolNetworkClient{
    private:
        int sockfd;
    public:
        SokolNetworkClient();
        ~SokolNetworkClient();

        int authRequest(QString, QString);
        int setupTcpConnection(QString, QString);
        ssize_t sokolNetworkSend(void*, size_t);
        ssize_t sokolNetworkRecv(void*, size_t);
        int sokolSendCommonRequest(uint32_t, uint32_t);
        int sokolReceiveCommonReply(uint16_t *, int *);
        void sokolNetworkPackUsbDevice(int, struct sokol_usb_device *);
        int getSocket() const { return sockfd; }
        void closeSocket() const { 
#ifdef _WIN32
            closesocket(sockfd); 
#else
            close(sockfd); 
#endif
        }
        static uint32_t sokolNetworkPackUint32(int pack, uint32_t num){

        uint32_t i;

            if (pack)
                i = htonl(num);
            else
                i = ntohl(num);

            return i;
            }
    private:
        int _setup_tcp_connect(const char*, const char*);
        int sokolNetworkSetNoDelay();
        int sokolNetworkSetKeepAlive();
        ssize_t SendRecv(void*, size_t, int);
        void sokolNetworkPackCommon(int pack, struct op_common *op_common);
        uint16_t sokolNetworkPackUint16(int, uint16_t);
};

#define CHANGE_BYTE_ORDER(pack, reply)  do {\
    (reply)->ndev = SokolNetworkClient::sokolNetworkPackUint32(pack, (reply)->ndev);\
} while (0)

#endif
