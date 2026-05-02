#include "qt_sokol_new_server.h"
#include <sys/stat.h>

sokolMainWindow::sokolMainWindow(QString port){

			w = new QWidget;

            ////////////////////////////////////////////////////////
            /// создаём модальное окно (ip/логин/пароль
            ///////////////////////////////////////////////////
			srvr = new NewSokolServer(this);
			srvr->setWindowFlags(Qt::Dialog);
            // srvr->setWindowFlags(windowFlags() & ~(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint));
            srvr->setWindowModality(Qt::WindowModal);

            driver = new SokolClientDriver;
            net    = new SokolNetworkClient;
            usbs   = new usbNames;

			//////////////////////////////////////////////
            /// таблица справа с информацией от USBIP
			//////////////////////////////////////////
			table = new QTableWidget(this);
			rows = 0;
            columns = 5;
			table->setColumnCount(columns);
			table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            table->setHorizontalHeaderLabels({"Хост","Имя устройства","Номер шины","Номер производителя","Номер продукта"});

			portname = port;

            a.clear();

			///////////////////////////////////////////////
			/// TreeView с ветками подключённых серверов и
			/// подветками подключённых портов
			///////////////////////////////////////
			treeview = new QTreeView(w);

            model = new QStandardItemModel();
            model->setHorizontalHeaderLabels({"Серверы и устройства"});
            root  = model->invisibleRootItem();
			treeview->setModel(model);
            treeview->setHeaderHidden(false);

            base = new QStandardItem(QSysInfo::machineHostName());
            base->setEditable(false);
            base->setData(ROOT_ID, Qt::UserRole + 1);
            base->setData(QColor(Qt::magenta), Qt::ForegroundRole);
            root->appendRow(base);

            /////////////////////////////////////////////////////////
            /// установить обработчик контекстного меню
            ///////////////////////////////////////////////////////
			treeview->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(treeview, &QWidget::customContextMenuRequested,
					this, &sokolMainWindow::ShowContextMenu);

			layout = new QGridLayout(w);
			layout->addWidget(treeview, 0, 0);
			layout->addWidget(table, 0, 1);
			w->setLayout(layout);

            this->setWindowTitle(tr("Сокол-Клиент v1.0"));
			this->setCentralWidget(w);
			this->resize(1600, 800);

            qDebug() << "SokolMainWindow class constructor done";
			///////////////////////////////
            /// показать главное окно
			////////////////////////////
			this->show();
}

sokolMainWindow::~sokolMainWindow(){
    delete driver;
    delete net;
    delete usbs;

	delete treeview;
	delete model;
	delete layout;
	delete table;
	delete srvr;
	delete w;

    a.clear();

    for(const auto& [i, p] : vhciPorts)driver->detach_port(p);

    qDebug() << "SokolMainWindow class destructor done";
}

#define MAX_BUFF 100
int sokolMainWindow::recordConnection(const char *host, const char *port, const char *busid, int rhport){

	int fd;
	char path[PATH_MAX+1];
	char buff[MAX_BUFF+1];
	int ret;

	ret = mkdir(VHCI_STATE_PATH, 0700);
	if (ret < 0) {

		if (errno == EEXIST) {
			struct stat s;

			ret = stat(VHCI_STATE_PATH, &s);
			if (ret < 0)
				return -1;
			if (!(s.st_mode & S_IFDIR))
				return -1;
		} else
			return -1;
	}

	snprintf(path, PATH_MAX, VHCI_STATE_PATH"/port%d", rhport);

	fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if (fd < 0)return -1;

	snprintf(buff, MAX_BUFF, "%s %s %s\n",
			host, port, busid);

	ret = write(fd, buff, strlen(buff));
	if (ret != (ssize_t) strlen(buff)) {
        net->closeSocket();
		return -1;
	}

    net->closeSocket();

	return 0;
}

