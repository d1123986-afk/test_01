#include"qt_sokol_client_driver.h"

SokolClientDriver::SokolClientDriver(){ qDebug() << "SokolClientDriver class constructor"; }

SokolClientDriver::~SokolClientDriver(){ qDebug() << "SokolClientDriver class destructor"; }

int SokolClientDriver::sokol_vhci_driver_open(void){

    int nports;
    struct udev_device *hc_device;

    udev_context = udev_new();
    if (!udev_context)return -1;

    hc_device =
        udev_device_new_from_subsystem_sysname(udev_context,
                               SOKOL_VHCI_BUS_TYPE,
                               SOKOL_VHCI_DEVICE_NAME);
    if (!hc_device)goto err;

    nports = getDriverPorts(hc_device);
    if (nports <= 0)goto err;

    vhci_driver = (struct sokol_vhci_driver*)calloc(1, sizeof(struct sokol_vhci_driver) +
            nports * sizeof(struct sokol_imported_device));
    if (!vhci_driver)goto err;

    vhci_driver->nports = nports;
    vhci_driver->hc_device = hc_device;
    vhci_driver->ncontrollers = getDriverControllers();

    if (vhci_driver->ncontrollers <= 0)goto err;

    if (refresh_imported_device_list())goto err;

    return 0;
err:
    udev_device_unref(hc_device);

    if (vhci_driver)
        free(vhci_driver);

    vhci_driver = NULL;

    udev_unref(udev_context);

    return -1;

}

void SokolClientDriver::sokol_vhci_driver_close(void){

    if (!vhci_driver)
        return;

    udev_device_unref(vhci_driver->hc_device);

    free(vhci_driver);

    vhci_driver = NULL;

    udev_unref(udev_context);
}

int SokolClientDriver::getDriverPorts(struct udev_device *hc_device){

	const char *attr_nports;

	attr_nports = udev_device_get_sysattr_value(hc_device, "nports");
    if (!attr_nports)return -1;

	return (int)strtoul(attr_nports, NULL, 10);
}

static int vhci_hcd_filter(const struct dirent *dirent)
{
	return !strncmp(dirent->d_name, "vhci_hcd.", 9);
}

int SokolClientDriver::getDriverControllers(void){

	struct dirent **namelist;
	struct udev_device *platform;
	int n;

	platform = udev_device_get_parent(vhci_driver->hc_device);
	if (platform == NULL)return -1;

	n = scandir(udev_device_get_syspath(platform), &namelist, vhci_hcd_filter, NULL);

		for (int i = 0; i < n; i++)
			free(namelist[i]);
		free(namelist);

	return n;
}


int SokolClientDriver::sokol_vhci_refresh_device_list(void){
	if (refresh_imported_device_list())
		goto err;

	return 0;
err:
	return -1;
}

int SokolClientDriver::sokol_vhci_get_free_port(uint32_t speed){

	for (int i = 0; i < vhci_driver->nports; i++) {

		switch (speed) {
		case	USB_SPEED_SUPER:
			if (vhci_driver->idev[i].hub != HUB_SPEED_SUPER)
				continue;
		break;
		default:
			if (vhci_driver->idev[i].hub != HUB_SPEED_HIGH)
				continue;
		break;
		}

		if (vhci_driver->idev[i].status == VDEV_ST_NULL)
			return vhci_driver->idev[i].port;
	}

	return -1;
}

int SokolClientDriver::sokol_vhci_attach_device(uint8_t port, int sockfd,
				uint8_t busnum, uint8_t devnum, uint32_t speed){

    char buff[256];
    char attach_attr_path[SOKOL_SYSFS_PATH];
	char attr_attach[] = "attach";
	const char *path;
	int devid;
	int ret;

	devid = busnum << 16 | devnum;

	snprintf(buff, sizeof(buff), "%u %d %u %u",
			port, sockfd, devid, speed);

    qDebug() << buff;

	path = udev_device_get_syspath(vhci_driver->hc_device);

	snprintf(attach_attr_path, sizeof(attach_attr_path), "%s/%s",
		 path, attr_attach);

    qDebug() << attach_attr_path;

	ret = write_sysfs_attribute(attach_attr_path, buff, strlen(buff));
    if (ret < 0)return -1;

	return 0;
}

