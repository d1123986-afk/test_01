#include "qt_sokol_client_network.h"

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <errno.h>
    #include <fcntl.h>
#endif

SokolNetworkClient::SokolNetworkClient(){

	sockfd = -1;
    qDebug() << "SokolNetworkClient class constructor";
}

SokolNetworkClient::~SokolNetworkClient(){

    qDebug() << "SokolNetworkClient class destructor";
#ifdef _WIN32
    WSACleanup();
#endif
}

uint16_t SokolNetworkClient::sokolNetworkPackUint16(int pack, uint16_t num){

	uint16_t i;

	if (pack)
		i = htons(num);
	else
		i = ntohs(num);

	return i;
}

void SokolNetworkClient::sokolNetworkPackUsbDevice(int pack, struct sokol_usb_device *udev){

    udev->sokolDeviceBusnum = sokolNetworkPackUint32(pack, udev->sokolDeviceBusnum);
    udev->sokolDeviceDevnum = sokolNetworkPackUint32(pack, udev->sokolDeviceDevnum);
    udev->sokolDeviceSpeed = sokolNetworkPackUint32(pack, udev->sokolDeviceSpeed);

    udev->sokolDeviceVendorId = sokolNetworkPackUint16(pack, udev->sokolDeviceVendorId);
    udev->sokolDeviceProductId = sokolNetworkPackUint16(pack, udev->sokolDeviceProductId);
    udev->sokolDeviceBcdDevice = sokolNetworkPackUint16(pack, udev->sokolDeviceBcdDevice);
}

void SokolNetworkClient::sokolNetworkPackCommon(int pack, struct op_common *op_common){

    op_common->version = sokolNetworkPackUint16(pack, op_common->version);
    op_common->code = sokolNetworkPackUint16(pack, op_common->code);
    op_common->status = sokolNetworkPackUint32(pack, op_common->status);
}

ssize_t SokolNetworkClient::SendRecv(void *buff, size_t bufflen,
			      int sending)
{
	ssize_t nbytes;
	ssize_t total = 0;

	if (!bufflen)
		return 0;

	do {
        if (sending){
            nbytes = send(sockfd, buff, bufflen, 0);
            qDebug() << "Send() done";
        }
        else{
            nbytes = recv(sockfd, buff, bufflen, MSG_WAITALL);
            qDebug() << "Recv() done";
        }

		if (nbytes <= 0)
			return -1;

		buff	 = (void *)((intptr_t) buff + nbytes);
		bufflen	-= nbytes;
		total	+= nbytes;

	} while (bufflen > 0);

	return total;
}

ssize_t SokolNetworkClient::sokolNetworkSend(void *buff, size_t bufflen)
{
    return SendRecv(buff, bufflen, 1);
}

ssize_t SokolNetworkClient::sokolNetworkRecv(void *buff, size_t bufflen)
{
    return SendRecv(buff, bufflen, 0);
}

int SokolNetworkClient::sokolNetworkSetNoDelay(){

	const int val = 1;
	int ret;

	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    if(ret < 0) qDebug() << "TCP_NODELAY failed";

	return ret;
}

int SokolNetworkClient::sokolNetworkSetKeepAlive(){

	const int val = 1;
	int ret;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
    if(ret < 0) qDebug() << "SO_KEEPALIVE failed";

	return ret;
}

int SokolNetworkClient::sokolSendCommonRequest(uint32_t code, uint32_t status)
{
	struct op_common op_common;
	int rc;

	memset(&op_common, 0, sizeof(op_common));

	op_common.version = USBIP_VERSION;
	op_common.code    = code;
	op_common.status  = status;

    sokolNetworkPackCommon(1, &op_common);

    // qDebug() << sockfd;

    rc = sokolNetworkSend(&op_common, sizeof(op_common));
	if (rc < 0) {
        qDebug() << "sokolNetworkSend() failed";
		return -1;
	}

    qDebug() << "Sent: " << rc;

	return 0;
}

int SokolNetworkClient::_setup_tcp_connect(const char *hostname, const char *service){
#ifdef _WIN32
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0) {
        qDebug() << "WSAStartup failed";
        return ret;
    }
#endif

	struct addrinfo hints, *res, *rp;
	int ret;

	memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_INET;
    hints.ai_socktype   = SOCK_STREAM;

	ret = getaddrinfo(hostname, service, &hints, &res);
	if (ret < 0) {
        qDebug() << "The host is unavailable now";
		return ret;
	}

	for (rp = res; rp; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (sockfd < 0)
			continue;

        sokolNetworkSetNoDelay();
        sokolNetworkSetKeepAlive();

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;

#ifdef _WIN32
        closesocket(sockfd);
#else
		close(sockfd);
#endif
	}

	freeaddrinfo(res);

	if (!rp)
		return EAI_SYSTEM;

    qDebug() << "<-[Connection with " << hostname << " established]->";
    return 0;
}

int SokolNetworkClient::sokolReceiveCommonReply(uint16_t *code, int *status){

	struct op_common op_common;
	int rc;

	memset(&op_common, 0, sizeof(op_common));

    rc = sokolNetworkRecv(&op_common, sizeof(op_common));
	if (rc < 0) {
        qDebug() << "sokolNetworkRecv() failed";
        return -1;
	}

    qDebug() << "Received: " << rc;

    sokolNetworkPackCommon(0, &op_common);

	if (op_common.version != USBIP_VERSION) {
        qDebug() << "Wrong version. Sorry.";
        return -1;
	}

	switch (*code) {
	case OP_UNSPEC:
		break;
	default:
		if (op_common.code != *code) {

			/* return error status */
			*status = ST_ERROR;
            return -1;
		}
	}

	*status = op_common.status;

	if (op_common.status != ST_OK) {

        return -1;
	}

	*code = op_common.code;

	return 0;
}

/////////////////////////////////////////
/// попытка авторизоваться на сервере
/// @param имя сокола-хоста
/// return результат авторизации
int SokolNetworkClient::authRequest(QString log, QString pass){

    struct auth auth_request;
    struct op_auth_reply rep;

    memset(&auth_request, 0, sizeof(struct auth));
    memset(&rep, 0, sizeof(struct op_auth_reply));

    rep.returncode = -18;

    QByteArray ll = log.toLocal8Bit();
    QByteArray pp = pass.toLocal8Bit();

    strncpy(auth_request.l_log, ll.data(), ll.size());
    strncpy(auth_request.p_pass, pp.data(), pp.size());

    qDebug() << auth_request.l_log;
    qDebug() << auth_request.p_pass;

    sokolNetworkSend(&auth_request, sizeof(struct auth));
    sokolNetworkRecv(&rep, sizeof(struct op_auth_reply));

    return rep.returncode;
}

int SokolNetworkClient::setupTcpConnection(QString hostname, QString portname){
	
	QByteArray h = hostname.toLocal8Bit();
	QByteArray p = portname.toLocal8Bit();
	const char* host = h.data();
	const char* port = p.data();
    return _setup_tcp_connect(host, port);
}