void sokolMainWindow::attachDevice(const QPoint &pos, QString device, QStandardItem* sokol){

	QModelIndex index = treeview->indexAt(pos);

	if(index.isValid()){

		if(isImported[device] == false){
			
            int good;
            int rhport = -1;
			int rc;

			QStandardItem *item = model->itemFromIndex(index);

			QString busid = busids[device];
			QByteArray bid = busid.toLocal8Bit();
            const char* bu = bid.data();

            for(unsigned i = 0; i < a.size(); i++){

                if(sokol->text() == a[i].sokolhostname){

                    rhport = driver->start_import_device(bu, sokol->text(), a[i].login, a[i].password, portname);
                    break;
                }
            }

            qDebug() << "rhport = " << rhport;
            if (rhport < 0){
                qDebug() << "[attachDevice] failed";
                return;
            }

			vhciPorts[device] = rhport;

            QByteArray h = sokol->text().toLocal8Bit();
			QByteArray p = portname.toLocal8Bit();
			const char* host = h.data();
			const char* port = p.data();
            rc = recordConnection(host, port, bu, rhport);
			if (rc < 0) {
                qDebug() << "[record_connection] failed";
                return;
			}

            // покрасить фон ячейки с именем устройства в зелёный
            for(int i = 0; i < table->rowCount(); i++){
                QTableWidgetItem* cell = this->table->item(i, 1);
                if(cell->text() == device){
                    cell->setBackground(QColor(8, 177, 23));
                    break;
                }
            }

			item->setData(QColor(Qt::green), Qt::ForegroundRole);
             /* запретить показывать контекстное меню для данного сокола */
            sokol->setData(SERVER_ID_BUSY, Qt::UserRole + 1);

			isImported[device] = true;
		}
        else QMessageBox::warning(this, "Ошибка", "Устройство " + device + " занято");
}
    qDebug() << "[attachDevice] done";
}

void sokolMainWindow::detachDevice(const QPoint& pos, QString device, QStandardItem* sokol){

    int ok;
	QModelIndex index = treeview->indexAt(pos);

		if(index.isValid()){

			if(isImported[device] == true){

				QStandardItem *item = model->itemFromIndex(index);

                ok = driver->detach_port(vhciPorts[device]);
                if(0 > ok){
                    qDebug() << "[detachDevice] failed";
                    return;
                }

                // покрасить фон ячейки с именем устройства обратно в красный
                for(int i = 0; i < table->rowCount(); i++){
                    QTableWidgetItem* cell = this->table->item(i, 1);
                    if(cell->text() == device){
                        cell->setBackground(QColor(255, 23, 23));
                        break;
                    }
                }

                item->setData(QColor(Qt::gray), Qt::ForegroundRole);
				isImported[device] = false;
                vhciPorts.erase(device);
                /* разрешить показывать контекстное меню для данного сокола */
                sokol->setData(SERVER_ID_DEFAULT, Qt::UserRole + 1);
		}
        else QMessageBox::warning(this, "Ошибка", "Устройство " + device + " не добавлено");
	}
        qDebug() << "[detachDevice] done";
}

void sokolMainWindow::deleteRows(QString server){

    qDebug() << "rowCount: " << table->rowCount();

    for(int i = 0; i < table->rowCount(); i++){

        QTableWidgetItem* cell = this->table->item(i, 0);
        qDebug() << "i -- " << i;

        if(cell){

            QString text = cell->text();
            qDebug() << i << text;
            if(text == server){
                table->removeRow(i);
                i--;
                if(rows > 0)rows--;
            }
        }
    }

    qDebug() << "rows: " << rows;
}

void sokolMainWindow::clearDevicesInfo(const std::vector<QString> &dlist){

    for(QString i : dlist)
    {
        isImported.erase(i);
        busids.erase(i);
    }
}