int SokolClientDriver::sokol_vhci_detach_device(uint8_t port){

    char detach_attr_path[SOKOL_SYSFS_PATH];
	char attr_detach[] = "detach";
    char buff[33];
	const char *path;
	int ret;

	snprintf(buff, sizeof(buff), "%u", port);

    qDebug() << port;

	path = udev_device_get_syspath(vhci_driver->hc_device);
	snprintf(detach_attr_path, sizeof(detach_attr_path), "%s/%s",
		 path, attr_detach);

    qDebug() << detach_attr_path;

	ret = write_sysfs_attribute(detach_attr_path, buff, strlen(buff));
    if (ret < 0)return -1;

	return 0;
}

int SokolClientDriver::refresh_imported_device_list(void){

	const char *attr_status;
	char status[128] = "status";
	int i, ret;

	for (i = 0; i < vhci_driver->ncontrollers; i++) {

		if (i > 0)snprintf(status, sizeof(status), "status.%d", i);

		attr_status = udev_device_get_sysattr_value(vhci_driver->hc_device, status);
        if (!attr_status)return -1;

		ret = parse_status(attr_status);
		if (ret != 0)
			return ret;
	}

	return 0;
}

int SokolClientDriver::parse_status(const char *value){

	int ret = 0;
	char *c;

	c = (char*)strchr(value, '\n');
	if (!c)return -1;
	c++;

	while (*c != '\0') {

		int port, status, speed, devid;
		int sockfd;
        char lbusid[SOKOL_SYSFS_BUS_ID];
		struct sokol_imported_device *idev;
		char hub[3];

		ret = sscanf(c, "%2s  %d %d %d %x %u %31s\n",
				hub, &port, &status, &speed,
				&devid, &sockfd, lbusid);

		idev = &vhci_driver->idev[port];
		memset(idev, 0, sizeof(*idev));

		if (strncmp("hs", hub, 2) == 0)
			idev->hub = HUB_SPEED_HIGH;
        else
			idev->hub = HUB_SPEED_SUPER;

		idev->port	= port;
		idev->status	= status;

		idev->devid	= devid;

		idev->busnum	= (devid >> 16);
		idev->devnum	= (devid & 0x0000ffff);

		if (idev->status != VDEV_ST_NULL
		    && idev->status != VDEV_ST_NOTASSIGNED) {
			idev = imported_device_init(idev, lbusid);
            if (!idev)return -1;
		}

		/* go to the next line */
		c = strchr(c, '\n');
		if (!c)
			break;
		c++;
	}

	return 0;
}

struct sokol_imported_device * SokolClientDriver::imported_device_init(struct sokol_imported_device *idev, char *busid){
	struct udev_device *sudev;

	sudev = udev_device_new_from_subsystem_sysname(udev_context,
						       "usb", busid);
    if (!sudev)goto err;

	read_usb_device(sudev, &idev->udev);
	udev_device_unref(sudev);

	return idev;
err:
	return NULL;
}

int SokolClientDriver::read_attr_value(struct udev_device *dev, const char *name, const char *format){

	const char *attr;
	int num = 0;
	int ret;

	attr = udev_device_get_sysattr_value(dev, name);
    if (!attr)goto err;

	ret = sscanf(attr, format, &num);
    if (ret < 1)
		if (strcmp(name, "bConfigurationValue") &&
                strcmp(name, "bNumInterfaces"))goto err;
err:
	return num;
}

int read_attr_speed(struct udev_device *dev){

	const char *speed;

	speed = udev_device_get_sysattr_value(dev, "speed");

    if (!speed)goto err;

    for (int i = 0; usb_speed_types[i].sString != NULL; i++)
		if (!strcmp(speed, usb_speed_types[i].sString))
			return usb_speed_types[i].sValue;

err:
	return USB_SPEED_UNKNOWN;
}

#define READ_ATTR(object, type, dev, name, format)			      \
	do {								      \
		(object)->name = (type) read_attr_value(dev, to_string(name), \
							format);	      \
	} while (0)


