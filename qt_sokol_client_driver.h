#ifndef QT_SOKOL_CLIENT_DRIVER
#define QT_SOKOL_CLIENT_DRIVER

#include <libudev.h>
#include <linux/usb/ch9.h>
#include <linux/usbip.h>
#include <dirent.h>
#include <fcntl.h>

#include "qt_sokol_client_network.h"

#define VHCI_STATE_PATH             "/var/run/vhci_hcd"
#define SOKOL_VHCI_BUS_TYPE			"platform"
#define SOKOL_VHCI_DEVICE_NAME		"vhci_hcd.0"

struct speedString {

    int sValue;
    const char *sString;
    const char *dSpeed;
};

static const struct speedString usb_speed_types[] = {

    { USB_SPEED_UNKNOWN, "unknown", "Unknown Speed"},
    { USB_SPEED_LOW,  "1.5", "Low Speed(1.5Mbps)"  },
    { USB_SPEED_FULL, "12",  "Full Speed(12Mbps)" },
    { USB_SPEED_HIGH, "480", "High Speed(480Mbps)" },
    { USB_SPEED_WIRELESS, "53.3-480", "Wireless"},
    { USB_SPEED_SUPER, "5000", "Super Speed(5000Mbps)" },
    { 0, NULL, NULL }
};

#define to_string(s)	#s

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

    /* /sys/devices/platform/vhci_hcd */
    struct udev_device *hc_device;

    int ncontrollers;
    int nports;

    struct sokol_imported_device idev[];
};

class SokolClientDriver{
    private:
        struct sokol_vhci_driver *vhci_driver;
        struct udev 			 *udev_context;
        SokolNetworkClient *client;
    public:
        SokolClientDriver();
        ~SokolClientDriver();

        // struct sokol_vhci_driver* getDriver(){return vhci_driver;}

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
private:
        int sokol_vhci_driver_open(void);
        void sokol_vhci_driver_close(void);
        int import_device(int sockfd, struct sokol_usb_device *udev);
        int refresh_imported_device_list(void);
        int getDriverPorts(struct udev_device *hc_device);
        int getDriverControllers(void);
        int write_sysfs_attribute(const char *attr_path, const char *new_value,
                                  size_t len);
};

#endif /* QT_SOKOL_CLIENT_DRIVER */