void sokolMainWindow::updateServerState(const QPoint& pos, QString server, const std::vector<QString> &bb){

	QModelIndex index = treeview->indexAt(pos);
    int success;

	if(index.isValid()){

        base->removeRow(index.row());

        for(unsigned i = 0; i < a.size(); i++){

            if(server == a[i].sokolhostname){

                success = net->setupTcpConnection(server, portname);
                if(success < 0){

                    QMessageBox::warning(this, "Ошибка", "Сокол " + server + " недоступен для переподключения");
                    //забыть сокола
                    deleteRows(server);
                    clearDevicesInfo(bb);
                    eraseServer(server);
                    return;

                }

                if(net->authRequest(a[i].login, a[i].password) == -18){

                    QMessageBox::warning(this, "Провал", "Ошибка авторизации");
                    clearDevicesInfo(bb);
                    return;

                }
                else break;
            }

        }

        clearDevicesInfo(bb);

        for(const auto& [d, b] : busids)qDebug() << server << ": " << "Device -> " << d << "BusID ->" << b;

        for(const auto& [d, i] : isImported)qDebug() << server << ": " << "Device -> " << d << "isImported ->" << i;

        deleteRows(server);
        getExportedDevicesList(server);
        table->setRowCount(rows);

    }

}

bool sokolMainWindow::eraseServer(QString sokol){

    for(unsigned i = 0; i < a.size(); i++){

        if(sokol == a[i].sokolhostname){

            a.erase(a.begin() + i);
            return true;
        }
    }

    return false;
}

