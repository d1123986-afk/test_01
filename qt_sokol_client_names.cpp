#include "qt_sokol_client_names.h"

static struct pool *pool_head;

unsigned int usbNames::hashnum(unsigned int num){
	unsigned int mask1 = HASH1 << 27, mask2 = HASH2 << 27;

	for (; mask1 >= HASH1; mask1 >>= 1, mask2 >>= 1)
		if (num & mask1)
			num ^= mask2;
	return num & (HASHSZ-1);
}

const char* usbNames::namesVendor(uint16_t vendorid)
{
	struct vendor *v;

	v = vendors[hashnum(vendorid)];
	for (; v; v = v->next)
		if (v->vendorid == vendorid)
			return v->name;
	return NULL;
}

const char* usbNames::namesProduct(uint16_t vendorid, uint16_t productid)
{
	struct product *p;

	p = products[hashnum((vendorid << 16) | productid)];
	for (; p; p = p->next)
		if (p->vendorid == vendorid && p->productid == productid)
			return p->name;
	return NULL;
}

const char* usbNames::namesClass(uint8_t Classid)
{
	struct Class *c;

	c = classes[hashnum(Classid)];
	for (; c; c = c->next)
		if (c->Classid == Classid)
			return c->name;
	return NULL;
}

const char* usbNames::namesSubclass(uint8_t Classid, uint8_t subClassid)
{
	struct subclass *s;

	s = subclasses[hashnum((Classid << 8) | subClassid)];
	for (; s; s = s->next)
		if (s->Classid == Classid && s->subClassid == subClassid)
			return s->name;
	return NULL;
}

const char* usbNames::namesProtocol(uint8_t Classid, uint8_t subClassid,
			   uint8_t protocolid){
	struct protocol *p;

	p = protocols[hashnum((Classid << 16) | (subClassid << 8)
			      | protocolid)];
	for (; p; p = p->next)
		if (p->Classid == Classid && p->subClassid == subClassid &&
		    p->protocolid == protocolid)
			return p->name;
	return NULL;
}

void* usbNames::myMalloc(size_t size){
	struct pool *p;

	p = (struct pool*)calloc(1, sizeof(struct pool));
	if (!p)
		return NULL;

	p->mem = calloc(1, size);
	if (!p->mem) {
		free(p);
		return NULL;
	}

	p->next = pool_head;
	pool_head = p;

	return p->mem;
}

void usbNames::usbids_free(void){
	struct pool *pool;

	if (!pool_head)
		return;

	for (pool = pool_head; pool != NULL; ) {
		struct pool *tmp;

		if (pool->mem)
			free(pool->mem);

		tmp = pool;
		pool = pool->next;
		free(tmp);
	}
}

