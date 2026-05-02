/* Windows stub implementation for VHCI driver */
/* This file provides Windows-compatible implementations for USBIP VHCI operations */

#include "qt_sokol_client_driver.h"

#ifdef _WIN32

SokolClientDriver::SokolClientDriver() { 
    vhci_handle = INVALID_HANDLE_VALUE;
    qDebug() << "SokolClientDriver class constructor (Windows)"; 
}

SokolClientDriver::~SokolClientDriver() { 
    qDebug() << "SokolClientDriver class destructor (Windows)"; 
}

int SokolClientDriver::sokol_vhci_driver_open(void) {
    /* Open Windows USBIP VHCI driver */
    vhci_handle = CreateFileA(
        "\\\\.\\USBIP_VHCI",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (vhci_handle == INVALID_HANDLE_VALUE) {
        qDebug() << "Failed to open USBIP VHCI driver. Error:" << GetLastError();
        return -1;
    }
    
    /* Initialize driver structure */
    vhci_driver = (struct sokol_vhci_driver*)calloc(1, sizeof(struct sokol_vhci_driver) +
            8 * sizeof(struct sokol_imported_device));  /* Assume 8 ports by default */
    if (!vhci_driver) {
        CloseHandle(vhci_handle);
        vhci_handle = INVALID_HANDLE_VALUE;
        return -1;
    }
    
    vhci_driver->nports = 8;
    vhci_driver->vhci_handle = vhci_handle;
    
    return 0;
}

void SokolClientDriver::sokol_vhci_driver_close(void) {
    if (vhci_driver) {
        free(vhci_driver);
        vhci_driver = NULL;
    }
    
    if (vhci_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(vhci_handle);
        vhci_handle = INVALID_HANDLE_VALUE;
    }
}

int SokolClientDriver::sokol_vhci_get_free_port(uint32_t speed) {
    if (!vhci_driver) return -1;
    
    for (int i = 0; i < vhci_driver->nports; i++) {
        if (vhci_driver->idev[i].status == VDEV_ST_NULL ||
            vhci_driver->idev[i].status == VDEV_ST_NOTASSIGNED) {
            return i;
        }
    }
    
    return -1;
}

int SokolClientDriver::windows_vhci_attach_device(uint8_t port, int sockfd, uint32_t devid, uint32_t speed) {
    if (vhci_handle == INVALID_HANDLE_VALUE) return -1;
    
    /* In Windows, we would use DeviceIoControl to attach the device */
    /* This is a simplified stub - actual implementation depends on usbip-win driver */
    
    struct {
        uint8_t port;
        int sockfd;
        uint32_t devid;
        uint32_t speed;
    } attach_params;
    
    attach_params.port = port;
    attach_params.sockfd = sockfd;
    attach_params.devid = devid;
    attach_params.speed = speed;
    
    DWORD bytesReturned;
    BOOL result = DeviceIoControl(
        vhci_handle,
        USBIP_VHCI_IOCTL_ATTACH,
        &attach_params,
        sizeof(attach_params),
        NULL,
        0,
        &bytesReturned,
        NULL
    );
    
    if (!result) {
        qDebug() << "DeviceIoControl ATTACH failed. Error:" << GetLastError();
        return -1;
    }
    
    /* Update internal state */
    vhci_driver->idev[port].status = VDEV_ST_USED;
    vhci_driver->idev[port].devid = devid;
    vhci_driver->idev[port].port = port;
    
    return 0;
}

int SokolClientDriver::windows_vhci_detach_device(uint8_t port) {
    if (vhci_handle == INVALID_HANDLE_VALUE) return -1;
    
    struct {
        uint8_t port;
    } detach_params;
    
    detach_params.port = port;
    
    DWORD bytesReturned;
    BOOL result = DeviceIoControl(
        vhci_handle,
        USBIP_VHCI_IOCTL_DETACH,
        &detach_params,
        sizeof(detach_params),
        NULL,
        0,
        &bytesReturned,
        NULL
    );
    
    if (!result) {
        qDebug() << "DeviceIoControl DETACH failed. Error:" << GetLastError();
        return -1;
    }
    
    /* Update internal state */
    vhci_driver->idev[port].status = VDEV_ST_NULL;
    vhci_driver->idev[port].devid = 0;
    
    return 0;
}

int SokolClientDriver::sokol_vhci_attach_device(uint8_t port, int sockfd,
                uint8_t busnum, uint8_t devnum, uint32_t speed) {
    uint32_t devid = (busnum << 16) | devnum;
    return windows_vhci_attach_device(port, sockfd, devid, speed);
}

int SokolClientDriver::sokol_vhci_detach_device(uint8_t port) {
    return windows_vhci_detach_device(port);
}

int SokolClientDriver::refresh_imported_device_list(void) {
    /* In Windows, we would query the driver for current device status */
    /* This is a stub implementation */
    if (!vhci_driver) return -1;
    
    /* Initialize all ports as NULL */
    for (int i = 0; i < vhci_driver->nports; i++) {
        if (vhci_driver->idev[i].status != VDEV_ST_USED) {
            vhci_driver->idev[i].status = VDEV_ST_NULL;
        }
    }
    
    return 0;
}

int SokolClientDriver::sokol_vhci_refresh_device_list(void) {
    return refresh_imported_device_list();
}

int SokolClientDriver::import_device(int sockfd, struct sokol_usb_device *udev) {
    int rc;
    int port;
    uint32_t speed = udev->sokolDeviceSpeed;
    
    rc = sokol_vhci_driver_open();
    if (rc < 0) {
        qDebug() << "Failed to open vhci_driver";
        goto err_out;
    }
    
    do {
        port = sokol_vhci_get_free_port(speed);
        if (port < 0) goto err_driver_close;
        
        rc = sokol_vhci_attach_device(port, sockfd, 
                                      udev->sokolDeviceBusnum,
                                      udev->sokolDeviceDevnum, 
                                      udev->sokolDeviceSpeed);
        if (rc < 0) goto err_driver_close;
        
    } while (rc < 0);
    
    sokol_vhci_driver_close();
    
    return port;
    
err_driver_close:
    sokol_vhci_driver_close();
err_out:
    return -1;
}

int SokolClientDriver::start_import_device(const char *busid, QString sokolhost, 
                                           QString login, QString password, 
                                           QString sokolport) {
    int rc, port;
    struct op_import_request request;
    struct op_import_reply   reply;
    uint16_t code = OP_REP_IMPORT;
    int status;
    
    memset(&request, 0, sizeof(request));
    memset(&reply, 0, sizeof(reply));
    
    client = new SokolNetworkClient;
    qDebug() << "driver network client created";
    
    rc = client->setupTcpConnection(sokolhost, sokolport);
    if(0 > rc) {
        delete client;
        return rc;
    }
    
    if(client->authRequest(login, password) == -18) {
        delete client;
        return -1;
    }
    
    rc = client->sokolSendCommonRequest(OP_REQ_IMPORT, 0);
    if (0 > rc) {
        delete client;
        return rc;
    }
    
    strncpy(request.busid, busid, SOKOL_SYSFS_BUS_ID-1);
    
    rc = client->sokolNetworkSend((void *) &request, sizeof(request));
    if (0 > rc) {
        delete client;
        return rc;
    }
    
    rc = client->sokolReceiveCommonReply(&code, &status);
    if (0 > rc) {
        delete client;
        return rc;
    }
    
    rc = client->sokolNetworkRecv((void *) &reply, sizeof(reply));
    if (0 > rc) {
        delete client;
        return rc;
    }
    
    client->sokolNetworkPackUsbDevice(0, &reply.udev);
    
    /* Verify busid matches */
    if (strncmp(reply.udev.sokolDeviceBusid, busid, SOKOL_SYSFS_BUS_ID)) {
        delete client;
        return -1;
    }
    
    qDebug() << "Importing device...";
    port = import_device(client->getSocket(), &reply.udev);
    client->closeSocket();
    delete client;
    qDebug() << "driver network client deleted";
    return port;
}

int SokolClientDriver::detach_port(int portnum) {
    int ret = 0;
    int i;
    struct sokol_imported_device *idev;
    bool found = false;
    
    ret = sokol_vhci_driver_open();
    if (ret < 0) {
        qDebug() << "Failed to open vhci_driver";
        return -1;
    }
    
    /* Check ports */
    for (i = 0; i < vhci_driver->nports; i++) {
        idev = &vhci_driver->idev[i];
        
        if (idev->port == portnum) {
            found = true;
            if (idev->status != VDEV_ST_NULL)
                break;
            
            goto call_driver_close;
        }
    }
    
    if (!found) {
        ret = -1;
        goto call_driver_close;
    }
    
    ret = !sokol_vhci_detach_device(portnum) ? 0 : -1;
    
call_driver_close:
    sokol_vhci_driver_close();
    
    return ret;
}

#endif /* _WIN32 */