void sokolMainWindow::deleteServer(const QPoint& pos, QString server, const std::vector<QString> &bb){

	/* Выясняем, какой сервер был выбран */
	QModelIndex index = treeview->indexAt(pos);
	/* Проверяем, что сервер был действительно выбран */
	if(index.isValid()){

        /* Задаём вопрос, стоит ли действительно удалять запись.
         * При положительном ответе удаляем запись
         * */
        if (QMessageBox::warning(this,
                                 tr("Удаление Сокола"),
                                 tr("Вы уверены, что хотите удалить Сокола ") + server + tr("?"),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        {
        	return;
        }
        else
        {
        	/* В противном случае удаляем запись о сервере */
            base->removeRow(index.row());

            if(eraseServer(server) == true)qDebug() << server << " deleted";
            else qDebug() << server << " not deleted";

            clearDevicesInfo(bb);

            deleteRows(server);
        }
	}
	for(const auto& [d, b] : busids)
        qDebug() << "[Device -> " << d << "BusID ->" << b;
	for(const auto& [d, i] : isImported)
        qDebug() << "[Device -> " << d << "isImported ->" << i;

}

void sokolMainWindow::showNewServerModalWindow(){

	srvr->host->clear();
	srvr->login->clear();
	srvr->password->clear();
	srvr->show();

}

void sokolMainWindow::addNewServer(){

    int ok;

            if(!hostname.isEmpty()){
                for (unsigned i = 0; i < a.size(); i++){

                        /* не добавлять один и тот же хост второй раз */
                        if(a[i].sokolhostname == hostname){

                            QMessageBox::warning(this, tr("Ошибка"), hostname + tr(" уже добавлен!"));
                            return;
                        }
                    }
                }
            else{

                    QMessageBox::warning(this, tr("Ошибка"), tr("Поле \"Имя сервера / IP\" не заполнено"));
                    return;
            }

            struct authInfo tmp;
            tmp.sokolhostname = hostname;
            tmp.login = login;
            tmp.password = password;
            a.push_back(tmp);

            ok = net->setupTcpConnection(hostname, portname);
            if(ok < 0){

                QMessageBox::warning(this, tr("Ошибка"), hostname + tr(" недоступен"));
                a.pop_back();
                return;

			}

            for(unsigned i = 0; i < a.size(); i++){

                if(hostname == a[i].sokolhostname){

                    if(net->authRequest(a[i].login, a[i].password) == -18){

                        net->closeSocket();
                        QMessageBox::warning(this, "Ошибка", "Неверный логин или пароль!");
                        a.erase(a.begin() + i);
                        return;
                    }else{

                        QMessageBox::information(this, "Успех", "Авторизация прошла успешно!");
                        break;
                    }
                }
            }

            /* если экспорт выполнен успешно - запомнить хост */
            if(!getExportedDevicesList(hostname))qDebug() << "getExportedDevicesList done";
            else qDebug() << tr("Хост ") + hostname + tr(" не ответил");
}

int sokolMainWindow::getExportedDevicesList(QString host){

	char product_name[128];
	char class_name[128];
	struct op_devlist_reply reply;
	uint16_t code = OP_REP_DEVLIST;
    struct sokol_usb_device udev;
    struct sokol_usb_interface uintf;
	QString busidtmp;
	int rc;
	int status;

    rc = net->sokolSendCommonRequest(OP_REQ_DEVLIST, 0);
    if (0 > rc) {
        net->closeSocket();
		return -1;
	}

    rc = net->sokolReceiveCommonReply(&code, &status);
    if (0 > rc) {
        net->closeSocket();
		return -1;
	}

	memset(&reply, 0, sizeof(reply));
    rc = net->sokolNetworkRecv(&reply, sizeof(reply));
    if (0 > rc) {
        net->closeSocket();
		return -1;
	}

    // net.sokolNetworkPackUint32(0, reply.ndev);
    CHANGE_BYTE_ORDER(0, &reply);

    sokol = new QStandardItem(host);
    sokol->setEditable(false);
    base->appendRow(sokol);

	// server->setData(QIcon(":/icons/file.png"), Qt::DecorationRole);
    sokol->setData(SERVER_ID_DEFAULT, Qt::UserRole + 1);

    if (reply.ndev == 0)sokol->setData(QColor(Qt::red), Qt::ForegroundRole);

    qDebug() << "Exported devices: " << reply.ndev;

	for (unsigned int i = 0; i < reply.ndev; i++) {

		memset(&udev, 0, sizeof(udev));
        rc = net->sokolNetworkRecv(&udev, sizeof(udev));
        if (0 > rc) {
            qDebug() << "failed to get a device from the host: " << host;
            net->closeSocket();
			return -1;
		}
        net->sokolNetworkPackUsbDevice(0, &udev);

        usbs->getProduct(product_name, sizeof(product_name),
                            udev.sokolDeviceVendorId, udev.sokolDeviceProductId);
        usbs->getClass(class_name, sizeof(class_name),
                            udev.sokolDeviceClass, udev.sokolDeviceSubClass,
                            udev.sokolDeviceProtocol);

		QStandardItem* newDevice;
		newDevice = new QStandardItem(product_name);
		newDevice->setData(DEVICE_ID, Qt::UserRole + 1);
        newDevice->setEditable(false);

        busidtmp				 	= udev.sokolDeviceBusid;
		busids[product_name] 		= busidtmp;
		isImported[product_name] 	= false;
		
        sokol->setData(QColor(Qt::darkCyan), Qt::ForegroundRole);
		// QString qstr1 = QString::fromStdString(std::to_string(udev.idVendor));
		// QString qstr2 = QString::fromStdString(std::to_string(udev.idProduct));
        QString vid = QString::number(udev.sokolDeviceVendorId, 16);
        QString pid = QString::number(udev.sokolDeviceProductId, 16);

        QTableWidgetItem *newHostName = new QTableWidgetItem(host);
        //ecru color
        newHostName->setBackground(QColor(224, 205, 149));
        QTableWidgetItem *newDeviceName = new QTableWidgetItem(product_name);
        /* неподключенное устройство в таблице подсвечено красным */
        newDeviceName->setBackground(QColor(255, 23, 23));
		QTableWidgetItem *newBusid = new QTableWidgetItem(busidtmp);
        //orange color
        newBusid->setBackground(QColor(255, 165, 0));
		QTableWidgetItem *newVid = new QTableWidgetItem(vid);
        //light blue color
        newVid->setBackground(QColor(144, 213, 255));
		QTableWidgetItem *newPid = new QTableWidgetItem(pid);
        //ivory color
        newPid->setBackground(QColor(255, 255, 227));

		rows++;
        table->setRowCount(rows);
        table->setItem(rows-1, 0, newHostName);
        table->setItem(rows-1, 1, newDeviceName);
        table->setItem(rows-1, 2, newBusid);
        table->setItem(rows-1, 3, newVid);
        table->setItem(rows-1, 4, newPid);

        newDevice->setData(QColor(Qt::gray), Qt::ForegroundRole);
        sokol->appendRow(newDevice);

        for(int j = 0; j < udev.sokolNumInterfaces; j++){
            rc = net->sokolNetworkRecv(&uintf, sizeof(uintf));
            if(0 > rc){
                qDebug() << "Failed to get a device interface";
                net->closeSocket();
				return -1;
			}

            usbs->getClass(class_name, sizeof(class_name),
					uintf.bInterfaceClass,
					uintf.bInterfaceSubClass,
					uintf.bInterfaceProtocol);

            qDebug() << "[$] " << j << class_name;
		}
	}
    net->closeSocket();
	return 0;
}
void sokolMainWindow::ShowContextMenu(const QPoint &pos){

	QModelIndex index = treeview->indexAt(pos);
	std::vector<QString>bbb;

	if(index.isValid()){

			QMenu* ContextMenu 	= new QMenu(this);
			QStandardItem *item = model->itemFromIndex(index);
			int itemID = item->data(Qt::UserRole + 1).toInt();
			QString entity 		= item->text();

        if(itemID == ROOT_ID){

            QAction add(tr("Добавить Сокол"), this);
            connect(&add, &QAction::triggered, this, &sokolMainWindow::showNewServerModalWindow);

            ContextMenu->addAction(&add);

            ContextMenu->exec(treeview->viewport()->mapToGlobal(pos));
        }
        else if(itemID == SERVER_ID_DEFAULT){

            QAction upd(tr("Обновить Сокол"), this);
            QAction del(tr("Удалить Сокол"), this);

			for(int row = 0; row < item->rowCount(); row++){
				QStandardItem *childItem = item->child(row);
				if(childItem){
					qDebug() << childItem->text();
					bbb.push_back(childItem->text());
				}
			}

			connect(&upd, &QAction::triggered, this, [this, pos, entity, bbb](){updateServerState(pos, entity, bbb);});
			connect(&del, &QAction::triggered, this, [this, pos, entity, bbb](){deleteServer(pos, entity, bbb);});

			ContextMenu->addAction(&upd);
			ContextMenu->addAction(&del);

			ContextMenu->exec(treeview->viewport()->mapToGlobal(pos));

		}
		else if(itemID == DEVICE_ID){

			QStandardItem* parentItem = item->parent();

            QAction attach(tr("Использовать ") + entity, this);
            QAction detach(tr("Отключить ") + entity, this);

            connect(&attach, &QAction::triggered, this, [this, pos, entity, parentItem](){attachDevice(pos, entity, parentItem);});
            connect(&detach, &QAction::triggered, this, [this, pos, entity, parentItem](){detachDevice(pos, entity, parentItem);});

			ContextMenu->addAction(&attach);
			ContextMenu->addAction(&detach);

			ContextMenu->exec(treeview->viewport()->mapToGlobal(pos));
		}
            //не забываем освободить память
			ContextMenu->deleteLater();
	}

    if(!bbb.empty()) bbb.clear();
}

int main(int argc, char** argv){

    QApplication app(argc, argv);
    QFont newFont("Times New Roman", 12, QFont::Bold, true);
    QApplication::setFont(newFont);
	sokolMainWindow window;
	return app.exec();

}
