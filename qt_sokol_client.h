/* 2328 == lines of code */
#ifndef QT_SOKOL_CLIENT_H
#define QT_SOKOL_CLIENT_H

#include<QMainWindow>
#include<QApplication>
#include<QMessageBox>
#include<QGridLayout>
#include<QInputDialog>
#include<QMenuBar>
#include<QTreeView>
#include<QAction>
#include<QTableWidget>
#include<QPushButton>
#include<QMenu>
#include<QLabel>
#include<QCheckBox>
#include<QHeaderView>
#include<QSysInfo>
#include<QDebug>
#include<QStandardItemModel>
#include<vector>
#include<map>

#include "qt_sokol_client_driver.h"
#include "qt_sokol_client_network.h"
#include "qt_sokol_client_names.h"

#define		SERVER_ID_DEFAULT	1200
#define		SERVER_ID_BUSY  	1201
#define		DEVICE_ID           1202
#define     ROOT_ID             1203

struct authInfo{

    QString sokolhostname;
    QString login;
    QString password;
};

class sokolMainWindow : public QMainWindow{
	private:
		QWidget *w;
		QTreeView *treeview;
        QStandardItemModel *model;
		QStandardItem *root;
        QStandardItem *base;
        QStandardItem *sokol;
        QGridLayout *layout;
		QTableWidget *table;

        SokolClientDriver* driver;
        SokolNetworkClient* net;
        usbNames* usbs;
		class NewSokolServer* srvr;

		int rows, columns;
	public:
		sokolMainWindow(QString port = "13240");
		~sokolMainWindow();
        int recordConnection(const char *host, const char *port, const char *busid, int rhport);
        int getExportedDevicesList(QString);
		int setTcpConnection();
        void ShowContextMenu(const QPoint & );
		void addNewServer();
		void updateServerState(const QPoint&, QString, const std::vector<QString>&);
		void deleteServer(const QPoint&, QString, const std::vector<QString>&);
        void attachDevice(const QPoint&, QString, QStandardItem*);
        void detachDevice(const QPoint&, QString, QStandardItem*);
        void deleteRows(QString);
		void setHostName(QString h){hostname = h;}
		void setLogin(QString l){login = l;}
		void setPassword(QString p){password = p;}
		void showNewServerModalWindow();
	private:
        bool eraseServer(QString);
        void clearDevicesInfo(const std::vector<QString> & );
	private:
        std::vector<struct authInfo> a;
		std::map<QString, QString> busids;
		std::map<QString, bool> isImported;
		std::map<QString, int> vhciPorts;
        QString portname;

        /* строки для чтения из модального окна */
        QString hostname;
		QString login;
		QString password;
};

#endif /* QT_SOKOL_CLIENT_H */
