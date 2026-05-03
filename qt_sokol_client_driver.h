#ifndef QT_SOKOL_CLIENT_DRIVER
#define QT_SOKOL_CLIENT_DRIVER

#ifdef _WIN32
    #include <windows.h>
    #include <setupapi.h>
    #include <cfgmgr32.h>
    #pragma comment(lib, "setupapi.lib")
#else
    #include <libudev.h>
    #include <linux/usb/ch9.h>
    #include <linux/usbip.h>
    #include <dirent.h>
    #include <fcntl.h>
#endif

#include "qt_sokol_client_network.h"

#ifdef _WIN32
    #define VHCI_STATE_PATH             ""
    #define SOKOL_VHCI_BUS_TYPE""
    #define SOKOL_VHCI_DEVICE_NAME"USBIPVHCI"
    /* Windows USBIP VHCI driver constants */
    #define USBIP_VHCI_IOCTL_BASE       FILE_DEVICE_UNKNOWN
    #define USBIP_VHCI_IOCTL_ATTACH     CTL_CODE(USBIP_VHCI_IOCTL_BASE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
    #define USBIP_VHCI_IOCTL_DETACH     CTL_CODE(USBIP_VHCI_IOCTL_BASE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#else
    #define VHCI_STATE_PATH             "/var/run/vhci_hcd"
    #define SOKOL_VHCI_BUS_TYPE"platform"
    #define SOKOL_VHCI_DEVICE_NAME"vhci_hcd.0"
#endif

struct speedString {

    int sValue;
    const char *sString;
    const char *dSpeed;
};

#ifdef _WIN32
/* Windows USB speed constants */
#define USB_SPEED_UNKNOWN       0
#define USB_SPEED_LOW           1
#define USB_SPEED_FULL          2
#define USB_SPEED_HIGH          3
#define USB_SPEED_WIRELESS      4
#define USB_SPEED_SUPER         5

/* Windows VHCI device status */
#define VDEV_ST_NULL            0x00
#define VDEV_ST_NOTASSIGNED     0x01
#define VDEV_ST_USED            0x02
#define VDEV_ST_ERROR           0x03
#else
#include <linux/usb/ch9.h>
#include <linux/usbip.h>
#endif

#define to_string(s)#s

#ifdef _WIN32
/* Windows USB speed strings */
static const struct speedString usb_speed_types[] = {
    { USB_SPEED_UNKNOWN, "unknown", "Unknown Speed"},
    { USB_SPEED_LOW,  "1.5", "Low Speed(1.5Mbps)"  },
    { USB_SPEED_FULL, "12",  "Full Speed(12Mbps)" },
    { USB_SPEED_HIGH, "480", "High Speed(480Mbps)" },
    { USB_SPEED_WIRELESS, "53.3-480", "Wireless"},
    { USB_SPEED_SUPER, "5000", "Super Speed(5000Mbps)" },
    { 0, NULL, NULL }
};
#else
static const struct speedString usb_speed_types[] = {
    { USB_SPEED_UNKNOWN, "unknown", "Unknown Speed"},
    { USB_SPEED_LOW,  "1.5", "Low Speed(1.5Mbps)"  },
    { USB_SPEED_FULL, "12",  "Full Speed(12Mbps)" },
    { USB_SPEED_HIGH, "480", "High Speed(480Mbps)" },
    { USB_SPEED_WIRELESS, "53.3-480", "Wireless"},
    { USB_SPEED_SUPER, "5000", "Super Speed(5000Mbps)" },
    { 0, NULL, NULL }
};
#endif

enum hub_speed{

    HUB_SPEED_HIGH = 0,
    HUB_SPEED_SUPER
};

struct sokol_imported_device{

    enum hub_speed hub;

    uint8_t port;
    uint32_t status;
    uint32_t devid;
    uint8_t busnum;
    uint8_t devnum;

    struct sokol_usb_device udev;
};

struct sokol_vhci_driver{

#ifdef _WIN32
    /* Windows USBIP VHCI driver handle */
    HANDLE vhci_handle;
    int nports;
    struct sokol_imported_device idev[];
#else
    /* /sys/devices/platform/vhci_hcd */
    struct udev_device *hc_device;

    int ncontrollers;
    int nports;

    struct sokol_imported_device idev[];
#endif
};

class SokolClientDriver{
    private:
        struct sokol_vhci_driver *vhci_driver;
#ifdef _WIN32
        HANDLE vhci_handle;
#else
        struct udev  *udev_context;
#endif
        SokolNetworkClient *client;
    public:
        SokolClientDriver();
        ~SokolClientDriver();

        // struct sokol_vhci_driver* getDriver(){return vhci_driver;}

#ifdef _WIN32
        int sokol_vhci_refresh_device_list(void);
        int sokol_vhci_get_free_port(uint32_t speed);
        int sokol_vhci_attach_device(uint8_t port, int sockfd,
uint8_t busnum, uint8_t devnum, uint32_t speed);
        int sokol_vhci_detach_device(uint8_t port);
        int start_import_device(const char *busid, QString sokolhost, QString login, QString password, QString sokolport);
        int detach_port(int);
        int windows_vhci_driver_open(void);
        void windows_vhci_driver_close(void);
        int windows_vhci_attach_device(uint8_t port, int sockfd, uint32_t devid, uint32_t speed);
        int windows_vhci_detach_device(uint8_t port);
#else
        int sokol_vhci_refresh_device_list(void);
        int sokol_vhci_get_free_port(uint32_t speed);
        int sokol_vhci_attach_device(uint8_t port, int sockfd,
uint8_t busnum, uint8_t devnum, uint32_t speed);
        int sokol_vhci_detach_device(uint8_t port);
        int parse_status(const char* value);
        sokol_imported_device *imported_device_init(struct sokol_imported_device *idev, char *busid);
        int read_usb_device(struct udev_device *sdev, struct sokol_usb_device *udev);
        int read_attr_value(struct udev_device *dev, const char *name, const char *format);
        int start_import_device(const char *busid, QString sokolhost, QString login, QString password, QString sokolport);
        int detach_port(int);
#endif
private:
#ifdef _WIN32
        int sokol_vhci_driver_open(void);
        void sokol_vhci_driver_close(void);
        int import_device(int sockfd, struct sokol_usb_device *udev);
        int refresh_imported_device_list(void);
#else
        int sokol_vhci_driver_open(void);
        void sokol_vhci_driver_close(void);
        int import_device(int sockfd, struct sokol_usb_device *udev);
        int refresh_imported_device_list(void);
        int getDriverPorts(struct udev_device *hc_device);
        int getDriverControllers(void);
        int write_sysfs_attribute(const char *attr_path, const char *new_value,
                                  size_t len);
#endif
};

#endif /* QT_SOKOL_CLIENT_DRIVER */
