#include "sendfile.h"
#include "talkwindowshell.h"
#include "windowmanager.h"
#include <QFileDialog>
#include <QMessageBox>

SendFile::SendFile(QWidget *parent)
	: BasicWindow(parent)
	, m_filePath("")
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	initTitleBar();
	setTitleBarTitle("", ":/Resources/MainWindow/qqlogoclassic.png");
	loadStyleSheet("SendFile");
	this->move(100, 400);

	TalkWindowShell* talkWindowShell = WindowManager::getInstance()->getTalkWindowShell();
	connect(this, &SendFile::sendFileClicked, talkWindowShell, &TalkWindowShell::updateSendTcpMsg);
}

SendFile::~SendFile()
{}

void SendFile::on_openBtn_clicked()
{
	m_filePath = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择文件"), "/", "*.txt;;*.doc;;*.*");
	ui.lineEdit->setText(m_filePath);
}

void SendFile::on_sendBtn_clicked()
{
	if (!m_filePath.isEmpty()) {
		QFile file(m_filePath);
		if (file.open(QIODevice::ReadOnly)) {
			QString str = file.readAll();
			int msgType = 2;
			QFileInfo fileInfo(m_filePath);
			QString fileName = fileInfo.fileName();

			emit sendFileClicked(str, msgType, fileName);
			file.close();
		}
		else {
			QMessageBox::information(this, QStringLiteral("提示"), QString::fromUtf8("发送文件%1失败").arg(m_filePath));
			m_filePath = "";
			ui.lineEdit->clear();
			this->close();
			return;
		}

		m_filePath = "";
		this->close();
	}
}

