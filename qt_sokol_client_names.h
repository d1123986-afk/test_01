#ifndef QT_SOKOL_CLIENT_NAMES
#define QT_SOKOL_CLIENT_NAMES

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
// #include <ctype.h>
#include <QDebug>
// #include <QMessageBox>
// #include <QWidget>

struct vendor {
	struct vendor *next;
	uint16_t vendorid;
	char name[1];
};

struct product {
	struct product *next;
	uint16_t vendorid, productid;
	char name[1];
};

struct Class {
	struct Class *next;
	uint8_t Classid;
	char name[1];
};

struct subclass {
	struct subclass *next;
	uint8_t Classid, subClassid;
	char name[1];
};

struct protocol {
	struct protocol *next;
	uint8_t Classid, subClassid, protocolid;
	char name[1];
};

struct genericstrtable {
	struct genericstrtable *next;
	unsigned int num;
	char name[1];
};

struct pool {
	struct pool *next;
	void *mem;
};

#define HASH1  0x10
#define HASH2  0x02
#define HASHSZ 16

class usbNames{
    private:
        struct vendor *vendors[HASHSZ] = {NULL,};
        struct product *products[HASHSZ] = {NULL,};
        struct Class *classes[HASHSZ] = {NULL,};
        struct subclass *subclasses[HASHSZ] = {NULL,};
        struct protocol *protocols[HASHSZ] = {NULL,};

        const char* namesVendor(uint16_t);
        const char* namesProduct(uint16_t,uint16_t);
        const char* namesClass(uint8_t);
        const char* namesSubclass(uint8_t,uint8_t);
        const char* namesProtocol(uint8_t,uint8_t,uint8_t);

        std::string usbids = "/usr/share/hwdata/usb.ids";
    public:
        usbNames();
        ~usbNames();
        void getProduct(char*,size_t,uint16_t,uint16_t);
        void getClass(char*,size_t,uint8_t,uint8_t,uint8_t);
    private:
        void usbids_init(const char*);
        void usbids_free(void);
        unsigned int hashnum(unsigned int);
        void* myMalloc(size_t);
        void parse(FILE*);
        int newVendor(const char*,uint16_t);
        int newProduct(const char*,uint16_t,uint16_t);
        int newClass(const char*,uint8_t);
        int newSubclass(const char*,uint8_t,uint8_t);
        int newProtocol(const char*,uint8_t,uint8_t,uint8_t);
};

#endif