void usbNames::parse(FILE* f){

	char buf[512], *cp;
	unsigned int linectr = 0;
	int lastvendor = -1;
	int lastClass = -1;
	int lastsubClass = -1;
	int lasthut = -1;
	int lastlang = -1;
	unsigned int u;

	while (fgets(buf, sizeof(buf), f)) {
		linectr++;
		/* remove line ends */
		cp = strchr(buf, '\r');
		if (cp)
			*cp = 0;
		cp = strchr(buf, '\n');
		if (cp)
			*cp = 0;
		if (buf[0] == '#' || !buf[0])
			continue;
		cp = buf;
		if (buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'Y' &&
		    buf[3] == 'S' && buf[4] == 'D' &&
		    buf[5] == 'E' && buf[6] == 'S' && /*isspace(buf[7])*/
		    buf[7] == ' ') {
			continue;
		}
		if (buf[0] == 'P' && buf[1] == 'H' &&
		    buf[2] == 'Y' && /*isspace(buf[3])*/ buf[3] == ' ') {
			continue;
		}
		if (buf[0] == 'B' && buf[1] == 'I' && buf[2] == 'A' &&
		    buf[3] == 'S' && /*isspace(buf[4])*/ buf[4] == ' ') {
			continue;
		}
		if (buf[0] == 'L' && /*isspace(buf[1])*/ buf[1] == ' ') {
			lasthut = lastClass = lastvendor = lastsubClass = -1;
			/*
			 * set 1 as pseudo-id to indicate that the parser is
			 * in a `L' section.
			 */
			lastlang = 1;
			continue;
		}
		if (buf[0] == 'C' && /*isspace(buf[1])*/ buf[1] == ' ') {
			/* Class spec */
			cp = buf+2;
			while (isspace(*cp))
				cp++;
			if (!isxdigit(*cp)) {
				continue;
			}
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				continue;
			}
			if (newClass(cp, u))
			lasthut = lastlang = lastvendor = lastsubClass = -1;
			lastClass = u;
			continue;
		}
		if (buf[0] == 'A' && buf[1] == 'T' && isspace(buf[2])) {
			/* audio terminal type spec */
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'C' && buf[2] == 'C'
		    && isspace(buf[3])) {
			/* HID Descriptor bCountryCode */
			continue;
		}
		if (isxdigit(*cp)) {
			/* vendor */
			u = strtoul(cp, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				continue;
			}
			if (newVendor(cp, u))
			lastvendor = u;
			lasthut = lastlang = lastClass = lastsubClass = -1;
			continue;
		}
		if (buf[0] == '\t' && isxdigit(buf[1])) {
			/* product or subClass spec */
			u = strtoul(buf+1, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {
				continue;
			}
			if (lastvendor != -1) {
				if (newProduct(cp, lastvendor, u))
				continue;
			}
			if (lastClass != -1) {
				if (newSubclass(cp, lastClass, u))
				lastsubClass = u;
				continue;
			}
			if (lasthut != -1) {
				/* do not store hut */
				continue;
			}
			if (lastlang != -1) {
				/* do not store langid */
				continue;
			}
			continue;
		}
		if (buf[0] == '\t' && buf[1] == '\t' && isxdigit(buf[2])) {
			/* protocol spec */
			u = strtoul(buf+2, &cp, 16);
			while (isspace(*cp))
				cp++;
			if (!*cp) {

				continue;
			}
			if (lastClass != -1 && lastsubClass != -1) {
				if (newProtocol(cp, lastClass, lastsubClass,
						 u))
				continue;
			}
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'I' &&
		    buf[2] == 'D' && /*isspace(buf[3])*/ buf[3] == ' ') {
			continue;
		}
		if (buf[0] == 'H' && buf[1] == 'U' &&
		    buf[2] == 'T' && /*isspace(buf[3])*/ buf[3] == ' ') {
			lastlang = lastClass = lastvendor = lastsubClass = -1;
			/*
			 * set 1 as pseudo-id to indicate that the parser is
			 * in a `HUT' section.
			 */
			lasthut = 1;
			continue;
		}
		if (buf[0] == 'R' && buf[1] == ' ')
			continue;

		if (buf[0] == 'V' && buf[1] == 'T')
			continue;
	}
}

int usbNames::newVendor(const char *name, uint16_t vendorid)
{
	struct vendor *v;
	unsigned int h = hashnum(vendorid);

	v = vendors[h];
	for (; v; v = v->next)
		if (v->vendorid == vendorid)
			return -1;
	v = (struct vendor*)myMalloc(sizeof(struct vendor) + strlen(name));
	if (!v)
		return -1;
	strcpy(v->name, name);
	v->vendorid = vendorid;
	v->next = vendors[h];
	vendors[h] = v;
	return 0;
}

int usbNames::newProduct(const char *name, uint16_t vendorid,
		       uint16_t productid)
{
	struct product *p;
	unsigned int h = hashnum((vendorid << 16) | productid);

	p = products[h];
	for (; p; p = p->next)
		if (p->vendorid == vendorid && p->productid == productid)
			return -1;
	p = (struct product*)myMalloc(sizeof(struct product) + strlen(name));
	if (!p)
		return -1;
	strcpy(p->name, name);
	p->vendorid = vendorid;
	p->productid = productid;
	p->next = products[h];
	products[h] = p;
	return 0;
}

