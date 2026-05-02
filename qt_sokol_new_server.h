#ifndef QT_SOKOL_NEW_SERVER
#define QT_SOKOL_NEW_SERVER

#include "qt_sokol_client.h"

class NewSokolServer : public QWidget{
	private:
		class sokolMainWindow *mwindow;
	public:
        NewSokolServer(QWidget *parent = nullptr);
        ~NewSokolServer();

		QLineEdit *host;
		QLineEdit *login;
		QLineEdit *password;

		QLabel *hostLabel;
		QLabel *loginLabel;
		QLabel *passwordLabel;

		QPushButton *ok_button;

		QCheckBox *sso_checkbox;

        void AddNewSokolServer();
};

NewSokolServer::NewSokolServer(QWidget *parent)
	:QWidget(parent){
		mwindow = (sokolMainWindow*)parent;

		this->resize(250, 350);
		this->setWindowTitle(tr("Новый сервер"));
        // this->setWindowFlags(this->windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint));

		host = new QLineEdit(this);
		hostLabel = new QLabel(this);
		hostLabel->setText(tr("Имя сервера / IP"));
		hostLabel->setGeometry(QRect(QPoint(28, 18), QSize(200, 20)));
		host->setGeometry(QRect(QPoint(28, 40), QSize(210, 30)));

		login = new QLineEdit(this);
		loginLabel = new QLabel(this);
		loginLabel->setText(tr("Логин"));
		loginLabel->setGeometry(QRect(QPoint(34, 80), QSize(200, 20)));
		login->setGeometry(QRect(QPoint(28, 100), QSize(210, 30)));

		password = new QLineEdit(this);
		password->setEchoMode(QLineEdit::Password);
		passwordLabel = new QLabel(this);
		passwordLabel->setText(tr("Пароль"));
		passwordLabel->setGeometry(QRect(QPoint(34, 138), QSize(200, 20)));
		password->setGeometry(QRect(QPoint(28, 158), QSize(210, 30)));

		ok_button = new QPushButton(this);
		ok_button->setGeometry(QRect(QPoint(63, 215), QSize(125, 35)));
		ok_button->setText(tr("Соединиться"));

        this->connect(ok_button, &QPushButton::clicked, this, &NewSokolServer::AddNewSokolServer);

		/* Checkbox "SSO", который делает Disabled поля login/password */
        sso_checkbox = new QCheckBox(tr("По SSO"), this);
        sso_checkbox->setChecked(false);
		sso_checkbox->setGeometry(QRect(QPoint(87, 255), QSize(125, 35)));

		this->connect(sso_checkbox, &QCheckBox::stateChanged, [this](int state){
			if(state == Qt::Unchecked){
                this->login->setEnabled(true);
                this->password->setEnabled(true);
			}
			else if(state == Qt::Checked){
                this->login->setEnabled(false);
                this->password->setEnabled(false);
			}
		});
}

NewSokolServer::~NewSokolServer(){

	delete host;
	delete login;
	delete password;

	delete hostLabel;
	delete loginLabel;
	delete passwordLabel;

	delete ok_button;

	delete sso_checkbox;

    qDebug() << "Memory for NewSokolServer class released";
}

void NewSokolServer::AddNewSokolServer(){

	mwindow->setHostName(host->text());
	mwindow->setLogin(login->text());
	mwindow->setPassword(password->text());
	mwindow->addNewServer();
	this->close();
}

#endif /* QT_SOKOL_NEW_SERVER */