int SokolClientDriver::read_usb_device(struct udev_device *sdev, struct sokol_usb_device *udev){

	uint32_t busnum, devnum;
	const char *path, *name;

    READ_ATTR(udev, uint8_t,  sdev, sokolDeviceClass,		"%02x\n");
    READ_ATTR(udev, uint8_t,  sdev, sokolDeviceSubClass,	"%02x\n");
    READ_ATTR(udev, uint8_t,  sdev, sokolDeviceProtocol,	"%02x\n");

    READ_ATTR(udev, uint16_t, sdev, sokolDeviceVendorId,		"%04x\n");
    READ_ATTR(udev, uint16_t, sdev, sokolDeviceProductId,		"%04x\n");
    READ_ATTR(udev, uint16_t, sdev, sokolDeviceBcdDevice,		"%04x\n");

    READ_ATTR(udev, uint8_t,  sdev, sokolConfigurationValue,	"%02x\n");
    READ_ATTR(udev, uint8_t,  sdev, sokolNumConfigurations,	"%02x\n");
    READ_ATTR(udev, uint8_t,  sdev, sokolNumInterfaces,		"%02x\n");

    READ_ATTR(udev, uint8_t,  sdev, sokolDeviceDevnum,			"%d\n");
    udev->sokolDeviceSpeed = read_attr_speed(sdev);

	path = udev_device_get_syspath(sdev);
	name = udev_device_get_sysname(sdev);

    strncpy(udev->sokolDevicePath,  path,  SOKOL_SYSFS_PATH - 1);
    udev->sokolDevicePath[SOKOL_SYSFS_PATH - 1] = '\0';
    strncpy(udev->sokolDeviceBusid, name, SOKOL_SYSFS_BUS_ID - 1);
    udev->sokolDeviceBusid[SOKOL_SYSFS_BUS_ID - 1] = '\0';

    sscanf(name, "%u-%u", &busnum, &devnum);
    udev->sokolDeviceBusnum = busnum;
    udev->sokolDeviceDevnum = devnum;

	return 0;
}

int SokolClientDriver::start_import_device(const char *busid, QString sokolhost, QString login, QString password, QString sokolport){

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
    if(0 > rc){
        delete client;
        return rc;
    }

    if(client->authRequest(login, password) == -18)return -1;

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
    // PACK_OP_IMPORT_REPLY(0, &reply);

    /* сопоставить идентификаторы шины */
    if (strncmp(reply.udev.sokolDeviceBusid, busid, SOKOL_SYSFS_BUS_ID)) {
        delete client;
        return rc;
    }

    qDebug() << "Importing this...";
    port = import_device(client->getSocket(), &reply.udev);
    client->closeSocket();
    delete client;
    qDebug() << "driver network client deleted";
    return port;
}


int SokolClientDriver::import_device(int sockfd, struct sokol_usb_device *udev){

    int rc;
    int port;
    uint32_t speed = udev->sokolDeviceSpeed;
    const char* no_vhci_hcd = "open vhci_driver (is vhci_hcd loaded?)";

    rc = sokol_vhci_driver_open();
    if (rc < 0) {
        qDebug() << no_vhci_hcd;
        goto err_out;
    }

    do {

        port = sokol_vhci_get_free_port(speed);
        if (port < 0)goto err_driver_close;

        rc = sokol_vhci_attach_device(port, sockfd, udev->sokolDeviceBusnum,
                                             udev->sokolDeviceDevnum, udev->sokolDeviceSpeed);
        if (rc < 0 && errno != EBUSY)goto err_driver_close;

    } while (rc < 0);

    sokol_vhci_driver_close();

    return port;

err_driver_close:
    sokol_vhci_driver_close();
err_out:
    return -1;
}

int SokolClientDriver::detach_port(int portnum){

    int ret = 0;
    char path[PATH_MAX+1];
    int i;
    struct sokol_imported_device *idev;
    bool found = false;

    ret = sokol_vhci_driver_open();
    if (ret < 0) {
        qDebug() << "open vhci_driver (is vhci_hcd loaded?)";
        return -1;
    }

    /* проверить порты */
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

    /* удалить файл состояния */
    snprintf(path, PATH_MAX, VHCI_STATE_PATH"/port%d", portnum);

    remove(path);
    rmdir(VHCI_STATE_PATH);

    ret = !sokol_vhci_detach_device(portnum) ? 0 : -1;

call_driver_close:
    sokol_vhci_driver_close();

    return ret;
}

int SokolClientDriver::write_sysfs_attribute(const char *attr_path, const char *new_value,
			  size_t len)
{
	int fd;
	int length;

	fd = open(attr_path, O_WRONLY);
    if (fd < 0)return -1;

	length = write(fd, new_value, len);
	if (length < 0) {

		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}