int usbNames::newClass(const char *name, uint8_t Classid)
{
	struct Class *c;
	unsigned int h = hashnum(Classid);

	c = classes[h];
	for (; c; c = c->next)
		if (c->Classid == Classid)
			return -1;
	c = (struct Class*)myMalloc(sizeof(struct Class) + strlen(name));
	if (!c)
		return -1;
	strcpy(c->name, name);
	c->Classid = Classid;
	c->next = classes[h];
	classes[h] = c;
	return 0;
}

int usbNames::newSubclass(const char *name, uint8_t Classid, uint8_t subClassid)
{
	struct subclass *s;
	unsigned int h = hashnum((Classid << 8) | subClassid);

	s = subclasses[h];
	for (; s; s = s->next)
		if (s->Classid == Classid && s->subClassid == subClassid)
			return -1;
	s = (struct subclass*)myMalloc(sizeof(struct subclass) + strlen(name));
	if (!s)
		return -1;
	strcpy(s->name, name);
	s->Classid = Classid;
	s->subClassid = subClassid;
	s->next = subclasses[h];
	subclasses[h] = s;
	return 0;
}

int usbNames::newProtocol(const char *name, uint8_t Classid, uint8_t subClassid,
			uint8_t protocolid)
{
	struct protocol *p;
	unsigned int h = hashnum((Classid << 16) | (subClassid << 8)
				 | protocolid);

	p = protocols[h];
	for (; p; p = p->next)
		if (p->Classid == Classid && p->subClassid == subClassid
		    && p->protocolid == protocolid)
			return -1;
	p = (struct protocol*)myMalloc(sizeof(struct protocol) + strlen(name));
	if (!p)
		return -1;
	strcpy(p->name, name);
	p->Classid = Classid;
	p->subClassid = subClassid;
	p->protocolid = protocolid;
	p->next = protocols[h];
	protocols[h] = p;
	return 0;
}

void usbNames::usbids_init(const char *n){

    FILE *fSrc;

    fSrc = fopen(n, "r");
    if (!fSrc){
        qDebug() << "Не удалось открыть файл";
        return;
    }

    qDebug() << "parse() start";
    parse(fSrc);
    qDebug() << "parse() done";
    fclose(fSrc);
}

void usbNames::getProduct(char *buff, size_t size, uint16_t vendor,
			     uint16_t product)
{
	const char *prod, *vend;

	prod = namesProduct(vendor, product);
	if (!prod)
		prod = "unknown product";


	vend = namesVendor(vendor);
	if (!vend)
		vend = "unknown vendor";

	snprintf(buff, size, "%s", vend);
}

void usbNames::getClass(char *buff, size_t size, uint8_t class1,
			   uint8_t subclass, uint8_t protocol)
{
	const char *c, *s, *p;

	if (class1 == 0 && subclass == 0 && protocol == 0) {
		snprintf(buff, size, "(Defined at Interface level) (%02x/%02x/%02x)", class1, subclass, protocol);
		return;
	}

	p = namesProtocol(class1, subclass, protocol);
	if (!p)
		p = "unknown protocol";

	s = namesSubclass(class1, subclass);
	if (!s)
		s = "unknown subclass";

	c = namesClass(class1);
	if (!c)
		c = "unknown class";

	snprintf(buff, size, "%s / %s / %s (%02x/%02x/%02x)", c, s, p, class1, subclass, protocol);
}

usbNames::usbNames(){

    usbids_init(usbids.c_str());
    qDebug() << "usbNames class constructor";

}

usbNames::~usbNames(){

    usbids_free();
    qDebug() << "usbNames class destructor";
}
